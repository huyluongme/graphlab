#include "sidebar_panel.h"
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cctype>

namespace GraphLab::UI {

    static std::string ToSuperscriptDigit(char c) {
        switch (c) {
            case '0': return "⁰";
            case '1': return "¹";
            case '2': return "²";
            case '3': return "³";
            case '4': return "⁴";
            case '5': return "⁵";
            case '6': return "⁶";
            case '7': return "⁷";
            case '8': return "⁸";
            case '9': return "⁹";
            default: return "";
        }
    }

    static std::string ToSuperscriptSymbol(char c) {
        switch (c) {
            case '+': return "⁺";
            case '-': return "⁻";
            case 'n': return "ⁿ";
            case 'x': return "ˣ";
            case 'y': return "ʸ";
            default: return "";
        }
    }

    static bool IsSuperscriptUTF8(const std::string& str, size_t pos, size_t& outByteLen) {
        if (pos >= str.length()) return false;
        unsigned char c = (unsigned char)str[pos];

        if (c == 0xC2 && pos + 1 < str.length()) {
            unsigned char c2 = (unsigned char)str[pos + 1];
            if (c2 == 0xB2 || c2 == 0xB3 || c2 == 0xB9) { // ², ³, ¹
                outByteLen = 2;
                return true;
            }
        }
        else if (c == 0xE2 && pos + 2 < str.length()) {
            unsigned char c2 = (unsigned char)str[pos + 1];
            unsigned char c3 = (unsigned char)str[pos + 2];
            if (c2 == 0x81) {
                if (c3 == 0xB0 || (c3 >= 0xB4 && c3 <= 0xBF)) { // ⁰, ⁴..⁹, ⁺, ⁻, ⁿ
                    outByteLen = 3;
                    return true;
                }
            }
        }
        return false;
    }

    static std::string BeautifyMathInput(const std::string& input) {
        std::string out;
        size_t i = 0;
        size_t len = input.length();
        bool inSuperscript = false;

        while (i < len) {
            size_t supLen = 0;
            if (IsSuperscriptUTF8(input, i, supLen)) {
                out += input.substr(i, supLen);
                i += supLen;
                inSuperscript = true;
                continue;
            }

            if (input[i] == '^') {
                if (i + 1 < len) {
                    std::string supDigit = ToSuperscriptDigit(input[i + 1]);
                    if (!supDigit.empty()) {
                        out += supDigit;
                        i += 2;
                        inSuperscript = true;
                        continue;
                    }
                    std::string supSym = ToSuperscriptSymbol(input[i + 1]);
                    if (!supSym.empty()) {
                        out += supSym;
                        i += 2;
                        inSuperscript = true;
                        continue;
                    }
                }
                out += '^';
                inSuperscript = false;
                i++;
                continue;
            }

            if (inSuperscript) {
                std::string sup = ToSuperscriptDigit(input[i]);
                if (!sup.empty()) {
                    out += sup;
                    i++;
                    continue;
                } else {
                    inSuperscript = false;
                }
            }

            if (input.compare(i, 4, "sqrt") == 0) {
                bool isWordBefore = (i > 0 && std::isalpha((unsigned char)input[i - 1]));
                if (!isWordBefore) {
                    out += "√";
                    i += 4;
                    inSuperscript = false;
                    continue;
                }
            }
            else if (input.compare(i, 4, "cbrt") == 0) {
                bool isWordBefore = (i > 0 && std::isalpha((unsigned char)input[i - 1]));
                if (!isWordBefore) {
                    out += "³√";
                    i += 4;
                    inSuperscript = false;
                    continue;
                }
            }
            else if (input.compare(i, 2, "pi") == 0) {
                bool isWordBefore = (i > 0 && std::isalpha((unsigned char)input[i - 1]));
                bool isWordAfter = (i + 2 < len && std::isalpha((unsigned char)input[i + 2]));
                if (!isWordBefore && !isWordAfter) {
                    out += "π";
                    i += 2;
                    inSuperscript = false;
                    continue;
                }
            }
            else if (input[i] == '*') {
                out += "·";
                i += 1;
                inSuperscript = false;
                continue;
            }

            out += input[i];
            inSuperscript = false;
            i++;
        }

        return out;
    }

    static int MathInputTextCallback(ImGuiInputTextCallbackData* data) {
        if (data->EventFlag == ImGuiInputTextFlags_CallbackEdit) {
            std::string currentText(data->Buf, data->BufTextLen);
            std::string beautified = BeautifyMathInput(currentText);
            if (beautified != currentText && beautified.length() < (size_t)data->BufSize) {
                data->DeleteChars(0, data->BufTextLen);
                data->InsertChars(0, beautified.c_str());
            }
        }
        return 0;
    }

    SidebarPanel::SidebarPanel() {
        AddExpression("(x² + y² - 1)³ - x² · y³");
    }

    void SidebarPanel::OnRenderUI() {
        ImGuiIO& io = ImGui::GetIO();
        float menu_bar_height = ImGui::GetFrameHeight();
        float sidebar_width = m_PanelWidth;
        float sidebar_height = io.DisplaySize.y - menu_bar_height;

        ImGui::SetNextWindowSize(ImVec2(sidebar_width, sidebar_height));
        ImGui::SetNextWindowPos(ImVec2(0.0f, menu_bar_height));

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
            ImGuiInputTextFlags inputFlags = ImGuiInputTextFlags_CallbackEdit;
            if (ImGui::InputText("##expr", exp.name, sizeof(exp.name), inputFlags, MathInputTextCallback)) {
                // Auto-beautify ASCII input (e.g. x^10 -> x¹⁰, pi -> π, * -> ·)
                std::string beautified = BeautifyMathInput(exp.name);
                snprintf(exp.name, sizeof(exp.name), "%s", beautified.c_str());
                exp.Recompile();
            }

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
        float r = 0.3f + (float)rand() / (float)RAND_MAX * 0.7f;
        float g = 0.3f + (float)rand() / (float)RAND_MAX * 0.7f;
        float b = 0.3f + (float)rand() / (float)RAND_MAX * 0.7f;
        ImVec4 color = ImVec4(r, g, b, 1.0f);

        Math::Expression exp(m_NextId++, expr, color);
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
