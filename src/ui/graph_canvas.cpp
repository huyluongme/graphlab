#include "graph_canvas.h"
#include "core/app.h"
#include "imgui.h"

#include <cmath>
#include <algorithm>

namespace GraphLab::UI {
    GraphCanvas::GraphCanvas() {}

    /**
     * @brief Renders the graph canvas UI.
     * 
     * This method draws the 2D graph canvas with a grid, axes, and zoom/pan functionality.
     */
    void GraphCanvas::OnRenderUI() {
        ImGuiIO& io = ImGui::GetIO();
        float menu_bar_height = ImGui::GetFrameHeight();
        float sidebar_width = App::Get().GetSidebarPanel().GetPanelWidth();
        ImVec2 canvasPos = ImVec2(sidebar_width, menu_bar_height);
        ImVec2 canvasSize = ImVec2(io.DisplaySize.x - sidebar_width, io.DisplaySize.y - menu_bar_height);

        ImGui::SetNextWindowPos(canvasPos);
        ImGui::SetNextWindowSize(canvasSize);

        ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration 
                               | ImGuiWindowFlags_NoMove 
                               | ImGuiWindowFlags_NoResize 
                               | ImGuiWindowFlags_NoSavedSettings
                               | ImGuiWindowFlags_NoBringToFrontOnFocus;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

        ImGui::Begin("Graph Canvas", nullptr, flags);
        ImGui::PopStyleVar(3);

        // Handle user input (zooming and panning)
        HandleInput(canvasPos, canvasSize);

        // Calculate origin position (center of canvas in screen coordinates)
        ImVec2 originScreen = ImVec2(
            canvasPos.x + canvasSize.x * 0.5f + m_PanOffset.x,
            canvasPos.y + canvasSize.y * 0.5f + m_PanOffset.y
        );

        ImDrawList* drawList = ImGui::GetWindowDrawList();

        // Draw the grid and axes
        DrawGridAndAxes(drawList, canvasPos, canvasSize, originScreen);

        // Render UI controls
        ImGui::SetCursorPos(ImVec2(15.0f, 15.0f));
        ImGui::BeginGroup();
        ImGui::TextDisabled("Zoom: %.1f px/unit | Scroll: Zoom | Drag: Pan", m_Zoom);
        if (ImGui::Button("Reset View (1:1)"))
            ResetView();

        ImGui::EndGroup();
        ImGui::End();
    }

    void GraphCanvas::ResetView() {
        m_PanOffset = ImVec2(0.0f, 0.0f);
        m_Zoom = 50.0f;
    }

    /**
     * @brief Converts world coordinates to screen coordinates.
     * @param world The world coordinates to convert.
     * @param originScreen The screen coordinates of the origin.
     * @return The screen coordinates of the world coordinates.
     */
    ImVec2 GraphCanvas::WorldToScreen(ImVec2 world, ImVec2 originScreen) const {
        return ImVec2(
            originScreen.x + world.x * m_Zoom,
            originScreen.y - world.y * m_Zoom
        );
    }

    /**
     * @brief Converts screen coordinates to world coordinates.
     * @param screen The screen coordinates to convert.
     * @param originScreen The screen coordinates of the origin.
     * @return The world coordinates of the screen coordinates.
     */
    ImVec2 GraphCanvas::ScreenToWorld(ImVec2 screen, ImVec2 originScreen) const {
        return ImVec2(
            (screen.x - originScreen.x) / m_Zoom,
            (originScreen.y - screen.y) / m_Zoom
        );
    }

    /**
     * @brief Handles user input for the graph canvas.
     * @param canvasPos The position of the canvas.
     * @param canvasSize The size of the canvas.
     */
    void GraphCanvas::HandleInput(ImVec2 canvasPos, ImVec2 canvasSize) {
        ImGuiIO& io = ImGui::GetIO();
        ImVec2 mousePos = io.MousePos;

        // Check if the mouse is hovering over the canvas
        bool isHovered = (
            mousePos.x >= canvasPos.x && mousePos.x <= canvasPos.x + canvasSize.x &&
            mousePos.y >= canvasPos.y && mousePos.y <= canvasPos.y + canvasSize.y
        );

        // Handle mouse wheel for zooming
        if (isHovered && io.MouseWheel != 0.0f) {
            float zoomFactor = (io.MouseWheel > 0.0f) ? 1.15f : (1.0f / 1.15f);
            m_Zoom = std::clamp(m_Zoom * zoomFactor, 5.0f, 1000.0f);
        }

        // Handle mouse click for panning
        if (isHovered && (ImGui::IsMouseClicked(ImGuiMouseButton_Left)
            || (ImGui::IsMouseClicked(ImGuiMouseButton_Middle) && !ImGui::IsAnyItemActive()))) {
            m_IsDragging = true;
            m_DragStartMouse = mousePos;
            m_DragStartPan = m_PanOffset;
        }

        // Handle mouse release for panning
        if (m_IsDragging) {
            if (ImGui::IsMouseDown(ImGuiMouseButton_Left)
                || ImGui::IsMouseDown(ImGuiMouseButton_Middle)) {
                ImVec2 delta = ImVec2(
                    mousePos.x - m_DragStartMouse.x,
                    mousePos.y - m_DragStartMouse.y
                );
                m_PanOffset = ImVec2(
                    m_DragStartPan.x + delta.x,
                    m_DragStartPan.y + delta.y
                );
            }
            else
                m_IsDragging = false;
        }
    }

    /**
     * @brief Draws the grid and axes on the graph canvas.
     * @param drawList The draw list to draw on.
     * @param canvasPos The position of the canvas.
     * @param canvasSize The size of the canvas.
     * @param originScreen The screen coordinates of the origin.
     */
    void GraphCanvas::DrawGridAndAxes(ImDrawList* drawList, ImVec2 canvasPos, ImVec2 canvasSize, ImVec2 originScreen) {
        ImU32 gridColor = IM_COL32(50, 50, 60, 200);
        ImU32 axisColor = IM_COL32(200, 200, 220, 255);
        ImU32 textColor = IM_COL32(160, 160, 180, 255);

        // Calculate grid step
        float stepWorld = 1.0f;
        while (stepWorld * m_Zoom < 40.0f) stepWorld *= 2.0f;
        while (stepWorld * m_Zoom > 120.0f) stepWorld /= 2.0f;
        float stepPixels = stepWorld * m_Zoom;

        // Draw Vertical Grid Lines
        float startX = std::floor((canvasPos.x - originScreen.x) / stepPixels) * stepPixels + originScreen.x;
        for (float x = startX; x <= canvasPos.x + canvasSize.x; x += stepPixels) {
            if (x >= canvasPos.x) {
                drawList->AddLine(ImVec2(x, canvasPos.y), ImVec2(x, canvasPos.y + canvasSize.y), gridColor, 1.0f);

                // Draw X Axis Numbers
                float worldX = (x - originScreen.x) / m_Zoom;
                if (std::abs(worldX) > 0.001f) {
                    char buf[16];
                    snprintf(buf, sizeof(buf), "%.1f", worldX);
                    drawList->AddText(ImVec2(x + 3.0f, std::clamp(originScreen.y + 3.0f, canvasPos.y, canvasPos.y + canvasSize.y - 20.0f)), textColor, buf);
                }
            }
        }

        // Draw Horizontal Grid Lines
        float startY = std::floor((canvasPos.y - originScreen.y) / stepPixels) * stepPixels + originScreen.y;
        for (float y = startY; y <= canvasPos.y + canvasSize.y; y += stepPixels) {
            if (y >= canvasPos.y) {
                drawList->AddLine(ImVec2(canvasPos.x, y), ImVec2(canvasPos.x + canvasSize.x, y), gridColor, 1.0f);

                // Draw Y Axis Numbers
                float worldY = (originScreen.y - y) / m_Zoom;
                if (std::abs(worldY) > 0.001f) {
                    char buf[16];
                    snprintf(buf, sizeof(buf), "%.1f", worldY);
                    drawList->AddText(ImVec2(std::clamp(originScreen.x + 4.0f, canvasPos.x + 4.0f, canvasPos.x + canvasSize.x - 40.0f), y - 14.0f), textColor, buf);
                }
            }
        }

        // Draw X and Y Axes
        if (originScreen.x >= canvasPos.x && originScreen.x <= canvasPos.x + canvasSize.x)
            drawList->AddLine(ImVec2(originScreen.x, canvasPos.y), ImVec2(originScreen.x, canvasPos.y + canvasSize.y), axisColor, 1.0f);

        if (originScreen.y >= canvasPos.y && originScreen.y <= canvasPos.y + canvasSize.y)
            drawList->AddLine(ImVec2(canvasPos.x, originScreen.y), ImVec2(canvasPos.x + canvasSize.x, originScreen.y), axisColor, 1.0f);

        // Draw Origin Label (0)
        drawList->AddText(ImVec2(originScreen.x + 5.0f, originScreen.y + 5.0f), textColor, "0");
    }
}
