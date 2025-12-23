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
                to_string((env().font_dir / "msyh.ttc").wstring()).data(),
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
                if (ready) {
                ready = ui().render([&] {
                        static constexpr float WIDTH{ 600 }, HEIGHT{ 600 };
                        ImGui::SetNextWindowSize({ WIDTH, HEIGHT }, ImGuiCond_FirstUseEver);
                    ImGui::SetNextWindowPos(
                            { (env().screen_width - WIDTH) / 2, (env().screen_height - HEIGHT) / 2 },
                        ImGuiCond_FirstUseEver
                    );
                        ImGui::Begin("Polymer", &running);
                        if (ImGui::Button("Show Demo")) {
                            show_demo = true;
                        }
                    ImGui::End();

                    if (show_demo) {
                        ImGui::ShowDemoWindow(&show_demo);
                    }
                });
            }
                else {
                    HRESULT status{ ui().device()->TestCooperativeLevel() };
                    if (status >= 0 || status == D3DERR_DEVICENOTRESET && ui().device().reset()) {
                        ready = true;
                    }
                    else {
                        std::this_thread::sleep_for(100ms);
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
