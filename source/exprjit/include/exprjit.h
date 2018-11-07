#ifndef EXPRJIT_H
#define EXPRJIT_H

#include <memory>
#include <string>

/**
 * @brief Evaluate math expressions via JIT compiler.
 */
class ExprJIT final
{
public:

    using Real = double;

    ExprJIT();
    ~ExprJIT();

    /**
     * @brief Compile an expression.
     * @param expression Expression to be compiled.
     * @return true on successful compilation, false on error.
     */
    bool compile(const std::string &expression);
    bool operator()(const std::string &expression) { return compile(expression); }

    /**
     * @brief Returns compilation error message.
     * @return
     */
    std::string error() const;

    /**
     * @brief Evaluate previously compiled expression.
     * @return
     */
    Real eval() const;
    Real operator()() const { return eval(); }

    /**
     * @brief Access symbol by its identifier.
     *
     * This method can be used to define new varibales
     * or change values of existing ones. When changing
     * variables values no recompilation is required -
     * previously compiled expression can be re-evaluated again.
     *
     * @param name Symbol's name.
     * @return
     */
    Real& operator[](const std::string &name);

private:
    ExprJIT(const ExprJIT&) = delete;
    ExprJIT& operator =(const ExprJIT&) = delete;

    // Implementation details.
    struct Impl;
    std::unique_ptr<Impl> d;
};

#endif // EXPRJIT_H
