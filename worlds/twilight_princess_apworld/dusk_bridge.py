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
from typing import Optional

HOST = "127.0.0.1"
PORT = 17354
_TIMEOUT = 3.0

_sock: Optional[socket.socket] = None
_buf = b""


# ---- dolphin_memory_engine-compatible surface ------------------------------

def hook() -> None:
    """Connect to the Dusklight AP module. Mirrors dolphin_memory_engine.hook()."""
    global _sock, _buf
    un_hook()
    try:
        s = socket.create_connection((HOST, PORT), timeout=_TIMEOUT)
        s.settimeout(_TIMEOUT)
        _sock = s
        _buf = b""
    except OSError:
        _sock = None


def is_hooked() -> bool:
    return _sock is not None


def un_hook() -> None:
    global _sock, _buf
    if _sock is not None:
        try:
            _sock.close()
        except OSError:
            pass
    _sock = None
    _buf = b""


def read_byte(offset: int) -> int:
    return read_bytes(offset, 1)[0]


def read_bytes(offset: int, length: int) -> bytes:
    resp = _txn(f"READ {offset} {length}")
    if not resp.startswith("OK"):
        raise RuntimeError(f"Dusk READ failed at {offset:#x}: {resp!r}")
    parts = resp.split(" ", 1)
    data = bytes.fromhex(parts[1]) if len(parts) > 1 else b""
    if len(data) != length:
        raise RuntimeError(f"Dusk READ short: wanted {length}, got {len(data)}")
    return data


def write_bytes(offset: int, data: bytes) -> None:
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
