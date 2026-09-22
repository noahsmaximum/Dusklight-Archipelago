#include "data_version.hpp"

#include <battery/embed.hpp>

#include <string_view>

namespace ap {
namespace {

// CRC-32 (the zlib/PKZIP one), so this matches Python's zlib.crc32 byte for byte.
uint32_t crc32_update(uint32_t crc, std::string_view bytes) {
    crc = ~crc;
    for (const char c : bytes) {
        crc ^= static_cast<unsigned char>(c);
        for (int bit = 0; bit < 8; ++bit) {
            crc = (crc >> 1) ^ (0xEDB88320u & (~(crc & 1u) + 1u));
        }
    }
    return ~crc;
}

// Python reads these with "\r" removed, so a checkout with CRLF line endings fingerprints
// the same as one with LF.
uint32_t crc32_without_cr(uint32_t crc, std::string_view bytes) {
    size_t start = 0;
    for (size_t i = 0; i < bytes.size(); ++i) {
        if (bytes[i] == '\r') {
            crc = crc32_update(crc, bytes.substr(start, i - start));
            start = i + 1;
        }
    }
    return crc32_update(crc, bytes.substr(start));
}

template <b::embed_string_literal path>
uint32_t add(uint32_t crc) {
    const auto file = b::embed<path>();
    return crc32_without_cr(crc, std::string_view{file.data(), file.size()});
}

}  // namespace

uint32_t data_version() {
    // Same files in the same order as data.py::data_version, which is what the apworld sends
    // in slot_data. tools/check_data_version.py keeps the two lists honest in CI: if they
    // ever disagree, the mod would refuse every seed rather than fail quietly.
    static const uint32_t value = [] {
        uint32_t crc = 0;
        crc = add<RANDO_DATA_PATH "items.yaml">(crc);
        crc = add<RANDO_DATA_PATH "locations.yaml">(crc);
        crc = add<RANDO_DATA_PATH "macros.yaml">(crc);
        crc = add<RANDO_DATA_PATH "settings_list.yaml">(crc);
        crc = add<RANDO_DATA_PATH "world/Root.yaml">(crc);
        crc = add<RANDO_DATA_PATH "world/overworld/Ordona Province.yaml">(crc);
        crc = add<RANDO_DATA_PATH "world/overworld/Faron Province.yaml">(crc);
        crc = add<RANDO_DATA_PATH "world/overworld/Eldin Province.yaml">(crc);
        crc = add<RANDO_DATA_PATH "world/overworld/Lanayru Province.yaml">(crc);
        crc = add<RANDO_DATA_PATH "world/overworld/Snowpeak Province.yaml">(crc);
        crc = add<RANDO_DATA_PATH "world/overworld/Gerudo Desert.yaml">(crc);
        crc = add<RANDO_DATA_PATH "world/dungeons/Forest Temple.yaml">(crc);
        crc = add<RANDO_DATA_PATH "world/dungeons/Goron Mines.yaml">(crc);
        crc = add<RANDO_DATA_PATH "world/dungeons/Lakebed Temple.yaml">(crc);
        crc = add<RANDO_DATA_PATH "world/dungeons/Arbiters Grounds.yaml">(crc);
        crc = add<RANDO_DATA_PATH "world/dungeons/Snowpeak Ruins.yaml">(crc);
        crc = add<RANDO_DATA_PATH "world/dungeons/Temple of Time.yaml">(crc);
        crc = add<RANDO_DATA_PATH "world/dungeons/City in the Sky.yaml">(crc);
        crc = add<RANDO_DATA_PATH "world/dungeons/Palace of Twilight.yaml">(crc);
        crc = add<RANDO_DATA_PATH "world/dungeons/Hyrule Castle.yaml">(crc);
        return crc;
    }();
    return value;
}

}  // namespace ap
