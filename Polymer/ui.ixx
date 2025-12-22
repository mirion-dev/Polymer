module;

#include "ui.h"

#include <d3d9.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx9.h>
#include <wil/resource.h>
#include <wil/com.h>

export module polymer.ui;

import std;
import polymer.error;
import polymer.env;

using Microsoft::WRL::ComPtr;

namespace polymer {

    static void window_class_deleter(ATOM atom) {
        if (UnregisterClassW(MAKEINTATOM(atom), env().current_module) == 0) {
            fatal_error_with_code("Failed to unregister the window class.");
        }
    }

    using WindowClass = wil::unique_any<ATOM, decltype(window_class_deleter), window_class_deleter>;

    class Ui {
        static constexpr auto WINDOW_CLASS_NAME{ L"polymer_ui" };

        D3DPRESENT_PARAMETERS _param{
            .SwapEffect             = D3DSWAPEFFECT_DISCARD,
            .Windowed               = true,
            .EnableAutoDepthStencil = true,
            .AutoDepthStencilFormat = D3DFMT_D16
        };

        WindowClass _window_class;
        wil::unique_hwnd _window;
        wil::com_ptr_t<IDirect3D9> _interface;
        wil::com_ptr_t<IDirect3DDevice9> _device;

    public:
        Ui() {
            WNDCLASSEXW window_class_data{
                sizeof(window_class_data),
                0,
                DefWindowProcW,
                0,
                0,
                env().current_module,
                nullptr,
                nullptr,
                nullptr,
                nullptr,
                WINDOW_CLASS_NAME,
                nullptr
            };
            _window_class.reset(RegisterClassExW(&window_class_data));
            if (_window_class == nullptr) {
                throw SystemError{ "Failed to register the window class." };
            }

            _window.reset(CreateWindowExW(
                0,
                MAKEINTATOM(_window_class.get()),
                nullptr,
                WS_POPUP,
                0,
                0,
                1,
                1,
                nullptr,
                nullptr,
                env().current_module,
                nullptr
            ));
            if (_window == nullptr) {
                throw SystemError{ "Failed to create the window." };
            }

            _interface = Direct3DCreate9(D3D_SDK_VERSION);
            if (_interface == nullptr) {
                throw RuntimeError{ "Failed to create the interface." };
            }

            if (_interface->CreateDevice(
                D3DADAPTER_DEFAULT,
                D3DDEVTYPE_HAL,
                _window.get(),
                D3DCREATE_HARDWARE_VERTEXPROCESSING,
                &_param,
                &_device
            ) < 0) {
                throw RuntimeError{ "Failed to create the device." };
            }

            IMGUI_CHECKVERSION();
            ImGui::CreateContext();
            ImGui_ImplWin32_Init(_window.get());
            ImGui_ImplDX9_Init(_device.get());
        }

        Ui(const Ui&) = delete;
        Ui& operator=(const Ui&) = delete;

        ~Ui() {
            ImGui_ImplDX9_Shutdown();
            ImGui_ImplWin32_Shutdown();
            ImGui::DestroyContext();
        }

        IDirect3DDevice9* device() {
            return _device.get();
        }

        D3DPRESENT_PARAMETERS& param() {
            return _param;
        }

        ImGuiIO& io() {
            return ImGui::GetIO();
        }

        ImGuiStyle& style() {
            return ImGui::GetStyle();
        }

        bool render(const auto& func) {
            ImGui_ImplDX9_NewFrame();
            ImGui_ImplWin32_NewFrame();
            ImGui::NewFrame();

            func();

            ImGui::Render();

            if (_device->BeginScene() < 0) {
                throw RuntimeError{ "Failed to begin a scene." };
            }

            ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());

            if (_device->EndScene() < 0) {
                throw RuntimeError{ "Failed to end a scene." };
            }

            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();

            return _device->Present(nullptr, nullptr, nullptr, nullptr) >= 0;
        }

        void reset_device() {
            ImGui_ImplDX9_InvalidateDeviceObjects();
            if (_device->Reset(&_param) < 0) {
                throw RuntimeError{ "Failed to reset the device." };
            }
            ImGui_ImplDX9_CreateDeviceObjects();
        }
    };

    static std::optional<Ui> ui_instance;

    export Ui& ui() {
        return ui_instance ? *ui_instance : ui_instance.emplace();
    }

    export bool process_messages() {
        bool running{ true };
        MSG message;
        while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE) != 0) {
            TranslateMessage(&message);
            DispatchMessageW(&message);
            if (message.message == WM_QUIT) {
                running = false;
            }
        }
        return running;
    }

}
