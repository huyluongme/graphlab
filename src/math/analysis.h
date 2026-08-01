#pragma once

#include "math/evaluator.h"
#include "math/expression.h"
#include <vector>
#include <string>
#include <optional>

namespace GraphLab::Math {

    enum class KeyPointType {
        Root,           // x-intercept: f(x) = 0
        YIntercept,     // y-intercept: f(0)
        LocalMin,       // Local minimum: f'(x) = 0, f''(x) > 0
        LocalMax,       // Local maximum: f'(x) = 0, f''(x) < 0
        Intersection    // Intersection between two functions: f1(x) = f2(x)
    };

    struct KeyPoint {
        double x = 0.0;
        double y = 0.0;
        KeyPointType type = KeyPointType::Root;
        int expressionId = -1;
        int secondaryExpressionId = -1;
        std::string label;

        KeyPoint() = default;
        KeyPoint(double px, double py, KeyPointType t, int expId, int secExpId = -1, std::string lbl = "")
            : x(px), y(py), type(t), expressionId(expId), secondaryExpressionId(secExpId), label(std::move(lbl)) {}
    };

    class Analysis {
    public:
        /**
         * @brief Finds all key points (roots, y-intercept, extrema, intersections) within the given x range.
         * @param expressions Active list of expressions.
         * @param xMin Minimum world X boundary.
         * @param xMax Maximum world X boundary.
         * @param samples Number of search sub-intervals across [xMin, xMax].
         * @return Vector of key points found.
         */
        static std::vector<KeyPoint> FindKeyPoints(
            const std::vector<Expression>& expressions,
            double xMin,
            double xMax,
            int samples = 300
        );

        /**
         * @brief Refines root of f(x) = 0 in bracket [a, b] using bisection method.
         */
        static std::optional<double> RefineRoot(
            const Evaluator& eval,
            double a,
            double b,
            double tol = 1e-6,
            int maxIter = 25
        );

        /**
         * @brief Refines extremum of f(x) in bracket [a, b] using Golden Section Search or derivative bisection.
         */
        static std::optional<double> RefineExtremum(
            const Evaluator& eval,
            double a,
            double b,
            bool isMax,
            double tol = 1e-5
        );

        /**
         * @brief Evaluates numerical derivative f'(x) using central difference.
         */
        static double Derivative(const Evaluator& eval, double x, double h = 1e-4);
    };

}
