#include "math/analysis.h"
#include <cmath>
#include <algorithm>

namespace GraphLab::Math {

    double Analysis::Derivative(const Evaluator& eval, double x, double h) {
        double fPlus = eval.Evaluate(x + h);
        double fMinus = eval.Evaluate(x - h);

        if (!std::isfinite(fPlus) || !std::isfinite(fMinus)) {
            return std::numeric_limits<double>::quiet_NaN();
        }

        return (fPlus - fMinus) / (2.0 * h);
    }

    std::optional<double> Analysis::RefineRoot(
        const Evaluator& eval,
        double a,
        double b,
        double tol,
        int maxIter
    ) {
        double fa = eval.Evaluate(a);
        double fb = eval.Evaluate(b);

        if (!std::isfinite(fa) || !std::isfinite(fb)) return std::nullopt;
        if (fa * fb > 0.0) return std::nullopt;

        if (std::abs(fa) < tol) return a;
        if (std::abs(fb) < tol) return b;

        for (int i = 0; i < maxIter; ++i) {
            double m = 0.5 * (a + b);
            double fm = eval.Evaluate(m);

            if (!std::isfinite(fm)) return std::nullopt;
            if (std::abs(b - a) < tol || std::abs(fm) < 1e-9) {
                return m;
            }

            if (fa * fm < 0.0) {
                b = m;
                fb = fm;
            } else {
                a = m;
                fa = fm;
            }
        }
        return 0.5 * (a + b);
    }

    std::optional<double> Analysis::RefineExtremum(
        const Evaluator& eval,
        double a,
        double b,
        bool isMax,
        double tol
    ) {
        const double phi = 0.618033988749895; // Golden ratio (sqrt(5) - 1) / 2
        double c = b - phi * (b - a);
        double d = a + phi * (b - a);

        double fc = eval.Evaluate(c);
        double fd = eval.Evaluate(d);

        if (!std::isfinite(fc) || !std::isfinite(fd)) return std::nullopt;

        int iterations = 0;
        while (std::abs(b - a) > tol && iterations < 30) {
            iterations++;
            if (isMax) {
                if (fc > fd) {
                    b = d;
                    d = c;
                    fd = fc;
                    c = b - phi * (b - a);
                    fc = eval.Evaluate(c);
                } else {
                    a = c;
                    c = d;
                    fc = fd;
                    d = a + phi * (b - a);
                    fd = eval.Evaluate(d);
                }
            } else {
                if (fc < fd) {
                    b = d;
                    d = c;
                    fd = fc;
                    c = b - phi * (b - a);
                    fc = eval.Evaluate(c);
                } else {
                    a = c;
                    c = d;
                    fc = fd;
                    d = a + phi * (b - a);
                    fd = eval.Evaluate(d);
                }
            }
            if (!std::isfinite(fc) || !std::isfinite(fd)) break;
        }

        double optX = 0.5 * (a + b);
        double optY = eval.Evaluate(optX);
        if (std::isfinite(optY)) return optX;

        return std::nullopt;
    }

    std::vector<KeyPoint> Analysis::FindKeyPoints(
        const std::vector<Expression>& expressions,
        double xMin,
        double xMax,
        int samples
    ) {
        std::vector<KeyPoint> keyPoints;
        if (xMin >= xMax || samples <= 0) return keyPoints;

        double dx = (xMax - xMin) / static_cast<double>(samples);

        // Filter valid explicit expressions
        std::vector<const Expression*> validExprs;
        for (const auto& expr : expressions) {
            if (expr.visible && expr.evaluator.IsValid() && !expr.evaluator.IsImplicit()) {
                validExprs.push_back(&expr);
            }
        }

        // 1. Single function key points (Y-Intercept, Roots, Extrema)
        for (const auto* pExpr : validExprs) {
            const auto& eval = pExpr->evaluator;
            int exprId = pExpr->id;

            // Y-Intercept
            if (xMin <= 0.0 && xMax >= 0.0) {
                double y0 = eval.Evaluate(0.0);
                if (std::isfinite(y0)) {
                    keyPoints.emplace_back(0.0, y0, KeyPointType::YIntercept, exprId, -1, "y-intercept");
                }
            }

            // Roots & Extrema sampling
            for (int k = 0; k < samples; ++k) {
                double x1 = xMin + k * dx;
                double x2 = x1 + dx;

                double y1 = eval.Evaluate(x1);
                double y2 = eval.Evaluate(x2);

                if (!std::isfinite(y1) || !std::isfinite(y2)) continue;

                // Root (x-intercept)
                if (y1 * y2 <= 0.0 && (y1 != 0.0 || y2 != 0.0)) {
                    auto rootX = RefineRoot(eval, x1, x2);
                    if (rootX.has_value()) {
                        keyPoints.emplace_back(rootX.value(), 0.0, KeyPointType::Root, exprId, -1, "root");
                    }
                }

                // Extrema
                double d1 = Derivative(eval, x1);
                double d2 = Derivative(eval, x2);

                if (std::isfinite(d1) && std::isfinite(d2) && d1 * d2 < 0.0) {
                    if (d1 > 0.0 && d2 < 0.0) { // Maxima
                        auto maxX = RefineExtremum(eval, x1, x2, true);
                        if (maxX.has_value()) {
                            double maxY = eval.Evaluate(maxX.value());
                            if (std::isfinite(maxY)) {
                                keyPoints.emplace_back(maxX.value(), maxY, KeyPointType::LocalMax, exprId, -1, "maximum");
                            }
                        }
                    } else if (d1 < 0.0 && d2 > 0.0) { // Minima
                        auto minX = RefineExtremum(eval, x1, x2, false);
                        if (minX.has_value()) {
                            double minY = eval.Evaluate(minX.value());
                            if (std::isfinite(minY)) {
                                keyPoints.emplace_back(minX.value(), minY, KeyPointType::LocalMin, exprId, -1, "minimum");
                            }
                        }
                    }
                }
            }
        }

        // 2. Intersections between pairs of functions
        for (size_t i = 0; i < validExprs.size(); ++i) {
            for (size_t j = i + 1; j < validExprs.size(); ++j) {
                const auto& eval1 = validExprs[i]->evaluator;
                const auto& eval2 = validExprs[j]->evaluator;

                auto diffEval = [&](double x) -> double {
                    return eval1.Evaluate(x) - eval2.Evaluate(x);
                };

                for (int k = 0; k < samples; ++k) {
                    double x1 = xMin + k * dx;
                    double x2 = x1 + dx;

                    double g1 = diffEval(x1);
                    double g2 = diffEval(x2);

                    if (!std::isfinite(g1) || !std::isfinite(g2)) continue;

                    if (g1 * g2 <= 0.0 && (g1 != 0.0 || g2 != 0.0)) {
                        // Refine root of difference
                        double a = x1, b = x2;
                        double ga = g1;
                        bool found = false;
                        double ix = 0.5 * (a + b);

                        for (int iter = 0; iter < 20; ++iter) {
                            ix = 0.5 * (a + b);
                            double gi = diffEval(ix);
                            if (!std::isfinite(gi)) break;
                            if (std::abs(b - a) < 1e-6 || std::abs(gi) < 1e-9) {
                                found = true;
                                break;
                            }
                            if (ga * gi < 0.0) {
                                b = ix;
                            } else {
                                a = ix;
                                ga = gi;
                            }
                        }

                        if (found) {
                            double iy = eval1.Evaluate(ix);
                            if (std::isfinite(iy)) {
                                keyPoints.emplace_back(
                                    ix, iy, KeyPointType::Intersection,
                                    validExprs[i]->id, validExprs[j]->id, "intersection"
                                );
                            }
                        }
                    }
                }
            }
        }

        // 3. Deduplicate key points that are too close (within 1e-3 distance)
        std::vector<KeyPoint> uniquePoints;
        for (const auto& pt : keyPoints) {
            bool isDuplicate = false;
            for (const auto& uPt : uniquePoints) {
                if (pt.type == uPt.type &&
                    std::abs(pt.x - uPt.x) < 1e-3 &&
                    std::abs(pt.y - uPt.y) < 1e-3) {
                    isDuplicate = true;
                    break;
                }
            }
            if (!isDuplicate) {
                uniquePoints.push_back(pt);
            }
        }

        return uniquePoints;
    }

} // namespace GraphLab::Math
