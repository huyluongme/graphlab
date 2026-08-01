#pragma once

#include "evaluator.h"
#include "imgui.h"
#include <string>
#include <vector>

namespace GraphLab::Math {

    struct PointEvaluatorPair {
        Evaluator evalX;
        Evaluator evalY;
    };

    /**
     * @brief Represents a mathematical function or 2D point list expression item in GraphLab.
     */
    struct Expression {
        int id = 0;
        char name[256] = "";                                // Buffer for equation text input (e.g. "sin(x)" or "(1,2),(3,4)")
        bool visible = true;                                // Render visibility toggle
        ImVec4 color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);      // Curve/Point color
        Evaluator evaluator;                                // Primary math evaluator parser

        // 2D Point Extension (Desmos Style - supports list of points e.g. (1,2),(3,4))
        bool isPoint = false;
        std::vector<PointEvaluatorPair> pointPairs;
        bool showLabel = false;
        char labelText[64] = "";
        bool connectLine = false;                           // Connect lines between points on this item card

        Expression() = default;
        Expression(int id, const std::string& exprStr, const ImVec4& col);

        /**
         * @brief Recompiles the expression text into RPN format or 2D Point evaluators.
         * @return True if parsing succeeded without syntax errors.
         */
        bool Recompile();
    };

}
