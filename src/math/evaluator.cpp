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

    static const std::vector<std::string> g_Functions = {
        "arcsin", "arccos", "arctan", "floor", "ceil",
        "sqrt", "cbrt", "asin", "acos", "atan",
        "sin", "cos", "tan", "abs", "exp", "log", "ln"
    };

    static bool CanEndFactor(Evaluator::TokenType type) {
        return type == Evaluator::TokenType::Number ||
               type == Evaluator::TokenType::VariableX ||
               type == Evaluator::TokenType::VariableY ||
               type == Evaluator::TokenType::Parameter ||
               type == Evaluator::TokenType::RightParen;
    }

    static bool CanStartFactor(Evaluator::TokenType type) {
        return type == Evaluator::TokenType::Number ||
               type == Evaluator::TokenType::VariableX ||
               type == Evaluator::TokenType::VariableY ||
               type == Evaluator::TokenType::Parameter ||
               type == Evaluator::TokenType::Function ||
               type == Evaluator::TokenType::LeftParen;
    }

    static int GetPrecedence(char op) {
        if (op == '+' || op == '-') return 1;
        if (op == '*' || op == '/') return 2;
        if (op == '^') return 3;
        return 0;
    }

    static std::string NormalizeUnicodeMath(const std::string& input) {
        std::string out;
        size_t i = 0;
        size_t len = input.length();
        bool inSuperscript = false;
        bool inPipeAbs = false;

        while (i < len) {
            unsigned char c = (unsigned char)input[i];

            if (c == '|') {
                if (!inPipeAbs) {
                    out += "abs(";
                    inPipeAbs = true;
                } else {
                    out += ")";
                    inPipeAbs = false;
                }
                i++;
                continue;
            }

            // 2-byte UTF-8 sequences
            if (c == 0xC2 && i + 1 < len) {
                unsigned char c2 = (unsigned char)input[i + 1];
                if (c2 == 0xB2) { // ²
                    if (!inSuperscript) { out += "^2"; inSuperscript = true; } else { out += "2"; }
                    i += 2; continue;
                }
                if (c2 == 0xB3) { // ³ or ³√
                    if (i + 4 < len && (unsigned char)input[i + 2] == 0xE2 && (unsigned char)input[i + 3] == 0x88 && (unsigned char)input[i + 4] == 0x9A) {
                        out += "cbrt"; inSuperscript = false; i += 5; continue;
                    }
                    if (!inSuperscript) { out += "^3"; inSuperscript = true; } else { out += "3"; }
                    i += 2; continue;
                }
                if (c2 == 0xB9) { // ¹
                    if (!inSuperscript) { out += "^1"; inSuperscript = true; } else { out += "1"; }
                    i += 2; continue;
                }
                if (c2 == 0xB7) { // ·
                    out += "*"; inSuperscript = false; i += 2; continue;
                }
            }
            else if (c == 0xC3 && i + 1 < len) {
                unsigned char c2 = (unsigned char)input[i + 1];
                if (c2 == 0x97) { // ×
                    out += "*"; inSuperscript = false; i += 2; continue;
                }
                if (c2 == 0xB7) { // ÷
                    out += "/"; inSuperscript = false; i += 2; continue;
                }
            }
            else if (c == 0xCF && i + 1 < len && (unsigned char)input[i + 1] == 0x80) { // π
                out += "pi"; inSuperscript = false; i += 2; continue;
            }
            else if (c == 0xCE && i + 1 < len && (unsigned char)input[i + 1] == 0xB8) { // θ
                out += "theta"; inSuperscript = false; i += 2; continue;
            }
            // 3-byte UTF-8 sequences (E2 81 xx for superscripts, E2 88 9A for sqrt, E2 89 xx for operators)
            else if (c == 0xE2 && i + 2 < len) {
                unsigned char c2 = (unsigned char)input[i + 1];
                unsigned char c3 = (unsigned char)input[i + 2];

                if (c2 == 0x81) {
                    if (c3 == 0xB0) { // ⁰
                        if (!inSuperscript) { out += "^0"; inSuperscript = true; } else { out += "0"; }
                        i += 3; continue;
                    }
                    if (c3 >= 0xB4 && c3 <= 0xB9) { // ⁴ ⁵ ⁶ ⁷ ⁸ ⁹
                        char digit = '4' + (c3 - 0xB4);
                        if (!inSuperscript) { out += "^"; out += digit; inSuperscript = true; } else { out += digit; }
                        i += 3; continue;
                    }
                    if (c3 == 0xBA) { // ⁺
                        if (!inSuperscript) { out += "^+"; inSuperscript = true; } else { out += "+"; }
                        i += 3; continue;
                    }
                    if (c3 == 0xBB) { // ⁻
                        if (!inSuperscript) { out += "^-"; inSuperscript = true; } else { out += "-"; }
                        i += 3; continue;
                    }
                    if (c3 == 0xBF) { // ⁿ
                        if (!inSuperscript) { out += "^n"; inSuperscript = true; } else { out += "n"; }
                        i += 3; continue;
                    }
                }
                else if (c2 == 0x88 && c3 == 0x9A) { // √
                    out += "sqrt"; inSuperscript = false; i += 3; continue;
                }
                else if (c2 == 0x88 && c3 == 0x9B) { // ∛
                    out += "cbrt"; inSuperscript = false; i += 3; continue;
                }
                else if (c2 == 0x89) {
                    if (c3 == 0xA5) { // ≥
                        out += ">="; inSuperscript = false; i += 3; continue;
                    }
                    if (c3 == 0xA4) { // ≤
                        out += "<="; inSuperscript = false; i += 3; continue;
                    }
                    if (c3 == 0xA0) { // ≠
                        out += "!="; inSuperscript = false; i += 3; continue;
                    }
                }
            }

            out += input[i];
            if (input[i] != '^' && !std::isalnum((unsigned char)input[i])) {
                inSuperscript = false;
            }
            i++;
        }

        return out;
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

        std::string normalizedExpression = NormalizeUnicodeMath(expression);
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

                case TokenType::Parameter: {
                    auto it = m_Params.find(token.value);
                    double pVal = (it != m_Params.end()) ? it->second : 1.0;
                    push(pVal);
                    break;
                }

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
                    if (fn == "sin")                          valStack[stackPtr - 1] = std::sin(arg);
                    else if (fn == "cos")                     valStack[stackPtr - 1] = std::cos(arg);
                    else if (fn == "tan")                     valStack[stackPtr - 1] = std::tan(arg);
                    else if (fn == "asin" || fn == "arcsin")  valStack[stackPtr - 1] = std::asin(arg);
                    else if (fn == "acos" || fn == "arccos")  valStack[stackPtr - 1] = std::acos(arg);
                    else if (fn == "atan" || fn == "arctan")  valStack[stackPtr - 1] = std::atan(arg);
                    else if (fn == "sqrt")                    valStack[stackPtr - 1] = (arg >= 0.0) ? std::sqrt(arg) : std::numeric_limits<double>::quiet_NaN();
                    else if (fn == "cbrt")                    valStack[stackPtr - 1] = std::cbrt(arg);
                    else if (fn == "abs")                     valStack[stackPtr - 1] = std::abs(arg);
                    else if (fn == "exp")                     valStack[stackPtr - 1] = std::exp(arg);
                    else if (fn == "ln")                      valStack[stackPtr - 1] = (arg > 0.0) ? std::log(arg) : std::numeric_limits<double>::quiet_NaN();
                    else if (fn == "log")                     valStack[stackPtr - 1] = (arg > 0.0) ? std::log10(arg) : std::numeric_limits<double>::quiet_NaN();
                    else if (fn == "floor")                   valStack[stackPtr - 1] = std::floor(arg);
                    else if (fn == "ceil")                    valStack[stackPtr - 1] = std::ceil(arg);
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
        m_ParamNames.clear();

        auto addToken = [&](Token newToken) {
            if (!tokens.empty() && CanEndFactor(tokens.back().type) && CanStartFactor(newToken.type)) {
                tokens.emplace_back(TokenType::Operator, "*", 0.0, GetPrecedence('*'), false);
            }
            tokens.push_back(newToken);
        };

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
                addToken(Token(TokenType::Number, numStr, std::stod(numStr)));
            }
            // 2. Identifiers (x, y, pi, e, sin, cos, sinx, cosx...)
            else if (std::isalpha(c)) {
                size_t start = i;
                while (i < len && std::isalpha(expression[i])) i++;
                std::string id = expression.substr(start, i - start);
                std::transform(id.begin(), id.end(), id.begin(), ::tolower);

                size_t pos = 0;
                size_t idLen = id.length();

                while (pos < idLen) {
                    std::string sub = id.substr(pos);

                    if (sub == "x") {
                        addToken(Token(TokenType::VariableX, "x"));
                        pos += 1;
                    }
                    else if (sub == "y") {
                        m_IsImplicit = true;
                        addToken(Token(TokenType::VariableY, "y"));
                        pos += 1;
                    }
                    else if (sub == "pi") {
                        addToken(Token(TokenType::Number, "pi", M_PI));
                        pos += 2;
                    }
                    else if (sub == "e") {
                        addToken(Token(TokenType::Number, "e", M_E));
                        pos += 1;
                    }
                    else {
                        // Check functions (longest first)
                        bool matchedFn = false;
                        for (const auto& fn : g_Functions) {
                            if (sub.compare(0, fn.length(), fn) == 0) {
                                addToken(Token(TokenType::Function, fn, 0.0, 2));
                                pos += fn.length();
                                matchedFn = true;
                                break;
                            }
                        }

                        if (!matchedFn) {
                            if (sub.compare(0, 2, "pi") == 0) {
                                addToken(Token(TokenType::Number, "pi", M_PI));
                                pos += 2;
                            }
                            else if (sub[0] == 'x') {
                                addToken(Token(TokenType::VariableX, "x"));
                                pos += 1;
                            }
                            else if (sub[0] == 'y') {
                                m_IsImplicit = true;
                                addToken(Token(TokenType::VariableY, "y"));
                                pos += 1;
                            }
                            else if (sub[0] == 'e') {
                                addToken(Token(TokenType::Number, "e", M_E));
                                pos += 1;
                            }
                            else if (std::isalpha(sub[0])) {
                                std::string paramName = sub.substr(0, 1);
                                addToken(Token(TokenType::Parameter, paramName));
                                if (std::find(m_ParamNames.begin(), m_ParamNames.end(), paramName) == m_ParamNames.end()) {
                                    m_ParamNames.push_back(paramName);
                                }
                                pos += 1;
                            }
                            else {
                                m_IsValid = false;
                                m_LastError = "Unknown identifier: '" + id + "'";
                                return {};
                            }
                        }
                    }
                }
            }
            // 3. Operators (+, -, *, /, ^)
            else if (IsOperator(c)) {
                // Handle Unary Minus (e.g., -x or (-5))
                if (c == '-' && (tokens.empty() || tokens.back().type == TokenType::Operator || tokens.back().type == TokenType::LeftParen)) {
                    addToken(Token(TokenType::Number, "0", 0.0));
                }
                addToken(Token(TokenType::Operator, std::string(1, c), 0.0, GetPrecedence(c), c == '^'));
                i++;
            }
            // 4. Parentheses
            else if (c == '(') { addToken(Token(TokenType::LeftParen, "(")); i++; }
            else if (c == ')') { addToken(Token(TokenType::RightParen, ")")); i++; }
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
                case TokenType::Parameter:
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

    void Evaluator::SetParam(const std::string& name, double value) {
        m_Params[name] = value;
    }

    void Evaluator::SetParams(const std::unordered_map<std::string, double>& params) {
        for (const auto& [name, val] : params) {
            m_Params[name] = val;
        }
    }

    double Evaluator::GetParam(const std::string& name) const {
        auto it = m_Params.find(name);
        return (it != m_Params.end()) ? it->second : 1.0;
    }
}
