#include "ui/main_menu_bar.h"
#include "core/app.h"
#include "utils/file_dialog.h"
#include "utils/project_serializer.h"
#include "imgui.h"

#include <GLFW/glfw3.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#include <vector>
#include <cmath>

namespace GraphLab::UI {

    void MainMenuBar::OnRenderUI() {
        if (ImGui::BeginMainMenuBar()) {
            // 1. FILE MENU (Import Project, Export Project, Export Graph Image with native file dialogs)
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("Import Project...", "Ctrl+O")) {
                    std::string path = Utils::FileDialog::Open(
                        App::Get().GetNativeWindow(),
                        "GraphLab Project (*.graphlab)\0*.graphlab\0All Files (*.*)\0*.*\0"
                    );
                    if (!path.empty()) {
                        ImportProject(path);
                    }
                }
                if (ImGui::MenuItem("Export Project...", "Ctrl+S")) {
                    std::string path = Utils::FileDialog::Save(
                        App::Get().GetNativeWindow(),
                        "GraphLab Project (*.graphlab)\0*.graphlab\0All Files (*.*)\0*.*\0",
                        "graphlab"
                    );
                    if (!path.empty()) {
                        ExportProject(path);
                    }
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Export PNG Image...", "Ctrl+E")) {
                    std::string path = Utils::FileDialog::Save(
                        App::Get().GetNativeWindow(),
                        "PNG Image (*.png)\0*.png\0All Files (*.*)\0*.*\0",
                        "png"
                    );
                    if (!path.empty()) {
                        App::Get().RequestExportImage(path);
                    }
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Exit", "Alt+F4")) {
                    App::Get().Close();
                }

                ImGui::EndMenu();
            }

            // 2. EDIT MENU
            if (ImGui::BeginMenu("Edit")) {
                if (ImGui::MenuItem("Add Function", "Ctrl+A")) {
                    App::Get().GetSidebarPanel().AddExpression();
                }
                if (ImGui::MenuItem("Clear All Functions")) {
                    App::Get().GetSidebarPanel().GetExpressions().clear();
                }
                ImGui::EndMenu();
            }

            // 3. VIEW MENU
            if (ImGui::BeginMenu("View")) {
                static bool show_grid = true;
                if (ImGui::MenuItem("Show Grid & Axes", nullptr, &show_grid)) {}
                ImGui::EndMenu();
            }

            // 4. HELP MENU
            if (ImGui::BeginMenu("Help")) {
                if (ImGui::MenuItem("About GraphLab")) {
                    m_ShowAboutPopup = true;
                }
                ImGui::EndMenu();
            }

            ImGui::EndMainMenuBar();
        }

        // Render Popups
        ShowAboutPopup();
        ShowNotificationPopup();
    }

    void MainMenuBar::PerformExportImage(const std::string& filepath) {
        int windowW = 0, windowH = 0;
        int fbW = 0, fbH = 0;
        glfwGetWindowSize(App::Get().GetNativeWindow(), &windowW, &windowH);
        glfwGetFramebufferSize(App::Get().GetNativeWindow(), &fbW, &fbH);

        if (fbW <= 0 || fbH <= 0 || windowW <= 0 || windowH <= 0) return;

        float scaleX = static_cast<float>(fbW) / static_cast<float>(windowW);
        float scaleY = static_cast<float>(fbH) / static_cast<float>(windowH);

        float sidebarW = App::Get().GetSidebarPanel().GetPanelWidth();
        float menuH = ImGui::GetFrameHeight();

        // Crop pixel coordinates for Graph Canvas area (excluding sidebar & menu bar)
        int cropX = static_cast<int>(sidebarW * scaleX);
        int cropY = 0; // Bottom of framebuffer
        int cropW = static_cast<int>((windowW - sidebarW) * scaleX);
        int cropH = static_cast<int>((windowH - menuH) * scaleY);

        if (cropW <= 0 || cropH <= 0) return;

        std::vector<unsigned char> pixels(3 * cropW * cropH);
        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        glReadPixels(cropX, cropY, cropW, cropH, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());

        // Flip vertically for STB write
        stbi_flip_vertically_on_write(1);

        int success = stbi_write_png(filepath.c_str(), cropW, cropH, 3, pixels.data(), cropW * 3);
        if (!success) {
            m_NotificationMessage = "Failed to export PNG image: Unable to write file.";
            m_ShowNotificationPopup = true;
            return;
        }

        m_NotificationMessage = "Graph image successfully exported to PNG:\n" + filepath;
        m_ShowNotificationPopup = true;
    }

    void MainMenuBar::ExportProject(const std::string& filepath) {
        const auto& expressions = App::Get().GetSidebarPanel().GetExpressions();
        const auto& params = App::Get().GetSidebarPanel().GetGlobalParams();
        const auto& canvas = App::Get().GetGraphCanvas();

        bool ok = Utils::ProjectSerializer::SaveProject(
            filepath,
            expressions,
            params,
            canvas.GetZoom(),
            canvas.GetPanOffset()
        );

        if (!ok) {
            m_NotificationMessage = "Failed to export project: Unable to write file.";
        } else {
            m_NotificationMessage = "Project successfully saved to:\n" + filepath;
        }
        m_ShowNotificationPopup = true;
    }

    void MainMenuBar::ImportProject(const std::string& filepath) {
        auto& sidebar = App::Get().GetSidebarPanel();
        auto& expressions = sidebar.GetExpressions();
        auto& params = sidebar.GetGlobalParams();

        float zoom = 50.0f;
        ImVec2 panOffset = ImVec2(0.0f, 0.0f);

        bool ok = Utils::ProjectSerializer::LoadProject(
            filepath,
            expressions,
            params,
            zoom,
            panOffset
        );

        if (!ok) {
            m_NotificationMessage = "Failed to open project file: " + filepath;
        } else {
            auto& canvas = App::Get().GetGraphCanvas();
            canvas.SetZoom(zoom);
            canvas.SetPanOffset(panOffset);
            m_NotificationMessage = "Project successfully loaded from:\n" + filepath;
        }
        m_ShowNotificationPopup = true;
    }

    void MainMenuBar::ShowNotificationPopup() {
        if (m_ShowNotificationPopup) {
            ImGui::OpenPopup("Notification");
            m_ShowNotificationPopup = false;
        }

        if (ImGui::BeginPopupModal("Notification", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("%s", m_NotificationMessage.c_str());
            ImGui::Separator();

            if (ImGui::Button("OK", ImVec2(120, 0))) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
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

            if (ImGui::Button("Close", ImVec2(120, 0))) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
    }

} // namespace GraphLab::UI
