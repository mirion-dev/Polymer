module;

#include "imgui.h"

#include <d3d9.h>
#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx9.h>
#include <ShlObj_core.h>

export module polymer.imgui;

import std;
import polymer.core;
import polymer.error;

namespace polymer {

    struct Platform {
        HMODULE current_module{};
        int screen_width{};
        int screen_height{};
        float scale_factor{};

        Platform() {
            ImGui_ImplWin32_EnableDpiAwareness();

            current_module = GetModuleHandleW(nullptr);
            if (current_module == nullptr) {
                fatal_error("Failed to get the current module.");
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
        }
    };

    static Platform platform;

    export class Window {
        static constexpr auto CLASS_NAME{ L"Polymer" };

        static inline Window* _instance{};

        HMODULE _module{};
        HWND _window{};
        std::function<std::optional<LRESULT>(HWND, UINT, WPARAM, LPARAM)> _handler;
        float _scale{};

        static LRESULT _message_handler(HWND window, UINT message, WPARAM param1, LPARAM param2) {
            if (_instance == nullptr) {
                fatal_error("The window does not exist.");
            }

            if (ImGui_ImplWin32_WndProcHandler(window, message, param1, param2)) {
                return 1;
            }

            if (message == WM_DESTROY) {
                PostQuitMessage(0);
                _instance->_window = nullptr;
                return 0;
            }

            std::optional result{ _instance->_handler(window, message, param1, param2) };
            return result ? *result : DefWindowProcW(window, message, param1, param2);
        }

    public:
        Window(const std::wstring& title, int width, int height, const decltype(_handler)& handler) {
            if (_instance != nullptr) {
                throw LogicError{ "The window already exists." };
            }
            _instance = this;

            _module = GetModuleHandleW(nullptr);
            if (_module == nullptr) {
                throw SystemError{ "Failed to get the module." };
            }

            WNDCLASSEXW window_class{
                sizeof(window_class),
                CS_CLASSDC,
                _message_handler,
                0,
                0,
                _module,
                nullptr,
                nullptr,
                nullptr,
                nullptr,
                CLASS_NAME,
                nullptr
            };
            if (RegisterClassExW(&window_class) == 0) {
                throw SystemError{ "Failed to register the window class." };
            }

            ImGui_ImplWin32_EnableDpiAwareness();
            _scale = ImGui_ImplWin32_GetDpiScaleForMonitor(MonitorFromPoint({}, MONITOR_DEFAULTTOPRIMARY));

            int screen_width{ GetSystemMetrics(SM_CXSCREEN) };
            int screen_height{ GetSystemMetrics(SM_CYSCREEN) };
            width = std::clamp(100, static_cast<int>(width * _scale), screen_width);
            height = std::clamp(100, static_cast<int>(height * _scale), screen_height);
            _handler = handler;
            _window = CreateWindowExW(
                0,
                CLASS_NAME,
                title.data(),
                WS_OVERLAPPEDWINDOW,
                (screen_width - width) / 2,
                (screen_height - height) / 2,
                width,
                height,
                nullptr,
                nullptr,
                _module,
                nullptr
            );
            if (_window == nullptr) {
                if (UnregisterClassW(CLASS_NAME, _module) == 0) {
                    fatal_error("Failed to create the window and failed to unregister the window class.");
                }
                throw SystemError{ "Failed to create the window." };
            }
        }

        Window(const Window&) = delete;
        Window& operator=(const Window&) = delete;

        ~Window() {
            if (_window != nullptr && DestroyWindow(_window) == 0) {
                fatal_error("Failed to destroy the window.");
            }
            if (UnregisterClassW(CLASS_NAME, _module) == 0) {
                fatal_error("Failed to unregister the window class.");
            }
            _instance = nullptr;
        }

        operator HWND() {
            return _window;
        }

        float scale() const {
            return _scale;
        }

        void show() {
            ShowWindow(_window, SW_SHOWDEFAULT);
        }
    };

    export class Device {
        static inline Device* _instance{};

        IDirect3D9* _interface;
        IDirect3DDevice9* _device;
        D3DPRESENT_PARAMETERS _param{
            .SwapEffect             = D3DSWAPEFFECT_DISCARD,
            .Windowed               = true,
            .EnableAutoDepthStencil = true,
            .AutoDepthStencilFormat = D3DFMT_D16
        };

    public:
        Device(Window& window) {
            if (_instance != nullptr) {
                throw LogicError{ "The device already exists." };
            }
            _instance = this;

            _interface = Direct3DCreate9(D3D_SDK_VERSION);
            if (_interface == nullptr) {
                throw RuntimeError{ "Failed to create the interface." };
            }

            if (_interface->CreateDevice(
                D3DADAPTER_DEFAULT,
                D3DDEVTYPE_HAL,
                window,
                D3DCREATE_HARDWARE_VERTEXPROCESSING,
                &_param,
                &_device
            ) < 0) {
                _interface->Release();
                throw RuntimeError{ "Failed to create the device." };
            }
        }

        Device(const Device&) = delete;
        Device& operator=(const Device&) = delete;

        ~Device() {
            _device->Release();
            _interface->Release();
            _instance = nullptr;
        }

        operator IDirect3DDevice9*() {
            return _device;
        }

        IDirect3DDevice9* operator->() {
            return _device;
        }

        D3DPRESENT_PARAMETERS& param() {
            return _param;
        }

        void reset() {
            ImGui_ImplDX9_InvalidateDeviceObjects();
            if (_device->Reset(&_param) < 0) {
                throw RuntimeError{ "Failed to reset the device." };
            }
            ImGui_ImplDX9_CreateDeviceObjects();
        }
    };

    export class Ui {
        static inline Ui* _instance{};

    public:
        Ui(Window& window, Device& device) {
            if (_instance != nullptr) {
                throw LogicError{ "The ui already exists." };
            }
            _instance = this;

            IMGUI_CHECKVERSION();
            ImGui::CreateContext();

            io().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard
                | ImGuiConfigFlags_DockingEnable
                | ImGuiConfigFlags_ViewportsEnable;
            io().ConfigDpiScaleFonts = true;
            io().ConfigDpiScaleViewports = true;
            io().IniFilename = nullptr;

            wchar_t* font_dir;
            bool ok{ SHGetKnownFolderPath(FOLDERID_Fonts, 0, nullptr, &font_dir) == 0 };
            CoTaskMemFree(font_dir);
            if (!ok) {
                throw RuntimeError{ "Failed to get the font folder." };
            }

            if (io().Fonts->AddFontFromFileTTF(
                (to_string(font_dir) + "/msyh.ttc").data(),
                18,
                nullptr,
                io().Fonts->GetGlyphRangesChineseFull()
            ) == nullptr) {
                throw RuntimeError{ "Failed to load the font." };
            }

            style().ScaleAllSizes(window.scale());
            style().WindowRounding = 10;
            style().FrameRounding = 10;

            ImGui_ImplWin32_Init(window);
            ImGui_ImplDX9_Init(device);
        }

        Ui(const Ui&) = delete;
        Ui& operator=(const Ui&) = delete;

        ~Ui() {
            ImGui_ImplDX9_Shutdown();
            ImGui_ImplWin32_Shutdown();
            ImGui::DestroyContext();
            _instance = nullptr;
        }

        ImGuiIO& io() {
            return ImGui::GetIO();
        }

        ImGuiStyle& style() {
            return ImGui::GetStyle();
        }
    };

}
