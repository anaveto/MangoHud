// Prints [CPU, POWER] and [CPU, UNCORE_POWER] as mangohud-next's CPU metrics compute them, next to the same
// values computed here from the RAPL counters over the same interval.
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <thread>
#include "cpu/cpu.hpp"

namespace fs = std::filesystem;

static uint64_t read_uj(const std::string& path) {
    std::ifstream f(path);
    uint64_t v = 0;
    f >> v;
    return v;
}

int main(int argc, char** argv) {
    const int n = argc > 1 ? atoi(argv[1]) : 10;
    const std::string pkg = "/sys/class/powercap/intel-rapl:0/energy_uj";
    std::string unc;
    for (const auto& e : fs::directory_iterator("/sys/class/powercap/intel-rapl:0")) {
        std::ifstream name(e.path() / "name");
        std::string s;
        if (std::getline(name, s) && s == "uncore")
            unc = (e.path() / "energy_uj").string();
    }
    printf("package %s, uncore %s\n", pkg.c_str(), unc.empty() ? "(none)" : unc.c_str());

    CPU cpu;
    cpu.poll();
    auto t0 = std::chrono::steady_clock::now();
    uint64_t p0 = read_uj(pkg), u0 = unc.empty() ? 0 : read_uj(unc);
    for (int i = 1; i <= n; i++) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        cpu.poll();
        auto t1 = std::chrono::steady_clock::now();
        uint64_t p1 = read_uj(pkg), u1 = unc.empty() ? 0 : read_uj(unc);
        double dt = std::chrono::duration<double>(t1 - t0).count();
        cpu_info_t info = cpu.get_info();
        printf("%2d  POWER %6.2f W (direct %6.2f)  UNCORE_POWER %6.2f W (direct %6.2f)\n", i, info.power,
               (p1 - p0) / dt / 1e6, info.uncore_power, (u1 - u0) / dt / 1e6);
        t0 = t1, p0 = p1, u0 = u1;
    }
    return 0;
}
