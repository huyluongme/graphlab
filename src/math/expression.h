#pragma once

#include "evaluator.h"
#include "imgui.h"
#include <string>

namespace GraphLab::Math {

    /**
     * @brief Represents a mathematical function expression item in GraphLab.
     */
    struct Expression {
        int id = 0;
        char name[256] = "";                                // Buffer for equation text input (e.g. "sin(x)")
        bool visible = true;                                // Render visibility toggle
        ImVec4 color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);      // Curve color
        Evaluator evaluator;                                // Math evaluator parser for this equation

        Expression() = default;
        Expression(int id, const std::string& exprStr, const ImVec4& col);

        /**
         * @brief Recompiles the expression text into RPN format.
         * @return True if parsing succeeded without syntax errors.
         */
        bool Recompile();
    };

}
