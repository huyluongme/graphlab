#include "sidebar_panel.h"
#include "ui/icons.h"
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cctype>
#include <algorithm>
#include <cmath>

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
        AddExpression();
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
        
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f, 5.0f));

        if (ImGui::Button(ICON_FA_PLUS "  Add Item"))
            AddExpression();

        ImGui::SameLine();
        float headerAvailW = ImGui::GetContentRegionAvail().x;
        float clearBtnW = ImGui::CalcTextSize(ICON_FA_TRASH_CAN "  Clear All").x + ImGui::GetStyle().FramePadding.x * 2.0f;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + std::max(0.0f, headerAvailW - clearBtnW));

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.20f, 0.22f, 0.28f, 0.70f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.75f, 0.20f, 0.20f, 0.85f));
        if (ImGui::Button(ICON_FA_TRASH_CAN "  Clear All")) {
            ClearAll();
        }
        ImGui::PopStyleColor(2);
        ImGui::PopStyleVar(2);

        ImGui::Separator();
        ImGui::Dummy(ImVec2(0.0f, 5.0f));

        ImGui::BeginChild("##item", ImVec2(0, 0), false);

        // Synchronize parameters and animations before rendering
        SyncParametersToExpressions();

        int id_to_remove = -1;
        std::unordered_map<std::string, bool> renderedParamsThisFrame;

        for (size_t i = 0; i < m_Expressions.size(); ++i) {
            auto& exp = m_Expressions[i];
            ImGui::PushID(exp.id);

            // Function Header Card - Standardized Frame Dimension & Spacing
            float btnDim = 28.0f;
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6.0f, 5.0f));
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6.0f, 4.0f));

            // Eye Icon Visibility Toggle Button
            ImGui::PushStyleColor(ImGuiCol_Button, exp.visible ? ImVec4(0.20f, 0.45f, 0.85f, 0.85f) : ImVec4(0.20f, 0.22f, 0.28f, 0.60f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, exp.visible ? ImVec4(0.25f, 0.55f, 0.95f, 1.00f) : ImVec4(0.28f, 0.30f, 0.38f, 0.80f));
            if (ImGui::Button(exp.visible ? ICON_FA_EYE : ICON_FA_EYE_SLASH, ImVec2(btnDim, btnDim))) {
                exp.visible = !exp.visible;
            }
            ImGui::PopStyleColor(2);
            
            // Color Picker Square
            ImGui::SameLine();
            ImGui::PushItemWidth(btnDim);
            ImGuiColorEditFlags color_flags = ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoAlpha;
            ImGui::ColorEdit4("##color", (float*)&exp.color, color_flags);
            ImGui::PopItemWidth();

            // Expression Input Text Box
            ImGui::SameLine();
            float availForInput = ImGui::GetContentRegionAvail().x - (btnDim + 6.0f);
            ImGui::SetNextItemWidth(std::max(40.0f, availForInput));
            ImGuiInputTextFlags inputFlags = ImGuiInputTextFlags_CallbackEdit;
            if (ImGui::InputText("##expr", exp.name, sizeof(exp.name), inputFlags, MathInputTextCallback)) {
                // Auto-beautify ASCII input (e.g. x^10 -> x¹⁰, pi -> π, * -> ·)
                std::string beautified = BeautifyMathInput(exp.name);
                snprintf(exp.name, sizeof(exp.name), "%s", beautified.c_str());
                exp.Recompile();
            }

            // Delete Button [X]
            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.20f, 0.22f, 0.28f, 0.60f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.80f, 0.20f, 0.20f, 0.90f));
            if (ImGui::Button(ICON_FA_XMARK, ImVec2(btnDim, btnDim)))
                id_to_remove = exp.id;
            ImGui::PopStyleColor(2);

            ImGui::PopStyleVar(3);

            // Evaluation Result Pill (Desmos style: = -0.598...) if function evaluates to a valid constant/value
            if (!exp.isPoint && exp.visible && exp.evaluator.IsValid() && !exp.evaluator.IsImplicit()) {
                double evalResult = exp.evaluator.Evaluate(0.0);
                if (std::isfinite(evalResult)) {
                    char resBuf[64];
                    std::snprintf(resBuf, sizeof(resBuf), "= %.6g", evalResult);
                    ImVec2 txtSize = ImGui::CalcTextSize(resBuf);
                    float rightOffset = std::max(10.0f, ImGui::GetWindowWidth() - txtSize.x - 35.0f);
                    ImGui::SetCursorPosX(rightOffset);
                    ImGui::TextDisabled("%s", resBuf);
                }
            }

            // Render 2D Point controls (Label: B & Lines) if expression is a Point
            if (exp.isPoint) {
                ImGui::Dummy(ImVec2(0.0f, 2.0f));
                ImGui::Indent(28.0f);

                ImGui::Checkbox("Label:", &exp.showLabel);
                ImGui::SameLine();
                ImGui::SetNextItemWidth(90.0f);
                ImGui::InputText("##labeltxt", exp.labelText, sizeof(exp.labelText));

                ImGui::SameLine();
                ImGui::Dummy(ImVec2(10.0f, 0.0f));
                ImGui::SameLine();

                ImGui::Checkbox("Lines", &exp.connectLine);

                ImGui::Unindent(28.0f);
            }

            // Render Desmos-style Parameter Slider Cards right underneath this expression card
            if (exp.visible && exp.evaluator.IsValid() && !exp.evaluator.GetParamNames().empty()) {
                for (const auto& pName : exp.evaluator.GetParamNames()) {
                    if (renderedParamsThisFrame[pName]) continue;
                    renderedParamsThisFrame[pName] = true;

                    auto& state = m_GlobalParams[pName];
                    ImGui::PushID(pName.c_str());

                    ImGui::Dummy(ImVec2(0.0f, 4.0f));

                    // Desmos Parameter Card Container Box
                    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 8.0f);
                    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.0f, 8.0f));
                    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.12f, 0.14f, 0.20f, 0.75f));
                    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.22f, 0.27f, 0.38f, 0.60f));

                    ImGui::BeginChild(pName.c_str(), ImVec2(0.0f, 72.0f), true, ImGuiWindowFlags_NoScrollbar);

                    // Row 1: Animation Play/Pause button + Title "a = -2.50" + Reset button
                    const char* playIcon = state.isPlaying ? ICON_FA_PAUSE : ICON_FA_PLAY;

                    float pBtnDim = 26.0f;
                    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);
                    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 0.0f));
                    ImGui::PushStyleColor(ImGuiCol_Button, state.isPlaying ? ImVec4(0.85f, 0.45f, 0.15f, 0.90f) : ImVec4(0.20f, 0.45f, 0.85f, 0.85f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, state.isPlaying ? ImVec4(0.95f, 0.55f, 0.20f, 1.00f) : ImVec4(0.25f, 0.55f, 0.95f, 1.00f));
                    if (ImGui::Button(playIcon, ImVec2(pBtnDim, pBtnDim))) {
                        state.isPlaying = !state.isPlaying;
                    }
                    ImGui::PopStyleColor(2);
                    ImGui::PopStyleVar(2);

                    ImGui::SameLine(0.0f, 8.0f);
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("%s = %.2f", pName.c_str(), state.value);

                    // Reset button aligned flush right (matching 26x26 squarish button)
                    float resetAvail = ImGui::GetContentRegionAvail().x - pBtnDim;
                    ImGui::SameLine(ImGui::GetCursorPosX() + std::max(0.0f, resetAvail));

                    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);
                    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 0.0f));
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.20f, 0.22f, 0.28f, 0.60f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.80f, 0.20f, 0.20f, 0.90f));
                    if (ImGui::Button(ICON_FA_ROTATE_LEFT, ImVec2(pBtnDim, pBtnDim))) {
                        state.value = 1.0;
                        state.minVal = -10.0f;
                        state.maxVal = 10.0f;
                        state.isPlaying = false;
                    }
                    ImGui::PopStyleColor(2);
                    ImGui::PopStyleVar(2);

                    // Row 2: Desmos Clean Slider Track flanked by Min & Max bounds
                    ImGui::Dummy(ImVec2(0.0f, 4.0f));

                    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);
                    ImGui::PushStyleVar(ImGuiStyleVar_GrabRounding, 5.0f);
                    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6.0f, 0.0f));

                    // Min Limit Input
                    ImGui::SetNextItemWidth(40.0f);
                    if (ImGui::InputFloat("##min", &state.minVal, 0.0f, 0.0f, "%.0f")) {
                        if (state.minVal >= state.maxVal) state.minVal = state.maxVal - 1.0f;
                    }
                    ImGui::SameLine();

                    // Slider Track
                    float valFloat = static_cast<float>(state.value);
                    float availForSlider = ImGui::GetContentRegionAvail().x - (40.0f + 6.0f);
                    ImGui::SetNextItemWidth(std::max(50.0f, availForSlider));

                    if (ImGui::SliderFloat("##slider", &valFloat, state.minVal, state.maxVal, "")) {
                        state.value = static_cast<double>(valFloat);
                    }
                    ImGui::SameLine();

                    // Max Limit Input
                    ImGui::SetNextItemWidth(40.0f);
                    if (ImGui::InputFloat("##max", &state.maxVal, 0.0f, 0.0f, "%.0f")) {
                        if (state.maxVal <= state.minVal) state.maxVal = state.minVal + 1.0f;
                    }

                    ImGui::PopStyleVar(3);

                    ImGui::EndChild();
                    ImGui::PopStyleColor(2);
                    ImGui::PopStyleVar(2);

                    ImGui::PopID();
                }
            }

            ImGui::PopID();
            ImGui::Dummy(ImVec2(0.0f, 6.0f));
        }

        if (id_to_remove != -1)
            RemoveExpression(id_to_remove);

        ImGui::EndChild();
        ImGui::End();
    }

    void SidebarPanel::SyncParametersToExpressions() {
        // 1. Collect all active parameters from expressions
        std::unordered_map<std::string, bool> activeParams;
        for (const auto& expr : m_Expressions) {
            if (!expr.visible || !expr.evaluator.IsValid()) continue;
            for (const auto& pName : expr.evaluator.GetParamNames()) {
                activeParams[pName] = true;
            }
        }

        // 2. Initialize newly discovered parameters
        for (const auto& [pName, _] : activeParams) {
            if (m_GlobalParams.find(pName) == m_GlobalParams.end()) {
                m_GlobalParams[pName] = ParamState{ 1.0, -10.0f, 10.0f, false, 2.0f };
            }
        }

        // 3. Update parameter Ping-Pong animations (oscillate smoothly between minVal and maxVal)
        float dt = ImGui::GetIO().DeltaTime;
        for (auto& [pName, state] : m_GlobalParams) {
            if (state.isPlaying) {
                if (state.animDirection == 0) state.animDirection = 1;

                state.value += static_cast<double>(dt * state.animSpeed * state.animDirection);

                if (state.value >= state.maxVal) {
                    state.value = state.maxVal;
                    state.animDirection = -1; // Reverse direction to decreasing
                } else if (state.value <= state.minVal) {
                    state.value = state.minVal;
                    state.animDirection = 1; // Reverse direction to increasing
                }
            }
        }

        // 4. Sync parameter values ONLY to expressions that actually use them
        for (auto& expr : m_Expressions) {
            for (const auto& pName : expr.evaluator.GetParamNames()) {
                auto it = m_GlobalParams.find(pName);
                if (it != m_GlobalParams.end()) {
                    expr.evaluator.SetParam(pName, it->second.value);
                }
            }
        }
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

    void SidebarPanel::ClearAll() {
        m_Expressions.clear();
        m_GlobalParams.clear();
        m_NextId = 1;
    }
}
