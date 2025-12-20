#include <Windows.h>

import std;
import polymer.error;
import polymer.app;

using namespace polymer;

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int) {
    try {
        app().run();
    }
    catch (const std::exception& error) {
        show_error(error.what());
    }
    return 0;
}
