module;

#include <d3d9.h>
#include <imgui.h>

export module polymer.app;

import std;
import polymer.core;
import polymer.error;
import polymer.env;
import polymer.ui;

using namespace std::literals;

namespace polymer {

    class App {
        bool _running{ true };

    public:
        App() {
            ImGuiIO& io{ ui().io() };
            io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard
                | ImGuiConfigFlags_DockingEnable
                | ImGuiConfigFlags_ViewportsEnable;
            io.ConfigDpiScaleFonts = true;
            io.ConfigDpiScaleViewports = true;
            io.IniFilename = nullptr;

            if (io.Fonts->AddFontFromFileTTF(
                reinterpret_cast<char*>((env().font_dir / "msyh.ttc").u8string().data()),
                18,
                nullptr,
                io.Fonts->GetGlyphRangesChineseSimplifiedCommon()
            ) == nullptr) {
                throw RuntimeError{ "Failed to load the font." };
            }

            ImGuiStyle& style{ ui().style() };
            style.WindowRounding = 10;
            style.FrameRounding = 10;
            style.ScaleAllSizes(env().scale_factor);
        }

        App(const App&) = delete;
        App& operator=(const App&) = delete;

        void run() {
            while (_running && process_messages()) {
                if (!ui().render([&] {
                    static constexpr float WIDTH{ 800 }, HEIGHT{ 600 };
                    ImGui::SetNextWindowSize({ WIDTH, HEIGHT }, ImGuiCond_FirstUseEver);
                    ImGui::SetNextWindowPos({ (env().screen_width - WIDTH) / 2, (env().screen_height - HEIGHT) / 2 },
                        ImGuiCond_FirstUseEver);
                    if (ImGui::Begin("Polymer", &_running)) {
                        ImGui::Text("Hello, world!");
                    }
                    ImGui::End();
                })) {
                    HRESULT status{ ui().device()->TestCooperativeLevel() };
                    if (status < 0) {
                        if (status == D3DERR_DEVICENOTRESET) {
                            ui().reset_device();
                        }
                        else {
                            std::this_thread::sleep_for(1s);
                        }
                    }
                }
            }
        }
    };

    static std::optional<App> app_instance;

    export App& app() {
        return app_instance ? *app_instance : app_instance.emplace();
    }

}
