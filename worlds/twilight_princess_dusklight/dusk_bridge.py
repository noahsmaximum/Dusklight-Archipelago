"""Dusklight transport backend for the Twilight Princess Archipelago client.

Drop-in replacement for `dolphin_memory_engine` as used by TPClient.py, but it
talks to the native `dusk::archipelago` module's localhost TCP text server
instead of attaching to Dolphin. All addresses passed in are OFFSETS into the
live dSv_info_c (the apworld's set_address() must use saveFileAddr = 0).

Protocol (see ../DESIGN.md): newline-delimited ASCII, Dusk is the server.
  HELLO                 -> OK <region> <ingame 0/1> <slotNameHex>
  READ <off> <len>      -> OK <hex>
  WRITE <off> <hex>     -> OK | ERR
  SAFE                  -> OK <0/1>

Integration patch for TPClient.py (5 small edits):
  1. Replace `import dolphin_memory_engine` with
         from . import dusk_bridge as dolphin_memory_engine
     (keeps every existing call site working unchanged), OR import as `mem`
     and rename calls.
  2. set_address(): set `saveFileAddr = 0` for all regions (offsets, not addrs),
     and take `regionCode` from dolphin_memory_engine.hello() instead of
     read_byte(0x80000003).
  3. In dolphin_sync_task connect block, replace the
         read_bytes(0x80000000,3) == b"GZ2" ...
     check with: `if dolphin_memory_engine.hello() is None: ...refused...`.
  4. _check_status(): `return dolphin_memory_engine.safe()`.
  5. Remove LINK_POINTER_ADDR / M_EVENT_STATUS_ADDR usage (folded into SAFE).
"""

import socket
import time
from typing import Optional

HOST = "127.0.0.1"
PORT = 17354
_TIMEOUT = 3.0

_sock: Optional[socket.socket] = None
_buf = b""

# --- read coalescing -------------------------------------------------------
# TPClient scans ~475 locations per tick, each a separate read. The native
# module only services its socket once per frame, so per-read round-trips cap
# throughput at ~1/frame -> a full scan took seconds and froze the client UI.
# Instead snapshot the whole dSv_info_c window in ONE read and serve every
# location/flag read from that snapshot (refreshed on a short TTL). The live
# item-queue scratch [0x8F0, 0x910) is NEVER cached -- give_items must always
# see the real queue / expected-index bytes.
_CACHE_LEN = 0x0A00     # covers all read offsets (save data, node tables, mMemory)
_CACHE_TTL = 0.02       # s; batches one scan yet stays fresh between watcher ticks
_SCRATCH_LO = 0x8F0     # item queue (8 bytes) + expected index (2 bytes)
_SCRATCH_HI = 0x910
_cache: Optional[bytes] = None
_cache_time = 0.0


# ---- dolphin_memory_engine-compatible surface ------------------------------

def hook() -> None:
    """Connect to the Dusklight AP module. Mirrors dolphin_memory_engine.hook()."""
    global _sock, _buf
    un_hook()
    try:
        s = socket.create_connection((HOST, PORT), timeout=_TIMEOUT)
        s.settimeout(_TIMEOUT)
        s.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
        _sock = s
        _buf = b""
    except OSError:
        _sock = None


def is_hooked() -> bool:
    return _sock is not None


def un_hook() -> None:
    global _sock, _buf, _cache
    if _sock is not None:
        try:
            _sock.close()
        except OSError:
            pass
    _sock = None
    _buf = b""
    _cache = None


def read_byte(offset: int) -> int:
    return read_bytes(offset, 1)[0]


def _read_live(offset: int, length: int) -> bytes:
    resp = _txn(f"READ {offset} {length}")
    if not resp.startswith("OK"):
        raise RuntimeError(f"Dusk READ failed at {offset:#x}: {resp!r}")
    parts = resp.split(" ", 1)
    data = bytes.fromhex(parts[1]) if len(parts) > 1 else b""
    if len(data) != length:
        raise RuntimeError(f"Dusk READ short: wanted {length}, got {len(data)}")
    return data


def _ensure_snapshot() -> None:
    """Refresh the cached dSv_info_c window snapshot if missing/expired."""
    global _cache, _cache_time
    now = time.monotonic()
    if _cache is None or (now - _cache_time) > _CACHE_TTL:
        try:
            _cache = _read_live(0, _CACHE_LEN)
            _cache_time = now
        except Exception:
            _cache = None


def invalidate() -> None:
    """Drop the cached snapshot (call after anything that mutates game memory)."""
    global _cache
    _cache = None


def read_bytes(offset: int, length: int) -> bytes:
    # Serve from the window snapshot, except reads that touch the live scratch
    # region (item queue / expected index) or fall outside the snapshot.
    if offset >= 0 and (offset >= _SCRATCH_HI or offset + length <= _SCRATCH_LO):
        _ensure_snapshot()
        if _cache is not None and offset + length <= len(_cache):
            return _cache[offset:offset + length]
    return _read_live(offset, length)


def write_bytes(offset: int, data: bytes) -> None:
    invalidate()  # memory is changing; the snapshot is now stale
    resp = _txn(f"WRITE {offset} {data.hex()}")
    if not resp.startswith("OK"):
        raise RuntimeError(f"Dusk WRITE failed at {offset:#x}: {resp!r}")


# ---- semantic ops (replace the out-of-struct GC reads) ---------------------

def hello() -> Optional[tuple[str, bool, bytes]]:
    """Return (region, ingame, slot_name_bytes) or None if not connected/refused."""
    try:
        resp = _txn("HELLO")
    except (OSError, RuntimeError):
        return None
    if not resp.startswith("OK"):
        return None
    f = resp.split(" ")
    region = f[1] if len(f) > 1 else "E"
    ingame = (len(f) > 2 and f[2] == "1")
    name = bytes.fromhex(f[3]) if len(f) > 3 and f[3] else b""
    return region, ingame, name


def safe() -> bool:
    """Native safe-to-give gate; replaces TPClient._check_status()."""
    try:
        resp = _txn("SAFE")
    except (OSError, RuntimeError):
        return False
    return resp.startswith("OK") and resp.strip().endswith("1")


# ---- transport -------------------------------------------------------------

def _txn(line: str) -> str:
    global _buf
    if _sock is None:
        raise RuntimeError("not hooked")
    try:
        _sock.sendall(line.encode("ascii") + b"\n")
        while b"\n" not in _buf:
            chunk = _sock.recv(4096)
            if not chunk:
                un_hook()
                raise RuntimeError("connection closed")
            _buf += chunk
        resp, _buf = _buf.split(b"\n", 1)
        return resp.decode("ascii", "replace")
    except OSError as e:
        un_hook()
        raise RuntimeError(str(e))
