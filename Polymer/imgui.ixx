module;

#include "imgui.h"

export module polymor.imgui;

import std;

namespace polymer {

    export class Window {
        static constexpr auto CLASS_NAME{ L"Polymer" };

        static inline Window* _instance{};

        HINSTANCE _module{};
        HWND _window{};
        std::function<void(UINT, UINT)> _on_resize;

        static LRESULT _message_handler(HWND window, UINT message, WPARAM param1, LPARAM param2) {
            if (_instance == nullptr) {
                FATAL_ERROR("The window does not exist");
            }

            if (ImGui_ImplWin32_WndProcHandler(window, message, param1, param2)) {
                return 1;
            }

            switch (message) {
            case WM_SIZE:
                if (param1 != SIZE_MINIMIZED) {
                    _instance->_on_resize(LOWORD(param2), HIWORD(param2));
                }
                return 0;
            case WM_SYSCOMMAND:
                if ((param1 & 0xfff0) == SC_KEYMENU) {
                    return 0;
                }
                break;
            case WM_DESTROY:
                PostQuitMessage(0);
                _instance->_window = nullptr;
                return 0;
            }
            return DefWindowProcW(window, message, param1, param2);
        }

    public:
        Window(
            HINSTANCE module,
            const std::wstring& title,
            UINT width,
            UINT height,
            const std::function<void(UINT, UINT)>& on_resize
        ) :
            _module{ module },
            _on_resize{ on_resize } {

            if (_instance != nullptr) {
                throw std::logic_error{ "The window already exists" };
            }
            _instance = this;

            WNDCLASSEXW window_class{
                sizeof(window_class),
                CS_CLASSDC,
                _message_handler,
                0,
                0,
                module,
                nullptr,
                nullptr,
                nullptr,
                nullptr,
                CLASS_NAME,
                nullptr
            };
            if (RegisterClassExW(&window_class) == 0) {
                throw std::runtime_error{ "Failed to register the window class" };
            }

            _window = CreateWindowExW(
                0,
                CLASS_NAME,
                title.data(),
                WS_OVERLAPPEDWINDOW,
                std::max(0u, (GetSystemMetrics(SM_CXSCREEN) - width) / 2),
                std::max(0u, (GetSystemMetrics(SM_CYSCREEN) - height) / 2),
                width,
                height,
                nullptr,
                nullptr,
                _module,
                nullptr
            );
            if (_window == nullptr) {
                if (UnregisterClassW(CLASS_NAME, _module) == 0) {
                    FATAL_ERROR("Failed to create the window and failed to unregister the window class");
                }
                throw std::runtime_error{ "Failed to create the window" };
            }
        }

        Window(const Window&) = delete;
        Window& operator=(const Window&) = delete;

        ~Window() {
            if (_window != nullptr && DestroyWindow(_window) == 0) {
                FATAL_ERROR("Failed to destroy the window");
            }
            if (UnregisterClassW(CLASS_NAME, _module) == 0) {
                FATAL_ERROR("Failed to unregister the window class");
            }
            _instance = nullptr;
        }

        HWND handle() const {
            return _window;
        }

        void show() const {
            ShowWindow(_window, SW_SHOWDEFAULT);
            if (UpdateWindow(_window) == 0) {
                throw std::runtime_error("Failed to update the window");
            }
        }
    };

}
