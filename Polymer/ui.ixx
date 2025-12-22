module;

#include "ui.h"

#include <d3d9.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx9.h>
#include <wil/resource.h>
#include <wrl/client.h> // including <wil/com.h> causes ICE

export module polymer.ui;

import std;
import polymer.error;
import polymer.env;

using namespace Microsoft;

namespace polymer {

    static LRESULT global_message_handler(HWND, UINT, WPARAM, LPARAM);

    using MessageHandler = std::function<LRESULT(HWND, UINT, WPARAM, LPARAM)>;

    class WindowClass {
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
                global_message_handler,
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

        std::wstring_view name() const {
            return _name;
        }
    };

    class Window {
        std::wstring _title;
        std::unique_ptr<MessageHandler> _message_handler;
        wil::unique_hwnd _handle;

    public:
        Window(
            WindowClass& window_class,
            std::wstring_view title,
            int width,
            int height,
            const MessageHandler& message_handler
        ) :
            _title{ title },
            _message_handler{ new MessageHandler{ message_handler } } {

            width = std::clamp(width, 100, env().screen_width);
            height = std::clamp(height, 100, env().screen_height);
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
                _message_handler.get()
            ));
            if (_handle == nullptr) {
                throw SystemError{ "Failed to create the window." };
            }
        }

        HWND get() {
            return _handle.get();
        }

        std::wstring_view title() const {
            return _title;
        }

        const MessageHandler& message_handler() const {
            return *_message_handler;
        }

        void show() {
            ShowWindow(_handle.get(), SW_SHOWDEFAULT);
        }
    };

    LRESULT global_message_handler(HWND window, UINT message, WPARAM param1, LPARAM param2) {
        if (ImGui_ImplWin32_WndProcHandler(window, message, param1, param2)) {
            return 1;
        }

        MessageHandler* message_handler;
        if (message == WM_NCCREATE) {
            message_handler = static_cast<MessageHandler*>(reinterpret_cast<LPCREATESTRUCT>(param2)->lpCreateParams);
            SetLastError(0);
            SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(message_handler));
            if (GetLastError() != 0) {
                fatal_error_with_code("Failed to set the user data.");
            }
        }
        else {
            message_handler = reinterpret_cast<MessageHandler*>(GetWindowLongPtrW(window, GWLP_USERDATA));
        }

        if (message_handler == nullptr) {
            return DefWindowProcW(window, message, param1, param2);
        }
        return (*message_handler)(window, message, param1, param2);
    }

    class Device {
        D3DPRESENT_PARAMETERS _param{
            .SwapEffect             = D3DSWAPEFFECT_DISCARD,
            .Windowed               = true,
            .EnableAutoDepthStencil = true,
            .AutoDepthStencilFormat = D3DFMT_D16
        };
        WRL::ComPtr<IDirect3D9> _interface;
        WRL::ComPtr<IDirect3DDevice9> _device;

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

    class Ui {
        WindowClass _window_class{ L"polymer_ui" };
        Window _window;
        Device _device{ _window };

    public:
        Ui(std::wstring_view title, int width, int height, const MessageHandler& message_handler) :
            _window{ _window_class, title, width, height, message_handler } {

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

        Window& window() {
            return _window;
        }

        Device& device() {
            return _device;
        }

        ImGuiIO& io() {
            return ImGui::GetIO();
        }

        ImGuiStyle& style() {
            return ImGui::GetStyle();
        }

        bool process_messages() {
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

        void render(const auto& func) {
            ImGui_ImplDX9_NewFrame();
            ImGui_ImplWin32_NewFrame();
            ImGui::NewFrame();
            func();
            ImGui::Render();
        }

        bool present(const auto& func) {
            if (_device->BeginScene() < 0) {
                throw RuntimeError{ "Failed to begin a scene." };
            }

            func();
            ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());

            if (_device->EndScene() < 0) {
                throw RuntimeError{ "Failed to end a scene." };
            }

            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();

            return _device->Present(nullptr, nullptr, nullptr, nullptr) >= 0;
        }
    };

    static std::optional<Ui> ui_instance;

    export Ui& ui() {
        if (!ui_instance) {
            throw LogicError{ "The ui does not exist." };
        }
        return *ui_instance;
    }

    export Ui& ui(std::wstring_view title, int width, int height, const MessageHandler& message_handler) {
        return ui_instance ? *ui_instance : ui_instance.emplace(title, width, height, message_handler);
    }

}
