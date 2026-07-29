#include "ui/main_menu_bar.h"
#include "core/app.h"
#include "imgui.h"

namespace GraphLab::UI {
    void MainMenuBar::OnRenderUI() {
        if (ImGui::BeginMainMenuBar()) {
            // 1. FILE MENU
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("New Workspace", "Ctrl+N")) {}
                if (ImGui::MenuItem("Open Project...", "Ctrl+O")) {}
                if (ImGui::MenuItem("Save Project", "Ctrl+S")) {}
                ImGui::Separator();
                if (ImGui::MenuItem("Export Graph Image (PNG)...", "Ctrl+E")) {}
                ImGui::Separator();
                if (ImGui::MenuItem("Exit", "Alt+F4"))
                    App::Get().Close();

                ImGui::EndMenu();
            }

            // 2. EDIT MENU
            if (ImGui::BeginMenu("Edit")) {
                if (ImGui::MenuItem("Add Function", "Ctrl+A")) {}
                if (ImGui::MenuItem("Clear All Functions")) {}
                ImGui::Separator();
                if (ImGui::MenuItem("Reset Colors")) {}
                ImGui::EndMenu();
            }

            // 3. VIEW MENU
            if (ImGui::BeginMenu("View")) {
                if (ImGui::MenuItem("Reset Viewport (Zoom 1:1)", "R")) {}
                ImGui::Separator();

                static bool show_sidebar = true;
                if (ImGui::MenuItem("Show Sidebar", nullptr, &show_sidebar)) {}

                static bool show_grid = true;
                if (ImGui::MenuItem("Show Grid Lines", nullptr, &show_grid)) {}

                static bool show_axis_numbers = true;
                if (ImGui::MenuItem("Show Axis Numbers", nullptr, &show_axis_numbers)) {}

                ImGui::EndMenu();
            }

            // 4. HELP MENU
            if (ImGui::BeginMenu("Help")) {
                if (ImGui::MenuItem("Math Syntax Guide")) {}
                if (ImGui::MenuItem("Keyboard Shortcuts")) {}
                ImGui::Separator();
                if (ImGui::MenuItem("About GraphLab"))
                    m_ShowAboutPopup = true;
                ImGui::EndMenu();
            }

            ImGui::EndMainMenuBar();
        }

        // Render About GraphLab Modal Popup
        ShowAboutPopup();
    }

    void MainMenuBar::ShowAboutPopup() {
        if (m_ShowAboutPopup) {
            ImGui::OpenPopup("About GraphLab");
            m_ShowAboutPopup = false;
        }

        if (ImGui::BeginPopupModal("About GraphLab", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("GraphLab - Graphing Calculator v1.0.0");
            ImGui::Separator();
            ImGui::Text("A modern C++ 2D graphing desktop application.");
            ImGui::Text("Powered by Dear ImGui & GLFW.");
            ImGui::Text("License: MIT");
            ImGui::Separator();

            if (ImGui::Button("Close", ImVec2(120, 0)))
            {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
    }

} // namespace UI
