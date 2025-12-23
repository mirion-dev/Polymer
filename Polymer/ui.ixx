module;

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

    class Ui {
        friend Ui& ui();

        static constexpr auto WINDOW_CLASS_NAME{ L"polymer_ui" };

        static void _window_class_deleter(ATOM window_class) {
            UnregisterClassW(MAKEINTATOM(window_class), env().current_module); // no failure handling
        }

        static void _imgui_win32_deleter(bool) {
            ImGui_ImplWin32_Shutdown();
        }

        static void _imgui_dx_deleter(bool) {
            ImGui_ImplDX9_Shutdown();
        }

        D3DPRESENT_PARAMETERS _param{
            .SwapEffect             = D3DSWAPEFFECT_DISCARD,
            .Windowed               = true,
            .EnableAutoDepthStencil = true,
            .AutoDepthStencilFormat = D3DFMT_D16
        };

        wil::unique_any<ATOM, decltype(&_window_class_deleter), _window_class_deleter> _window_class;
        wil::unique_hwnd _window;
        wil::com_ptr_t<IDirect3D9> _interface;
        wil::com_ptr_t<IDirect3DDevice9> _device;
        wil::unique_any<ImGuiContext*, decltype(&ImGui::DestroyContext), ImGui::DestroyContext> _context;
        wil::unique_any<bool, decltype(&_imgui_win32_deleter), _imgui_win32_deleter> _imgui_win32;
        wil::unique_any<bool, decltype(&_imgui_dx_deleter), _imgui_dx_deleter> _imgui_dx;

    public:
        class IDevice {
            friend Ui;

            Ui* _ui{};

            IDevice(Ui* ui) :
                _ui{ ui } {}

        public:
            IDevice() = default;

            operator IDirect3DDevice9*() {
                return _ui->_device.get();
            }

            IDirect3DDevice9* operator->() {
                return _ui->_device.get();
            }

            bool reset() {
                ImGui_ImplDX9_InvalidateDeviceObjects();
                if (_ui->_device->Reset(&_ui->_param) < 0) {
                    return false;
                }
                ImGui_ImplDX9_CreateDeviceObjects();
                return true;
            }
        };

    private:
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
            if (!_window_class) {
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
            if (!_window) {
                throw SystemError{ "Failed to create the window." };
            }

            _interface.attach(Direct3DCreate9(D3D_SDK_VERSION));
            if (!_interface) {
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
            _context.reset(ImGui::CreateContext());
            if (!_context) {
                throw RuntimeError{ "Failed to create the context." };
            }

            _imgui_win32.reset(ImGui_ImplWin32_Init(_window.get()));
            if (!_imgui_win32) {
                throw RuntimeError{ "Failed to initialize ImGui (Win32)." };
            }

            _imgui_dx.reset(ImGui_ImplDX9_Init(_device.get()));
            if (!_imgui_dx) {
                throw RuntimeError{ "Failed to initialize ImGui (DirectX)." };
            }

            ImGuiIO& io{ this->io() };
            io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
            io.ConfigViewportsNoAutoMerge = true;
        }

        Ui(const Ui&) = delete;
        Ui& operator=(const Ui&) = delete;

    public:
        IDevice device() {
            return this;
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
                return false;
            }
            ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
            if (_device->EndScene() < 0) {
                return false;
            }

            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();

            return _device->Present(nullptr, nullptr, nullptr, nullptr) >= 0;
        }
    };

    export Ui& ui() {
        static Ui instance;
        return instance;
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
