#include "lib/fex/lexer.h"

#include <fstream>
#include <iostream>
#include <sstream>

namespace fex
{

    bool Lexer::IsNumber()
    {
        char c = Peek();
        return (c >= '0' && c <= '9');
    }

    bool Lexer::IsWhitespace()
    {
        char c = Peek();
        return (c == ' ' || c == '\t' || c == '\r' || c == '\n');
    }

    bool Lexer::IsHexAlpha()
    {
        char c = Peek();
        return ((c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'));
    }

    bool Lexer::IsAlpha()
    {
        char c = Peek();
        return ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'));
    }

    bool Lexer::IsAlphaNumber()
    {
        return IsAlpha() || IsNumber();
    };

    char Lexer::Peek()
    {
        return data_[index_];
    }

    char Lexer::Next()
    {
        char c = Peek();
        index_++;
        return c;
    }

    Token Lexer::ConsumeKeyword(Token identifier)
    {
        const std::string &value = identifier.string_value();

        if (value == "extern")
        {
            return Token(Token::Type::kExtern, identifier.filename(), identifier.line_number());
        }
        if (value == "const")
        {
            return Token(Token::Type::kConst, identifier.filename(), identifier.line_number());
        }
        if (value == "struct")
        {
            return Token(Token::Type::kStruct, identifier.filename(), identifier.line_number());
        }

        return identifier;
    }

    Token Lexer::ConsumeIdentifier()
    {
        std::string identifer = "";

        while (IsAlphaNumber() || Peek() == '_')
        {
            identifer += Next();
        }

        return ConsumeKeyword(Token(Token::Type::kIdentifier, filename_, line_number_, identifer));
    }

    Token Lexer::ConsumeNumber()
    {
        std::string identifer = "";

        if (Peek() == '0')
        {
            identifer += Next();
            if (Peek() == 'x')
            {
                identifer += Next();
            }

            while (IsNumber() || IsHexAlpha())
            {
                identifer += Next();
            }

            return Token(Token::Type::kNumber, filename_, line_number_, std::stoi(identifer, nullptr, 16));
        }

        while (IsNumber())
        {
            identifer += Next();
        }

        return Token(Token::Type::kNumber, filename_, line_number_, std::stoi(identifer));
    }

    // TODO: Doesn't currently support escape characters
    Token Lexer::ConsumeString()
    {
        std::string value = "";
        if (Next() != '\"')
        {
            // Error
        }

        // TODO: error if we never see a quote
        while (Peek() != '\"')
        {
            value += Next();
        }
        Next(); // Consume final quote
        return Token(Token::Type::kString, filename_, line_number_, value);
    }

    Token Lexer::ConsumeMacro()
    {
        Token id = ConsumeIdentifier();

        if (id.string_value() == "ifdef")
        {
            return Token(Token::Type::kIfDef, filename_, line_number_);
        }
        if (id.string_value() == "ifndef")
        {
            return Token(Token::Type::kIfNDef, filename_, line_number_);
        }
        if (id.string_value() == "define")
        {
            return Token(Token::Type::kDefine, filename_, line_number_);
        }
        if (id.string_value() == "endif")
        {
            return Token(Token::Type::kEndIf, filename_, line_number_);
        }

        if (id.string_value() == "include")
        {
            return Token(Token::Type::kInclude, filename_, line_number_);
        }

        return Token(Token::Type::kDefine, filename_, line_number_);
    }

    std::vector<Token> Lexer::LexString(const std::string &data)
    {
        filename_ = "string literal";
        line_number_ = 1;
        index_ = 0;
        data_ = data;

        return Lex();
    }

    std::vector<Token> Lexer::LexFile(const std::string &path)
    {
        filename_ = path;
        line_number_ = 1;

        std::ifstream file;
        file.open(path);

        std::stringstream stream;
        stream << file.rdbuf();

        index_ = 0;
        data_ = stream.str();

        file.close();

        return Lex();
    }

    void Lexer::LexFileDumpTokens(const std::string &path, const std::string &out)
    {
        std::ofstream file;
        file.open(out);

        for (Token token : LexFile(path))
        {
            file << token.ToString() << std::endl;
        }

        file.close();
    }

    std::vector<Token> Lexer::Lex()
    {
        std::vector<Token> tokens;

        while (index_ < data_.length())
        {
            while (IsWhitespace())
            {
                if (Peek() == '\n')
                {
                    line_number_++;
                }
                Next();
            }

            if (IsAlpha())
            {
                tokens.push_back(ConsumeIdentifier());
                continue;
            }

            if (IsNumber())
            {
                tokens.push_back(ConsumeNumber());
                continue;
            }

            switch (Peek())
            {
            case '*':
                Next();
                tokens.push_back(Token(Token::Type::kTimes, filename_, line_number_));
                break;
            case '-':
                Next();
                tokens.push_back(Token(Token::Type::kMinus, filename_, line_number_));
                break;
            case '+':
                Next();
                tokens.push_back(Token(Token::Type::kPlus, filename_, line_number_));
                break;
            case '(':
                Next();
                tokens.push_back(Token(Token::Type::kOpenParen, filename_, line_number_));
                break;
            case ')':
                Next();
                tokens.push_back(Token(Token::Type::kCloseParen, filename_, line_number_));
                break;
            case '&':
                Next();
                if (Peek() == '&')
                {
                    Next();
                    tokens.push_back(Token(Token::Type::kLogicalAnd, filename_, line_number_));
                    break;
                }
                tokens.push_back(Token(Token::Type::kBitAnd, filename_, line_number_));
                break;
            case '|':
                Next();
                if (Peek() == '|')
                {
                    Next();
                    tokens.push_back(Token(Token::Type::kLogicalOr, filename_, line_number_));
                    break;
                }
                tokens.push_back(Token(Token::Type::kBitOr, filename_, line_number_));
                break;
            case '^':
                Next();
                tokens.push_back(Token(Token::Type::kBitXor, filename_, line_number_));
                break;
            case ',':
                Next();
                tokens.push_back(Token(Token::Type::kComma, filename_, line_number_));
                break;
            case '=':
                Next();
                tokens.push_back(Token(Token::Type::kEqual, filename_, line_number_));
                break;
            case ';':
                Next();
                tokens.push_back(Token(Token::Type::kSemicolon, filename_, line_number_));
                break;
            case '[':
                Next();
                tokens.push_back(Token(Token::Type::kOpenSquare, filename_, line_number_));
                break;
            case ']':
                Next();
                tokens.push_back(Token(Token::Type::kCloseSquare, filename_, line_number_));
                break;
            case '{':
                Next();
                tokens.push_back(Token(Token::Type::kOpenCurly, filename_, line_number_));
                break;
            case '}':
                Next();
                tokens.push_back(Token(Token::Type::kCloseCurly, filename_, line_number_));
                break;
            case '.':
                Next();
                tokens.push_back(Token(Token::Type::kPeriod, filename_, line_number_));
                break;
            case '_':
                Next();
                tokens.push_back(Token(Token::Type::kUnderscore, filename_, line_number_));
                break;
            case '#':
                Next();
                tokens.push_back(ConsumeMacro());
                break;
            case '\"':
                tokens.push_back(ConsumeString());
                break;
            case '<':
                Next();
                if (Peek() == '<')
                {
                    Next();
                    tokens.push_back(Token(Token::Type::kLeftShift, filename_, line_number_));
                    break;
                }
                if (Peek() == '=')
                {
                    Next();
                    tokens.push_back(Token(Token::Type::kLessThanEqual, filename_, line_number_));
                    break;
                }
                tokens.push_back(Token(Token::Type::kLessThan, filename_, line_number_));
                break;
            case '>':
                Next();
                if (Peek() == '>')
                {
                    Next();
                    tokens.push_back(Token(Token::Type::kRightShift, filename_, line_number_));
                    break;
                }
                if (Peek() == '=')
                {
                    Next();
                    tokens.push_back(Token(Token::Type::kGreaterThanEqual, filename_, line_number_));
                    break;
                }
                tokens.push_back(Token(Token::Type::kGreaterThan, filename_, line_number_));
                break;

            case '/':
                Next();
                switch (Peek())
                {
                case '/':
                    while (Next() != '\n')
                        ;
                    continue;
                case '*':
                    while (Next() != '*')
                        ;
                    Next(); // last /
                    continue;
                default:
                    tokens.push_back(Token(Token::Type::kDivide, filename_, line_number_));
                    continue;
                }

            case '\0':
                Next();
                break;

            default:
                char c = Next();
                std::cout << "[WARNING] Unable to lex unknown char: '" << c << "' (0x" << std::hex << (int)c << ")" << std::endl;
                break;
            }
        }

        return tokens;
    }

    std::string Token::ToString() const
    {
        std::string out = filename() + ":" + std::to_string(line_number()) + " - ";
        switch (type())
        {
        case Token::Type::kIfDef:
            out += "Macro: IfDef";
            break;
        case Token::Type::kIfNDef:
            out += "Macro: IfNDef";
            break;
        case Token::Type::kDefine:
            out += "Macro: Define";
            break;
        case Token::Type::kEndIf:
            out += "Macro: EndIf";
            break;
        case Token::Type::kInclude:
            out += "Macro: Include";
            break;
        case Token::Type::kNumber:
            out += "Number: " + std::to_string(int_value());
            break;
        case Token::Type::kString:
            out += "String: " + string_value();
            break;
        case Token::Type::kIdentifier:
            out += "Identifier: " + string_value();
            break;
        case Token::Type::kOpenParen:
            out += "Symbol: (";
            break;
        case Token::Type::kCloseParen:
            out += "Symbol: )";
            break;
        case Token::Type::kLessThan:
            out += "Symbol: <";
            break;
        case Token::Type::kGreaterThan:
            out += "Symbol: >";
            break;
        case Token::Type::kLeftShift:
            out += "Symbol: <<";
            break;
        case Token::Type::kRightShift:
            out += "Symbol: >>";
            break;
        case Token::Type::kPlus:
            out += "Symbol: +";
            break;
        case Token::Type::kMinus:
            out += "Symbol: -";
            break;
        case Token::Type::kTimes:
            out += "Symbol: *";
            break;
        case Token::Type::kDivide:
            out += "Symbol: /";
            break;
        case Token::Type::kBitXor:
            out += "Symbol: ^";
            break;
        case Token::Type::kBitAnd:
            out += "Symbol: &";
            break;
        case Token::Type::kBitOr:
            out += "Symbol: |";
            break;
        case Token::Type::kQuote:
            out += "Symbol: \"";
            break;
        case Token::Type::kComma:
            out += "Symbol: ,";
            break;
        case Token::Type::kLessThanEqual:
            out += "Symbol: <=";
            break;
        case Token::Type::kGreaterThanEqual:
            out += "Symbol: >=";
            break;
        case Token::Type::kEqual:
            out += "Symbol: =";
            break;
        case Token::Type::kLogicalAnd:
            out += "Symbol: &&";
            break;
        case Token::Type::kLogicalOr:
            out += "Symbol: ||";
            break;
        case Token::Type::kSemicolon:
            out += "Symbol: ;";
            break;
        case Token::Type::kExtern:
            out += "Keyword: extern";
            break;
        case Token::Type::kConst:
            out += "Keyword: const";
            break;
        case Token::Type::kStruct:
            out += "Keyword: struct";
            break;
        case Token::Type::kOpenSquare:
            out += "Symbol: [";
            break;
        case Token::Type::kCloseSquare:
            out += "Symbol: ]";
            break;
        case Token::Type::kOpenCurly:
            out += "Symbol: {";
            break;
        case Token::Type::kCloseCurly:
            out += "Symbol: }";
            break;
        case Token::Type::kPeriod:
            out += "Symbol: .";
            break;
        case Token::Type::kUnderscore:
            out += "Symbol: _";
            break;
        }

        return out;
    }

} // namespace fex
