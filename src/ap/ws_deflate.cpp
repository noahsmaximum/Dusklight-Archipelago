#include "ws_deflate.hpp"

#include <miniz.h>

#include <algorithm>
#include <cctype>
#include <cstring>
#include <vector>

namespace ap {
namespace {

std::string_view trim(std::string_view s) {
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front()))) {
        s.remove_prefix(1);
    }
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back()))) {
        s.remove_suffix(1);
    }
    return s;
}

std::vector<std::string_view> split(std::string_view s, char sep) {
    std::vector<std::string_view> out;
    for (size_t start = 0;;) {
        const size_t end = s.find(sep, start);
        out.push_back(trim(s.substr(start, end == std::string_view::npos ? end : end - start)));
        if (end == std::string_view::npos) {
            return out;
        }
        start = end + 1;
    }
}

bool valid_window_bits(std::string_view value) {
    if (value.size() >= 2 && value.front() == '"' && value.back() == '"') {
        value = value.substr(1, value.size() - 2);
    }
    if (value.empty() || value.size() > 2 ||
        !std::all_of(value.begin(), value.end(), [](char c) { return c >= '0' && c <= '9'; }))
    {
        return false;
    }
    const int bits = std::stoi(std::string(value));
    return bits >= 8 && bits <= 15;
}

// The trailer every compressed message had removed before it was sent (RFC 7692 7.2.2).
constexpr char kTail[] = {'\x00', '\x00', '\xff', '\xff'};

}  // namespace

bool parse_deflate_response(std::string_view header, DeflateParams& out, std::string& error) {
    out = {};
    header = trim(header);
    if (header.empty()) {
        return true;  // the server declined; messages will simply arrive uncompressed
    }
    for (const std::string_view extension : split(header, ',')) {
        const std::vector<std::string_view> parts = split(extension, ';');
        if (parts.front() != "permessage-deflate" || out.accepted) {
            error = "the server accepted a WebSocket extension that wasn't offered";
            return false;
        }
        out.accepted = true;
        for (size_t i = 1; i < parts.size(); ++i) {
            const std::string_view param = parts[i];
            const size_t eq = param.find('=');
            const std::string_view name = trim(param.substr(0, eq));
            const std::string_view value =
                eq == std::string_view::npos ? std::string_view{} : trim(param.substr(eq + 1));
            if (name == "server_no_context_takeover" && value.empty()) {
                out.serverNoContextTakeover = true;
            } else if (name == "client_no_context_takeover" && value.empty()) {
                // Only restricts what we compress, and we don't compress.
            } else if (name == "server_max_window_bits" && valid_window_bits(value)) {
                // Any window up to 15 bits inflates with the 15-bit window used below.
            } else if (name == "client_max_window_bits" && (value.empty() || valid_window_bits(value))) {
                // Only restricts what we compress.
            } else {
                error = "the server answered permessage-deflate with a parameter we can't honour";
                return false;
            }
        }
    }
    return true;
}

struct Inflater::Impl {
    mz_stream stream{};
    bool ready = false;

    bool start() {
        if (ready) {
            return true;
        }
        std::memset(&stream, 0, sizeof(stream));
        // Negative window bits: raw deflate, no zlib header, which is what the RFC uses.
        ready = mz_inflateInit2(&stream, -MZ_DEFAULT_WINDOW_BITS) == MZ_OK;
        return ready;
    }

    void stop() {
        if (ready) {
            mz_inflateEnd(&stream);
            ready = false;
        }
    }

    ~Impl() { stop(); }
};

Inflater::Inflater() : mImpl{std::make_unique<Impl>()} {}
Inflater::~Inflater() = default;

void Inflater::reset() {
    mImpl->stop();
}

bool Inflater::inflate_message(
    std::string& data, size_t maxBytes, bool resetAfter, std::string& error) {
    if (!mImpl->start()) {
        error = "could not start the decompressor";
        return false;
    }
    std::string input = std::move(data);
    input.append(kTail, sizeof(kTail));
    std::string output;

    mz_stream& s = mImpl->stream;
    s.next_in = reinterpret_cast<const unsigned char*>(input.data());
    s.avail_in = static_cast<unsigned int>(input.size());
    bool ended = false;
    unsigned char buffer[64 * 1024];
    for (;;) {
        s.next_out = buffer;
        s.avail_out = sizeof(buffer);
        const int result = mz_inflate(&s, MZ_SYNC_FLUSH);
        const size_t produced = sizeof(buffer) - s.avail_out;
        if (output.size() + produced > maxBytes) {
            error = "the server sent a compressed message that inflates past the size limit";
            reset();
            return false;
        }
        output.append(reinterpret_cast<const char*>(buffer), produced);
        if (result == MZ_STREAM_END) {
            ended = true;  // a final block: the next message starts a fresh stream
            break;
        }
        if (result != MZ_OK && result != MZ_BUF_ERROR) {
            error = "the server sent compressed data that doesn't decompress";
            reset();
            return false;
        }
        if (s.avail_in == 0 && s.avail_out != 0) {
            break;  // all input consumed and nothing left pending
        }
        if (result == MZ_BUF_ERROR && produced == 0) {
            error = "the server sent a compressed message that ends early";
            reset();
            return false;
        }
    }
    if (ended || resetAfter) {
        reset();
    }
    data = std::move(output);
    return true;
}

}  // namespace ap
