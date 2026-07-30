#pragma once

#include "imgui.h"
#include "math/expression.h"
#include <string>
#include <vector>

namespace GraphLab::UI {

    class SidebarPanel {
    public:
        SidebarPanel();
        ~SidebarPanel() = default;

        void OnRenderUI();
        const std::vector<Math::Expression>& GetExpressions() const { return m_Expressions; }
        void AddExpression(const std::string& expr = "");
        void RemoveExpression(int id);
        float GetPanelWidth() const { return m_PanelWidth; }

    private:
        std::vector<Math::Expression> m_Expressions;
        int m_NextId = 1;
        float m_PanelWidth = 340.0f;
    };
}
