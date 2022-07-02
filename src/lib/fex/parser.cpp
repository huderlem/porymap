#include "lib/fex/parser.h"

#include <iostream>
#include <regex>
#include <vector>

namespace fex
{
    int Parser::GetPrecedence(const Token &token)
    {
        switch (token.type())
        {
        case Token::Type::kTimes:
            return 3;
        case Token::Type::kDivide:
            return 3;
        case Token::Type::kPlus:
            return 4;
        case Token::Type::kMinus:
            return 4;
        case Token::Type::kLeftShift:
            return 5;
        case Token::Type::kRightShift:
            return 5;
        case Token::Type::kBitAnd:
            return 8;
        case Token::Type::kBitXor:
            return 9;
        case Token::Type::kBitOr:
            return 10;

        default:
        {
            std::cout << "Asked for precedence of unmapped token: " << token.ToString() << std::endl;
            return 0;
        }
        }
    }

    std::vector<Token> Parser::ToPostfix()
    {
        std::vector<Token::Type> types = {
            Token::Type::kNumber,
            Token::Type::kIdentifier,
            Token::Type::kOpenParen,
            Token::Type::kCloseParen,
            Token::Type::kLeftShift,
            Token::Type::kRightShift,
            Token::Type::kPlus,
            Token::Type::kMinus,
            Token::Type::kTimes,
            Token::Type::kDivide,
            Token::Type::kBitXor,
            Token::Type::kBitAnd,
            Token::Type::kBitOr,
        };

        std::vector<Token> output;
        std::vector<Token> stack;

        while (std::find(types.begin(), types.end(), Peek().type()) != types.end())
        {
            Token token = Next();
            if (token.type() == Token::Type::kNumber || token.type() == Token::Type::kIdentifier)
            {
                output.push_back(token);
            }
            else if (token.type() == Token::Type::kOpenParen)
            {
                stack.push_back(token);
            }
            else if (token.type() == Token::Type::kCloseParen)
            {
                while (!stack.empty() && stack.back().type() != Token::Type::kOpenParen)
                {
                    Token back = stack.back();
                    stack.pop_back();
                    output.push_back(back);
                }

                // Next();

                if (!stack.empty())
                {
                    // pop the left parenthesis token
                    stack.pop_back();
                }
                else
                {
                    std::cout << "Mismatched parentheses detected in expression!" << std::endl;
                }
            }
            else
            {
                // token is an operator
                while (!stack.empty() && stack.back().type() != Token::Type::kOpenParen && GetPrecedence(stack.back()) <= GetPrecedence(token))
                {
                    Token back = stack.back();
                    stack.pop_back();
                    output.push_back(back);
                }
                stack.push_back(token);
            }
        }

        while (!stack.empty())
        {
            if (stack.back().type() == Token::Type::kOpenParen || stack.back().type() == Token::Type::kCloseParen)
            {
                std::cout << "Mismatched parentheses detected in expression!" << std::endl;
            }
            else
            {
                Token back = stack.back();
                stack.pop_back();
                output.push_back(back);
            }
        }

        return output;
    }

    Token Parser::Peek() { return tokens_[index_]; }

    Token Parser::Next()
    {
        Token t = Peek();
        index_++;
        return t;
    }

    int Parser::ResolveIdentifier(const Token &token)
    {
        std::string iden_val = token.string_value();

        if (top_level_.find(iden_val) == top_level_.end())
        {
            std::cout << "[WARNING] Unknown identifier " << iden_val << std::endl;
            return 0;
        }

        return top_level_[iden_val];
    }

    bool Parser::IsOperator(const Token &token)
    {
        std::vector<Token::Type> types = {
            Token::Type::kLeftShift,
            Token::Type::kRightShift,
            Token::Type::kPlus,
            Token::Type::kMinus,
            Token::Type::kTimes,
            Token::Type::kDivide,
            Token::Type::kBitXor,
            Token::Type::kBitAnd,
            Token::Type::kBitOr,
        };
        return std::find(types.begin(), types.end(), token.type()) != types.end();
    }

    int Parser::EvaluateExpression(std::vector<Token> tokens)
    {
        std::vector<Token> stack;
        for (Token token : tokens)
        {
            if (IsOperator(token) && stack.size() > 1)
            {
                int op2 = stack.back().int_value();
                stack.pop_back();
                int op1 = stack.back().int_value();
                stack.pop_back();
                int result = 0;
                if (token.type() == Token::Type::kTimes)
                {
                    result = op1 * op2;
                }
                if (token.type() == Token::Type::kDivide)
                {
                    result = op1 / op2;
                }
                if (token.type() == Token::Type::kPlus)
                {
                    result = op1 + op2;
                }
                if (token.type() == Token::Type::kMinus)
                {
                    result = op1 - op2;
                }
                if (token.type() == Token::Type::kLeftShift)
                {
                    result = op1 << op2;
                }
                if (token.type() == Token::Type::kRightShift)
                {
                    result = op1 >> op2;
                }
                if (token.type() == Token::Type::kBitAnd)
                {
                    result = op1 & op2;
                }
                if (token.type() == Token::Type::kBitXor)
                {
                    result = op1 ^ op2;
                }
                if (token.type() == Token::Type::kBitOr)
                {
                    result = op1 | op2;
                }

                stack.push_back(Token(Token::Type::kNumber, token.filename(), token.line_number(), result));
            }

            if (token.type() == Token::Type::kNumber)
            {
                stack.push_back(token);
            }

            if (token.type() == Token::Type::kIdentifier)
            {
                stack.push_back(Token(Token::Type::kNumber, token.filename(), token.line_number(), ResolveIdentifier(token)));
            }
        }
        return stack.size() ? stack.back().int_value() : 0;
    }

    bool Parser::IsParamMacro()
    {
        int save_index = index_;

        if (Peek().type() != Token::Type::kOpenParen)
        {
            return false;
        }

        Next(); // Consume open so next if doesn't see it

        while (Peek().type() != Token::Type::kCloseParen)
        {
            // Nested parens aren't allowed in param list
            if (Peek().type() == Token::Type::kOpenParen)
            {
                index_ = save_index;
                return false;
            }

            Next();
        }
        // Consume closing
        Next();

        std::vector<Token::Type> types = {
            Token::Type::kNumber,
            Token::Type::kIdentifier,
            Token::Type::kOpenParen,
            Token::Type::kCloseParen,
            Token::Type::kLeftShift,
            Token::Type::kRightShift,
            Token::Type::kPlus,
            Token::Type::kMinus,
            Token::Type::kTimes,
            Token::Type::kDivide,
            Token::Type::kBitXor,
            Token::Type::kBitAnd,
            Token::Type::kBitOr,
        };

        // read value before resetting.
        bool out = std::find(types.begin(), types.end(), Peek().type()) != types.end();
        index_ = save_index;
        return out;
    }

    DefineStatement Parser::ParseDefine()
    {
        if (Next().type() != Token::Type::kDefine)
        {
            // error
        }

        if (Peek().type() != Token::Type::kIdentifier)
        {
            // error
        }

        std::string identifer = Next().string_value();
        int value = 0;

        if (IsParamMacro())
        {
            std::cout << "[WARNING] Macro:" << identifer << " has parameters which is not currently supported. Returning a dummy value instead." << std::endl;
            value = 0xDEAD;
            // Parameters (x, y, x) Expression
            Next();

            while (Peek().type() != Token::Type::kCloseParen)
            {
                auto formal = Next().string_value();
                if (Peek().type() == Token::Type::kComma)
                {
                    Next();
                }
            }

            Next();

            // In all current use cases, the macro is #define MACRO(a, b, c) (( something ))
            // we have consumed through the parameter list at this point. Consume all remaing
            // contents inside of parens
            if (Peek().type() != Token::Type::kOpenParen)
            {
                std::cout << "[FATAL] Must seen open parenthesis to continue processing." << std::endl;
                abort();
            }

            Next();
            int paren_count = 1;
            while (paren_count > 0)
            {
                if (Peek().type() == Token::Type::kOpenParen)
                {
                    paren_count++;
                }
                if (Peek().type() == Token::Type::kCloseParen)
                {
                    paren_count--;
                }

                Next();
            }
        }
        else
        {
            value = EvaluateExpression(ToPostfix());
        }

        top_level_[identifer] = value;
        return DefineStatement(identifer, value);
    }

    std::map<std::string, int> Parser::ReadDefines(const std::string &filename, std::vector<std::string> matching)
    {
        std::map<std::string, int> out;

        Lexer lexer;
        auto tokens = lexer.LexFile(filename);
        auto defines = Parse(tokens);

        for (const auto &define : defines)
        {
            for (const std::string &match : matching)
            {
                if (std::regex_match(define.name(), std::regex(match)))
                {
                    out[define.name()] = define.value();
                }
            }
        }

        return out;
    }

    ArrayValue Parser::ParseObject()
    {
        if (Peek().type() == Token::Type::kOpenSquare)
        {
            Next(); // [
            std::string identifier = Next().string_value();
            Next(); // ]
            Next(); // =
            std::unique_ptr<ArrayValue> value = std::unique_ptr<ArrayValue>(new ArrayValue(ParseObject()));

            std::pair<std::string, std::unique_ptr<ArrayValue>> pair(identifier, std::move(value));
            return ArrayValue::ValuePair(std::move(pair));
        }

        if (Peek().type() == Token::Type::kOpenCurly)
        {
            std::vector<ArrayValue> values;
            Next(); // {
            values.push_back(ParseObject());
            while(Peek().type() == Token::Type::kComma) {
                Next();
                values.push_back(ParseObject());
            }
            Next(); // }

            if (values.size() == 1) {
                return std::move(values[0]);
            }

            return ArrayValue::ValueList(std::move(values));
        }

        if (Peek().type() == Token::Type::kNumber)
        {
            int value = Next().int_value();
            return ArrayValue::Number(value);
        }

        if (Peek().type() == Token::Type::kBitAnd)
        {
            // Just skip past any reference indicators before identifiers.
            // This is not the right way to handle this, but it's good enough
            // for our parsing needs.
            Next(); // &
        }

        if (Peek().type() == Token::Type::kIdentifier)
        {
            std::vector<ArrayValue> idens = {};
            idens.push_back(ArrayValue::Identifier(Next().string_value()));

            // NELEMS(...)
            if (Peek().type() == Token::Type::kOpenParen)
            {
                while (Peek().type() != Token::Type::kCloseParen)
                {
                    std::string out = Next().ToString();
                }
                Next(); // )
            }

            // ABC | DEF | GHI
            while (Peek().type() == Token::Type::kBitOr) {
                Next();
                idens.push_back(ArrayValue::Identifier(Next().string_value()));
            }

            if (idens.size() == 1)
            {
                return std::move(idens[0]);
            }

            return ArrayValue::ValueList(std::move(idens));
        }

        if (Peek().type() == Token::Type::kUnderscore)
        {
            Next(); // _
            Next(); // (
            std::string value = Next().string_value();
            Next(); // )
            return ArrayValue::String(value);
        }

        if (Peek().type() == Token::Type::kPeriod)
        {
            Next(); // .
            std::string identifier = Next().string_value();
            Next(); // =

            std::unique_ptr<ArrayValue> value = std::unique_ptr<ArrayValue>(new ArrayValue(ParseObject()));

            std::pair<std::string, std::unique_ptr<ArrayValue>> pair(identifier, std::move(value));
            return ArrayValue::ValuePair(std::move(pair));
        }

        return ArrayValue::Empty();
    }

    std::vector<Array> Parser::ParseTopLevelArrays(std::vector<Token> tokens)
    {
        index_ = 0;
        tokens_ = std::move(tokens);

        std::vector<Array> items;

        while (index_ < tokens_.size())
        {
            while (Next().type() != Token::Type::kConst)
                ;
            Next(); // struct

            std::string type = Next().string_value();
            std::string name = Next().string_value();

            Array value(type, name);

            Next(); // [
            Next(); // ]
            Next(); // =
            value.Add(ParseObject());
            Next(); // ;

            items.push_back(std::move(value));
        }

        return items;
    }

    std::map<std::string, ArrayValue> Parser::ParseTopLevelObjects(std::vector<Token> tokens)
    {
        index_ = 0;
        tokens_ = std::move(tokens);

        std::map<std::string, ArrayValue> items;

        while (index_ < tokens_.size())
        {
            while (Next().type() != Token::Type::kConst)
                ;
            Next(); // struct

            Next(); // type
            std::string name = Next().string_value();

            Next(); // =
            items[name] = ParseObject();
            Next(); // ;
        }

        return items;
    }

    std::vector<DefineStatement> Parser::Parse(std::vector<Token> tokens)
    {
        index_ = 0;
        tokens_ = std::move(tokens);
        std::vector<DefineStatement> statements;

        while (index_ < tokens_.size())
        {
            switch (Peek().type())
            {
            case Token::Type::kDefine:
                statements.push_back(ParseDefine());
                break;

            default:
                Next();
                break;
            }
        }

        return statements;
    }

} // namespace fex
