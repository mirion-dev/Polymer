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
        std::filesystem::path current_path;
        DWORD current_process_id{};

        int screen_width{};
        int screen_height{};
        float scale_factor{};

        std::filesystem::path font_dir;

        Environment() {
            ImGui_ImplWin32_EnableDpiAwareness();

            current_module = GetModuleHandleW(nullptr);
            if (current_module == nullptr) {
                fatal_error_with_code("Failed to get the current module.");
            }

            std::wstring path;
            DWORD capacity{ MAX_PATH };
            bool ok;
            do {
                path.resize_and_overwrite(
                    capacity,
                    [&](wchar_t* data, std::size_t) -> DWORD {
                        DWORD size{ GetModuleFileNameW(current_module, data, capacity) };
                        if (size == 0) {
                            fatal_error_with_code("Failed to get the current path.");
                        }

                        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
                            ok = false;
                            capacity *= 2;
                            return 0;
                        }

                        ok = true;
                        return size;
                    }
                );
            } while (!ok);
            current_path = std::move(path);

            current_process_id = GetCurrentProcessId();

            screen_width = GetSystemMetrics(SM_CXSCREEN);
            if (screen_width == 0) {
                fatal_error("Failed to get the screen width.");
            }

            screen_height = GetSystemMetrics(SM_CYSCREEN);
            if (screen_height == 0) {
                fatal_error("Failed to get the screen height.");
            }

            scale_factor = ImGui_ImplWin32_GetDpiScaleForMonitor(MonitorFromWindow(nullptr, MONITOR_DEFAULTTOPRIMARY));

            wchar_t* dir;
            auto _{ wil::scope_exit([&] { CoTaskMemFree(dir); }) };
            if (SHGetKnownFolderPath(FOLDERID_Fonts, 0, nullptr, &dir) != 0) {
                fatal_error("Failed to get the font folder.");
            }
            font_dir = dir;
        }

        Environment(const Environment&) = delete;
        Environment& operator=(const Environment&) = delete;
    };

    static Environment env_instance;

    export Environment& env() {
        return env_instance;
    }

}
