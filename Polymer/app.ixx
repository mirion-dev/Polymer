export module polymer.app;

import std;

namespace polymer {

    class App {
    public:
        App() = default;
        App(const App&) = delete;
        App& operator=(const App&) = delete;

        void run() {}
    };

    static std::optional<App> app_instance;

    export App& app() {
        return app_instance ? *app_instance : app_instance.emplace();
    }

}
