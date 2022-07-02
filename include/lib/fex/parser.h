#ifndef INCLUDE_CORE_PARSER_H
#define INCLUDE_CORE_PARSER_H

#include <map>
#include <string>
#include <vector>

#include "array.h"
#include "array_value.h"
#include "define_statement.h"
#include "lexer.h"

namespace fex
{
    class Parser
    {
    public:
        Parser() = default;

        std::vector<DefineStatement> Parse(std::vector<Token> tokens);
        std::vector<Array> ParseTopLevelArrays(std::vector<Token> tokens);
        std::map<std::string, ArrayValue> ParseTopLevelObjects(std::vector<Token> tokens);

        std::map<std::string, int> ReadDefines(const std::string &filename, std::vector<std::string> matching);

    private:
        int EvaluateExpression(std::vector<Token> tokens);
        int ResolveIdentifier();
        int ResolveIdentifier(const Token &token);
        int GetPrecedence(const Token &token);
        bool IsOperator(const Token &token);
        bool IsParamMacro();
        std::vector<Token> ToPostfix();
        DefineStatement ParseDefine();

        ArrayValue ParseObject();

        Token Peek();
        Token Next();

        unsigned long index_;
        std::vector<Token> tokens_;

        std::map<std::string, int> top_level_;
    };
} // namespace fex

#endif // INCLUDE_CORE_PARSER_H
