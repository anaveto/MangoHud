// Calls xe_drm_base::poll() N times on an xe card node and reports the growth of the heap in use (glibc mallinfo2).
// Built once with xe_drm.cpp from master and once with the fix (#2209), to show the leak and its absence.
#include <malloc.h>
#include <cstdio>
#include <cstdlib>
#include "xe_drm.hpp"

int main(int argc, char** argv) {
    const char* card = argc > 1 ? argv[1] : "/dev/dri/by-path/pci-0000:00:02.0-card";
    const int n = argc > 2 ? atoi(argv[2]) : 10000;
    xe_drm_base d;
    if (!d.setup(card)) {
        fprintf(stderr, "cannot open %s\n", card);
        return 1;
    }
    d.poll(); // the first poll is not counted
    const size_t before = mallinfo2().uordblks;
    for (int i = 0; i < n; i++)
        d.poll();
    const size_t after = mallinfo2().uordblks;
    printf("%d polls: heap in use %zu -> %zu bytes, %+.1f bytes per poll\n", n, before, after,
           ((double)after - (double)before) / n);
    return 0;
}
