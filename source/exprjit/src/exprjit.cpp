#include <cmath>
#include <istream>
#include <sstream>
#include <map>
#include <functional>

#include "NativeJIT/CodeGen/ExecutionBuffer.h"
#include "NativeJIT/CodeGen/FunctionBuffer.h"
#include "NativeJIT/Function.h"

#include "exprjit.h"

// Size of the JIT compiler buffers
constexpr size_t CODE_BUFFER_SIZE = 16384;

namespace nj = NativeJIT;

// Enable generated code assembly output
//#define EXPRJIT_ENABLE_ASM_OUTPUT


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
//  Standard functions
//----------------------------------------------------------

namespace func {

// 1-argument functions
static ExprJIT::Real abs(ExprJIT::Real x) { return ::abs(x); }
static ExprJIT::Real sqrt(ExprJIT::Real x) { return ::sqrt(x); }
static ExprJIT::Real exp(ExprJIT::Real x) { return ::exp(x); }
static ExprJIT::Real exp2(ExprJIT::Real x) { return ::exp2(x); }
static ExprJIT::Real log(ExprJIT::Real x) { return ::log(x); }
static ExprJIT::Real log2(ExprJIT::Real x) { return ::log2(x); }
static ExprJIT::Real log10(ExprJIT::Real x) { return ::log10(x); }
static ExprJIT::Real sin(ExprJIT::Real x) { return ::sin(x); }
static ExprJIT::Real cos(ExprJIT::Real x) { return ::cos(x); }
static ExprJIT::Real tan(ExprJIT::Real x) { return ::tan(x); }
static ExprJIT::Real asin(ExprJIT::Real x) { return ::asin(x); }
static ExprJIT::Real acos(ExprJIT::Real x) { return ::acos(x); }
static ExprJIT::Real atan(ExprJIT::Real x) { return ::atan(x); }
static ExprJIT::Real sinh(ExprJIT::Real x) { return ::sinh(x); }
static ExprJIT::Real cosh(ExprJIT::Real x) { return ::cosh(x); }
static ExprJIT::Real tanh(ExprJIT::Real x) { return ::tanh(x); }
static ExprJIT::Real asinh(ExprJIT::Real x) { return ::asinh(x); }
static ExprJIT::Real acosh(ExprJIT::Real x) { return ::acosh(x); }
static ExprJIT::Real atanh(ExprJIT::Real x) { return ::atanh(x); }
static ExprJIT::Real round(ExprJIT::Real x) { return ::round(x); }
static ExprJIT::Real ceil(ExprJIT::Real x) { return ::ceil(x); }
static ExprJIT::Real floor(ExprJIT::Real x) { return ::floor(x); }

// 2-argument functions
static ExprJIT::Real min(ExprJIT::Real x, ExprJIT::Real y) { return (x < y) ? x : y; }
static ExprJIT::Real max(ExprJIT::Real x, ExprJIT::Real y) { return (x > y) ? x : y; }
static ExprJIT::Real pow(ExprJIT::Real x, ExprJIT::Real y) { return ::pow(x, y); }
static ExprJIT::Real mod(ExprJIT::Real x, ExprJIT::Real y) { return ::fmod(x, y); }
static ExprJIT::Real atan2(ExprJIT::Real x, ExprJIT::Real y) { return ::atan2(x, y); }
static ExprJIT::Real hypot(ExprJIT::Real x, ExprJIT::Real y) { return ::hypot(x, y); }

// 3-argument functions
static ExprJIT::Real clamp(ExprJIT::Real x, ExprJIT::Real a, ExprJIT::Real b)
{
    if (x < a) {
        return a;
    } else if (x > b) {
        return b;
    }
    return x;
}

} // namespace func

//----------------------------------------------------------
//  Symbols table
//----------------------------------------------------------
class SymbolTable
{
public:

    SymbolTable()
        : m_vars()
    {
        defineStandardFunctions();
    }

    void defineStandardFunctions()
    {
        setFunc("abs",      func::abs);
        setFunc("sqrt",     func::sqrt);
        setFunc("exp",      func::exp);
        setFunc("exp2",     func::exp2);
        setFunc("log",      func::log);
        setFunc("log2",     func::log2);
        setFunc("log10",    func::log10);
        setFunc("sin",      func::sin);
        setFunc("cos",      func::cos);
        setFunc("tan",      func::tan);
        setFunc("asin",     func::asin);
        setFunc("acos",     func::acos);
        setFunc("atan",     func::atan);
        setFunc("sinh",     func::sinh);
        setFunc("cosh",     func::cosh);
        setFunc("tanh",     func::tanh);
        setFunc("asinh",    func::asinh);
        setFunc("acosh",    func::acosh);
        setFunc("atanh",    func::atanh);
        setFunc("round",    func::round);
        setFunc("ceil",     func::ceil);
        setFunc("floor",    func::floor);

        setFunc("min",      func::min);
        setFunc("max",      func::max);
        setFunc("pow",      func::pow);
        setFunc("mod",      func::mod);
        setFunc("atan2",    func::atan2);
        setFunc("hypot",    func::hypot);

        setFunc("clamp",    func::clamp);
    }

    ExprJIT::Real* varPtr(const std::string &name)
    {
        if (m_vars.find(name) != m_vars.end()) {
            return &m_vars[name];
        }
        return nullptr;
    }

    ExprJIT::Function1Ptr func1Ptr(const std::string &name)
    {
        if (m_func1.find(name) != m_func1.end()) {
            return m_func1.at(name);
        }
        return nullptr;
    }

    ExprJIT::Function2Ptr func2Ptr(const std::string &name)
    {
        if (m_func2.find(name) != m_func2.end()) {
            return m_func2.at(name);
        }
        return nullptr;
    }

    ExprJIT::Function3Ptr func3Ptr(const std::string &name)
    {
        if (m_func3.find(name) != m_func3.end()) {
            return m_func3.at(name);
        }
        return nullptr;
    }

    void setVar(const std::string &name, const ExprJIT::Real value)
    {
        m_vars[name] = value;
    }

    void setFunc(const std::string &name, ExprJIT::Function1Ptr func)
    {
        m_func1[name] = func;
    }

    void setFunc(const std::string &name, ExprJIT::Function2Ptr func)
    {
        m_func2[name] = func;
    }

    void setFunc(const std::string &name, ExprJIT::Function3Ptr func)
    {
        m_func3[name] = func;
    }

private:

    std::map<std::string, ExprJIT::Real> m_vars;
    std::map<std::string, ExprJIT::Function1Ptr> m_func1;   ///< With 1 argument
    std::map<std::string, ExprJIT::Function2Ptr> m_func2;   ///< With 2 arguments
    std::map<std::string, ExprJIT::Function3Ptr> m_func3;   ///< With 3 arguments

};

//----------------------------------------------------------
//  Expression parser
//----------------------------------------------------------
class Parser final
{
public:
    using Node = nj::Node<ExprJIT::Real>;

    Parser(nj::ExpressionNodeFactory &nodeFactory, SymbolTable &symbols)
        : m_nodeFactory(nodeFactory),
          m_symbols(symbols),
          m_error(false),
          m_message(),
          m_symbolsCache()
    {
    }

    inline bool error() const { return m_error; }
    const std::string& message() const { return m_message; }

    Node& parse(const std::string &str, bool &isConstant, ExprJIT::Real &constValue)
    {
        m_symbolsCache.clear();

        std::istringstream ss(str);
        return parseExpression(ss, isConstant, constValue);
    }

private:

// Helper macro to generate parsing error messages
#define PARSE_ERR m_error=true; MessageConstructor(m_message) << in.tellg() << ": "

    static void markUnused(Node &node)
    {
        // TODO: What is the right way to optimize-out a node?
        node.IncrementParentCount();
    }

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
     * @param value Parsed value
     * @return Floating point immediate value JIT node.
     */
    Node& parseNumber(std::istream &in, ExprJIT::Real &value)
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

        value = tmp;
        return m_nodeFactory.Immediate(tmp);
    }

    /**
     * @brief Parse an expression
     * @param in
     * @return
     */
    Node& parseExpression(std::istream &in, bool &isConstant, ExprJIT::Real &constValue)
    {
        return parseAddSub(in, isConstant, constValue);
    }

    /**
     * @brief Parse a symbol (function or names constant).
     * @param in
     * @return
     */
    Node& parseSymbol(std::istream &in, bool &isConstant, ExprJIT::Real &constValue)
    {
        isConstant = false;

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
            in.get();
            bool isC0 = false;
            ExprJIT::Real cVal0;
            Node &arg0 = parseTerm(in, isC0, cVal0);
            Parser::skipSpace(in);
            c = in.peek();
            if (c == ')') {
                in.get();
                // 1-argument function
                auto funcPtr = m_symbols.func1Ptr(identifier);
                if (funcPtr != nullptr) {
                    if (isC0) {
                        isConstant = true;
                        constValue = funcPtr(cVal0);
                        markUnused(arg0);
                        return m_nodeFactory.Immediate(constValue);
                    }
                    auto &func = m_nodeFactory.Immediate(funcPtr);
                    return m_nodeFactory.Call(func, arg0);
                }
            } else if (c == ',') {
                in.get();
                bool isC1 = false;
                ExprJIT::Real cVal1;
                Node &arg1 = parseTerm(in, isC1, cVal1);
                Parser::skipSpace(in);
                c = in.peek();
                if (c == ')') {
                    in.get();
                    // 2-arguments
                    auto funcPtr = m_symbols.func2Ptr(identifier);
                    if (funcPtr != nullptr) {
                        if (isC0 && isC1) {
                            isConstant = true;
                            constValue = funcPtr(cVal0, cVal1);
                            markUnused(arg0);
                            markUnused(arg1);
                            return m_nodeFactory.Immediate(constValue);
                        }
                        auto &func = m_nodeFactory.Immediate(funcPtr);
                        return m_nodeFactory.Call(func, arg0, arg1);
                    }
                } else if (c == ',') {
                    in.get();
                    bool isC2 = false;
                    ExprJIT::Real cVal2;
                    Node &arg2 = parseTerm(in, isC2, cVal2);
                    Parser::skipSpace(in);
                    c = in.peek();
                    if (c == ')') {
                        in.get();
                        // 3-arguments
                        auto funcPtr = m_symbols.func3Ptr(identifier);
                        if (funcPtr != nullptr) {
                            if (isC0 && isC1 && isC2) {
                                isConstant = true;
                                constValue = funcPtr(cVal0, cVal1, cVal2);
                                markUnused(arg0);
                                markUnused(arg1);
                                markUnused(arg2);
                                return m_nodeFactory.Immediate(constValue);
                            }
                            auto &func = m_nodeFactory.Immediate(funcPtr);
                            return m_nodeFactory.Call(func, arg0, arg1, arg2);
                        }
                    } else if (c == ',') {
                        // Too many arguments
                        PARSE_ERR << "Too many arguments for '" << identifier << "' function call";
                        return m_nodeFactory.Immediate(0.0);
                    }
                }
            }

        } else {

            // Variable reference
            ExprJIT::Real *ptr = m_symbols.varPtr(identifier);
            if (ptr != nullptr) {
                // Look of there is a cached node for this variable
                auto it = m_symbolsCache.find(identifier);
                if (it == m_symbolsCache.end()) {
                    // Create the reference node and cache it
                    auto &imm = m_nodeFactory.Immediate(ptr);
                    auto &node = m_nodeFactory.Deref(imm);
                    m_symbolsCache.insert(std::pair<std::string, Node&>(identifier, node));
                    return node;
                } else {
                    // Return cached node
                    return it->second;
                }
            }
        }

        PARSE_ERR << "Unknown symbol '" << identifier << "'";
        return m_nodeFactory.Immediate(0.0);
    }

    Node& parseTerm(std::istream &in, bool &isConstant, ExprJIT::Real &constValue)
    {
        isConstant = false;

        skipSpace(in);
        auto c = in.peek();
        if ((c >= '0' && c <= '9')) {
            isConstant = true;
            return parseNumber(in, constValue);
        } else if (c == '-') {
            in.get();
            c = in.peek();
            if ((c >= '0' && c <= '9')) {
                in.putback('-');
                isConstant = true;
                return parseNumber(in, constValue);
            } else {
                bool isC = false;
                ExprJIT::Real cVal;
                Node &rnode = parseTerm(in, isC, cVal);
                if (isC) {
                    markUnused(rnode);
                    cVal = -cVal;
                    isConstant = true;
                    constValue = cVal;
                    return m_nodeFactory.Immediate(cVal);
                } else {
                    Node &lnode = m_nodeFactory.Immediate(0.0);
                    return m_nodeFactory.Sub(lnode, rnode);
                }
            }
        } else if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c == '_')) {
            return parseSymbol(in, isConstant, constValue);
        } else if (c == '(') {
            in.get();
            auto &n = parseExpression(in, isConstant, constValue);
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

    Node& parseMulDiv(std::istream &in, bool &isConstant, ExprJIT::Real &constValue)
    {
        bool isC = false;
        ExprJIT::Real cVal;

        isConstant = false;

        ExprJIT::Real accumulator = 1.0;
        std::vector<std::reference_wrapper<Node> > mulNodes;
        std::vector<std::reference_wrapper<Node> > divNodes;

        Node &lnode = parseTerm(in, isC, cVal);
        if (isC) {
            accumulator = cVal;
            markUnused(lnode);
        } else {
            mulNodes.push_back(lnode);
        }

        if (!error()) {
            bool done = false;
            do {
                skipSpace(in);
                auto c = in.peek();
                if (c == '*') {
                    in.get();
                    Node &rnode = parseTerm(in, isC, cVal);
                    if (isC) {
                        accumulator *= cVal;
                        markUnused(rnode);
                    } else {
                        mulNodes.push_back(rnode);
                    }
                } else if (c == '/') {
                    in.get();
                    Node &rnode = parseTerm(in, isC, cVal);
                    if (isC) {
                        accumulator /= cVal;
                        markUnused(rnode);
                    } else {
                        divNodes.push_back(rnode);
                    }

                } else {
                    done = true;
                }
            } while (!done);
        }

        // There is no division node so we have to implement is as a function call

        if (!mulNodes.empty()) {
            std::reference_wrapper<Node> n = mulNodes.front();
            for (size_t i = 1; i < mulNodes.size(); i++) {
                Node &p = mulNodes.at(i);
                n = m_nodeFactory.Mul(n.get(), p);
            }
            if (!divNodes.empty()) {
                std::reference_wrapper<Node> p = divNodes.front();
                for (size_t i = 1; i < divNodes.size(); i++) {
                    Node &d = divNodes.at(i);
                    p = m_nodeFactory.Mul(p.get(), d);
                }
                auto &call = m_nodeFactory.Immediate(Parser::invf);
                p = m_nodeFactory.Call(call, p.get());
                n = m_nodeFactory.Mul(n.get(), p.get());
            }
            if (accumulator != 1.0) {
                n = m_nodeFactory.Mul(n.get(), m_nodeFactory.Immediate(accumulator));
            }
            return n.get();
        }

        if (!divNodes.empty()) {
            std::reference_wrapper<Node> n = divNodes.front();
            for (size_t i = 1; i < divNodes.size(); i++) {
                Node &p = divNodes.at(i);
                n = m_nodeFactory.Mul(n.get(), p);
            }
            auto &call = m_nodeFactory.Immediate(Parser::invf);
            n = m_nodeFactory.Call(call, n.get());
            if (accumulator != 1.0) {
                n = m_nodeFactory.Mul(n.get(), m_nodeFactory.Immediate(accumulator));
            }
            return n.get();
        }

        isConstant = true;
        constValue = accumulator;
        return m_nodeFactory.Immediate(constValue);
    }

    Node& parseAddSub(std::istream &in, bool &isConstant, ExprJIT::Real &constValue)
    {
        bool isC = false;
        ExprJIT::Real cVal;

        isConstant = false;

        ExprJIT::Real accumulator = 0.0;
        std::vector<std::reference_wrapper<Node> > addNodes;
        std::vector<std::reference_wrapper<Node> > subNodes;

        Node &lnode = parseMulDiv(in, isC, cVal);
        if (isC) {
            accumulator = cVal;
            markUnused(lnode);
        } else {
            addNodes.push_back(lnode);
        }

        if (!error()) {
            bool done = false;
            do {
                skipSpace(in);
                auto c = in.peek();
                if (c == '+') {
                    in.get();
                    Node &rnode = parseMulDiv(in, isC, cVal);
                    if (isC) {
                        accumulator += cVal;
                        markUnused(rnode);
                    } else {
                        addNodes.push_back(rnode);
                    }
                } else if (c == '-') {
                    in.get();
                    Node &rnode = parseMulDiv(in, isC, cVal);
                    if (isC) {
                        accumulator -= cVal;
                        markUnused(rnode);
                    } else {
                        subNodes.push_back(rnode);
                    }
                } else {
                    done = true;
                }
            } while (!done);
        }

        if (!addNodes.empty()) {
            std::reference_wrapper<Node> n = addNodes.front();
            for (size_t i = 1; i < addNodes.size(); i++) {
                Node &p = addNodes.at(i);
                n = m_nodeFactory.Add(n.get(), p);
            }
            for (auto &&p : subNodes) {
                n = m_nodeFactory.Sub(n.get(), p.get());
            }
            if (accumulator != 0.0) {
                n = m_nodeFactory.Add(n.get(), m_nodeFactory.Immediate(accumulator));
            }
            return n;
        }

        if (!subNodes.empty()) {
            std::reference_wrapper<Node> n = m_nodeFactory.Immediate(accumulator);
            for (auto &&p : subNodes) {
                n = m_nodeFactory.Sub(n.get(), p.get());
            }
            return n;
        }

        isConstant = true;
        constValue = accumulator;
        return m_nodeFactory.Immediate(constValue);
    }


#undef PARSE_ERR

    nj::ExpressionNodeFactory &m_nodeFactory;
    SymbolTable &m_symbols; ///< External variables.
    bool m_error;           ///< Parsing error flag.
    std::string m_message;  ///< Error message.

    /// Cache for variables Deref nodes.
    std::map<std::string, Node&> m_symbolsCache;
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

    static ExprJIT::Real returnZero()
    {
        return 0.0;
    }

    Compiler(SymbolTable &symbols, const size_t capacity = CODE_BUFFER_SIZE)
        : codeAllocator(capacity),
          allocator(capacity),
          code(codeAllocator, static_cast<unsigned>(capacity)),
          expression(allocator, code),
          func(Compiler::returnZero),
          parser(expression, symbols)
    {
        // This will output generated assembly
#ifdef EXPRJIT_ENABLE_ASM_OUTPUT
        code.EnableDiagnostics(std::cout);
#endif
    }

    bool compile(const std::string &str)
    {
        bool isConstant;
        ExprJIT::Real constValue;

        Parser::Node& node = parser.parse(str, isConstant, constValue);
        if (!parser.error()) {
            func = expression.Compile(node);
        }

        return !parser.error();
    }

    ExprJIT::Real eval() const
    {
        // func should always point to a valid function,
        // either to returnZero() one or the one being compiled.
        return func();
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
//  ExprJIT::SymbolReference
//----------------------------------------------------------

ExprJIT::SymbolReference::SymbolReference(ExprJIT &exprjit, const std::string &name)
    : m_exprjit(exprjit),
      m_name(name)
{
}

ExprJIT::SymbolReference& ExprJIT::SymbolReference::operator =(const ExprJIT::Real value)
{
    m_exprjit.setSymbol(m_name, value);
    return *this;
}

ExprJIT::SymbolReference& ExprJIT::SymbolReference::operator =(Function1Ptr func)
{
    m_exprjit.setSymbol(m_name, func);
    return *this;
}

ExprJIT::SymbolReference& ExprJIT::SymbolReference::operator =(Function2Ptr func)
{
    m_exprjit.setSymbol(m_name, func);
    return *this;
}

ExprJIT::SymbolReference& ExprJIT::SymbolReference::operator =(Function3Ptr func)
{
    m_exprjit.setSymbol(m_name, func);
    return *this;
}

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

ExprJIT::SymbolReference ExprJIT::operator[](const std::string &name)
{
    return SymbolReference(*this, name);
}

void ExprJIT::setSymbol(const std::string &name, const ExprJIT::Real value)
{
    d->symbols.setVar(name, value);
}

void ExprJIT::setSymbol(const std::string &name, Function1Ptr func)
{
    d->symbols.setFunc(name, func);
}

void ExprJIT::setSymbol(const std::string &name, Function2Ptr func)
{
    d->symbols.setFunc(name, func);
}

void ExprJIT::setSymbol(const std::string &name, Function3Ptr func)
{
    d->symbols.setFunc(name, func);
}
