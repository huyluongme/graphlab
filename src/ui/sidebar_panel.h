#pragma once

#include "imgui.h"
#include <string>
#include <vector>

namespace GraphLab::UI {
    struct Expression {
        int id;
        char expression[256];
        bool visible;
        ImVec4 color;
    };

    class SidebarPanel {
    public:
        SidebarPanel();
        ~SidebarPanel() = default;

        void OnRenderUI();
        const std::vector<Expression>& GetExpressions() const { return m_Expressions; }
        void AddExpression(const std::string& expr = "");
        void RemoveExpression(int id);

    private:
        std::vector<Expression> m_Expressions;
        int m_NextId = 1;
    };
}
