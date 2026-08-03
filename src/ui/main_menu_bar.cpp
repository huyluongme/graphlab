#include "ui/main_menu_bar.h"
#include "ui/icons.h"
#include "version.h"
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
                if (ImGui::MenuItem(ICON_FA_FOLDER_OPEN "  Import Project...", "Ctrl+O")) {
                    std::string path = Utils::FileDialog::Open(
                        App::Get().GetNativeWindow(),
                        "GraphLab Project (*.graphlab)\0*.graphlab\0All Files (*.*)\0*.*\0"
                    );
                    if (!path.empty()) {
                        ImportProject(path);
                    }
                }
                if (ImGui::MenuItem(ICON_FA_FLOPPY_DISK "  Export Project...", "Ctrl+S")) {
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
                if (ImGui::MenuItem(ICON_FA_IMAGE "  Export PNG Image...", "Ctrl+E")) {
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
                if (ImGui::MenuItem(ICON_FA_RIGHT_FROM_BRACKET "  Exit", "Alt+F4")) {
                    App::Get().Close();
                }

                ImGui::EndMenu();
            }

            // 2. VIEW MENU
            if (ImGui::BeginMenu("View")) {
                auto& canvas = App::Get().GetGraphCanvas();

                bool showGrid = canvas.IsGridEnabled();
                if (ImGui::MenuItem(ICON_FA_TABLE_CELLS "  Grid & Axes", nullptr, &showGrid)) {
                    canvas.SetGridEnabled(showGrid);
                }

                bool showAxisLabels = canvas.IsAxisLabelsEnabled();
                if (ImGui::MenuItem(ICON_FA_HASHTAG "  Axis Numbers", nullptr, &showAxisLabels)) {
                    canvas.SetAxisLabelsEnabled(showAxisLabels);
                }

                ImGui::Separator();

                bool showKeyPoints = canvas.IsKeyPointsEnabled();
                if (ImGui::MenuItem(ICON_FA_BULLSEYE "  Key Points Markers", nullptr, &showKeyPoints)) {
                    canvas.SetKeyPointsEnabled(showKeyPoints);
                }

                bool showTrace = canvas.IsTraceModeEnabled();
                if (ImGui::MenuItem(ICON_FA_CROSSHAIRS "  Trace Hover Inspection", nullptr, &showTrace)) {
                    canvas.SetTraceModeEnabled(showTrace);
                }

                ImGui::Separator();

                bool showToolbar = canvas.IsToolbarEnabled();
                if (ImGui::MenuItem(ICON_FA_SLIDERS "  Canvas Info Toolbar", nullptr, &showToolbar)) {
                    canvas.SetToolbarEnabled(showToolbar);
                }

                ImGui::Separator();

                if (ImGui::MenuItem(ICON_FA_LOCATION_CROSSHAIRS "  Reset Viewport (1:1)", "Home")) {
                    canvas.ResetView();
                }

                ImGui::EndMenu();
            }

            // 3. HELP MENU
            if (ImGui::BeginMenu("Help")) {
                if (ImGui::MenuItem(ICON_FA_BOOK "  Math Syntax Guide...", "F1")) {
                    m_ShowMathGuidePopup = true;
                }
                if (ImGui::MenuItem(ICON_FA_KEYBOARD "  Shortcuts & Controls...")) {
                    m_ShowShortcutsPopup = true;
                }
                ImGui::Separator();
                if (ImGui::MenuItem(ICON_FA_CIRCLE_INFO "  About GraphLab")) {
                    m_ShowAboutPopup = true;
                }
                ImGui::EndMenu();
            }

            ImGui::EndMainMenuBar();
        }

        // Render Popups
        ShowMathGuidePopup();
        ShowShortcutsPopup();
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

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16.0f, 14.0f));
        if (ImGui::BeginPopupModal("Notification", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("%s", m_NotificationMessage.c_str());
            ImGui::Dummy(ImVec2(0.0f, 6.0f));
            ImGui::Separator();
            ImGui::Dummy(ImVec2(0.0f, 6.0f));

            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);
            if (ImGui::Button("OK", ImVec2(120.0f, 28.0f))) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::PopStyleVar();
            ImGui::EndPopup();
        }
        ImGui::PopStyleVar(2);
    }

    void MainMenuBar::ShowMathGuidePopup() {
        if (m_ShowMathGuidePopup) {
            ImGui::OpenPopup("Math Syntax Guide");
            m_ShowMathGuidePopup = false;
        }

        ImGui::SetNextWindowSize(ImVec2(580.0f, 420.0f), ImGuiCond_FirstUseEver);
        bool isOpen = true;
        ImGuiWindowFlags flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(14.0f, 14.0f));
        if (ImGui::BeginPopupModal("Math Syntax Guide", &isOpen, flags)) {
            if (!isOpen) {
                ImGui::CloseCurrentPopup();
            }

            ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "GraphLab Supported Mathematical Functions & Syntax");
            ImGui::Separator();
            ImGui::Dummy(ImVec2(0.0f, 4.0f));

            ImGuiTableFlags tableFlags = ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit;

            ImGui::PushStyleVar(ImGuiStyleVar_TabRounding, 5.0f);
            if (ImGui::BeginTabBar("MathSyntaxTabs")) {
                if (ImGui::BeginTabItem("Trigonometry")) {
                    ImGui::Dummy(ImVec2(0.0f, 4.0f));
                    if (ImGui::BeginTable("TrigTable", 3, tableFlags)) {
                        ImGui::TableSetupColumn("Function", ImGuiTableColumnFlags_WidthFixed, 110.0f);
                        ImGui::TableSetupColumn("Description", ImGuiTableColumnFlags_WidthFixed, 200.0f);
                        ImGui::TableSetupColumn("Example", ImGuiTableColumnFlags_WidthStretch);
                        ImGui::TableHeadersRow();

                        auto AddRow = [](const char* func, const char* desc, const char* ex) {
                            ImGui::TableNextRow();
                            ImGui::TableSetColumnIndex(0); ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "%s", func);
                            ImGui::TableSetColumnIndex(1); ImGui::Text("%s", desc);
                            ImGui::TableSetColumnIndex(2); ImGui::TextDisabled("%s", ex);
                        };

                        AddRow("sin(x), cos(x)", "Sine / Cosine", "y = sin(2*x)");
                        AddRow("tan(x), cot(x)", "Tangent / Cotangent", "y = tan(x)");
                        AddRow("asin(x)", "ArcSine (Inverse Sine)", "y = asin(x)");
                        AddRow("acos(x)", "ArcCosine", "y = acos(x)");
                        AddRow("atan(x)", "ArcTangent", "y = atan(x)");

                        ImGui::EndTable();
                    }
                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("Powers & Logs")) {
                    ImGui::Dummy(ImVec2(0.0f, 4.0f));
                    if (ImGui::BeginTable("PowersTable", 3, tableFlags)) {
                        ImGui::TableSetupColumn("Function", ImGuiTableColumnFlags_WidthFixed, 110.0f);
                        ImGui::TableSetupColumn("Description", ImGuiTableColumnFlags_WidthFixed, 200.0f);
                        ImGui::TableSetupColumn("Example", ImGuiTableColumnFlags_WidthStretch);
                        ImGui::TableHeadersRow();

                        auto AddRow = [](const char* func, const char* desc, const char* ex) {
                            ImGui::TableNextRow();
                            ImGui::TableSetColumnIndex(0); ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "%s", func);
                            ImGui::TableSetColumnIndex(1); ImGui::Text("%s", desc);
                            ImGui::TableSetColumnIndex(2); ImGui::TextDisabled("%s", ex);
                        };

                        AddRow("x^y", "Power", "y = x^3 - 2*x");
                        AddRow("sqrt(x)", "Square Root", "y = sqrt(16 - x^2)");
                        AddRow("cbrt(x)", "Cube Root", "y = cbrt(x)");
                        AddRow("abs(x)", "Absolute Value", "y = abs(x - 2)");
                        AddRow("log(x)", "Natural Logarithm (ln)", "y = log(x)");
                        AddRow("log10(x)", "Base-10 Logarithm", "y = log10(x)");
                        AddRow("exp(x)", "Exponential e^x", "y = exp(-x^2)");

                        ImGui::EndTable();
                    }
                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("Equations & Points")) {
                    ImGui::Dummy(ImVec2(0.0f, 4.0f));
                    if (ImGui::BeginTable("EqTable", 3, tableFlags)) {
                        ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 120.0f);
                        ImGui::TableSetupColumn("Format", ImGuiTableColumnFlags_WidthFixed, 210.0f);
                        ImGui::TableSetupColumn("Example", ImGuiTableColumnFlags_WidthStretch);
                        ImGui::TableHeadersRow();

                        auto AddRow = [](const char* type, const char* fmt, const char* ex) {
                            ImGui::TableNextRow();
                            ImGui::TableSetColumnIndex(0); ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "%s", type);
                            ImGui::TableSetColumnIndex(1); ImGui::Text("%s", fmt);
                            ImGui::TableSetColumnIndex(2); ImGui::TextDisabled("%s", ex);
                        };

                        AddRow("Implicit Circle", "(x - a)^2 + (y - b)^2 = r^2", "(x-2)^2 + (y+1)^2 = 16");
                        AddRow("Implicit Curve", "F(x, y) = 0", "x^2 + y^2 = 25");
                        AddRow("Inequalities", "y < f(x), x^2 + y^2 <= r^2", "y < x^2 - 4, x^2 + y^2 <= 16");
                        AddRow("2D Point Tuple", "(x, y)", "(1, 2)");
                        AddRow("Point List", "(x1, y1), (x2, y2)", "(-2, 0), (0, 4), (2, 0)");
                        AddRow("Constants", "pi, e", "y = sin(pi * x)");

                        ImGui::EndTable();
                    }
                    ImGui::EndTabItem();
                }
                ImGui::EndTabBar();
            }
            ImGui::PopStyleVar(); // TabRounding

            ImGui::Dummy(ImVec2(0.0f, 6.0f));
            ImGui::Separator();
            ImGui::Dummy(ImVec2(0.0f, 6.0f));

            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);
            if (ImGui::Button("Close", ImVec2(100.0f, 28.0f))) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::PopStyleVar();
            ImGui::EndPopup();
        }
        ImGui::PopStyleVar(2);
    }

    void MainMenuBar::ShowShortcutsPopup() {
        if (m_ShowShortcutsPopup) {
            ImGui::OpenPopup("Shortcuts & Controls");
            m_ShowShortcutsPopup = false;
        }

        ImGui::SetNextWindowSize(ImVec2(520.0f, 340.0f), ImGuiCond_FirstUseEver);
        bool isOpen = true;
        ImGuiWindowFlags flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(14.0f, 14.0f));
        if (ImGui::BeginPopupModal("Shortcuts & Controls", &isOpen, flags)) {
            if (!isOpen) {
                ImGui::CloseCurrentPopup();
            }

            ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Mouse & Keyboard Interaction Shortcuts");
            ImGui::Separator();
            ImGui::Dummy(ImVec2(0.0f, 4.0f));

            ImGuiTableFlags tableFlags = ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit;
            if (ImGui::BeginTable("ShortcutsTable", 2, tableFlags)) {
                ImGui::TableSetupColumn("Action", ImGuiTableColumnFlags_WidthFixed, 180.0f);
                ImGui::TableSetupColumn("Shortcut / Input", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableHeadersRow();

                auto AddRow = [](const char* action, const char* shortcut) {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0); ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "%s", action);
                    ImGui::TableSetColumnIndex(1); ImGui::TextDisabled("%s", shortcut);
                };

                AddRow("Pan / Drag Canvas", "Click & Drag Mouse or Middle Mouse");
                AddRow("Cursor-Centered Zoom", "Scroll Mouse Wheel");
                AddRow("Inspect / Pin Point", "Left-Click on Key Point Marker");
                AddRow("Reset Viewport (1:1)", "Home Key or 'Reset View' Button");
                AddRow("Import Project", "Ctrl + O");
                AddRow("Export Project", "Ctrl + S");
                AddRow("Export PNG Image", "Ctrl + E");

                ImGui::EndTable();
            }

            ImGui::Dummy(ImVec2(0.0f, 6.0f));
            ImGui::Separator();
            ImGui::Dummy(ImVec2(0.0f, 6.0f));

            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);
            if (ImGui::Button("Close", ImVec2(100.0f, 28.0f))) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::PopStyleVar();
            ImGui::EndPopup();
        }
        ImGui::PopStyleVar(2);
    }

    void MainMenuBar::ShowAboutPopup() {
        if (m_ShowAboutPopup) {
            ImGui::OpenPopup("About GraphLab");
            m_ShowAboutPopup = false;
        }

        bool isOpen = true;
        ImGuiWindowFlags flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16.0f, 14.0f));
        if (ImGui::BeginPopupModal("About GraphLab", &isOpen, flags)) {
            if (!isOpen) {
                ImGui::CloseCurrentPopup();
            }

            ImGui::Text("%s", GRAPHLAB_FULL_NAME);
            ImGui::Separator();
            ImGui::Dummy(ImVec2(0.0f, 4.0f));
            ImGui::Text("A modern C++ 2D graphing desktop application.");
            ImGui::Text("Engine: Marching Squares + Bisection Refinement");
            ImGui::Text("GUI & Graphics: Dear ImGui + OpenGL 3.3 + GLFW");
            ImGui::Text("License: MIT");
            ImGui::Dummy(ImVec2(0.0f, 6.0f));
            ImGui::Separator();
            ImGui::Dummy(ImVec2(0.0f, 6.0f));

            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);
            if (ImGui::Button("Close", ImVec2(120.0f, 28.0f))) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::PopStyleVar();
            ImGui::EndPopup();
        }
        ImGui::PopStyleVar(2);
    }

} // namespace GraphLab::UI
