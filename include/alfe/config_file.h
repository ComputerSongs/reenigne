#include "alfe/main.h"

#ifndef INCLUDED_CONFIG_FILE_H
#define INCLUDED_CONFIG_FILE_H

#include "alfe/hash_table.h"
#include "alfe/space.h"
#include "alfe/any.h"
#include "alfe/type.h"
#include "alfe/value.h"
#include "alfe/set.h"

int parseHexadecimalCharacter(CharacterSource* source, Span* span)
{
    CharacterSource s = *source;
    int c = s.get(span);
    if (c >= '0' && c <= '9') {
        *source = s;
        return c - '0';
    }
    if (c >= 'A' && c <= 'F') {
        *source = s;
        return c + 10 - 'A';
    }
    if (c >= 'a' && c <= 'f') {
        *source = s;
        return c + 10 - 'a';
    }
    return -1;
}

class Structure
{
public:
    template<class T> T get(String name) { return get(name).value<T>(); }
    virtual bool has(String name) = 0;
    virtual TypedValue get(String name) = 0;
    virtual void set(String name, TypedValue value) = 0;
};

class ConfigFile : public Structure
{
public:
    void addOption(String name, Type type)
    {
        _options.add(name, TypedValue(type));
    }
    template<class T> void addDefaultOption(String name, Type type,
        const T& defaultValue)
    {
        _options.add(name, TypedValue(type, Any(defaultValue)));
    }
    template<class T> void addDefaultOption(String name, const T& defaultValue)
    {
        _options.add(name, TypedValue(defaultValue));
    }
    void addType(Type type) { _types.add(type.toString(), type); }

    void load(File file)
    {
        _file = file;
        String contents = file.contents();
        CharacterSource source(contents, file.path());
        Space::parse(&source);
        do {
            CharacterSource s = source;
            if (s.get() == -1)
                break;
            s = source;
            Identifier identifier = parseIdentifier(&s);
            String name = identifier.name();
            Span span;
            if (name == "include") {
                TypedValue e = parseExpression(&s).convertTo(StringType());
                Space::assertCharacter(&s, ';', &span);
                load(e.value<String>());
                source = s;
                continue;
            }
            if (name[0] >= 'A' && name[0] <= 'Z') {
                if (!_types.hasKey(name))
                    identifier.span().throwError("Unknown type " + name);
                Type type = _types[name];
                Identifier objectIdentifier = parseIdentifier(&s);
                String objectName = objectIdentifier.name();
                if (has(name))
                    objectIdentifier.span().throwError(name +
                        " already exists");
                TypedValue value = TypedValue(
                    StructuredType(String(), List<StructuredType::Member>()),
                    Value<HashTable<String, TypedValue>>());
                if (Space::parseCharacter(&s, '=', &span))
                    value = parseExpression(&s);
                Space::assertCharacter(&s, ';', &span);
                source = s;
                _options[objectName] = value.convertTo(type);
                continue;
            }
            TypedValue left = parseDotExpression(&source);
            Space::assertCharacter(&source, '=', &span);
            TypedValue loadedExpression = parseExpression(&source);
            LValueType lValueType(left.type());
            if (!lValueType.valid())
                left.span().throwError("LValue required");
            Type type = lValueType.inner();
            LValue p = left.value<LValue>();
            TypedValue e = rValue(loadedExpression).convertTo(type);
            Space::assertCharacter(&source, ';', &span);
            p.set(e);
            _options[name].setValue(e.value());
        } while (true);
        for (auto i = _options.begin(); i != _options.end(); ++i) {
            if (!i.value().valid())
                throw Exception(file.path() + ": " + i.key() +
                    " not defined and no default is available.");
        }
    }
    TypedValue get(String name) { return rValue(_options[name]); }
    bool has(String name) { return _options.hasKey(name); }
    void set(String name, TypedValue value) { _options[name] = value; }
    File file() const { return _file; }
private:
    class Identifier
    {
    public:
        Identifier() { }
        Identifier(String name, Span span) : _name(name), _span(span) { }
        String name() const { return _name; }
        Span span() const { return _span; }
        bool valid() const { return !_name.empty(); }
    private:
        String _name;
        Span _span;
    };

    Identifier parseIdentifier(CharacterSource* source)
    {
        CharacterSource s = *source;
        Location startLocation = s.location();
        int startOffset = s.offset();
        Span startSpan;
        Span endSpan;
        int c = s.get(&startSpan);
        if (!(c >= 'A'&& c <= 'Z') && !(c >= 'a' && c <= 'z'))
            return Identifier();
        CharacterSource s2;
        do {
            s2 = s;
            c = s.get(&endSpan);
            if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                (c >= '0' && c <= '9') || c == '_')
                continue;
            break;
        } while (true);
        int endOffset = s2.offset();
        Location endLocation = s2.location();
        Space::parse(&s2);
        String name = s2.subString(startOffset, endOffset);
        *source = s2;
        return Identifier(name, startSpan + endSpan);
    }
    void throwError(CharacterSource* source)
    {
        source->location().throwError("Expected expression");
    }

    TypedValue combine(TypedValue left, TypedValue right)
    {
        if (left.valid())
            return TypedValue(StringType(),
                left.value<String>() + right.value<String>(),
                left.span() + right.span());
        return right;
    }

    TypedValue parseDoubleQuotedString(CharacterSource* source)
    {
        Span span;
        Span startSpan;
        if (!source->parse('"', &startSpan))
            return TypedValue();
        Span stringStartSpan = startSpan;
        Span stringEndSpan = startSpan;
        int startOffset = source->offset();
        int endOffset;
        String insert;
        int n;
        int nn;
        String string;
        TypedValue expression;
        TypedValue part;
        do {
            CharacterSource s = *source;
            endOffset = s.offset();
            int c = s.get(&span);
            if (c < 0x20 && c != 10) {
                if (c == -1)
                    source->location().throwError("End of file in string");
                source->throwUnexpected("printable character");
            }
            *source = s;
            switch (c) {
                case '"':
                    string += s.subString(startOffset, endOffset);
                    Space::parse(source);
                    return combine(expression, TypedValue(StringType(), string,
                        stringStartSpan + span));
                case '\\':
                    string += s.subString(startOffset, endOffset);
                    c = s.get(&stringEndSpan);
                    if (c < 0x20) {
                        if (c == 10)
                            source->location().throwError(
                                "End of line in string");
                        if (c == -1)
                            source->location().throwError(
                                "End of file in string");
                        source->throwUnexpected("escaped character");
                    }
                    *source = s;
                    switch (c) {
                        case 'n':
                            insert = "\n";
                            break;
                        case 't':
                            insert = codePoint(9);
                            break;
                        case '$':
                            insert = "$";
                            break;
                        case '"':
                            insert = "\"";
                            break;
                        case '\'':
                            insert = "'";
                            break;
                        case '`':
                            insert = "`";
                            break;
                        case '\\':
                            insert = "\\";
                            break;
                        case 'U':
                            source->assert('+', &stringEndSpan);
                            n = 0;
                            for (int i = 0; i < 4; ++i) {
                                nn = parseHexadecimalCharacter(source,
                                    &stringEndSpan);
                                if (nn == -1)
                                    source->
                                        throwUnexpected("hexadecimal digit");
                                n = (n << 4) | nn;
                            }
                            nn = parseHexadecimalCharacter(source,
                                &stringEndSpan);
                            if (nn != -1) {
                                n = (n << 4) | nn;
                                nn = parseHexadecimalCharacter(source,
                                    &stringEndSpan);
                                if (nn != -1)
                                    n = (n << 4) | nn;
                            }
                            insert = codePoint(n);
                            break;
                        default:
                            source->throwUnexpected("escaped character");
                    }
                    string += insert;
                    startOffset = source->offset();
                    break;
                case '$':
                    {
                        Identifier i = parseIdentifier(source);
                        if (!i.valid()) {
                            if (Space::parseCharacter(source, '(', &span)) {
                                part = rValue(parseExpression(source));
                                source->assert(')', &span);
                            }
                            else
                                source->location().throwError("Expected "
                                    "identifier or parenthesized expression");
                        }
                        String s = i.name();
                        if (s[0] >= 'a' && s[0] <= 'z')
                            part = rValue(valueOfIdentifier(i));
                        else
                            i.span().throwError("Expected identifier or "
                            "parenthesized expression");
                    }
                    part = part.convertTo(StringType());
                    string += s.subString(startOffset, endOffset);
                    startOffset = source->offset();
                    expression = combine(expression, TypedValue(StringType(),
                        string, stringStartSpan + stringEndSpan));
                    string = "";
                    expression = combine(expression, part);
                    break;
                default:
                    stringEndSpan = span;
            }
        } while (true);
    }

    TypedValue parseEmbeddedLiteral(CharacterSource* source)
    {
        Span startSpan;
        Span endSpan;
        if (!source->parse('#', &startSpan))
            return TypedValue();
        if (!source->parse('#', &endSpan))
            return TypedValue();
        if (!source->parse('#', &endSpan))
            return TypedValue();
        int startOffset = source->offset();
        Location location = source->location();
        CharacterSource s = *source;
        do {
            int c = s.get();
            if (c == -1)
                source->location().throwError("End of file in string");
            if (c == 10)
                break;
            *source = s;
        } while (true);
        int endOffset = source->offset();
        String terminator = source->subString(startOffset, endOffset);
        startOffset = s.offset();
        CharacterSource terminatorSource(terminator);
        int cc = terminatorSource.get();
        String string;
        do {
            *source = s;
            int c = s.get();
            if (c == -1)
                source->location().throwError("End of file in string");
            if (cc == -1) {
                if (c != '#')
                    continue;
                CharacterSource s2 = s;
                if (s2.get() != '#')
                    continue;
                if (s2.get(&endSpan) != '#')
                    continue;
                string += s.subString(startOffset, source->offset());
                *source = s2;
                Space::parse(source);
                return TypedValue(StringType(), string, startSpan + endSpan);
            }
            else
                if (c == cc) {
                    CharacterSource s2 = s;
                    CharacterSource st = terminatorSource;
                    do {
                        int ct = st.get();
                        if (ct == -1) {
                            if (s2.get() != '#')
                                break;
                            if (s2.get() != '#')
                                break;
                            if (s2.get(&endSpan) != '#')
                                break;
                            string +=
                                s.subString(startOffset, source->offset());
                            *source = s2;
                            Space::parse(source);
                            return TypedValue(StringType(), string,
                                startSpan + endSpan);
                        }
                        int cs = s2.get();
                        if (ct != cs)
                            break;
                    } while (true);
                }
            if (c == 10) {
                string += s.subString(startOffset, source->offset()) +
                    codePoint(10);
                startOffset = s.offset();
            }
        } while (true);
    }

    TypedValue parseInteger(CharacterSource* source)
    {
        CharacterSource s = *source;
        int n = 0;
        Span span;
        int c = s.get(&span);
        if (c < '0' || c > '9')
            return TypedValue();
        if (c == '0') {
            CharacterSource s2 = s;
            Span span2;
            int c = s2.get(&span2);
            if (c == 'x') {
                bool okay = false;
                int n = 0;
                do {
                    c = s2.get(&span2);
                    if (c >= '0' && c <= '9')
                        n = n*0x10 + c - '0';
                    else
                        if (c >= 'A' && c <= 'F')
                            n = n*0x10 + c + 10 - 'A';
                        else
                            if (c >= 'a' && c <= 'f')
                                n = n*0x10 + c + 10 - 'a';
                            else
                                if (okay) {
                                    Space::parse(source);
                                    return TypedValue(IntegerType(), n, span);
                                }
                                else
                                    return TypedValue();
                    okay = true;
                    *source = s2;
                    span += span2;
                } while (true);
            }
        }
        do {
            n = n*10 + c - '0';
            *source = s;
            Span span2;
            c = s.get(&span2);
            if (c < '0' || c > '9') {
                Space::parse(source);
                return TypedValue(IntegerType(), n, span);
            }
            span += span2;
        } while (true);
    }
    TypedValue valueOfIdentifier(const Identifier& i)
    {
        String s = i.name();
        static String trueKeyword("true");
        if (s == trueKeyword)
            return TypedValue(BooleanType(), true, i.span());
        static String falseKeyword("false");
        if (s == falseKeyword)
            return TypedValue(BooleanType(), false, i.span());
        if (_enumeratedValues.hasKey(s)) {
            TypedValue value = _enumeratedValues[s];
            return TypedValue(value.type(), value.value(), i.span());
        }
        if (!_options.hasKey(s))
            i.span().throwError("Unknown identifier " + s);
        return TypedValue(LValueType(_options[s].type()), LValue(this, s),
            i.span());
    }
    TypedValue parseExpressionElement(CharacterSource* source)
    {
        Location location = source->location();
        TypedValue e = parseDoubleQuotedString(source);
        if (e.valid())
            return e;
        e = parseEmbeddedLiteral(source);
        if (e.valid())
            return e;
        e = parseInteger(source);
        if (e.valid())
            return e;
        Identifier i = parseIdentifier(source);
        if (i.valid()) {
            String name = i.name();
            if (name[0] >= 'a' && name[0] <= 'z') {
                Span span;
                if (Space::parseCharacter(source, ':', &span)) {
                    // Avoid interpreting labels as values
                    return TypedValue(LabelType(), name, i.span() + span);
                }
                return valueOfIdentifier(i);
            }
            // i is a type identifier
            if (!_types.hasKey(name))
                i.span().throwError("Unknown type " + name);
            StructuredType type = _types[name];
            if (!type.valid())
                i.span().throwError("Only structure types can be constructed");
            const Array<StructuredType::Member>* members = type.members();
            List<Any> values;
            Span span;
            Space::assertCharacter(source, '(', &span);
            for (int i = 0; i < members->count(); ++i) {
                if (i > 0)
                    Space::assertCharacter(source, ',', &span);
                TypedValue value = rValue(parseExpression(source)).
                    convertTo((*members)[i].type());
                values.add(value.value());
            }
            Space::assertCharacter(source, ')', &span);
            return TypedValue(type, values, e.span() + span);
        }
        Span span;
        if (Space::parseCharacter(source, '{', &span)) {
            Value<HashTable<String, TypedValue> > table;
            List<StructuredType::Member> members;
            Span span2;
            int n = 0;
            do {
                TypedValue e = rValue(parseExpression(source));
                String name;
                String memberName;
                if (e.type() == LabelType()) {
                    TypedValue i = e;
                    name = e.value<String>();
                    e = rValue(parseExpression(source));
                    if (!e.valid())
                        source->location().throwError("Expected expression");
                    if (table->hasKey(name))
                        i.span().throwError(name + " already defined.");
                    memberName = name;
                }
                else
                    name = String::Decimal(n);
                ++n;
                table->add(name, e);
                members.add(StructuredType::Member(memberName, e.type()));
            } while (Space::parseCharacter(source, ',', &span2));
            Space::assertCharacter(source, '}', &span2);
            return TypedValue(StructuredType("", members), table,
                span + span2);
        }
        if (Space::parseCharacter(source, '(', &span)) {
            e = parseExpression(source);
            Space::assertCharacter(source, ')', &span);
            return e;
        }
        return TypedValue();
    }
    TypedValue parseDotExpression(CharacterSource* source)
    {
        TypedValue e = parseExpressionElement(source);
        if (!e.valid())
            return e;
        do {
            Span span;
            if (Space::parseCharacter(source, '.', &span)) {
                Identifier i = parseIdentifier(source);
                if (!i.valid())
                    source->location().throwError("Identifier expected");
                String name = i.name();
                Span s = e.span() + i.span();
                LValueType lValueType(e.type());
                if (!lValueType.valid()) {
                    auto m = e.value<Value<HashTable<String, TypedValue>>>();
                    if (!(*m).hasKey(name))
                        s.throwError("Expression has no member named " + name);
                    e = (*m)[name];
                    e = TypedValue(e.type(), e.value(), s);
                }
                else {
                    Structure* p =
                        e.value<LValue>().rValue().value<Structure*>();
                    if (!p->has(name))
                        s.throwError("Expression has no member named " + name);
                    e = TypedValue(LValueType::wrap(p->get(name).type()),
                        LValue(p, name), s);
                }
            }
        } while (true);
        return e;
    }

    TypedValue parseUnaryExpression(CharacterSource* source)
    {
        Span span;
        if (Space::parseCharacter(source, '-', &span)) {
            TypedValue e = rValue(parseUnaryExpression(source));
            span = span + e.span();
            if (e.type() != IntegerType())
                span.throwError("Only numbers can be negated");
            return TypedValue(IntegerType(), -e.value<int>(), span);
        }
        return parseDotExpression(source);
    }

    TypedValue parseMultiplicativeExpression(CharacterSource* source)
    {
        TypedValue e = parseUnaryExpression(source);
        if (!e.valid())
            return e;
        do {
            Span span;
            if (Space::parseCharacter(source, '*', &span)) {
                e = rValue(e);
                TypedValue e2 = rValue(parseUnaryExpression(source));
                if (!e2.valid())
                    throwError(source);
                bool okay = false;
                if (e.type() == IntegerType()) {
                    if (e2.type() == IntegerType()) {
                        e = TypedValue(IntegerType(),
                            e.value<int>() * e2.value<int>(),
                            e.span() + e2.span());
                        okay = true;
                    }
                    else
                        if (e2.type() == StringType()) {
                            e = TypedValue(StringType(),
                                e2.value<String>() * e.value<int>(),
                                e.span() + e2.span());
                            okay = true;
                        }
                }
                else
                    if (e.type() == StringType()) {
                        if (e2.type() == IntegerType()) {
                            e = TypedValue(StringType(),
                                e.value<String>() * e2.value<int>(),
                                e.span() + e2.span());
                            okay = true;
                        }
                    }
                if (!okay)
                    span.throwError("Don't know how to multiply type " +
                        e.type().toString() + " and type " +
                        e2.type().toString() + ".");
                continue;
            }
            if (Space::parseCharacter(source, '/', &span)) {
                e = rValue(e);
                TypedValue e2 = rValue(parseUnaryExpression(source));
                if (!e2.valid())
                    throwError(source);
                if (e.type() == IntegerType() && e2.type() == IntegerType())
                    e = TypedValue(IntegerType(),
                        e.value<int>() / e2.value<int>(),
                        e.span() + e2.span());
                else
                    span.throwError("Don't know how to divide type " +
                        e.type().toString() + " by type " +
                        e2.type().toString() + ".");
                continue;
            }
            return e;
        } while (true);
    }

    TypedValue parseExpression(CharacterSource* source)
    {
        TypedValue e = parseMultiplicativeExpression(source);
        if (!e.valid())
            throwError(source);
        do {
            Span span;
            if (Space::parseCharacter(source, '+', &span)) {
                e = rValue(e);
                TypedValue e2 = rValue(parseMultiplicativeExpression(source));
                if (!e2.valid())
                    throwError(source);
                if (e.type() == IntegerType() && e2.type() == IntegerType())
                    e = TypedValue(IntegerType(),
                        e.value<int>() + e2.value<int>(),
                        e.span() + e2.span());
                else
                    if (e.type() == StringType() && e2.type() == StringType())
                        e = TypedValue(StringType(),
                            e.value<String>() + e2.value<String>(),
                            e.span() + e2.span());
                    else
                        span.throwError("Don't know how to add type " +
                            e2.type().toString() + " to type " +
                            e.type().toString() + ".");
                continue;
            }
            if (Space::parseCharacter(source, '-', &span)) {
                e = rValue(e);
                TypedValue e2 = rValue(parseMultiplicativeExpression(source));
                if (!e2.valid())
                    throwError(source);
                if (e.type() == IntegerType() && e2.type() == IntegerType())
                    e = TypedValue(IntegerType(),
                        e.value<int>() - e2.value<int>(),
                        e.span() + e2.span());
                else
                    span.throwError("Don't know how to subtract type " +
                        e2.type().toString() + " from type " +
                        e.type().toString() + ".");
                continue;
            }
            return e;
        } while (true);
    }

    static Type rValueType(Type type)
    {
        if (LValueType(type).valid())
            return LValueType(type).inner();
        return type;
    }
    static TypedValue rValue(TypedValue value)
    {
        LValueType lValueType(value.type());
        if (lValueType.valid())
            return TypedValue(lValueType.inner(),
                value.value<LValue>().rValue(), value.span());
        return value;
    }

    class LValueType : public Type
    {
    public:
        LValueType(const Tyco& other) : Type(other) { }
        static LValueType wrap(const Type& inner)
        {
            if (LValueType(inner).valid())
                return inner;
            return LValueType(new Implementation(inner));
        }
        bool valid() const { return implementation() != 0; }
        Type inner() const { return implementation()->inner(); }
    private:
        LValueType(const Implementation* implementation)
          : Type(implementation) { }

        class Implementation : public Type::Implementation
        {
        public:
            Implementation(Type inner) : _inner(inner) { }
            Type inner() const { return _inner; }
            String toString() const
            {
                return String("LValue<") + _inner.toString() + ">";
            }
        private:
            Type _inner;
        };

        const Implementation* implementation() const
        {
            return _implementation.referent<Implementation>();
        }
    };

    class LValue
    {
    public:
        LValue(Structure* structure, String name)
          : _structure(structure), _name(name) { }
        TypedValue rValue() const { return _structure->get(_name); }
        void set(TypedValue value) const { _structure->set(_name, value); }
    private:
        Structure* _structure;
        String _name;
    };

    HashTable<String, TypedValue> _options;
    HashTable<String, TypedValue> _enumeratedValues;
    HashTable<String, Type> _types;
    Set<Type> _typeSet;
    File _file;
};

#endif // INCLUDED_CONFIG_FILE_H
