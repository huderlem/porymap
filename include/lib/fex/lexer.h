#ifndef INCLUDE_CORE_LEXER_H
#define INCLUDE_CORE_LEXER_H

#include <string>
#include <vector>

namespace fex
{
    class Token
    {
    public:
        enum class Type
        {
            // Macros
            kIfDef,
            kIfNDef,
            kDefine,
            kEndIf,
            kInclude,

            // Identifiers
            kIdentifier,

            // Keywords
            kExtern,
            kConst,
            kStruct,

            // Literals
            kNumber,
            kString,

            // Symbols
            kOpenParen,
            kCloseParen,
            kLessThan,
            kGreaterThan,
            kLessThanEqual,
            kGreaterThanEqual,
            kEqual,
            kLeftShift,
            kRightShift,
            kPlus,
            kMinus,
            kTimes,
            kDivide,
            kBitXor,
            kBitAnd,
            kBitOr,
            kLogicalAnd,
            kLogicalOr,
            kQuote,
            kComma,
            kSemicolon,
            kOpenSquare,
            kCloseSquare,
            kOpenCurly,
            kCloseCurly,
            kPeriod,
            kUnderscore,
        };

        Token(Type type, std::string filename, int line_number) : type_(type), filename_(filename), line_number_(line_number) {}
        Token(Type type, std::string filename, int line_number, std::string string_value) : type_(type), string_value_(string_value) , filename_(filename), line_number_(line_number) {}
        Token(Type type, std::string filename, int line_number, int int_value) : type_(type), int_value_(int_value), filename_(filename), line_number_(line_number) {}

        Type type() const { return type_; }
        const std::string &string_value() const { return string_value_; }
        int int_value() const { return int_value_; }

        const std::string &filename() const { return filename_; }
        int line_number() const { return line_number_; }

        std::string ToString() const;

    private:
        Type type_;
        std::string string_value_;
        int int_value_;

        std::string filename_ = "";
        int line_number_ = 0;
    };

    class Lexer
    {
    public:
        Lexer() = default;
        ~Lexer() = default;

        std::vector<Token> LexFile(const std::string &path);
        std::vector<Token> LexString(const std::string &data);
        void LexFileDumpTokens(const std::string &path, const std::string &out);

    private:
        std::vector<Token> Lex();
        char Peek();
        char Next();
        bool IsNumber();
        bool IsAlpha();
        bool IsHexAlpha();
        bool IsAlphaNumber();
        bool IsWhitespace();

        Token ConsumeIdentifier();
        Token ConsumeKeyword(Token identifier);
        Token ConsumeNumber();
        Token ConsumeString();
        Token ConsumeMacro();

        std::string ReadIdentifier();

        std::string data_ = "";
        uint32_t index_ = 0;

        std::string filename_ = "";
        int line_number_ = 1;
    };
} // namespace fex

#endif // INCLUDE_CORE_LEXER_H
