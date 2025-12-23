module;

#include <imgui_impl_win32.h>
#include <ShlObj_core.h>
#include <wil/resource.h>

export module polymer.env;

import std;
import polymer.core;
import polymer.error;

namespace polymer {

    struct Environment {
        HMODULE current_module{};
        int screen_width{};
        int screen_height{};
        float scale_factor{};
        std::string font_dir;

        Environment() {
            ImGui_ImplWin32_EnableDpiAwareness();

            current_module = GetModuleHandleW(nullptr);
            if (current_module == nullptr) {
                fatal_error_with_code("Failed to get the current module.");
            }

            screen_width = GetSystemMetrics(SM_CXSCREEN);
            if (screen_width == 0) {
                fatal_error("Failed to get the screen width.");
            }

            screen_height = GetSystemMetrics(SM_CYSCREEN);
            if (screen_height == 0) {
                fatal_error("Failed to get the screen height.");
            }

            scale_factor = ImGui_ImplWin32_GetDpiScaleForMonitor(MonitorFromWindow(nullptr, MONITOR_DEFAULTTOPRIMARY));

            wchar_t* buffer;
            auto _{ wil::scope_exit([&] { CoTaskMemFree(buffer); }) };
            if (SHGetKnownFolderPath(FOLDERID_Fonts, 0, nullptr, &buffer) != 0) {
                fatal_error("Failed to get the font folder.");
            }
            font_dir = to_string(buffer);
        }

        Environment(const Environment&) = delete;
        Environment& operator=(const Environment&) = delete;
    };

    static Environment env_instance;

    export Environment& env() {
        return env_instance;
    }

}
