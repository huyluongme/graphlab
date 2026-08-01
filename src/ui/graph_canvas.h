#pragma once

#include "imgui.h"
#include "math/evaluator.h"
#include "math/analysis.h"
#include <vector>
#include <unordered_map>
#include <string>
#include <optional>

namespace GraphLab::UI {
    struct CachedPolyline {
        std::vector<ImVec2> worldPoints;
        bool closed = false;
    };

    struct ImplicitRenderCache {
        std::vector<CachedPolyline> polylines;
        float sampledZoom = 0.0f;
        ImVec2 sampledCanvasSize = ImVec2(0.0f, 0.0f);
        ImVec2 sampledPanOffset = ImVec2(0.0f, 0.0f);
        bool valid = false;
    };

    /**
     * @brief 2D Graph Canvas UI class.
     */
    class GraphCanvas {
    public:
        GraphCanvas();
        ~GraphCanvas() = default;

        void OnRenderUI();

        bool IsKeyPointsEnabled() const { return m_EnableKeyPoints; }
        void SetKeyPointsEnabled(bool enabled) { m_EnableKeyPoints = enabled; }

        bool IsTraceModeEnabled() const { return m_EnableTraceMode; }
        void SetTraceModeEnabled(bool enabled) { m_EnableTraceMode = enabled; }

        float GetZoom() const { return m_Zoom; }
        void SetZoom(float zoom) { m_Zoom = zoom; }

        ImVec2 GetPanOffset() const { return m_PanOffset; }
        void SetPanOffset(ImVec2 pan) { m_PanOffset = pan; }

        bool IsHideOverlayUI() const { return m_HideOverlayUI; }
        void SetHideOverlayUI(bool hide) { m_HideOverlayUI = hide; }

    private:
        void ResetView();
        ImVec2 WorldToScreen(ImVec2 world, ImVec2 originScreen) const;
        ImVec2 ScreenToWorld(ImVec2 screen, ImVec2 originScreen) const;
        void HandleInput(ImVec2 canvasPos, ImVec2 canvasSize);
        void DrawGridAndAxes(ImDrawList* drawList, ImVec2 canvasPos, ImVec2 canvasSize, ImVec2 originScreen);

        void DrawExpressions(ImDrawList* drawList, ImVec2 canvasPos, ImVec2 canvasSize, ImVec2 originScreen);
        void DrawExplicitFunction(ImDrawList* drawList, const Math::Evaluator& evaluator, ImU32 color, ImVec2 canvasPos, ImVec2 canvasSize, ImVec2 originScreen);
        void DrawImplicitFunction(ImDrawList* drawList, const Math::Evaluator& evaluator, ImU32 color, ImVec2 canvasPos, ImVec2 canvasSize, ImVec2 originScreen);

        void UpdateAndDrawKeyPointsAndTrace(ImDrawList* drawList, ImVec2 canvasPos, ImVec2 canvasSize, ImVec2 originScreen);
        void RenderDesmosTooltip(ImDrawList* drawList, ImVec2 screenPos, const std::string& title, double xVal, double yVal, ImU32 accentColor);

    private:
        ImVec2 m_PanOffset = ImVec2(0.0f, 0.0f);        // Offset from center to origin
        float m_Zoom = 50.0f;                           // Pixels per world unit
        bool m_IsDragging = false;                      // Is the canvas being dragged?
        ImVec2 m_DragStartMouse = ImVec2(0.0f, 0.0f);   // Mouse position at drag start
        ImVec2 m_DragStartPan = ImVec2(0.0f, 0.0f);     // Pan offset at drag start

        // Key Points & Value Inspector / Hover Trace State
        bool m_EnableKeyPoints = true;
        bool m_EnableTraceMode = true;
        bool m_HideOverlayUI = false;                   // Hide overlay buttons/text during clean image export
        std::vector<Math::KeyPoint> m_CachedKeyPoints;
        std::optional<Math::KeyPoint> m_PinnedPoint;

        // Render Cache & Interaction Timers
        std::unordered_map<std::string, ImplicitRenderCache> m_ImplicitCaches;
        float m_LastInteractionTime = -1000.0f;
    };
}
