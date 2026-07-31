#include "evaluator.h"
#include <vector>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <limits>
#include <iostream>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef M_E
#define M_E 2.71828182845904523536
#endif

namespace GraphLab::Math {

    static bool IsOperator(char c) {
        return c == '+' || c == '-' || c == '*' || c == '/' || c == '^';
    }

    static bool IsFunction(const std::string& str) {
        static const std::vector<std::string> funcs = {
            "sin", "cos", "tan", "asin", "acos", "atan",
            "sqrt", "cbrt", "abs", "exp", "ln", "log", "floor", "ceil"
        };
        return std::find(funcs.begin(), funcs.end(), str) != funcs.end();
    }

    static int GetPrecedence(char op) {
        if (op == '+' || op == '-') return 1;
        if (op == '*' || op == '/') return 2;
        if (op == '^') return 3;
        return 0;
    }

    Evaluator::Evaluator(const std::string& expression) {
        Parse(expression);
    }

    /**
     * @brief Parses an infix mathematical expression string into RPN format.
     */
    bool Evaluator::Parse(const std::string& expression) {
        m_Expression = expression;
        m_LastError.clear();
        m_IsValid = false;
        m_IsEquation = false;

        if (expression.empty()) {
            m_LastError = "Expression is empty.";
            return false;
        }

        std::string normalizedExpression = expression;
        const size_t equalPos = normalizedExpression.find('=');

        if (equalPos != std::string::npos) {
            m_IsEquation = true;

            // Reject multiple '=' characters.
            if (normalizedExpression.find('=', equalPos + 1) != std::string::npos) {
                m_LastError = "Only one '=' operator is supported.";
                return false;
            }

            std::string left = normalizedExpression.substr(0, equalPos);
            std::string right = normalizedExpression.substr(equalPos + 1);

            if (left.find_first_not_of(" \t\r\n") == std::string::npos ||
                right.find_first_not_of(" \t\r\n") == std::string::npos) {
                m_LastError = "Both sides of the equation are required.";
                return false;
            }

            normalizedExpression = "(" + left + ")-(" + right + ")";
        }

        std::vector<Token> tokens = Tokenize(normalizedExpression);
        if (tokens.empty()) {
            if (m_LastError.empty())
                m_LastError = "Failed to tokenize expression.";
            return false;
        }

        if (!ConvertToRPN(tokens)) {
            return false;
        }

        m_IsValid = true;
        return true;
    }

    /**
     * @brief Evaluates 1D explicit function y = f(x) at given x value.
     */
    double Evaluator::Evaluate(double x) const {
        return Evaluate(x, 0.0);
    }

    /**
     * @brief Evaluates 2D function F(x, y) at given (x, y) coordinate using RPN stack.
     */
    double Evaluator::Evaluate(double x, double y) const {
        if (!m_IsValid || m_RPNTokens.empty())
            return std::numeric_limits<double>::quiet_NaN();

        // Zero-Heap Allocation CPU Stack (Fast nanosecond evaluation)
        double valStack[64];
        int stackPtr = 0;

        auto push = [&](double val) {
            if (stackPtr < 64) valStack[stackPtr++] = val;
        };

        for (const auto& token : m_RPNTokens) {
            switch (token.type) {
                case TokenType::Number:
                    push(token.numberValue);
                    break;

                case TokenType::VariableX:
                    push(x);
                    break;

                case TokenType::VariableY:
                    push(y);
                    break;

                case TokenType::Operator: {
                    if (stackPtr < 1) return std::numeric_limits<double>::quiet_NaN();
                    char op = token.value[0];

                    double b = valStack[--stackPtr];
                    double a = (stackPtr > 0) ? valStack[--stackPtr] : 0.0;

                    if (op == '+')      valStack[stackPtr++] = a + b;
                    else if (op == '-') valStack[stackPtr++] = a - b;
                    else if (op == '*') valStack[stackPtr++] = a * b;
                    else if (op == '/') valStack[stackPtr++] = (b != 0.0) ? (a / b) : std::numeric_limits<double>::quiet_NaN();
                    else if (op == '^') valStack[stackPtr++] = std::pow(a, b);
                    break;
                }

                case TokenType::Function: {
                    if (stackPtr < 1) return std::numeric_limits<double>::quiet_NaN();
                    double arg = valStack[stackPtr - 1];
                    
                    const std::string& fn = token.value;
                    if (fn == "sin")        valStack[stackPtr - 1] = std::sin(arg);
                    else if (fn == "cos")   valStack[stackPtr - 1] = std::cos(arg);
                    else if (fn == "tan")   valStack[stackPtr - 1] = std::tan(arg);
                    else if (fn == "asin")  valStack[stackPtr - 1] = std::asin(arg);
                    else if (fn == "acos")  valStack[stackPtr - 1] = std::acos(arg);
                    else if (fn == "atan")  valStack[stackPtr - 1] = std::atan(arg);
                    else if (fn == "sqrt")  valStack[stackPtr - 1] = (arg >= 0.0) ? std::sqrt(arg) : std::numeric_limits<double>::quiet_NaN();
                    else if (fn == "cbrt")  valStack[stackPtr - 1] = std::cbrt(arg);
                    else if (fn == "abs")   valStack[stackPtr - 1] = std::abs(arg);
                    else if (fn == "exp")   valStack[stackPtr - 1] = std::exp(arg);
                    else if (fn == "ln")    valStack[stackPtr - 1] = (arg > 0.0) ? std::log(arg) : std::numeric_limits<double>::quiet_NaN();
                    else if (fn == "log")   valStack[stackPtr - 1] = (arg > 0.0) ? std::log10(arg) : std::numeric_limits<double>::quiet_NaN();
                    else if (fn == "floor") valStack[stackPtr - 1] = std::floor(arg);
                    else if (fn == "ceil")  valStack[stackPtr - 1] = std::ceil(arg);
                    break;
                }

                default:
                    break;
            }
        }

        return (stackPtr == 1) ? valStack[0] : std::numeric_limits<double>::quiet_NaN();
    }
    
    /**
     * @brief Lexical analyzer: converts string into a vector of tokens.
     */
    std::vector<Evaluator::Token> Evaluator::Tokenize(const std::string& expression) {
        std::vector<Token> tokens;
        size_t i = 0;
        size_t len = expression.length();
        m_IsImplicit = false;

        while (i < len) {
            char c = expression[i];

            if (std::isspace(c)) {
                i++;
                continue;
            }

            // 1. Numbers (3.14, 42, .5)
            if (std::isdigit(c) || c == '.') {
                size_t start = i;
                while (i < len && (std::isdigit(expression[i]) || expression[i] == '.')) i++;
                std::string numStr = expression.substr(start, i - start);
                tokens.emplace_back(TokenType::Number, numStr, std::stod(numStr));
            }
            // 2. Identifiers (x, y, pi, e, sin, cos...)
            else if (std::isalpha(c)) {
                size_t start = i;
                while (i < len && std::isalpha(expression[i])) i++;
                std::string id = expression.substr(start, i - start);
                std::transform(id.begin(), id.end(), id.begin(), ::tolower);

                if (id == "x")            tokens.emplace_back(TokenType::VariableX, "x");
                else if (id == "y")       { m_IsImplicit = true; tokens.emplace_back(TokenType::VariableY, "y"); }
                else if (id == "pi")      tokens.emplace_back(TokenType::Number, "pi", M_PI);
                else if (id == "e")       tokens.emplace_back(TokenType::Number, "e", M_E);
                else if (IsFunction(id))  tokens.emplace_back(TokenType::Function, id);
                else {
                    m_IsValid = false;
                    m_LastError = "Unknown identifier: '" + id + "'";
                    return {};
                }
            }
            // 3. Operators (+, -, *, /, ^)
            else if (IsOperator(c)) {
                // Handle Unary Minus (e.g., -x or (-5))
                if (c == '-' && (tokens.empty() || tokens.back().type == TokenType::Operator || tokens.back().type == TokenType::LeftParen)) {
                    tokens.emplace_back(TokenType::Number, "0", 0.0);
                }
                tokens.emplace_back(TokenType::Operator, std::string(1, c), 0.0, GetPrecedence(c), c == '^');
                i++;
            }
            // 4. Parentheses
            else if (c == '(') { tokens.emplace_back(TokenType::LeftParen, "("); i++; }
            else if (c == ')') { tokens.emplace_back(TokenType::RightParen, ")"); i++; }
            else {
                m_LastError = std::string("Unexpected character: '") + c + "'";
                return {};
            }
        }

        return tokens;
    }

    /**
     * @brief Shunting-Yard parser: converts infix tokens to RPN postfix tokens.
     */
    bool Evaluator::ConvertToRPN(const std::vector<Token>& tokens) {
        m_RPNTokens.clear();
        std::vector<Token> opStack;

        for (const auto& token : tokens) {
            switch (token.type) {
                case TokenType::Number:
                case TokenType::VariableX:
                case TokenType::VariableY:
                    m_RPNTokens.push_back(token);
                    break;
                case TokenType::Operator:
                    while (!opStack.empty() && 
                           (opStack.back().type == TokenType::Operator || opStack.back().type == TokenType::Function) && 
                           ((!token.isRightAssociative && token.precedence <= opStack.back().precedence) || 
                            (token.isRightAssociative && token.precedence < opStack.back().precedence))) {
                        m_RPNTokens.push_back(opStack.back());
                        opStack.pop_back();
                    }
                    opStack.push_back(token);
                    break;
                case TokenType::Function:
                    opStack.push_back(token);
                    break;
                case TokenType::LeftParen:
                    opStack.push_back(token);
                    break;
                case TokenType::RightParen:
                    while (!opStack.empty() && opStack.back().type != TokenType::LeftParen) {
                        m_RPNTokens.push_back(opStack.back());
                        opStack.pop_back();
                    }
                    if (opStack.empty()) {
                        m_LastError = "Mismatched parentheses.";
                        return false;
                    }
                    opStack.pop_back();

                    if (!opStack.empty() && opStack.back().type == TokenType::Function) {
                        m_RPNTokens.push_back(opStack.back());
                        opStack.pop_back();
                    }
                    break;
            }
        }

        while (!opStack.empty()) {
            if (opStack.back().type == TokenType::LeftParen) {
                m_LastError = "Mismatched parentheses.";
                return false;
            }
            m_RPNTokens.push_back(opStack.back());
            opStack.pop_back();
        }

        return true;
    }

    /**
     * @brief Prints RPN tokens for debugging purposes.
     */
    void Evaluator::DebugPrintTokens(const std::string& expression) {
        if (!Parse(expression)) {
            std::cout << "Parse Error: " << m_LastError << std::endl;
            return;
        }

        std::cout << "--- RPN Tokens for: \"" << expression << "\" ---" << std::endl;
        for (const auto& t : m_RPNTokens) {
            std::cout << "Token [Type: " << (int)t.type << ", Value: '" << t.value << "']" << std::endl;
        }
    }
}
