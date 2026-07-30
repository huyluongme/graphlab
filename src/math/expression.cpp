#include "expression.h"
#include <cstdio>
#include <cstring>

namespace GraphLab::Math {
    Expression::Expression(int id, const std::string& exprStr, const ImVec4& col)
        : id(id), visible(true), color(col) {
        snprintf(name, sizeof(name), "%s", exprStr.c_str());
        Recompile();
    }

    bool Expression::Recompile() {
        return evaluator.Parse(name);
    }
}
