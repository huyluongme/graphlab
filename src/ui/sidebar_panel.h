#pragma once

#include "imgui.h"
#include "math/expression.h"
#include <string>
#include <vector>
#include <unordered_map>

namespace GraphLab::UI {

    struct ParamState {
        double value = 1.0;
        float minVal = -10.0f;
        float maxVal = 10.0f;
        bool isPlaying = false;
        float animSpeed = 2.0f;
        int animDirection = 1; // 1 = increasing (+), -1 = decreasing (-)
    };

    class SidebarPanel {
    public:
        SidebarPanel();
        ~SidebarPanel() = default;

        void OnRenderUI();
        const std::vector<Math::Expression>& GetExpressions() const { return m_Expressions; }
        std::vector<Math::Expression>& GetExpressions() { return m_Expressions; }
        void AddExpression(const std::string& expr = "");
        void RemoveExpression(int id);
        void ClearAll();
        float GetPanelWidth() const { return m_PanelWidth; }

        const std::unordered_map<std::string, ParamState>& GetGlobalParams() const { return m_GlobalParams; }
        std::unordered_map<std::string, ParamState>& GetGlobalParams() { return m_GlobalParams; }

    private:
        void SyncParametersToExpressions();

    private:
        std::vector<Math::Expression> m_Expressions;
        std::unordered_map<std::string, ParamState> m_GlobalParams;
        int m_NextId = 1;
        float m_PanelWidth = 320.0f;
    };

}
