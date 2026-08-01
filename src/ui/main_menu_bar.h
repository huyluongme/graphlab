#pragma once

#include <string>

namespace GraphLab::UI {
    /**
     * @brief Main navigation menu bar component for GraphLab.
     */
    class MainMenuBar {
    public:
        MainMenuBar() = default;
        ~MainMenuBar() = default;

        void OnRenderUI();
        void PerformExportImage(const std::string& filepath);

    private:
        void ShowMathGuidePopup();
        void ShowShortcutsPopup();
        void ShowAboutPopup();
        void ShowNotificationPopup();
        void ImportProject(const std::string& filepath = "workspace.graphlab");
        void ExportProject(const std::string& filepath = "workspace.graphlab");

    private:
        bool m_ShowMathGuidePopup = false;
        bool m_ShowShortcutsPopup = false;
        bool m_ShowAboutPopup = false;
        bool m_ShowNotificationPopup = false;
        std::string m_NotificationMessage;
    };
}
