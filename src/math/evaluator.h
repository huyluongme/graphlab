#pragma once

#include <string>
#include <vector>
#include <unordered_map>

namespace GraphLab::Math {
    /**
     * @brief Math expression evaluator engine using Shunting-Yard and RPN.
     */
    class Evaluator {
    public:
        /**
         * @brief Mathematical token types.
         */
        enum class TokenType {
            Number,     // 3.14, 42, -1.5
            VariableX,  // x
            VariableY,  // y
            Parameter,  // a, b, c, k, m, etc.
            Operator,   // +, -, *, /, ^
            Function,   // sin, cos, tan, sqrt, abs, etc.
            LeftParen,  // (
            RightParen  // )
        };

        /**
         * @brief Representation of a mathematical token.
         */
        struct Token {
            TokenType type;
            std::string value;
            double numberValue = 0.0;
            int precedence = 0;
            bool isRightAssociative = false;

            Token(TokenType t, std::string val = "", double numVal = 0.0, int prec = 0, bool rightAssoc = false)
                : type(t), value(std::move(val)), numberValue(numVal), precedence(prec), isRightAssociative(rightAssoc) {}
        };

        Evaluator() = default;
        explicit Evaluator(const std::string& expression);
        ~Evaluator() = default;

        /**
         * @brief Parses an infix mathematical expression string into RPN format.
         * @param expression The expression string (e.g. "a * sin(b * x + c)")
         * @return True if parsing succeeded without syntax errors.
         */
        bool Parse(const std::string& expression);

        /**
         * @brief Evaluates 1D explicit function y = f(x) at given x value.
         * @param x The world X coordinate value.
         * @return The resulting y value, or NaN if undefined.
         */
        double Evaluate(double x) const;

        /**
         * @brief Evaluates 2D function F(x, y) at given (x, y) coordinate.
         * @param x The world X coordinate value.
         * @param y The world Y coordinate value.
         * @return The resulting evaluated value, or NaN if undefined.
         */
        double Evaluate(double x, double y) const;

        /**
         * @brief Parameter management functions.
         */
        void SetParam(const std::string& name, double value);
        void SetParams(const std::unordered_map<std::string, double>& params);
        double GetParam(const std::string& name) const;
        const std::vector<std::string>& GetParamNames() const { return m_ParamNames; }
        const std::unordered_map<std::string, double>& GetParams() const { return m_Params; }

        /**
         * @brief Prints RPN tokens for debugging purposes.
         */
        void DebugPrintTokens(const std::string& expression);

        bool IsValid() const { return m_IsValid; }
        bool IsImplicit() const { return m_IsImplicit; }
        bool IsEquation() const { return m_IsEquation; }
        const std::string& GetError() const { return m_LastError; }
        const std::string& GetExpression() const { return m_Expression; }

    private:
        /**
         * @brief Lexical analyzer: converts string into a vector of tokens.
         */
        std::vector<Token> Tokenize(const std::string& expression);

        /**
         * @brief Shunting-Yard parser: converts infix tokens to RPN postfix tokens.
         */
        bool ConvertToRPN(const std::vector<Token>& tokens);

    private:
        std::string m_Expression;
        std::vector<Token> m_RPNTokens;
        std::vector<std::string> m_ParamNames;
        std::unordered_map<std::string, double> m_Params;
        bool m_IsValid = false;
        bool m_IsImplicit = false;
        bool m_IsEquation = false;
        std::string m_LastError;
    };
}
