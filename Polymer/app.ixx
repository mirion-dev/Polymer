module;

#include <d3d9.h>
#include <imgui.h>
#include <imgui_stdlib.h>

export module polymer.app;

import std;
import polymer.core;
import polymer.error;
import polymer.env;
import polymer.overlay;
import polymer.ui;

using namespace std::literals;

namespace polymer {

    class App {
        friend App& app();

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

    public:
        static void run() {
            bool running{ true };
            bool ready{ true };

            std::vector<std::string> targets(overlay().patches_name.size());
            ImVec4 info_color;
            std::string info;
            while (running && process_messages()) {
                if (ready) {
                    ready = ui().render([&] {
                        static constexpr float WIDTH{ 640 }, HEIGHT{ 480 };
                        ImGui::SetNextWindowSize({ WIDTH, HEIGHT }, ImGuiCond_FirstUseEver);
                        ImGui::SetNextWindowPos(
                            { (env().screen_width - WIDTH) / 2, (env().screen_height - HEIGHT) / 2 },
                            ImGuiCond_FirstUseEver
                        );
                        ImGui::Begin("Polymer", &running);
                        for (auto&& [target, name] : std::views::zip(targets, overlay().patches_name)) {
                            ImGui::InputText(to_string(name.wstring()).data(), &target);
                        }
                        if (!info.empty()) {
                            ImGui::TextColored(info_color, info.data());
                        }
                        if (ImGui::Button("Apply")) {
                            if (targets[0].empty()) {
                                info_color = { 1, 0, 0, 1 };
                                info = "The target cannot be empty.";
                            }
                            else {
                                info_color = { 1, 1, 1, 1 };
                                info = "OK, you did it.";
                            }
                        }
                        ImGui::End();
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

    export App& app() {
        static App instance;
        return instance;
    }

}
