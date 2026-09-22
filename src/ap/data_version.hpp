#pragma once

// Fingerprint of the randomizer's logic data embedded in this build, computed exactly as
// the apworld computes it (data.py::data_version). The apworld sends its own in slot_data;
// if the two disagree, the mod and the apworld disagree about the world, so the mod refuses
// the seed instead of quietly building it with different logic.

#include <cstdint>

namespace ap {

uint32_t data_version();

}  // namespace ap
