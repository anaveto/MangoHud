// Battery power by status, on fake power_supply directories (test only).
// Each case sets up 1-2 batteries, runs BatteryStats::update() and checks
// current_watt (drain) and charge_watt against the expected values.
#include <spdlog/spdlog.h>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <filesystem.h>
#include "battery.h"

namespace fs = ghc::filesystem;

struct Bat { const char* status; long power_uw; long current_ua; long voltage_uv; };

static void put(const fs::path& d, const char* name, const std::string& v) {
    std::ofstream(d / name) << v << "\n";
}

static int failures;

static void run(const char* name, std::vector<Bat> bats, float drain, float charge) {
    fs::path root = fs::temp_directory_path() / "battery-test";
    fs::remove_all(root);
    Battery_Stats = BatteryStats();
    for (size_t i = 0; i < bats.size(); i++) {
        fs::path d = root / ("BAT" + std::to_string(i));
        fs::create_directories(d);
        put(d, "type", "Battery");
        put(d, "status", bats[i].status);
        put(d, "charge_now", "1000000");
        put(d, "charge_full", "2000000");
        if (bats[i].power_uw)
            put(d, "power_now", std::to_string(bats[i].power_uw));
        else {
            put(d, "current_now", std::to_string(bats[i].current_ua));
            put(d, "voltage_now", std::to_string(bats[i].voltage_uv));
        }
        Battery_Stats.battPath[i] = d.string();
    }
    Battery_Stats.batt_count = bats.size();
    Battery_Stats.batt_check = true;
    Battery_Stats.update();
    bool ok = std::fabs(Battery_Stats.current_watt - drain) < 0.01f &&
              std::fabs(Battery_Stats.charge_watt - charge) < 0.01f;
    printf("%s %-44s drain %6.2f W (want %6.2f)  charge %6.2f W (want %6.2f)\n", ok ? "PASS" : "FAIL", name,
           Battery_Stats.current_watt, drain, Battery_Stats.charge_watt, charge);
    failures += !ok;
}

int main() {
    // the Claw's BAT1 while charging, 2026-11-16: 2.910 A x 15.598 V
    run("charging, current x voltage", {{"Charging", 0, 2910000, 15598000}}, 0, 45.39f);
    run("charging, negative current", {{"Charging", 0, -2910000, 15598000}}, 0, 45.39f);
    run("charging, power_now", {{"Charging", 45000000, 0, 0}}, 0, 45.0f);
    run("discharging, current x voltage", {{"Discharging", 0, 1000000, 15000000}}, 15.0f, 0);
    run("discharging, negative current", {{"Discharging", 0, -1000000, 15000000}}, 15.0f, 0);
    run("discharging, power_now", {{"Discharging", 12500000, 0, 0}}, 12.5f, 0);
    run("not charging (charge limit)", {{"Not charging", 0, 0, 15000000}}, 0, 0);
    run("full", {{"Full", 0, 10000, 15000000}}, 0, 0);
    run("unknown", {{"Unknown", 0, 1000000, 15000000}}, 0, 0);
    run("two: discharging 10 W, charging 5 W", {{"Discharging", 10000000, 0, 0}, {"Charging", 5000000, 0, 0}}, 10.0f, 5.0f);
    run("two: charging 20 W, not charging", {{"Charging", 20000000, 0, 0}, {"Not charging", 0, 0, 15000000}}, 0, 20.0f);
    run("two: charging 20 W, charging 4 W", {{"Charging", 20000000, 0, 0}, {"Charging", 0, 1000000, 4000000}}, 0, 24.0f);
    // the same battery unplugged: the charge rate must go back to 0
    run("charging, then ...", {{"Charging", 30000000, 0, 0}}, 0, 30.0f);
    run("... discharging", {{"Discharging", 9000000, 0, 0}}, 9.0f, 0);
    printf("%d failure(s)\n", failures);
    return failures != 0;
}
