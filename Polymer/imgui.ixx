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
        std::function<std::optional<LRESULT>(HWND, UINT, WPARAM, LPARAM)> _handler;

        static LRESULT _message_handler(HWND window, UINT message, WPARAM param1, LPARAM param2) {
            if (_instance == nullptr) {
                FATAL_ERROR("The window does not exist");
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
        Window(
            HINSTANCE module,
            const std::wstring& title,
            UINT width,
            UINT height,
            const decltype(_handler)& handler
        ) :
            _module{ module },
            _handler{ handler } {

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

        operator HWND() {
            return _window;
        }

        void show() {
            ShowWindow(_window, SW_SHOWDEFAULT);
            if (UpdateWindow(_window) == 0) {
                throw std::runtime_error("Failed to update the window");
            }
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
            .AutoDepthStencilFormat = D3DFMT_D16,
            .PresentationInterval   = D3DPRESENT_INTERVAL_ONE
        };

    public:
        Device(HWND window) {
            if (_instance != nullptr) {
                throw std::logic_error{ "The device already exists" };
            }
            _instance = this;

            _interface = Direct3DCreate9(D3D_SDK_VERSION);
            if (_interface == nullptr) {
                throw std::runtime_error{ "Failed to create the interface" };
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
                throw std::runtime_error{ "Failed to create the device" };
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
                throw std::runtime_error{ "Failed to reset the device" };
            }
            ImGui_ImplDX9_CreateDeviceObjects();
        }
    };

}
