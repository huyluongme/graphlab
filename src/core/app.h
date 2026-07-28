#pragma once

#include "ui/main_menu_bar.h"
#include <string>
#include <cstdint>

struct GLFWwindow;

namespace GraphLab
{
    /**
     * @brief Configuration specification for application initialization.
     */
    struct AppSpec {
        std::string Title = "GraphLab";
        uint32_t Width = 1280;
        uint32_t Height = 720;
        bool VSync = true;
    };

    /**
     * @brief Core Application class managing window lifecycle, graphics context, and rendering loops.
     */
    class App
    {
    public:
        explicit App(const AppSpec& spec = AppSpec());
        ~App();

        // Prevent copying and moving to guarantee singleton resource ownership
        App(const App&) = delete;
        App(App&&) = delete;
        App& operator=(const App&) = delete;
        App& operator=(App&&) = delete;

        int Run();
        void Close() { m_Running = false; }
        void RenderFrame();

        static App& Get() { return *s_Instance; }
        static bool HasInstance() { return s_Instance != nullptr; }

        GLFWwindow* GetNativeWindow() const { return m_Window; }
        const AppSpec& GetSpec() const { return m_Spec; }

    private:
        bool Init();
        void SetAppIcon();
        void OnUpdate();
        void OnRenderUI();
        void Shutdown();

    private:
        AppSpec m_Spec;
        GLFWwindow* m_Window = nullptr;
        bool m_Running = false;

        UI::MainMenuBar m_MainMenuBar;

        static App* s_Instance;
    };
}
