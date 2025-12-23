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
            io.IniFilename = nullptr;

            if (io.Fonts->AddFontFromFileTTF(
                (env().font_dir + "\\msyh.ttc").data(),
                18,
                nullptr,
                io.Fonts->GetGlyphRangesChineseSimplifiedCommon()
            ) == nullptr) {
                throw RuntimeError{ "Failed to load the font." };
            }

            ImGuiStyle& style{ ui().style() };
            style.ScaleAllSizes(env().scale_factor);
            style.WindowPadding.x = 15;
            style.FramePadding.x = 10;
            style.FrameRounding = 10;
            style.GrabRounding = 10;
            style.TabRounding = 10;
            style.ScrollbarRounding = 10;
            style.WindowMenuButtonPosition = ImGuiDir_None;
        }

        App(const App&) = delete;
        App& operator=(const App&) = delete;

        void run() {
            bool running{ true };
            bool ready{ true };

            bool show_demo{};
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
                    ImGui::SetNextWindowSize({ 600, 600 }, ImGuiCond_FirstUseEver);
                    ImGui::SetNextWindowPos(
                        { (env().screen_width - 600.f) / 2, (env().screen_height - 600.f) / 2 },
                        ImGuiCond_FirstUseEver
                    );
                    if (ImGui::Begin("Polymer", &running)) {
                        ImGui::Text("Hello, world!");
                        if (ImGui::Button("Show Demo")) {
                            show_demo = true;
                        }
                    }
                    ImGui::End();

                    if (show_demo) {
                        ImGui::ShowDemoWindow(&show_demo);
                    }
                });
            }
        }
    };

    static std::optional<App> app_instance;

    export App& app() {
        return app_instance ? *app_instance : app_instance.emplace();
    }

}
