#include "graph_canvas.h"
#include "core/app.h"
#include "imgui.h"

#include <cmath>
#include <algorithm>
#include <vector>
#include <deque>
#include <functional>
#include <unordered_map>

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

        // Render mathematical expressions dynamically from Sidebar
        DrawExpressions(drawList, canvasPos, canvasSize, originScreen);

        // Update and render interactive key points and hover trace overlay
        UpdateAndDrawKeyPointsAndTrace(drawList, canvasPos, canvasSize, originScreen);

        // Render UI controls
        ImGui::SetCursorPos(ImVec2(15.0f, 15.0f));
        ImGui::BeginGroup();

        if (ImGui::Button("Reset View (1:1)"))
            ResetView();

        ImGui::SameLine();
        ImGui::Checkbox("Key Points", &m_EnableKeyPoints);
        ImGui::SameLine();
        ImGui::Checkbox("Trace Hover", &m_EnableTraceMode);

        ImGui::TextDisabled("FPS: %.1f", io.Framerate);
        ImGui::TextDisabled("Frame: %.2f ms", 1000.0f / std::max(io.Framerate, 1.0f));
        ImGui::TextDisabled("Zoom: %.1f", m_Zoom);
        ImGui::TextDisabled("Pan: (%.2f, %.2f)", m_PanOffset.x, m_PanOffset.y);

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
            m_LastInteractionTime = static_cast<float>(ImGui::GetTime());
        }

        // Handle mouse click for panning
        if (isHovered && (ImGui::IsMouseClicked(ImGuiMouseButton_Left)
            || (ImGui::IsMouseClicked(ImGuiMouseButton_Middle) && !ImGui::IsAnyItemActive()))) {
            m_IsDragging = true;
            m_DragStartMouse = mousePos;
            m_DragStartPan = m_PanOffset;
            m_LastInteractionTime = static_cast<float>(ImGui::GetTime());
        }

        // Handle mouse release for panning
        if (m_IsDragging) {
            m_LastInteractionTime = static_cast<float>(ImGui::GetTime());
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

    /**
     * @brief Loops over visible equations in SidebarPanel and renders each one.
     */
    void GraphCanvas::DrawExpressions(ImDrawList* drawList, ImVec2 canvasPos, ImVec2 canvasSize, ImVec2 originScreen) {
        const auto& expressions = App::Get().GetSidebarPanel().GetExpressions();

        // 1. Draw curves (explicit and implicit functions)
        for (const auto& exp : expressions) {
            if (!exp.visible || !exp.evaluator.IsValid() || exp.isPoint)
                continue;

            ImU32 color = ImGui::ColorConvertFloat4ToU32(exp.color);

            if (exp.evaluator.IsEquation() || exp.evaluator.IsImplicit()) {
                DrawImplicitFunction(drawList, exp.evaluator, color, canvasPos, canvasSize, originScreen);
            } else {
                DrawExplicitFunction(drawList, exp.evaluator, color, canvasPos, canvasSize, originScreen);
            }
        }

        // 2. Draw 2D Points, Custom Labels, and Connecting Lines (Desmos Style)
        for (const auto& exp : expressions) {
            if (!exp.visible || !exp.isPoint || exp.pointPairs.empty()) continue;

            ImU32 color = ImGui::ColorConvertFloat4ToU32(exp.color);
            std::vector<ImVec2> itemScreenPoints;
            itemScreenPoints.reserve(exp.pointPairs.size());

            // Evaluate all points in this expression item
            for (const auto& pair : exp.pointPairs) {
                double wx = pair.evalX.Evaluate(0.0);
                double wy = pair.evalY.Evaluate(0.0);
                if (std::isfinite(wx) && std::isfinite(wy)) {
                    itemScreenPoints.push_back(WorldToScreen(ImVec2(static_cast<float>(wx), static_cast<float>(wy)), originScreen));
                }
            }

            // Draw connecting line / polyline between points ON THIS ITEM CARD if connectLine is true
            if (exp.connectLine && itemScreenPoints.size() >= 2) {
                drawList->AddPolyline(
                    itemScreenPoints.data(),
                    static_cast<int>(itemScreenPoints.size()),
                    color,
                    ImDrawFlags_None,
                    2.0f
                );
            }

            // Draw point dot markers and labels
            for (size_t pIdx = 0; pIdx < itemScreenPoints.size(); ++pIdx) {
                ImVec2 screenPt = itemScreenPoints[pIdx];

                if (screenPt.x >= canvasPos.x && screenPt.x <= canvasPos.x + canvasSize.x &&
                    screenPt.y >= canvasPos.y && screenPt.y <= canvasPos.y + canvasSize.y) {

                    // Desmos-style Point Marker: Solid colored outer ring + white inner dot core
                    drawList->AddCircleFilled(screenPt, 7.0f, color);
                    drawList->AddCircleFilled(screenPt, 3.0f, IM_COL32(255, 255, 255, 255));
                    drawList->AddCircle(screenPt, 7.0f, IM_COL32(20, 20, 25, 200), 0, 1.5f);

                    // Render Point Label adjacent to point if showLabel is enabled
                    if (exp.showLabel) {
                        char labelBuf[128];
                        if (exp.labelText[0] != '\0') {
                            if (exp.pointPairs.size() == 1) {
                                snprintf(labelBuf, sizeof(labelBuf), "%s", exp.labelText);
                            } else {
                                snprintf(labelBuf, sizeof(labelBuf), "%s%d", exp.labelText, static_cast<int>(pIdx + 1));
                            }
                        } else {
                            double wx = exp.pointPairs[pIdx].evalX.Evaluate(0.0);
                            double wy = exp.pointPairs[pIdx].evalY.Evaluate(0.0);
                            snprintf(labelBuf, sizeof(labelBuf), "(%.3g, %.3g)", wx, wy);
                        }

                        ImVec2 txtPos = ImVec2(screenPt.x + 10.0f, screenPt.y - 14.0f);
                        ImVec2 txtSize = ImGui::CalcTextSize(labelBuf);

                        // Subtle background shadow pill for text readability
                        drawList->AddRectFilled(
                            ImVec2(txtPos.x - 3.0f, txtPos.y - 2.0f),
                            ImVec2(txtPos.x + txtSize.x + 3.0f, txtPos.y + txtSize.y + 2.0f),
                            IM_COL32(20, 24, 32, 210),
                            4.0f
                        );
                        drawList->AddText(txtPos, IM_COL32(245, 245, 255, 255), labelBuf);
                    }
                }
            }
        }
    }

    /**
     * @brief Renders 1D explicit function y = f(x) by sampling horizontal screen columns with sub-pixel domain bisection and asymptote handling.
     */
    void GraphCanvas::DrawExplicitFunction(ImDrawList* drawList, const Math::Evaluator& evaluator, ImU32 color, ImVec2 canvasPos, ImVec2 canvasSize, ImVec2 originScreen) {
        float startX = canvasPos.x;
        float endX = canvasPos.x + canvasSize.x;
        float step = 1.0f; // 1 pixel sampling step

        float minY = canvasPos.y - 200.0f;
        float maxY = canvasPos.y + canvasSize.y + 200.0f;

        auto sampleWorld = [&](float sx, float& outClampedY) -> bool {
            float wx = (sx - originScreen.x) / m_Zoom;
            double wy = evaluator.Evaluate(wx);
            if (std::isnan(wy) || std::isinf(wy)) return false;
            float sy = originScreen.y - (float)wy * m_Zoom;
            outClampedY = std::clamp(sy, minY, maxY);
            return true;
        };

        std::vector<ImVec2> points;
        points.reserve((size_t)(endX - startX) + 10);
        bool wasValid = false;

        for (float screenX = startX; screenX <= endX; screenX += step) {
            float clampedY = 0.0f;
            bool isValid = sampleWorld(screenX, clampedY);

            if (isValid && !wasValid) {
                // Sub-pixel bisection refinement for domain entry (e.g. x -> 0+ for log(x)^2 or sqrt(x))
                float leftX = screenX - step;
                float rightX = screenX;
                float boundaryY = clampedY;

                for (int iter = 0; iter < 10; ++iter) {
                    float midX = (leftX + rightX) * 0.5f;
                    float midY = 0.0f;
                    if (sampleWorld(midX, midY)) {
                        rightX = midX;
                        boundaryY = midY;
                    } else {
                        leftX = midX;
                    }
                }
                points.push_back(ImVec2(rightX, boundaryY));
            }
            else if (!isValid && wasValid) {
                // Sub-pixel bisection refinement for domain exit
                float leftX = screenX - step;
                float rightX = screenX;
                float boundaryY = points.back().y;

                for (int iter = 0; iter < 10; ++iter) {
                    float midX = (leftX + rightX) * 0.5f;
                    float midY = 0.0f;
                    if (sampleWorld(midX, midY)) {
                        leftX = midX;
                        boundaryY = midY;
                    } else {
                        rightX = midX;
                    }
                }
                points.push_back(ImVec2(leftX, boundaryY));
                if (points.size() > 1) {
                    drawList->AddPolyline(points.data(), (int)points.size(), color, 0, 2.5f);
                }
                points.clear();
                wasValid = false;
                continue;
            }

            if (!isValid) {
                wasValid = false;
                continue;
            }

            if (!points.empty()) {
                float lastY = points.back().y;

                // Detect true vertical asymptote jump (+inf to -inf or -inf to +inf)
                bool isAsymptoteJump = (lastY <= minY + 10.0f && clampedY >= maxY - 10.0f) ||
                                       (lastY >= maxY - 10.0f && clampedY <= minY + 10.0f);

                if (isAsymptoteJump) {
                    if (points.size() > 1) {
                        drawList->AddPolyline(points.data(), (int)points.size(), color, 0, 2.5f);
                    }
                    points.clear();
                }
            }

            points.push_back(ImVec2(screenX, clampedY));
            wasValid = true;
        }

        if (points.size() > 1) {
            drawList->AddPolyline(points.data(), (int)points.size(), color, 0, 2.5f);
        }
    }


    /**
     * @brief Renders 2D implicit function F(x, y) = 0 using a 3-Layer Architecture:
     *  1. Evaluator: Evaluates F(x,y) for grid vertices exactly once.
     *  2. ContourSampler: Origin-anchored grid + 4-iteration bisection refinement + EdgeKey topology.
     *  3. GraphRenderer: World-coordinate render cache + ImGui AddPolyline with smooth miter joins.
     */
    void GraphCanvas::DrawImplicitFunction(ImDrawList* drawList, const Math::Evaluator& evaluator, ImU32 color, ImVec2 canvasPos, ImVec2 canvasSize, ImVec2 originScreen) {

        const std::string& exprStr = evaluator.GetExpression();
        auto& cache = m_ImplicitCaches[exprStr];

        float now = static_cast<float>(ImGui::GetTime());
        bool interacting = m_IsDragging || (now - m_LastInteractionTime < 0.15f);

        bool needsRebuild = !cache.valid
                         || (!interacting && (cache.sampledZoom != m_Zoom || cache.sampledCanvasSize.x != canvasSize.x || cache.sampledCanvasSize.y != canvasSize.y));

        if (needsRebuild) {
            constexpr float cellSize = 2.0f;

            // 1. Anchor Grid to world coordinate origin (originScreen)
            float gridStartX = originScreen.x + std::floor((canvasPos.x - originScreen.x) / cellSize) * cellSize;
            float gridStartY = originScreen.y + std::floor((canvasPos.y - originScreen.y) / cellSize) * cellSize;

            int cols = static_cast<int>(std::ceil((canvasPos.x + canvasSize.x - gridStartX) / cellSize)) + 1;
            int rows = static_cast<int>(std::ceil((canvasPos.y + canvasSize.y - gridStartY) / cellSize)) + 1;

            if (cols >= 2 && rows >= 2) {
                // 2. Evaluate grid vertices EXACTLY ONCE
                std::vector<double> values(static_cast<size_t>(cols) * rows);
                auto ValueAt = [&](int x, int y) -> double& {
                    return values[static_cast<size_t>(y) * cols + x];
                };

                for (int y = 0; y < rows; ++y) {
                    float screenY = gridStartY + static_cast<float>(y) * cellSize;
                    double worldY = (originScreen.y - screenY) / m_Zoom;
                    for (int x = 0; x < cols; ++x) {
                        float screenX = gridStartX + static_cast<float>(x) * cellSize;
                        double worldX = (screenX - originScreen.x) / m_Zoom;
                        ValueAt(x, y) = evaluator.Evaluate(worldX, worldY);
                    }
                }

                // 3. Root refinement: Linear interpolation initial guess + 4 bisection iterations
                auto RefineZero = [&](ImVec2 pA, ImVec2 pB, double fA, double fB) -> ImVec2 {
                    if (!std::isfinite(fA) || !std::isfinite(fB))
                        return ImVec2((pA.x + pB.x) * 0.5f, (pA.y + pB.y) * 0.5f);

                    ImVec2 a = pA, b = pB;
                    double fa = fA, fb = fB;

                    double denominator = fa - fb;
                    double t = 0.5;
                    if (std::abs(denominator) > 1e-15)
                        t = std::clamp(fa / denominator, 0.0, 1.0);

                    ImVec2 estimate(
                        a.x + static_cast<float>(t) * (b.x - a.x),
                        a.y + static_cast<float>(t) * (b.y - a.y)
                    );

                    for (int iter = 0; iter < 4; ++iter) {
                        ImVec2 midpoint((a.x + b.x) * 0.5f, (a.y + b.y) * 0.5f);
                        double worldX = (midpoint.x - originScreen.x) / m_Zoom;
                        double worldY = (originScreen.y - midpoint.y) / m_Zoom;
                        double fm = evaluator.Evaluate(worldX, worldY);

                        if (!std::isfinite(fm)) break;

                        if (std::signbit(fa) == std::signbit(fm)) {
                            a = midpoint;
                            fa = fm;
                        } else {
                            b = midpoint;
                            fb = fm;
                        }
                    }

                    ImVec2 refined((a.x + b.x) * 0.5f, (a.y + b.y) * 0.5f);
                    float intervalLength = std::hypot(b.x - a.x, b.y - a.y);
                    return intervalLength < 0.4f ? refined : estimate;
                };

                // 4. EdgeKey topology
                struct EdgeKey {
                    int gx, gy, orient;
                    bool operator==(const EdgeKey& o) const {
                        return gx == o.gx && gy == o.gy && orient == o.orient;
                    }
                };

                struct EdgeKeyHash {
                    std::size_t operator()(const EdgeKey& k) const {
                        return std::hash<uint64_t>()(((uint64_t)(uint32_t)k.gy * 8192ull + (uint32_t)k.gx) | ((uint64_t)k.orient << 32));
                    }
                };

                std::unordered_map<EdgeKey, int, EdgeKeyHash> edgeToNode;
                std::vector<ImVec2> nodeWorldPos;
                std::vector<std::vector<int>> adj;
                edgeToNode.reserve(4000);
                nodeWorldPos.reserve(4000);
                adj.reserve(4000);

                auto GetNode = [&](EdgeKey key, ImVec2 pA, ImVec2 pB, double fA, double fB) -> int {
                    auto it = edgeToNode.find(key);
                    if (it != edgeToNode.end()) return it->second;
                    int id = static_cast<int>(nodeWorldPos.size());
                    ImVec2 screenPt = RefineZero(pA, pB, fA, fB);
                    ImVec2 worldPt = ScreenToWorld(screenPt, originScreen);
                    nodeWorldPos.push_back(worldPt);
                    adj.push_back({});
                    edgeToNode[key] = id;
                    return id;
                };

                auto AddEdge = [&](int nA, int nB) {
                    if (nA == nB) return;
                    if (std::find(adj[nA].begin(), adj[nA].end(), nB) == adj[nA].end()) {
                        adj[nA].push_back(nB);
                        adj[nB].push_back(nA);
                    }
                };

                struct EdgePair { int edgeA, edgeB; };
                static constexpr EdgePair cases[16][2] = {
                    {{-1, -1}, {-1, -1}}, // 0
                    {{ 3,  0}, {-1, -1}}, // 1
                    {{ 0,  1}, {-1, -1}}, // 2
                    {{ 3,  1}, {-1, -1}}, // 3
                    {{ 1,  2}, {-1, -1}}, // 4
                    {{-1, -1}, {-1, -1}}, // 5
                    {{ 0,  2}, {-1, -1}}, // 6
                    {{ 3,  2}, {-1, -1}}, // 7
                    {{ 2,  3}, {-1, -1}}, // 8
                    {{ 2,  0}, {-1, -1}}, // 9
                    {{-1, -1}, {-1, -1}}, // 10
                    {{ 2,  1}, {-1, -1}}, // 11
                    {{ 1,  3}, {-1, -1}}, // 12
                    {{ 1,  0}, {-1, -1}}, // 13
                    {{ 0,  3}, {-1, -1}}, // 14
                    {{-1, -1}, {-1, -1}}  // 15
                };

                for (int y = 0; y < rows - 1; ++y) {
                    float qy = gridStartY + static_cast<float>(y) * cellSize;
                    for (int x = 0; x < cols - 1; ++x) {
                        float qx = gridStartX + static_cast<float>(x) * cellSize;

                        double fBL = ValueAt(x,     y + 1);
                        double fBR = ValueAt(x + 1, y + 1);
                        double fTR = ValueAt(x + 1, y    );
                        double fTL = ValueAt(x,     y    );

                        int ms = 0;
                        if (!std::signbit(fBL)) ms |= 1;
                        if (!std::signbit(fBR)) ms |= 2;
                        if (!std::signbit(fTR)) ms |= 4;
                        if (!std::signbit(fTL)) ms |= 8;

                        if (ms == 0 || ms == 15) continue;

                        ImVec2 pBL = { qx,            qy + cellSize };
                        ImVec2 pBR = { qx + cellSize, qy + cellSize };
                        ImVec2 pTR = { qx + cellSize, qy            };
                        ImVec2 pTL = { qx,            qy            };

                        EdgeKey kBot = { x,     y + 1, 0 };
                        EdgeKey kTop = { x,     y,     0 };
                        EdgeKey kLft = { x,     y,     1 };
                        EdgeKey kRgt = { x + 1, y,     1 };

                        auto nBot = [&]{ return GetNode(kBot, pBL, pBR, fBL, fBR); };
                        auto nTop = [&]{ return GetNode(kTop, pTL, pTR, fTL, fTR); };
                        auto nLft = [&]{ return GetNode(kLft, pTL, pBL, fTL, fBL); };
                        auto nRgt = [&]{ return GetNode(kRgt, pTR, pBR, fTR, fBR); };

                        auto GetNodeByEdge = [&](int edge) -> int {
                            switch (edge) {
                                case 0: return nBot();
                                case 1: return nRgt();
                                case 2: return nTop();
                                case 3: return nLft();
                                default: return nBot();
                            }
                        };

                        auto AddSegmentByEdges = [&](int eA, int eB) {
                            AddEdge(GetNodeByEdge(eA), GetNodeByEdge(eB));
                        };

                        if (ms == 5) {
                            double det = fBL * fTR - fBR * fTL;
                            if (det > 0.0) {
                                AddSegmentByEdges(3, 2);
                                AddSegmentByEdges(0, 1);
                            } else {
                                AddSegmentByEdges(3, 0);
                                AddSegmentByEdges(1, 2);
                            }
                        } else if (ms == 10) {
                            double det = fBL * fTR - fBR * fTL;
                            if (det > 0.0) {
                                AddSegmentByEdges(3, 0);
                                AddSegmentByEdges(1, 2);
                            } else {
                                AddSegmentByEdges(3, 2);
                                AddSegmentByEdges(0, 1);
                            }
                        } else {
                            const auto& p = cases[ms][0];
                            if (p.edgeA != -1 && p.edgeB != -1) {
                                AddSegmentByEdges(p.edgeA, p.edgeB);
                            }
                        }
                    }
                }

                // 5. Build CachedPolylines with consecutive duplicate filtering
                cache.polylines.clear();
                std::vector<bool> visited(nodeWorldPos.size(), false);

                for (int start = 0; start < static_cast<int>(nodeWorldPos.size()); ++start) {
                    if (visited[start] || adj[start].empty()) continue;

                    std::deque<int> chain;
                    chain.push_back(start);
                    visited[start] = true;

                    auto grow = [&](bool forward) {
                        while (true) {
                            int tip = forward ? chain.back() : chain.front();
                            int next = -1;
                            for (int nb : adj[tip]) {
                                if (!visited[nb]) { next = nb; break; }
                            }
                            if (next == -1) break;
                            visited[next] = true;
                            if (forward) chain.push_back(next); else chain.push_front(next);
                        }
                    };

                    grow(true);
                    grow(false);

                    if (chain.size() < 2) continue;

                    bool isClosed = false;
                    for (int nb : adj[chain.back()]) {
                        if (nb == chain.front()) {
                            isClosed = true;
                            break;
                        }
                    }

                    CachedPolyline cPoly;
                    cPoly.closed = isClosed;
                    cPoly.worldPoints.reserve(chain.size());

                    constexpr float minDistanceSquared = 0.01f; // Filter sub-pixel duplicates

                    for (int id : chain) {
                        const ImVec2& wPt = nodeWorldPos[id];
                        if (!cPoly.worldPoints.empty()) {
                            ImVec2 lastScreen = WorldToScreen(cPoly.worldPoints.back(), originScreen);
                            ImVec2 currScreen = WorldToScreen(wPt, originScreen);
                            float dx = currScreen.x - lastScreen.x;
                            float dy = currScreen.y - lastScreen.y;
                            if (dx * dx + dy * dy < minDistanceSquared)
                                continue;
                        }
                        cPoly.worldPoints.push_back(wPt);
                    }

                    if (cPoly.worldPoints.size() >= 2) {
                        cache.polylines.push_back(std::move(cPoly));
                    }
                }

                cache.sampledZoom = m_Zoom;
                cache.sampledCanvasSize = canvasSize;
                cache.valid = true;
            }
        }

        // 6. Render cached polylines by transforming worldPoints to screen coordinates
        std::vector<ImVec2> screenPoints;
        for (const auto& poly : cache.polylines) {
            if (poly.worldPoints.size() < 2) continue;

            screenPoints.clear();
            screenPoints.reserve(poly.worldPoints.size());

            for (const auto& wPt : poly.worldPoints) {
                screenPoints.push_back(WorldToScreen(wPt, originScreen));
            }

            drawList->AddPolyline(
                screenPoints.data(),
                static_cast<int>(screenPoints.size()),
                color,
                poly.closed ? ImDrawFlags_Closed : ImDrawFlags_None,
                2.0f
            );
        }
    }

    void GraphCanvas::RenderDesmosTooltip(
        ImDrawList* drawList,
        ImVec2 screenPos,
        const std::string& title,
        double xVal,
        double yVal,
        ImU32 accentColor
    ) {
        char buf[128];
        if (title.empty()) {
            std::snprintf(buf, sizeof(buf), "(%.3f, %.3f)", xVal, yVal);
        } else {
            std::snprintf(buf, sizeof(buf), "%s\n(%.3f, %.3f)", title.c_str(), xVal, yVal);
        }

        ImVec2 textSize = ImGui::CalcTextSize(buf);
        float padding = 8.0f;
        ImVec2 boxSize = ImVec2(textSize.x + padding * 2.0f, textSize.y + padding * 2.0f);

        ImVec2 boxMin = ImVec2(screenPos.x - boxSize.x * 0.5f, screenPos.y - boxSize.y - 12.0f);
        ImVec2 boxMax = ImVec2(boxMin.x + boxSize.x, boxMin.y + boxSize.y);

        // Draw sleek dark background with rounded corners and border accent
        drawList->AddRectFilled(boxMin, boxMax, IM_COL32(20, 24, 32, 230), 6.0f);
        drawList->AddRect(boxMin, boxMax, accentColor, 6.0f, 0, 1.5f);

        ImVec2 textPos = ImVec2(boxMin.x + padding, boxMin.y + padding);
        drawList->AddText(textPos, IM_COL32(240, 245, 255, 255), buf);
    }

    void GraphCanvas::UpdateAndDrawKeyPointsAndTrace(
        ImDrawList* drawList,
        ImVec2 canvasPos,
        ImVec2 canvasSize,
        ImVec2 originScreen
    ) {
        const auto& expressions = App::Get().GetSidebarPanel().GetExpressions();
        if (expressions.empty()) return;

        ImVec2 worldTL = ScreenToWorld(canvasPos, originScreen);
        ImVec2 worldBR = ScreenToWorld(ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y), originScreen);
        double xMin = std::min(static_cast<double>(worldTL.x), static_cast<double>(worldBR.x));
        double xMax = std::max(static_cast<double>(worldTL.x), static_cast<double>(worldBR.x));

        // 1. Calculate Key Points inside visible range
        if (m_EnableKeyPoints) {
            m_CachedKeyPoints = Math::Analysis::FindKeyPoints(expressions, xMin, xMax, 250);
        }

        ImGuiIO& io = ImGui::GetIO();
        ImVec2 mousePos = io.MousePos;
        bool isCanvasHovered = (
            mousePos.x >= canvasPos.x && mousePos.x <= canvasPos.x + canvasSize.x &&
            mousePos.y >= canvasPos.y && mousePos.y <= canvasPos.y + canvasSize.y
        );

        std::optional<Math::KeyPoint> hoveredKeyPoint;
        float minKeyPointDist = 12.0f;

        if (m_EnableKeyPoints && isCanvasHovered) {
            for (const auto& kp : m_CachedKeyPoints) {
                ImVec2 screenPt = WorldToScreen(ImVec2(static_cast<float>(kp.x), static_cast<float>(kp.y)), originScreen);
                float dx = screenPt.x - mousePos.x;
                float dy = screenPt.y - mousePos.y;
                float dist = std::sqrt(dx * dx + dy * dy);
                if (dist < minKeyPointDist) {
                    minKeyPointDist = dist;
                    hoveredKeyPoint = kp;
                }
            }
        }

        // Handle left click to pin/unpin point inspection
        if (isCanvasHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !m_IsDragging) {
            if (hoveredKeyPoint.has_value()) {
                m_PinnedPoint = hoveredKeyPoint;
            } else {
                m_PinnedPoint = std::nullopt;
            }
        }

        // 2. Draw Key Points markers
        if (m_EnableKeyPoints) {
            for (const auto& kp : m_CachedKeyPoints) {
                ImVec2 screenPt = WorldToScreen(ImVec2(static_cast<float>(kp.x), static_cast<float>(kp.y)), originScreen);
                if (screenPt.x < canvasPos.x || screenPt.x > canvasPos.x + canvasSize.x ||
                    screenPt.y < canvasPos.y || screenPt.y > canvasPos.y + canvasSize.y) {
                    continue;
                }

                bool isHovered = hoveredKeyPoint.has_value() &&
                                 std::abs(hoveredKeyPoint->x - kp.x) < 1e-4 &&
                                 std::abs(hoveredKeyPoint->y - kp.y) < 1e-4;

                ImU32 accentCol = IM_COL32(180, 180, 180, 220);
                if (kp.type == Math::KeyPointType::Root) accentCol = IM_COL32(220, 220, 220, 240);
                else if (kp.type == Math::KeyPointType::YIntercept) accentCol = IM_COL32(100, 200, 255, 240);
                else if (kp.type == Math::KeyPointType::LocalMax || kp.type == Math::KeyPointType::LocalMin) accentCol = IM_COL32(255, 200, 80, 240);
                else if (kp.type == Math::KeyPointType::Intersection) accentCol = IM_COL32(255, 100, 200, 240);

                float rOuter = isHovered ? 7.0f : 5.0f;
                float rInner = isHovered ? 4.0f : 3.0f;

                drawList->AddCircleFilled(screenPt, rOuter, IM_COL32(20, 20, 25, 200));
                drawList->AddCircleFilled(screenPt, rInner, accentCol);
                drawList->AddCircle(screenPt, rOuter, accentCol, 0, 1.5f);

                if (isHovered) {
                    RenderDesmosTooltip(drawList, screenPt, kp.label, kp.x, kp.y, accentCol);
                }
            }
        }

        // 3. Hover Trace Mode (if no keypoint is hovered)
        if (m_EnableTraceMode && isCanvasHovered && !hoveredKeyPoint.has_value() && !m_IsDragging) {
            ImVec2 mouseWorld = ScreenToWorld(mousePos, originScreen);
            double bestDist = 25.0; // pixel distance threshold
            ImVec2 bestTraceScreen;
            double bestWorldX = 0.0, bestWorldY = 0.0;
            std::string bestExprLabel;
            ImU32 bestColor = IM_COL32(255, 255, 255, 255);
            bool foundTrace = false;

            for (const auto& expr : expressions) {
                if (!expr.visible || !expr.evaluator.IsValid() || expr.evaluator.IsImplicit()) continue;

                double yVal = expr.evaluator.Evaluate(mouseWorld.x);
                if (std::isfinite(yVal)) {
                    ImVec2 screenPt = WorldToScreen(ImVec2(mouseWorld.x, static_cast<float>(yVal)), originScreen);
                    float dx = screenPt.x - mousePos.x;
                    float dy = screenPt.y - mousePos.y;
                    double dist = std::sqrt(dx * dx + dy * dy);

                    if (dist < bestDist) {
                        bestDist = dist;
                        bestTraceScreen = screenPt;
                        bestWorldX = mouseWorld.x;
                        bestWorldY = yVal;
                        bestExprLabel = expr.name;
                        bestColor = ImGui::ColorConvertFloat4ToU32(expr.color);
                        foundTrace = true;
                    }
                }
            }

            if (foundTrace) {
                drawList->AddCircleFilled(bestTraceScreen, 6.0f, IM_COL32(20, 20, 25, 200));
                drawList->AddCircleFilled(bestTraceScreen, 4.0f, bestColor);
                drawList->AddCircle(bestTraceScreen, 6.0f, bestColor, 0, 2.0f);

                RenderDesmosTooltip(drawList, bestTraceScreen, bestExprLabel, bestWorldX, bestWorldY, bestColor);
            }
        }

        // 4. Render Pinned Point if active
        if (m_PinnedPoint.has_value()) {
            ImVec2 screenPt = WorldToScreen(ImVec2(static_cast<float>(m_PinnedPoint->x), static_cast<float>(m_PinnedPoint->y)), originScreen);
            drawList->AddCircleFilled(screenPt, 8.0f, IM_COL32(255, 255, 255, 255));
            drawList->AddCircle(screenPt, 8.0f, IM_COL32(0, 120, 255, 255), 0, 2.5f);
            RenderDesmosTooltip(drawList, screenPt, m_PinnedPoint->label + " (Pinned)", m_PinnedPoint->x, m_PinnedPoint->y, IM_COL32(0, 120, 255, 255));
        }
    }
}

