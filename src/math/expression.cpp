#include "expression.h"
#include <cstdio>
#include <cstring>
#include <algorithm>

namespace GraphLab::Math {
    Expression::Expression(int id, const std::string& exprStr, const ImVec4& col)
        : id(id), visible(true), color(col) {
        snprintf(name, sizeof(name), "%s", exprStr.c_str());
        Recompile();
    }

    bool Expression::Recompile() {
        pointPairs.clear();
        isPoint = false;

        std::string str(name);
        size_t first = str.find_first_not_of(" \t\r\n");
        size_t last = str.find_last_not_of(" \t\r\n");

        if (first == std::string::npos || last == std::string::npos) {
            return evaluator.Parse(name);
        }

        std::string trimmed = str.substr(first, last - first + 1);

        // Split top-level tuple chunks separated by commas outside parentheses: (1,2),(3,4)
        std::vector<std::string> tupleChunks;
        std::string currentChunk;
        int parenDepth = 0;

        for (size_t i = 0; i < trimmed.length(); ++i) {
            char c = trimmed[i];
            if (c == '(') {
                parenDepth++;
                currentChunk += c;
            } else if (c == ')') {
                parenDepth = std::max(0, parenDepth - 1);
                currentChunk += c;
            } else if (c == ',' && parenDepth == 0) {
                if (!currentChunk.empty()) {
                    tupleChunks.push_back(currentChunk);
                    currentChunk.clear();
                }
            } else {
                currentChunk += c;
            }
        }
        if (!currentChunk.empty()) {
            tupleChunks.push_back(currentChunk);
        }

        // Try parsing each chunk as a 2D point tuple (x_expr, y_expr)
        for (auto& chunk : tupleChunks) {
            // Trim spaces in chunk
            size_t cFirst = chunk.find_first_not_of(" \t\r\n");
            size_t cLast = chunk.find_last_not_of(" \t\r\n");
            if (cFirst == std::string::npos || cLast == std::string::npos) continue;
            std::string cTrimmed = chunk.substr(cFirst, cLast - cFirst + 1);

            if (cTrimmed.length() >= 3 && cTrimmed.front() == '(' && cTrimmed.back() == ')') {
                std::string inner = cTrimmed.substr(1, cTrimmed.length() - 2);

                int innerParenDepth = 0;
                size_t commaPos = std::string::npos;
                for (size_t i = 0; i < inner.length(); ++i) {
                    if (inner[i] == '(') innerParenDepth++;
                    else if (inner[i] == ')') innerParenDepth = std::max(0, innerParenDepth - 1);
                    else if (inner[i] == ',' && innerParenDepth == 0) {
                        commaPos = i;
                        break;
                    }
                }

                if (commaPos != std::string::npos) {
                    std::string leftStr = inner.substr(0, commaPos);
                    std::string rightStr = inner.substr(commaPos + 1);

                    PointEvaluatorPair pair;
                    bool okX = pair.evalX.Parse(leftStr);
                    bool okY = pair.evalY.Parse(rightStr);

                    if (okX && okY) {
                        pointPairs.push_back(std::move(pair));
                    }
                }
            }
        }

        if (!pointPairs.empty()) {
            isPoint = true;
            return true;
        }

        isPoint = false;
        return evaluator.Parse(name);
    }
}
