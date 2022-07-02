#ifndef INCLUDE_CORE_DEFINE_STATEMENT_H
#define INCLUDE_CORE_DEFINE_STATEMENT_H

#include <string>

namespace fex
{
    class DefineStatement
    {
    public:
        DefineStatement(std::string name, int value) : name_(name), value_(value) {}

        const std::string &name() const { return name_; }
        int value() const { return value_; }

    private:
        std::string name_;
        int value_;
    };
} // namespace fex

#endif // INCLUDE_CORE_DEFINE_STATEMENT_H
