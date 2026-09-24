// Offline check of the in-game seed rebuild: slot_data.json -> official generator (AP mode).
// usage: ap_gen_test <slot_data.json> <work dir> [--tracker <stages.json> <out.json>]
//
// --tracker also builds the in-game tracker's logic (src/ap/ap_tracker.cpp) from the work dir
// and answers each stage of stages.json: {"stages": [{"received": [item ids], "checked":
// [location names]}, ...]} -> out.json {"build_ms": n, "stages": [{"reachable": [...], "ms": n}]}.
#include "../generator/randomizer.hpp"
#include "../generator/logic/world.hpp"
#include "../src/ap/ap_tracker.hpp"
#include "../src/ap/data_version.hpp"

#include <chrono>

#include <string_view>

#include <nlohmann/json.hpp>
#include <yaml-cpp/yaml.h>

#include <filesystem>
#include <fstream>
#include <iostream>

int main(int argc, char** argv) {
    // Lets tools/check_data_version.py compare the mod's fingerprint of the logic data
    // with the apworld's, so the two implementations cannot drift apart unnoticed.
    if (argc == 2 && std::string_view{argv[1]} == "--data-version") {
        std::cout << "data_version " << ap::data_version() << "\n";
        return 0;
    }
    if (argc < 3) {
        std::cerr << "usage: ap_gen_test <slot_data.json> <work dir> [--tracker <stages> <out>]\n"
                     "       ap_gen_test --data-version\n";
        return 2;
    }
    namespace fs = std::filesystem;
    auto sd = nlohmann::json::parse(std::ifstream(argv[1]));
    const fs::path base = argv[2];
    fs::create_directories(base);

    YAML::Node settings;
    settings["Seed"] = "AP-" + sd.value("seed", std::string{}) + "-test";
    settings["Plandomizer"] = true;
    settings["Generate Spoiler Log"] = false;
    settings["Starting Inventory"] = YAML::Node(YAML::NodeType::Map);
    settings["Excluded Locations"] = YAML::Node(YAML::NodeType::Sequence);
    settings["Mixed Entrance Pools"] = YAML::Node(YAML::NodeType::Sequence);
    for (const auto& [k, v] : sd["settings"].items()) settings[k] = v.get<std::string>();
    std::ofstream(base / "settings.yaml") << YAML::Dump(settings);

    YAML::Node plando, locs(YAML::NodeType::Map);
    for (const auto& [loc, v] : sd["placements"].items())
        locs[loc] = v.is_string() ? v.get<std::string>() : v.value("item", std::string{"Archipelago Item"});
    plando["World 1"]["Locations"] = locs;
    std::ofstream(base / "plando.yaml") << YAML::Dump(plando);
    YAML::Node prefs;
    prefs["Plandomizer Path"] = (base / "plando.yaml").generic_string();
    std::ofstream(base / "preferences.yaml") << YAML::Dump(prefs);

    randomizer::g_archipelagoMode = true;
    randomizer::Randomizer r{base};
    auto err = r.Generate();
    if (err) {
        std::cout << "GENERATION FAILED: " << *err << "\n";
        return 1;
    }
    auto* world = r.GetWorld();
    int ok = 0, bad = 0;
    for (const auto& [loc, v] : sd["placements"].items()) {
        const std::string want = v.is_string() ? v.get<std::string>() : "Archipelago Item";
        const std::string got = world->GetLocation(loc)->GetCurrentItem()->GetName();
        bool bottle = want == "Empty Bottle" && got.starts_with("Bottle");
        if (got == want || bottle) ++ok; else { ++bad; if (bad < 10) std::cout << "MISMATCH " << loc << ": " << got << " != " << want << "\n"; }
    }
    int empty = 0;
    for (auto* l : world->GetAllLocations()) if (l->IsEmpty()) ++empty;
    std::cout << "hash " << r.GetConfig().GetHash() << " placements ok " << ok << " bad " << bad
              << " empty locations " << empty << "\n";
    if (bad != 0 || empty != 0) {
        return 1;
    }
    if (argc >= 6 && std::string_view{argv[3]} == "--tracker") {
        using clock = std::chrono::steady_clock;
        auto ms = [](clock::time_point t) {
            return std::chrono::duration<double, std::milli>(clock::now() - t).count();
        };
        randomizer::g_archipelagoMode = false;
        auto t0 = clock::now();
        std::string error;
        auto logic = ap::tracker::Logic::build(base, error);
        if (!logic) {
            std::cout << "TRACKER BUILD FAILED: " << error << "\n";
            return 1;
        }
        nlohmann::json out{{"build_ms", ms(t0)}, {"stages", nlohmann::json::array()}};
        std::unordered_set<std::string> checks;
        for (const auto& [name, id] : sd["location_ids"].items()) checks.insert(name);
        std::vector<std::string> unshuffled = sd.contains("unshuffled_dungeons")
            ? sd["unshuffled_dungeons"].get<std::vector<std::string>>()
            : logic->guess_unshuffled(checks);
        auto query = nlohmann::json::parse(std::ifstream(argv[4]));
        for (const auto& stage : query["stages"]) {
            std::vector<uint16_t> received = stage.value("received", std::vector<uint16_t>{});
            std::unordered_set<std::string> checked;
            for (const auto& n : stage.value("checked", nlohmann::json::array())) checked.insert(n);
            auto t = clock::now();
            auto reach = logic->reachable(received, checks, checked, unshuffled);
            out["stages"].push_back({{"reachable", reach}, {"ms", ms(t)}});
        }
        std::ofstream(argv[5]) << out.dump();
        std::cout << "tracker build " << out["build_ms"].get<double>() << " ms, "
                  << out["stages"].size() << " stages" << "\n";
    }
    return 0;
}
