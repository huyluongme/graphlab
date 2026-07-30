#pragma once

#include "imgui.h"

namespace GraphLab::UI {
    /**
     * @brief 2D Graph Canvas UI class.
     */
    class GraphCanvas {
    public:
        GraphCanvas();
        ~GraphCanvas() = default;

        void OnRenderUI();

    private:
        void ResetView();
        ImVec2 WorldToScreen(ImVec2 world, ImVec2 originScreen) const;
        ImVec2 ScreenToWorld(ImVec2 screen, ImVec2 originScreen) const;
        void HandleInput(ImVec2 canvasPos, ImVec2 canvasSize);
        void DrawGridAndAxes(ImDrawList* drawList, ImVec2 canvasPos, ImVec2 canvasSize, ImVec2 originScreen);

    private:
        ImVec2 m_PanOffset = ImVec2(0.0f, 0.0f);        // Offset from center to origin
        float m_Zoom = 50.0f;                           // Pixels per world unit
        bool m_IsDragging = false;                      // Is the canvas being dragged?
        ImVec2 m_DragStartMouse = ImVec2(0.0f, 0.0f);   // Mouse position at drag start
        ImVec2 m_DragStartPan = ImVec2(0.0f, 0.0f);     // Pan offset at drag start
    };
}
