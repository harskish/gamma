#include "GammaCore.hpp"
#include <cstdlib>

extern "C" {
    _declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
}

int main(int argc, char * argv[]) {
    GammaCore core;
    core.mainLoop();

    return EXIT_SUCCESS;
}
