#pragma once

// WebSocket permessage-deflate (RFC 7692), receive side.
//
// Archipelago servers compress what they send and warn clients that don't negotiate it that
// they may stop working. The client offers the extension, inflates compressed messages, and
// keeps sending uncompressed ones, which the RFC allows message by message: what we send is
// small, and it keeps the compressor out of the send path entirely.
//
// Free of dusklight headers so tools/tls_test.cpp can test it on the host.

#include <cstddef>
#include <memory>
#include <string>
#include <string_view>

namespace ap {

struct DeflateParams {
    bool accepted = false;                 // the server agreed to permessage-deflate
    bool serverNoContextTakeover = false;  // it resets its compressor after every message
};

// Reads the server's Sec-WebSocket-Extensions response header (every occurrence joined with
// ","; empty if there was none). An extension or parameter we didn't offer is an error,
// because the server may now be framing messages in a way we can't read.
bool parse_deflate_response(std::string_view header, DeflateParams& out, std::string& error);

class Inflater {
public:
    Inflater();
    ~Inflater();
    Inflater(const Inflater&) = delete;
    Inflater& operator=(const Inflater&) = delete;

    // Decompresses one whole message (its frames' payloads, concatenated) in place. Without
    // server_no_context_takeover the server's compressor carries its window from message to
    // message, so ours has to as well: this stream lives as long as the connection.
    // Refuses to produce more than maxBytes, so a small message can't inflate into a huge one.
    bool inflate_message(std::string& data, size_t maxBytes, bool resetAfter, std::string& error);
    void reset();

private:
    struct Impl;
    std::unique_ptr<Impl> mImpl;
};

}  // namespace ap
