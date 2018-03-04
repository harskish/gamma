#include "GammaCore.hpp"
#include <cstdlib>

#ifdef _MSC_VER 
extern "C" {
    _declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
}
#endif

int main(int argc, char * argv[]) {
    GammaCore core;
    core.mainLoop();

    return EXIT_SUCCESS;
}
