#ifndef INCLUDE_CORE_ARRAY_H
#define INCLUDE_CORE_ARRAY_H

#include <string>
#include <vector>

#include "array_value.h"

namespace fex
{
    class Array
    {
    public:
        Array(std::string type, std::string name) : type_(type), name_(name) {}

        void Add(ArrayValue value)
        {
            values_.push_back(std::move(value));
        }

        const std::string &type() const
        {
            return type_;
        }
        const std::string &name() const
        {
            return name_;
        }
        const std::vector<ArrayValue> &values() const
        {
            return values_;
        }

        std::vector<ArrayValue> release_values()
        {
            return std::move(values_);
        }

        std::string ToString() const
        {
            std::string out = name_ + ":\n";
            for (const ArrayValue &v : values_)
            {
                out += v.ToString() + "\n";
            }
            return out;
        }

    private:
        std::string type_;
        std::string name_;
        std::vector<ArrayValue> values_;
    };
} // namespace fex

#endif // INCLUDE_CORE_ARRAY_H
