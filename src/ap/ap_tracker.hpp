#pragma once

// Tracker logic: the seed's world rebuilt from the same settings and plando the save was
// generated from, kept around to answer "what can I reach with what I have?".
//
// No mod services in here, so tools/ap_gen_test can drive it offline.

#include <cstdint>
#include <filesystem>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_set>
#include <vector>

namespace randomizer {
class Randomizer;
}

namespace ap::tracker {

// The generator runs in AP mode through a global flag, so seed generation and tracker builds
// take turns: hold this around any Randomizer::Generate().
inline std::mutex g_generatorMutex;

class Logic {
public:
    // Rebuilds the world from `base` (settings.yaml, plando.yaml, preferences.yaml, as written
    // for the seed). Takes seconds: call it on a worker thread, never two at once.
    static std::unique_ptr<Logic> build(const std::filesystem::path& base, std::string& error);
    ~Logic();

    // Which of `checks` (AP location names) are reachable right now.
    //   received: items the server gave us (items.yaml ids), including starting inventory.
    //   checked:  checks already collected; their own-world items count as owned.
    // Every location that isn't a check (events, vanilla and unshuffled-dungeon contents) gives
    // up its item once it's in reach, the way playing the game would. Checks never do: what
    // they hold is either in `checked` or not found yet.
    //   unshuffled: dungeons left vanilla (slot_data "unshuffled_dungeons"); their keys count
    //               as held from the start, as the apworld's logic has them.
    std::unordered_set<std::string> reachable(const std::vector<uint16_t>& received,
        const std::unordered_set<std::string>& checks,
        const std::unordered_set<std::string>& checked,
        const std::vector<std::string>& unshuffled) const;

    // For slot data from before "unshuffled_dungeons": dungeons whose only checks are NPC
    // gifts (an unshuffled dungeon keeps those, e.g. Snowpeak Ruins Mansion Map).
    std::vector<std::string> guess_unshuffled(const std::unordered_set<std::string>& checks) const;

private:
    Logic() = default;
    std::unique_ptr<randomizer::Randomizer> mRando;
};

}  // namespace ap::tracker
