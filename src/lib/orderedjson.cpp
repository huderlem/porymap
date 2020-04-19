/* Copyright (c) 2013 Dropbox, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "orderedjson.h"
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <limits>

namespace poryjson {

static const int max_depth = 200;

using std::map;
using std::make_shared;
using std::initializer_list;
using std::move;

/* Helper for representing null - just a do-nothing struct, plus comparison
 * operators so the helpers in JsonValue work. We can't use nullptr_t because
 * it may not be orderable.
 */
struct NullStruct {
    bool operator==(NullStruct) const { return true; }
    bool operator<(NullStruct) const { return false; }
};

/* * * * * * * * * * * * * * * * * * * *
 * Serialization
 */

static void dump(NullStruct, QString &out, int *indent) {
    if (!out.endsWith(": ")) out += QString(*indent * 2, ' ');
    out += "null";
}

static void dump(double value, QString &out, int *indent) {
    if (!out.endsWith(": ")) out += QString(*indent * 2, ' ');
    if (std::isfinite(value)) {
        char buf[32];
        snprintf(buf, sizeof buf, "%.17g", value);
        out += buf;
    } else {
        out += "null";
    }
}

static void dump(int value, QString &out, int *indent) {
    if (!out.endsWith(": ")) out += QString(*indent * 2, ' ');
    char buf[32];
    snprintf(buf, sizeof buf, "%d", value);
    out += buf;
}

static void dump(bool value, QString &out, int *indent) {
    if (!out.endsWith(": ")) out += QString(*indent * 2, ' ');
    out += value ? "true" : "false";
}

static void dump(const QString &value, QString &out, int *indent, bool isKey = false) {
    if (!isKey && !out.endsWith(": ")) out += QString(*indent * 2, ' ');
    out += '"';
    for (int i = 0; i < value.length(); i++) {
        const char ch = value[i].unicode();
        if (ch == '\\') {
            out += "\\\\";
        } else if (ch == '"') {
            out += "\\\"";
        } else if (ch == '\b') {
            out += "\\b";
        } else if (ch == '\f') {
            out += "\\f";
        } else if (ch == '\n') {
            out += "\\n";
        } else if (ch == '\r') {
            out += "\\r";
        } else if (ch == '\t') {
            out += "\\t";
        } else if (static_cast<uint8_t>(ch) <= 0x1f) {
            char buf[8];
            snprintf(buf, sizeof buf, "\\u%04x", ch);
            out += buf;
        } else if (static_cast<uint8_t>(ch) == 0xe2 && static_cast<uint8_t>(value[i+1].unicode()) == 0x80
                   && static_cast<uint8_t>(value[i+2].unicode()) == 0xa8) {
            out += "\\u2028";
            i += 2;
        } else if (static_cast<uint8_t>(ch) == 0xe2 && static_cast<uint8_t>(value[i+1].unicode()) == 0x80
                   && static_cast<uint8_t>(value[i+2].unicode()) == 0xa9) {
            out += "\\u2029";
            i += 2;
        } else {
            out += ch;
        }
    }
    out += '"';
}

static void dump(const Json::array &values, QString &out, int *indent) {
    bool first = true;
    if (!out.endsWith(": ")) out += QString(*indent * 2, ' ');
    out += "[\n";
    *indent += 1;
    for (const auto &value : values) {
        if (!first) {
            out += ",\n";
        }
        value.dump(out, indent);
        first = false;
    }
    *indent -= 1;
    out += "\n" + QString(*indent * 2, ' ') + "]";
}

static void dump(const Json::object &values, QString &out, int *indent) {
    bool first = true;
    if (!out.endsWith(": ")) out += QString(*indent * 2, ' ');
    out += "{\n";
    *indent += 1;
    for (auto kv : values) {
        if (!first) {
            out += ",\n";
        }
        out += QString(*indent * 2, ' ');
        dump(kv.first, out, indent, true);
        out += ": ";
        kv.second.dump(out, indent);
        first = false;
    }
    *indent -= 1;
    out += "\n" + QString(*indent * 2, ' ') + "}";
}


void Json::dump(QString &out, int *indent) const {
    m_ptr->dump(out, indent);
}

/* * * * * * * * * * * * * * * * * * * *
 * Value wrappers
 */

template <Json::Type tag, typename T>
class Value : public JsonValue {
protected:

    // Constructors
    explicit Value(const T &value) : m_value(value) {}
    explicit Value(T &&value)      : m_value(move(value)) {}

    // Get type tag
    Json::Type type() const override {
        return tag;
    }

    // Comparisons
    bool equals(const JsonValue * other) const override {
        return m_value == static_cast<const Value<tag, T> *>(other)->m_value;
    }
    bool less(const JsonValue * other) const override {
        return m_value < static_cast<const Value<tag, T> *>(other)->m_value;
    }

    const T m_value;
    void dump(QString &out, int *indent) const override { poryjson::dump(m_value, out, indent); }
};

class JsonDouble final : public Value<Json::NUMBER, double> {
    double number_value() const override { return m_value; }
    int int_value() const override { return static_cast<int>(m_value); }
    bool equals(const JsonValue * other) const override { return m_value == other->number_value(); }
    bool less(const JsonValue * other)   const override { return m_value <  other->number_value(); }
public:
    explicit JsonDouble(double value) : Value(value) {}
};

class JsonInt final : public Value<Json::NUMBER, int> {
    double number_value() const override { return m_value; }
    int int_value() const override { return m_value; }
    bool equals(const JsonValue * other) const override { return m_value == other->number_value(); }
    bool less(const JsonValue * other)   const override { return m_value <  other->number_value(); }
public:
    explicit JsonInt(int value) : Value(value) {}
};

class JsonBoolean final : public Value<Json::BOOL, bool> {
    bool bool_value() const override { return m_value; }
public:
    explicit JsonBoolean(bool value) : Value(value) {}
};

class JsonString final : public Value<Json::STRING, QString> {
    const QString &string_value() const override { return m_value; }
public:
    explicit JsonString(const QString &value) : Value(value) {}
    explicit JsonString(QString &&value)      : Value(move(value)) {}
};

class JsonArray final : public Value<Json::ARRAY, Json::array> {
    const Json::array &array_items() const override { return m_value; }
    const Json & operator[](int i) const override;
public:
    explicit JsonArray(const Json::array &value) : Value(value) {}
    explicit JsonArray(Json::array &&value)      : Value(move(value)) {}
};

class JsonObject final : public Value<Json::OBJECT, Json::object> {
    const Json::object &object_items() const override { return m_value; }
    const Json & operator[](const QString &key) const override;
public:
    explicit JsonObject(const Json::object &value) : Value(value) {}
    explicit JsonObject(Json::object &&value)      : Value(move(value)) {}
};

class JsonNull final : public Value<Json::NUL, NullStruct> {
public:
    JsonNull() : Value({}) {}
};

/* * * * * * * * * * * * * * * * * * * *
 * Static globals - static-init-safe
 */
struct Statics {
    const std::shared_ptr<JsonValue> null = make_shared<JsonNull>();
    const std::shared_ptr<JsonValue> t = make_shared<JsonBoolean>(true);
    const std::shared_ptr<JsonValue> f = make_shared<JsonBoolean>(false);
    const QString empty_string;
    const QVector<Json> empty_vector;
    const Json::object empty_map;
    Statics() {}
};

static const Statics & statics() {
    static const Statics s {};
    return s;
}

static const Json & static_null() {
    // This has to be separate, not in Statics, because Json() accesses statics().null.
    static const Json json_null;
    return json_null;
}

/* * * * * * * * * * * * * * * * * * * *
 * Constructors
 */

Json::Json() noexcept                  : m_ptr(statics().null) {}
Json::Json(std::nullptr_t) noexcept    : m_ptr(statics().null) {}
Json::Json(double value)               : m_ptr(make_shared<JsonDouble>(value)) {}
Json::Json(int value)                  : m_ptr(make_shared<JsonInt>(value)) {}
Json::Json(bool value)                 : m_ptr(value ? statics().t : statics().f) {}
Json::Json(const QString &value)        : m_ptr(make_shared<JsonString>(value)) {}
Json::Json(QString &&value)             : m_ptr(make_shared<JsonString>(move(value))) {}
Json::Json(const char * value)         : m_ptr(make_shared<JsonString>(value)) {}
Json::Json(const Json::array &values)  : m_ptr(make_shared<JsonArray>(values)) {}
Json::Json(Json::array &&values)       : m_ptr(make_shared<JsonArray>(move(values))) {}
Json::Json(const Json::object &values) : m_ptr(make_shared<JsonObject>(values)) {}
Json::Json(Json::object &&values)      : m_ptr(make_shared<JsonObject>(move(values))) {}

/* * * * * * * * * * * * * * * * * * * *
 * Accessors
 */

Json::Type Json::type()                           const { return m_ptr->type();         }
double Json::number_value()                       const { return m_ptr->number_value(); }
int Json::int_value()                             const { return m_ptr->int_value();    }
bool Json::bool_value()                           const { return m_ptr->bool_value();   }
const QString & Json::string_value()               const { return m_ptr->string_value(); }
const QVector<Json> & Json::array_items()          const { return m_ptr->array_items();  }
const Json::object & Json::object_items()    const { return m_ptr->object_items(); }
const Json & Json::operator[] (int i)          const { return (*m_ptr)[i];           }
const Json & Json::operator[] (const QString &key) const { return (*m_ptr)[key];         }

double                    JsonValue::number_value()              const { return 0; }
int                       JsonValue::int_value()                 const { return 0; }
bool                      JsonValue::bool_value()                const { return false; }
const QString &            JsonValue::string_value()              const { return statics().empty_string; }
const QVector<Json> &      JsonValue::array_items()               const { return statics().empty_vector; }
const Json::object & JsonValue::object_items()              const { return statics().empty_map; }
const Json &              JsonValue::operator[] (int)         const { return static_null(); }
const Json &              JsonValue::operator[] (const QString &) const { return static_null(); }

const Json & JsonObject::operator[] (const QString &key) const {
    static auto iter = m_value.find(key);
    return (iter == m_value.end()) ? static_null() : (*iter).second;
}
const Json & JsonArray::operator[] (int i) const {
    if (i >= m_value.size()) return static_null();
    else return m_value[i];
}

/* * * * * * * * * * * * * * * * * * * *
 * Comparison
 */

bool Json::operator== (const Json &other) const {
    if (m_ptr == other.m_ptr)
        return true;
    if (m_ptr->type() != other.m_ptr->type())
        return false;

    return m_ptr->equals(other.m_ptr.get());
}

bool Json::operator< (const Json &other) const {
    if (m_ptr == other.m_ptr)
        return false;
    if (m_ptr->type() != other.m_ptr->type())
        return m_ptr->type() < other.m_ptr->type();

    return m_ptr->less(other.m_ptr.get());
}

/* * * * * * * * * * * * * * * * * * * *
 * Parsing
 */

/* esc(c)
 *
 * Format char c suitable for printing in an error message.
 */
static inline QString esc(char c) {
    char buf[12];
    if (static_cast<uint8_t>(c) >= 0x20 && static_cast<uint8_t>(c) <= 0x7f) {
        snprintf(buf, sizeof buf, "'%c' (%d)", c, c);
    } else {
        snprintf(buf, sizeof buf, "(%d)", c);
    }
    return QString(buf);
}

static inline bool in_range(long x, long lower, long upper) {
    return (x >= lower && x <= upper);
}

namespace {
/* JsonParser
 *
 * Object that tracks all state of an in-progress parse.
 */
struct JsonParser final {

    /* State
     */
    const QString &str;
    int i;
    QString &err;
    bool failed;
    const JsonParse strategy;

    /* fail(msg, err_ret = Json())
     *
     * Mark this parse as failed.
     */
    Json fail(QString &&msg) {
        return fail(move(msg), Json());
    }

    template <typename T>
    T fail(QString &&msg, const T err_ret) {
        if (!failed)
            err = std::move(msg);
        failed = true;
        return err_ret;
    }

    /* consume_whitespace()
     *
     * Advance until the current character is non-whitespace.
     */
    void consume_whitespace() {
        while (i < str.length() && (str[i] == ' ' || str[i] == '\r' || str[i] == '\n' || str[i] == '\t'))
            i++;
    }

    /* consume_comment()
     *
     * Advance comments (c-style inline and multiline).
     */
    bool consume_comment() {
      bool comment_found = false;
      if (str[i] == '/') {
        i++;
        if (i == str.size())
          return fail("unexpected end of input after start of comment", false);
        if (str[i] == '/') { // inline comment
          i++;
          // advance until next line, or end of input
          while (i < str.size() && str[i] != '\n') {
            i++;
          }
          comment_found = true;
        }
        else if (str[i] == '*') { // multiline comment
          i++;
          if (i > str.size()-2)
            return fail("unexpected end of input inside multi-line comment", false);
          // advance until closing tokens
          while (!(str[i] == '*' && str[i+1] == '/')) {
            i++;
            if (i > str.size()-2)
              return fail(
                "unexpected end of input inside multi-line comment", false);
          }
          i += 2;
          comment_found = true;
        }
        else
          return fail("malformed comment", false);
      }
      return comment_found;
    }

    /* consume_garbage()
     *
     * Advance until the current character is non-whitespace and non-comment.
     */
    void consume_garbage() {
      consume_whitespace();
      if(strategy == JsonParse::COMMENTS) {
        bool comment_found = false;
        do {
          comment_found = consume_comment();
          if (failed) return;
          consume_whitespace();
        }
        while(comment_found);
      }
    }

    /* get_next_token()
     *
     * Return the next non-whitespace character. If the end of the input is reached,
     * flag an error and return 0.
     */
    char get_next_token() {
        consume_garbage();
        if (failed) return static_cast<char>(0);
        if (i == str.size())
            return fail("unexpected end of input", static_cast<char>(0));

        return str[i++].unicode();
    }

    /* encode_utf8(pt, out)
     *
     * Encode pt as UTF-8 and add it to out.
     */
    void encode_utf8(long pt, QString & out) {
        if (pt < 0)
            return;

        if (pt < 0x80) {
            out += static_cast<char>(pt);
        } else if (pt < 0x800) {
            out += static_cast<char>((pt >> 6) | 0xC0);
            out += static_cast<char>((pt & 0x3F) | 0x80);
        } else if (pt < 0x10000) {
            out += static_cast<char>((pt >> 12) | 0xE0);
            out += static_cast<char>(((pt >> 6) & 0x3F) | 0x80);
            out += static_cast<char>((pt & 0x3F) | 0x80);
        } else {
            out += static_cast<char>((pt >> 18) | 0xF0);
            out += static_cast<char>(((pt >> 12) & 0x3F) | 0x80);
            out += static_cast<char>(((pt >> 6) & 0x3F) | 0x80);
            out += static_cast<char>((pt & 0x3F) | 0x80);
        }
    }

    /* parse_string()
     *
     * Parse a QString, starting at the current position.
     */
    QString parse_string() {
        QString out;
        long last_escaped_codepoint = -1;
        while (true) {
            if (i == str.size())
                return fail("unexpected end of input in QString", "");

            char ch = str[i++].unicode();

            if (ch == '"') {
                encode_utf8(last_escaped_codepoint, out);
                return out;
            }

            if (in_range(ch, 0, 0x1f))
                return fail(QString("unescaped " + esc(ch) + " in QString"), QString());

            // The usual case: non-escaped characters
            if (ch != '\\') {
                encode_utf8(last_escaped_codepoint, out);
                last_escaped_codepoint = -1;
                out += ch;
                continue;
            }

            // Handle escapes
            if (i == str.size())
                return fail("unexpected end of input in QString", "");

            ch = str[i++].unicode();

            if (ch == 'u') {
                // Extract 4-byte escape sequence
                QString esc = str.right(i).left(4);
                // Explicitly check length of the substring. The following loop
                // relies on std::QString returning the terminating NUL when
                // accessing str[length]. Checking here reduces brittleness.
                if (esc.length() < 4) {
                    return fail(QString("bad \\u escape: " + esc), "");
                }
                for (unsigned j = 0; j < 4; j++) {
                    if (!in_range(esc[j].unicode(), 'a', 'f') && !in_range(esc[j].unicode(), 'A', 'F')
                            && !in_range(esc[j].unicode(), '0', '9'))
                        return fail(QString("bad \\u escape: " + esc), "");
                }

                long codepoint = esc.toLong(nullptr, 16);

                // JSON specifies that characters outside the BMP shall be encoded as a pair
                // of 4-hex-digit \u escapes encoding their surrogate pair components. Check
                // whether we're in the middle of such a beast: the previous codepoint was an
                // escaped lead (high) surrogate, and this is a trail (low) surrogate.
                if (in_range(last_escaped_codepoint, 0xD800, 0xDBFF)
                        && in_range(codepoint, 0xDC00, 0xDFFF)) {
                    // Reassemble the two surrogate pairs into one astral-plane character, per
                    // the UTF-16 algorithm.
                    encode_utf8((((last_escaped_codepoint - 0xD800) << 10)
                                 | (codepoint - 0xDC00)) + 0x10000, out);
                    last_escaped_codepoint = -1;
                } else {
                    encode_utf8(last_escaped_codepoint, out);
                    last_escaped_codepoint = codepoint;
                }

                i += 4;
                continue;
            }

            encode_utf8(last_escaped_codepoint, out);
            last_escaped_codepoint = -1;

            if (ch == 'b') {
                out += '\b';
            } else if (ch == 'f') {
                out += '\f';
            } else if (ch == 'n') {
                out += '\n';
            } else if (ch == 'r') {
                out += '\r';
            } else if (ch == 't') {
                out += '\t';
            } else if (ch == '"' || ch == '\\' || ch == '/') {
                out += ch;
            } else {
                return fail(QString("invalid escape character " + esc(ch)), "");
            }
        }
    }

    /* parse_number()
     *
     * Parse a double.
     */
    Json parse_number() {
        unsigned start_pos = i;

        if (str[i] == '-')
            i++;

        // Integer part
        if (str[i] == '0') {
            i++;
            if (in_range(str[i].unicode(), '0', '9'))
                return fail("leading 0s not permitted in numbers");
        } else if (in_range(str[i].unicode(), '1', '9')) {
            i++;
            while (in_range(str[i].unicode(), '0', '9'))
                i++;
        } else {
            return fail(QString("invalid " + esc(str[i].unicode()) + " in number"));
        }

        if (str[i] != '.' && str[i] != 'e' && str[i] != 'E'
                && (i - start_pos) <= static_cast<unsigned>(std::numeric_limits<int>::digits10)) {
            return std::atoi(str.toStdString().c_str() + start_pos);
        }

        // Decimal part
        if (str[i] == '.') {
            i++;
            if (!in_range(str[i].unicode(), '0', '9'))
                return fail("at least one digit required in fractional part");

            while (in_range(str[i].unicode(), '0', '9'))
                i++;
        }

        // Exponent part
        if (str[i] == 'e' || str[i] == 'E') {
            i++;

            if (str[i] == '+' || str[i] == '-')
                i++;

            if (!in_range(str[i].unicode(), '0', '9'))
                return fail("at least one digit required in exponent");

            while (in_range(str[i].unicode(), '0', '9'))
                i++;
        }

        return std::strtod(str.toStdString().c_str() + start_pos, nullptr);
    }

    /* expect(str, res)
     *
     * Expect that 'str' starts at the character that was just read. If it does, advance
     * the input and return res. If not, flag an error.
     */
    Json expect(const QString &expected, Json res) {
        assert(i != 0);
        i--;
        QString result = str.right(str.size() - i).left(expected.length());
        if (result == expected) {
            i += expected.length();
            return res;
        } else {
            return fail(QString("parse error: expected " + expected + ", got " + str.right(i).left(expected.length())));
        }
    }

    /* parse_json()
     *
     * Parse a JSON object.
     */
    Json parse_json(int depth) {
        if (depth > max_depth) {
            return fail("exceeded maximum nesting depth");
        }

        char ch = get_next_token();
        if (failed)
            return Json();

        if (ch == '-' || (ch >= '0' && ch <= '9')) {
            i--;
            return parse_number();
        }

        if (ch == 't')
            return expect("true", true);

        if (ch == 'f')
            return expect("false", false);

        if (ch == 'n')
            return expect("null", Json());

        if (ch == '"')
            return parse_string();

        if (ch == '{') {
            Json::object data;
            ch = get_next_token();
            if (ch == '}')
                return data;

            while (1) {
                if (ch != '"')
                    return fail(QString("expected '\"' in object, got " + esc(ch)));

                QString key = parse_string();
                if (failed)
                    return Json();

                ch = get_next_token();
                if (ch != ':')
                    return fail(QString("expected ':' in object, got " + esc(ch)));

                data[std::move(key)] = parse_json(depth + 1);
                if (failed)
                    return Json();

                ch = get_next_token();
                if (ch == '}')
                    break;
                if (ch != ',')
                    return fail(QString("expected ',' in object, got " + esc(ch)));

                ch = get_next_token();
            }
            return data;
        }

        if (ch == '[') {
            QVector<Json> data;
            ch = get_next_token();
            if (ch == ']')
                return data;

            while (1) {
                i--;
                data.push_back(parse_json(depth + 1));
                if (failed)
                    return Json();

                ch = get_next_token();
                if (ch == ']')
                    break;
                if (ch != ',')
                    return fail(QString("expected ',' in list, got " + esc(ch)));

                ch = get_next_token();
                (void)ch;
            }
            return data;
        }

        return fail(QString("expected value, got " + esc(ch)));
    }
};
}//namespace {

Json Json::parse(const QString &in, QString &err, JsonParse strategy) {
    JsonParser parser { in, 0, err, false, strategy };
    Json result = parser.parse_json(0);

    // Check for any trailing garbage
    parser.consume_garbage();
    if (parser.failed)
        return Json();
    if (parser.i != in.size())
        return parser.fail(QString("unexpected trailing " + esc(in[parser.i].unicode())));

    return result;
}

} // namespace poryjson
