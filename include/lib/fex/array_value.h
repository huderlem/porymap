#ifndef INCLUDE_CORE_ARRAY_VALUE_H
#define INCLUDE_CORE_ARRAY_VALUE_H

#include <string>
#include <vector>
#include <memory>

namespace fex
{
    class ArrayValue
    {
    public:
        enum class Type
        {
            kNumber,     // Number literal
            kString,     // String literal
            kIdentifier, // Identifier

            kValueList, // Value, Value, Value, Value
            kValuePair, // Identifier = ArrayValue

            kEmpty,
        };

        Type type() const
        {
            return type_;
        }

        const std::vector<ArrayValue> &values() const
        {
            return values_;
        }

        std::vector<ArrayValue> release_values()
        {
            return std::move(values_);
        }

        const std::pair<std::string, std::unique_ptr<ArrayValue>> &pair() const
        {
            return pair_;
        }

        const std::string &string_value() const
        {
            return string_value_;
        }

        int int_value() const
        {
            return int_value_;
        }

        void set_type(const Type& type)
        {
            type_ = type;
        }

        void set_string_value(const std::string& value)
        {
            string_value_ = value;
        }

        void set_int_value(int value)
        {
            int_value_ = value;
        }

        void set_values(std::vector<ArrayValue> values)
        {
            values_ = std::move(values);
        }

        std::string ToString() const
        {
            switch (type_)
            {
            case Type::kEmpty:
                return "kEmpty: {}";
            case Type::kNumber:
                return "kNumber: " + std::to_string(int_value_);
            case Type::kString:
                return "kString: \"" + string_value_ + "\"";
            case Type::kIdentifier:
                return "kIdentifier: " + string_value_;
            case Type::kValueList:
            {
                std::string out = "kValueList: {\n";
                for (const ArrayValue &v : values_)
                {
                    out += "\t" + v.ToString() + ",\n";
                }
                return out + "}\n";
            }
            case Type::kValuePair:
                return "kValuePair: " + pair_.first + " = " + pair_.second->ToString() + "\n";
            }
        }

        static ArrayValue Empty()
        {
            return ArrayValue(ArrayValue::Type::kEmpty);
        }

        static ArrayValue Number(int value)
        {
            return ArrayValue(ArrayValue::Type::kNumber, value);
        }

        static ArrayValue String(std::string value)
        {
            return ArrayValue(ArrayValue::Type::kString, value);
        }

        static ArrayValue Identifier(std::string value)
        {
            return ArrayValue(ArrayValue::Type::kIdentifier, value);
        }

        static ArrayValue ValueList(std::vector<ArrayValue> values)
        {
            return ArrayValue(ArrayValue::Type::kValueList, std::move(values));
        }
        static ArrayValue ValuePair(std::pair<std::string, std::unique_ptr<ArrayValue>> value)
        {
            return ArrayValue(ArrayValue::Type::kValuePair, std::move(value));
        }

        ArrayValue(Type type) : type_(type) {}
        ArrayValue(Type type, int value) : type_(type), int_value_(value) {}
        ArrayValue(Type type, std::string value) : type_(type), string_value_(value) {}
        ArrayValue(Type type, std::vector<ArrayValue> values) : type_(type), values_(std::move(values)) {}
        ArrayValue(Type type, std::pair<std::string, std::unique_ptr<ArrayValue>> pair) : type_(type), pair_(std::move(pair)) {}
        ArrayValue() {}

    private:
        Type type_;

        // Number
        int int_value_;
        // String, Identifier
        std::string string_value_;
        // ValueList
        std::vector<ArrayValue> values_;
        // ValuePair
        std::pair<std::string, std::unique_ptr<ArrayValue>> pair_ = std::pair<std::string, std::unique_ptr<ArrayValue>>("", nullptr);
    };
} // namespace fex

#endif // INCLUDE_CORE_ARRAY_VALUE_H
