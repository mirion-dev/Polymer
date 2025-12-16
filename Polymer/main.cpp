#include <Windows.h>

import std;
import polymer.error;
import polymer.app;

using namespace polymer;

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int) {
    try {
        App{}.run();
    }
    catch (const std::exception& error) {
        fatal_error(error.what());
    }
    return 0;
}
