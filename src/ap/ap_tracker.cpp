#include "ap_tracker.hpp"

#include "../../generator/logic/fill.hpp"
#include "../../generator/logic/search.hpp"
#include "../../generator/logic/world.hpp"
#include "../../generator/randomizer.hpp"

#include <algorithm>

namespace ap::tracker {

namespace search = randomizer::logic::search;

constexpr const char* kDungeons[] = {"Forest Temple", "Goron Mines", "Lakebed Temple",
    "Arbiters Grounds", "Snowpeak Ruins", "Temple of Time", "City in the Sky",
    "Palace of Twilight", "Hyrule Castle"};

Logic::~Logic() = default;

std::unique_ptr<Logic> Logic::build(const std::filesystem::path& base, std::string& error) {
    std::lock_guard lock{g_generatorMutex};
    std::unique_ptr<Logic> logic{new Logic};
    logic->mRando = std::make_unique<randomizer::Randomizer>(base);
    randomizer::g_archipelagoMode = true;
    auto err = logic->mRando->Generate();
    randomizer::g_archipelagoMode = false;
    if (err.has_value()) {
        error = *err;
        return nullptr;
    }
    auto* world = logic->mRando->GetWorld();
    if (world == nullptr) {
        error = "no world was built";
        return nullptr;
    }
    // Generation caches which forms and times can pass each exit, from a search holding the
    // world's item pool. In AP mode that pool is empty (everything is placed by plando, much of
    // it in other games), so the cache would only allow what's passable with nothing and the
    // tracker could never get past the first few areas. Re-cache assuming any number of every
    // item: that's the question the cache answers ("could this form ever pass here?").
    randomizer::logic::item_pool::ItemPool everything;
    for (const auto& [name, item] : world->GetItemTable()) {
        everything.insert(everything.end(), 100, item.get());
    }
    // The caching search itself reads the cache (a missing entry means "any form"), so the old
    // one has to go first or it just reproduces itself.
    for (auto& w : logic->mRando->GetWorlds()) {
        w->GetExitTimeFormCache().clear();
    }
    randomizer::logic::fill::CacheExitTimeForms(logic->mRando->GetWorlds(), everything);
    return logic;
}

std::unordered_set<std::string> Logic::reachable(const std::vector<uint16_t>& received,
    const std::unordered_set<std::string>& checks,
    const std::unordered_set<std::string>& checked,
    const std::vector<std::string>& unshuffled) const {
    auto* world = mRando->GetWorld();
    randomizer::logic::item_pool::ItemPool owned;
    for (const uint16_t id : received) {
        auto* item = world->GetItem(id, true);  // Nothing for ids this world doesn't know
        if (item != nullptr && item != randomizer::logic::item::Nothing.get()) {
            owned.push_back(item);
        }
    }
    for (const auto& name : checked) {
        randomizer::logic::location::Location* location = nullptr;
        try {
            location = world->GetLocation(name);
        } catch (const std::exception&) {
            continue;  // not a location of this world
        }
        auto* item = location->GetCurrentItem();
        // Other worlds' items sit here as the "Archipelago Item" placeholder; ours are real.
        if (item != nullptr && !location->IsEmpty() && item->GetName() != "Archipelago Item") {
            owned.push_back(item);
        }
    }

    // Unshuffled dungeons hold their keys from the start, in logic only, exactly as the apworld
    // does (TPWorld.create_regions, "held"): the randomizer's key logic is conservative, and
    // with a dungeon's other items also at home it deadlocks (the Gale Boomerang sits behind an
    // all-keys door while some keys need the boomerang).
    for (auto* l : world->GetAllLocations()) {
        auto* item = l->GetCurrentItem();
        if (l->IsEmpty() || item == nullptr || checks.contains(l->GetName()) ||
            !l->HasCategories("Dungeon") ||
            std::ranges::none_of(unshuffled, [&](const std::string& d) { return l->HasCategories(d); }))
        {
            continue;
        }
        const std::string name = item->GetName();
        if (item->IsDungeonSmallKey() || item->IsBigKey() || name == "Ordon Pumpkin" ||
            name == "Ordon Cheese") {
            owned.push_back(item);
        }
    }

    search::Search s{search::SearchMode::ACCESSIBLE_LOCATIONS, &mRando->GetWorlds(), owned,
        world->GetID()};
    s._collectFilter = [&checks](const randomizer::logic::location::Location* location) {
        return !checks.contains(location->GetName());
    };
    s.SearchWorlds();
    std::unordered_set<std::string> out;
    for (const auto* location : s._visitedLocations) {
        std::string name = location->GetName();
        if (checks.contains(name)) {
            out.insert(std::move(name));
        }
    }
    return out;
}

std::vector<std::string> Logic::guess_unshuffled(
    const std::unordered_set<std::string>& checks) const {
    std::vector<std::string> out;
    const auto locations = mRando->GetWorld()->GetAllLocations();
    for (const char* dungeon : kDungeons) {
        const bool shuffled = std::ranges::any_of(locations, [&](const auto* l) {
            return checks.contains(l->GetName()) && l->HasCategories("Dungeon") &&
                   l->HasCategories(std::string{dungeon}) && !l->HasCategories("Npc");
        });
        if (!shuffled) {
            out.emplace_back(dungeon);
        }
    }
    return out;
}

}  // namespace ap::tracker
