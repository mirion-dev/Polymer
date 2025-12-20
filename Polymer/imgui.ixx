module;

#include "imgui.h"

#include <d3d9.h>
#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx9.h>

#include <wrl/client.h> // including <wil/com.h> causes ICE
#include <wil/resource.h>

export module polymer.imgui;

import std;
import polymer.error;

using Microsoft::WRL::ComPtr;

namespace polymer {

    export struct Environment {
        HMODULE current_module{};
        int screen_width{};
        int screen_height{};
        float scale_factor{};

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
        }

        Environment(const Environment&) = delete;
        Environment& operator=(const Environment&) = delete;
    };

    static Environment env_instance;

    export Environment& env() {
        return env_instance;
    }

    export class WindowClass {
        static LRESULT _handler(HWND, UINT, WPARAM, LPARAM);

        static void _deleter(ATOM atom) {
            if (UnregisterClassW(MAKEINTATOM(atom), env().current_module) == 0) {
                fatal_error_with_code("Failed to unregister the window class.");
            }
        }

        std::wstring _name;
        wil::unique_any<ATOM, decltype(_deleter), _deleter> _atom;

    public:
        WindowClass(std::wstring_view name) :
            _name{ name } {

            WNDCLASSEXW window_class{
                sizeof(window_class),
                0,
                _handler,
                0,
                0,
                env().current_module,
                nullptr,
                nullptr,
                nullptr,
                nullptr,
                _name.data(),
                nullptr
            };
            _atom.reset(RegisterClassExW(&window_class));
            if (_atom == nullptr) {
                throw SystemError{ "Failed to register the window class." };
            }
        }

        const wchar_t* get() {
            return MAKEINTATOM(_atom.get());
        }
    };

    export class Window {
        friend WindowClass;

        using Handler = std::function<LRESULT(HWND, UINT, WPARAM, LPARAM)>;

        std::wstring _title;
        std::unique_ptr<Handler> _handler;
        wil::unique_hwnd _handle;

    public:
        Window(WindowClass& window_class, std::wstring_view title, int width, int height, const Handler& handler) :
            _title{ title },
            _handler{ new Handler{ handler } } {

            width = std::clamp(static_cast<int>(width * env().scale_factor), 100, env().screen_width);
            height = std::clamp(static_cast<int>(height * env().scale_factor), 100, env().screen_height);
            _handle.reset(CreateWindowExW(
                0,
                window_class.get(),
                _title.data(),
                WS_OVERLAPPEDWINDOW,
                (env().screen_width - width) / 2,
                (env().screen_height - height) / 2,
                width,
                height,
                nullptr,
                nullptr,
                env().current_module,
                _handler.get()
            ));
            if (_handle == nullptr) {
                throw SystemError{ "Failed to create the window." };
            }
        }

        HWND get() {
            return _handle.get();
        }

        void show() {
            ShowWindow(_handle.get(), SW_SHOWDEFAULT);
        }
    };

    LRESULT WindowClass::_handler(HWND window, UINT message, WPARAM param1, LPARAM param2) {
        if (ImGui_ImplWin32_WndProcHandler(window, message, param1, param2)) {
            return 1;
        }

        Window::Handler* handler;
        if (message == WM_NCCREATE) {
            handler = static_cast<Window::Handler*>(reinterpret_cast<LPCREATESTRUCT>(param2)->lpCreateParams);
            SetLastError(0);
            SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(handler));
            if (GetLastError() != 0) {
                fatal_error_with_code("Failed to set the user data.");
            }
        }
        else {
            handler = reinterpret_cast<Window::Handler*>(GetWindowLongPtrW(window, GWLP_USERDATA));
        }

        if (handler == nullptr) {
            return DefWindowProcW(window, message, param1, param2);
        }
        return (*handler)(window, message, param1, param2);
    }

    export class Device {
        D3DPRESENT_PARAMETERS _param{
            .SwapEffect             = D3DSWAPEFFECT_DISCARD,
            .Windowed               = true,
            .EnableAutoDepthStencil = true,
            .AutoDepthStencilFormat = D3DFMT_D16
        };
        ComPtr<IDirect3D9> _interface;
        ComPtr<IDirect3DDevice9> _device;

    public:
        Device(Window& window) {
            _interface = Direct3DCreate9(D3D_SDK_VERSION);
            if (_interface == nullptr) {
                throw RuntimeError{ "Failed to create the interface." };
            }

            if (_interface->CreateDevice(
                D3DADAPTER_DEFAULT,
                D3DDEVTYPE_HAL,
                window.get(),
                D3DCREATE_HARDWARE_VERTEXPROCESSING,
                &_param,
                &_device
            ) < 0) {
                _interface->Release();
                throw RuntimeError{ "Failed to create the device." };
            }
        }

        IDirect3DDevice9* get() {
            return _device.Get();
        }

        D3DPRESENT_PARAMETERS& param() {
            return _param;
        }

        IDirect3DDevice9* operator->() {
            return get();
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
    public:
        Ui(Window& window, Device& device) {
            IMGUI_CHECKVERSION();
            ImGui::CreateContext();
            ImGui_ImplWin32_Init(window.get());
            ImGui_ImplDX9_Init(device.get());
        }

        Ui(const Ui&) = delete;
        Ui& operator=(const Ui&) = delete;

        ~Ui() {
            ImGui_ImplDX9_Shutdown();
            ImGui_ImplWin32_Shutdown();
            ImGui::DestroyContext();
        }

        ImGuiIO& io() {
            return ImGui::GetIO();
        }

        ImGuiStyle& style() {
            return ImGui::GetStyle();
        }
    };

    static std::optional<Ui> ui_instance;

    export Ui& ui() {
        if (!ui_instance) {
            throw LogicError{ "The ui does not exist." };
        }
        return *ui_instance;
    }

    export Ui& ui(Window& window, Device& device) {
        return ui_instance ? *ui_instance : ui_instance.emplace(window, device);
    }

}
