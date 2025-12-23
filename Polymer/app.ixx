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
    public:
        App() {
            ImGuiIO& io{ ui().io() };
            io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_DockingEnable;
            io.ConfigDpiScaleFonts = true;
            io.ConfigDpiScaleViewports = true;
            io.IniFilename = nullptr;

            if (io.Fonts->AddFontFromFileTTF(
                (env().font_dir + "\\msyh.ttc").data(),
                18,
                nullptr,
                io.Fonts->GetGlyphRangesChineseSimplifiedCommon()
            ) == nullptr) {
                throw RuntimeError{ "Failed to load the font." };
            }

            ui().style().ScaleAllSizes(env().scale_factor);
        }

        App(const App&) = delete;
        App& operator=(const App&) = delete;

        void run() {
            bool running{ true };
            bool ready{ true };
            while (running && process_messages()) {
                if (!ready) {
                    HRESULT status{ ui().device()->TestCooperativeLevel() };
                    if (status < 0 && status != D3DERR_DEVICENOTRESET
                        || status == D3DERR_DEVICENOTRESET && !ui().device().reset()) {
                        std::this_thread::sleep_for(100ms);
                        continue;
                    }
                }

                ready = ui().render([&] {
                    ImGui::ShowDemoWindow();

                    static constexpr float WIDTH{ 800 }, HEIGHT{ 600 };
                    ImGui::SetNextWindowSize({ WIDTH, HEIGHT }, ImGuiCond_FirstUseEver);
                    ImGui::SetNextWindowPos(
                        { (env().screen_width - WIDTH) / 2, (env().screen_height - HEIGHT) / 2 },
                        ImGuiCond_FirstUseEver
                    );
                    if (ImGui::Begin("Polymer", &running)) {
                        ImGui::Text("Hello, world!");
                    }
                    ImGui::End();
                });
            }
        }
    };

    static std::optional<App> app_instance;

    export App& app() {
        return app_instance ? *app_instance : app_instance.emplace();
    }

}
