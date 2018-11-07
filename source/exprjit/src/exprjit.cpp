#include <cmath>
#include <istream>
#include <sstream>
#include <map>

#include "NativeJIT/CodeGen/ExecutionBuffer.h"
#include "NativeJIT/CodeGen/FunctionBuffer.h"
#include "NativeJIT/Function.h"

#include "exprjit.h"

// Size of the JIT compiler buffers
constexpr size_t CODE_BUFFER_SIZE = 8192;

namespace nj = NativeJIT;

//----------------------------------------------------------
//  Helper class to contruct error messages
//----------------------------------------------------------
class MessageConstructor final
{
public:
    MessageConstructor(std::string &str)
        : m_str(str)
    {
    }

    ~MessageConstructor()
    {
        m_str = m_ss.str();
    }

    template <typename T>
    MessageConstructor& operator << (const T &v)
    {
        m_ss << v;
        return *this;
    }

private:
    std::stringstream m_ss;
    std::string &m_str;
};


//----------------------------------------------------------
//  Symbols table
//----------------------------------------------------------
class SymbolTable
{
public:

    SymbolTable()
    {
    }

    bool hasVar(const std::string &name) const
    {
        return m_vars.find(name) != m_vars.end();
    }

    ExprJIT::Real* varPtr(const std::string &name)
    {
        ExprJIT::Real *ptr = nullptr;
        if (hasVar(name)) {
            ptr = &m_vars[name];
        }
        return ptr;
    }

    ExprJIT::Real& operator[](const std::string &name)
    {
        return m_vars[name];
    }


private:

    std::map<std::string, ExprJIT::Real> m_vars;
};

//----------------------------------------------------------
//  Expression parser
//----------------------------------------------------------
class Parser final
{
public:
    using Node = nj::Node<ExprJIT::Real>;

    typedef ExprJIT::Real (*FunctionPtr)(ExprJIT::Real);

    Parser(nj::ExpressionNodeFactory &nodeFactory, SymbolTable &symbols)
        : m_nodeFactory(nodeFactory),
          m_symbols(symbols),
          m_error(false),
          m_message()
    {
    }

    inline bool error() const { return m_error; }
    const std::string& message() const { return m_message; }

    Node& parse(const std::string &str)
    {
        std::istringstream ss(str);
        return parseExpression(ss);
    }

private:

// Helper macro to generate parsing error messages
#define PARSE_ERR m_error=true; MessageConstructor(m_message) << in.tellg() << ": "

    /**
     * @brief Tells whether  a character is a shitespace.
     * @param c
     * @return
     */
    static inline bool isSpace(int c)
    {
        return (c == ' ') || (c == '\t') || (c == '\r') || (c == '\n');
    }

    /**
     * @brief Skip whitespaces in the input stream.
     *
     * This will advance the input stream until there is a non-whitespace
     * character in there, or end of stream.
     *
     * @param in
     */
    static void skipSpace(std::istream &in)
    {
        while(Parser::isSpace(in.peek())) {
            in.get();
        }
    }

    /**
     * @brief Extract a positive integer value from the stream.
     * @param in
     * @param value
     * @return
     */
    static bool parseInteger(std::istream &in, long long int &value)
    {
        long long int tmp = 0;
        bool ok = false;
        auto c = in.peek();
        while (c >= '0' && c <= '9') {
            tmp = 10*tmp + in.get() - '0';
            c = in.peek();
            ok = true;
        }

        if (ok) {
            value = tmp;
        }

        return ok;
    }

    /**
     * @brief 1/x function
     *
     * NativeJIT does not provide division operation, so we have
     * to implement it as a function.
     *
     * @param x
     * @return
     */
    static ExprJIT::Real invf(ExprJIT::Real x)
    {
        return 1.0 / x;
    }

    /**
     * @brief Parse a floating point number.
     * @param in
     * @return Floating point immediate value JIT node.
     */
    Node& parseNumber(std::istream &in)
    {
        ExprJIT::Real tmp = 0.0;
        long long int tmp_i = 0;
        long long int tmp_r = 0;
        long long int tmp_e = 0;
        bool neg = false;

        Parser::skipSpace(in);

        auto c = in.peek();
        if (c == '-') {
            in.get();
            neg = true;
        }

        bool ok = parseInteger(in, tmp_i);
        if (ok) {
            c = in.peek();
            if (c == '.') {
                in.get();
                ok = parseInteger(in, tmp_r);
                c = in.peek();
            }
            if (ok && (c == 'E' || c == 'e')) {
                in.get();
                c = in.peek();
                bool neg = false;
                if (c == '-') {
                    in.get();
                    neg = true;
                }
                ok = parseInteger(in, tmp_e);
                if (ok && neg) {
                    tmp_e = -tmp_e;
                }
            }
        }

        if (ok) {
            tmp = static_cast<ExprJIT::Real>(tmp_i);
            long long int i = tmp_r;
            long long int r = 1;
            while (i > 0) {
                i /= 10;
                r *= 10;
            }
            tmp += static_cast<ExprJIT::Real>(tmp_r) / static_cast<ExprJIT::Real>(r);

            if (tmp_e != 0) {
                tmp *= pow(10.0, tmp_e);
            }

            if (neg) {
                tmp *= -1.0;
            }
        } else {
            PARSE_ERR << "Unable to parse number";
        }

        return m_nodeFactory.Immediate(tmp);
    }

    /**
     * @brief Parse an expression
     * @param in
     * @return
     */
    Node& parseExpression(std::istream &in)
    {
        return parseAddSub(in);
    }

    /**
     * @brief Parse a symbol (function or names constant).
     * @param in
     * @return
     */
    Node& parseSymbol(std::istream &in)
    {
        skipSpace(in);
        auto c = in.peek();
        std::string identifier;

        while ((c >= 'a' && c <= 'z') ||
               (c >= 'A' && c <= 'Z') ||
               (c >= '0' && c <= '9') ||
               (c == '_')) {

            identifier += static_cast<char>(in.get());
            c = in.peek();
        }

        skipSpace(in);
        c = in.peek();
        if (c == '(') {
            // Function call
            auto it = Parser::stdFunctions.find(identifier);
            if (it != Parser::stdFunctions.end()) {
                Node &arg = parseTerm(in);
                auto &call = m_nodeFactory.Immediate(it->second);
                return m_nodeFactory.Call(call, arg);
            }
        } else {
            // Variable reference
            ExprJIT::Real *ptr = m_symbols.varPtr(identifier);
            if (ptr != nullptr) {
                auto &imm = m_nodeFactory.Immediate(ptr);
                return m_nodeFactory.Deref(imm);
            }
        }

        PARSE_ERR << "Unknown symbol '" << identifier << "'";
        return m_nodeFactory.Immediate(0.0);
    }

    Node& parseTerm(std::istream &in)
    {
        skipSpace(in);
        auto c = in.peek();
        if ((c >= '0' && c <= '9')) {
            return parseNumber(in);
        } else if (c == '-') {
            in.get();
            c = in.peek();
            if ((c >= '0' && c <= '9')) {
                in.putback('-');
                return parseNumber(in);
            } else {
                Node &lnode = m_nodeFactory.Immediate(0.0);
                Node &rnode = parseTerm(in);
                return m_nodeFactory.Sub(lnode, rnode);
            }
        } else if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c == '_')) {
            return parseSymbol(in);
        } else if (c == '(') {
            in.get();
            auto &n = parseExpression(in);
            if (!m_error) {
                skipSpace(in);
                c = in.peek();
                if (c == ')') {
                    in.get();
                } else {
                    PARSE_ERR << "Expected ')'";
                }
            }
            return n;
        } else {
            PARSE_ERR << "Unexpected character '" << static_cast<char>(c) << "'";
        }

        return m_nodeFactory.Immediate(0.0);
    }

    Node& parseMulDiv(std::istream &in)
    {
        Node &lnode = parseTerm(in);
        if (!error()) {
            bool done = false;
            do {
                skipSpace(in);
                auto c = in.peek();
                if (c == '*') {
                    in.get();
                    Node &rnode = parseMulDiv(in);
                    return m_nodeFactory.Mul(lnode, rnode);
                } else if (c == '/') {
                    in.get();
                    Node &rnode = parseMulDiv(in);
                    // There is no division node so we have to implement is as a function call
                    auto &call = m_nodeFactory.Immediate(Parser::invf);
                    auto &rrnode = m_nodeFactory.Call(call, rnode);
                    return m_nodeFactory.Mul(lnode, rrnode);
                } else {
                    done = true;
                }
            } while (!done);
        }

        return lnode;
    }

    Node& parseAddSub(std::istream &in)
    {
        Node &lnode = parseMulDiv(in);
        if (!error()) {
            bool done = false;
            do {
                skipSpace(in);
                auto c = in.peek();
                if (c == '+') {
                    in.get();
                    Node &rnode = parseAddSub(in);
                    return m_nodeFactory.Add(lnode, rnode);
                } else if (c == '-') {
                    in.get();
                    Node &rnode = parseAddSub(in);
                    return m_nodeFactory.Sub(lnode, rnode);
                } else {
                    done = true;
                }
            } while (!done);
        }

        return lnode;
    }


#undef PARSE_ERR

    nj::ExpressionNodeFactory &m_nodeFactory;
    SymbolTable &m_symbols;
    bool m_error;           ///< Parsing error flag.
    std::string m_message;  ///< Error message.

    /// Standard functions
    const static std::map<std::string, FunctionPtr> stdFunctions;
};

//----------------------------------------------------------
//  Standard functions
//----------------------------------------------------------

namespace func {

static ExprJIT::Real sin(ExprJIT::Real x) { return ::sin(x); }
static ExprJIT::Real cos(ExprJIT::Real x) { return ::cos(x); }
static ExprJIT::Real sqrt(ExprJIT::Real x) { return ::sqrt(x); }

}

const std::map<std::string, Parser::FunctionPtr> Parser::stdFunctions = {
    { "sqrt",   func::sqrt },
    { "sin",    func::sin },
    { "cos",    func::cos }
};

//----------------------------------------------------------
//  NativeJIT compiler wrapper
//----------------------------------------------------------
struct Compiler
{
    nj::ExecutionBuffer codeAllocator;
    nj::Allocator allocator;
    nj::FunctionBuffer code;
    nj::Function<ExprJIT::Real> expression;
    nj::Function<ExprJIT::Real>::FunctionType func = nullptr;

    Parser parser;

    Compiler(SymbolTable &symbols, const size_t capacity = CODE_BUFFER_SIZE)
        : codeAllocator(capacity),
          allocator(capacity),
          code(codeAllocator, static_cast<unsigned>(capacity)),
          expression(allocator, code),
          func(nullptr),
          parser(expression, symbols)
    {
    }

    bool compile(const std::string &str)
    {
        Parser::Node& node = parser.parse(str);
        if (!parser.error()) {
            func = expression.Compile(node);
        }

        return func != nullptr;
    }

    ExprJIT::Real eval() const
    {
        if (func != nullptr) {
            return func();
        }
        return 0.0;
    }

};

//----------------------------------------------------------
//  ExprJIT private implementation
//----------------------------------------------------------

struct ExprJIT::Impl
{
    SymbolTable symbols;
    std::unique_ptr<Compiler> compiler;

    Impl()
        : compiler(nullptr)
    {
    }

    bool compile(const std::string &str)
    {
        compiler = std::make_unique<Compiler>(symbols, CODE_BUFFER_SIZE);
        return compiler->compile(str);
    }

    std::string error() const
    {
        if (compiler != nullptr) {
            return compiler->parser.message();
        }

        return std::string();
    }

    ExprJIT::Real eval() const
    {
        return compiler->eval();
    }

};

//----------------------------------------------------------
//  ExprJIT public interface
//----------------------------------------------------------

ExprJIT::ExprJIT()
    : d(std::make_unique<Impl>())
{
}

ExprJIT::~ExprJIT() = default;

bool ExprJIT::compile(const std::string &str)
{
    return d->compile(str);
}

std::string ExprJIT::error() const
{
    return d->error();
}

ExprJIT::Real ExprJIT::eval() const
{
    return d->eval();
}

ExprJIT::Real& ExprJIT::operator[](const std::string &name)
{
    return d->symbols[name];
}
