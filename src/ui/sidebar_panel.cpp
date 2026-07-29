#include "sidebar_panel.h"

namespace GraphLab::UI {
    SidebarPanel::SidebarPanel() {}

    void SidebarPanel::OnRenderUI(){
        ImGuiIO& io = ImGui::GetIO();
        float menu_bar_height = ImGui::GetFrameHeight();
        float sidebar_width = 340.0f;
        float sidebar_height = io.DisplaySize.y - menu_bar_height;
        ImGui::SetNextWindowSize(ImVec2(sidebar_width, sidebar_height));
        ImGui::SetNextWindowPos(ImVec2(0, menu_bar_height));
        ImGuiWindowFlags flags = ImGuiWindowFlags_NoMove | 
                                 ImGuiWindowFlags_NoResize |
                                 ImGuiWindowFlags_NoCollapse |
                                 ImGuiWindowFlags_NoSavedSettings;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.0f, 10.0f));

        ImGui::Begin("Expressions", nullptr, flags);
        ImGui::PopStyleVar(3);
        
        if (ImGui::Button("+ Add Item", ImVec2(120.0f, 24.0f)))
            AddExpression();

        ImGui::Separator();
        ImGui::Dummy(ImVec2(0.0f, 5.0f));

        ImGui::BeginChild("##item", ImVec2(0, 0), false);

        int id_to_remove = -1;
        for (size_t i = 0; i < m_Expressions.size(); ++i) {
            auto& exp = m_Expressions[i];
            ImGui::PushID(exp.id);
            ImGui::Checkbox("##visible", &exp.visible);
            ImGui::SameLine();
            ImGuiColorEditFlags color_flags = ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoAlpha;
            ImGui::ColorEdit4("##color", (float*)&exp.color, color_flags);
            ImGui::SameLine();
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 30.0f);
            ImGui::InputText("##expr", exp.expression, sizeof(exp.expression));
            ImGui::SameLine();
            if (ImGui::Button("X", ImVec2(22.0f, 22.0f)))
                id_to_remove = exp.id;
            ImGui::PopID();
        }

        if (id_to_remove != -1)
            RemoveExpression(id_to_remove);

        ImGui::EndChild();
        ImGui::End();
    }

    void SidebarPanel::AddExpression(const std::string& expr) {
        Expression exp;
        exp.id = m_NextId++;
        snprintf(exp.expression, sizeof(exp.expression), "%s", expr.c_str());
        exp.visible = true;

        float r = 0.3f + (float)rand() / (float)RAND_MAX * 0.7f;
        float g = 0.3f + (float)rand() / (float)RAND_MAX * 0.7f;
        float b = 0.3f + (float)rand() / (float)RAND_MAX * 0.7f;
        exp.color = ImVec4(r, g, b, 1.0f);
        m_Expressions.push_back(exp);
    }

    void SidebarPanel::RemoveExpression(int id) {
        for (auto it = m_Expressions.begin(); it != m_Expressions.end(); ++it) {
            if (it->id == id) {
                m_Expressions.erase(it);
                break;
            }
        }
    }
}
