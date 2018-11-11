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

    using Function1Ptr = ExprJIT::Real (*)(ExprJIT::Real);
    using Function2Ptr = ExprJIT::Real (*)(ExprJIT::Real, ExprJIT::Real);
    using Function3Ptr = ExprJIT::Real (*)(ExprJIT::Real, ExprJIT::Real, ExprJIT::Real);

    // Symbol identifier wrapper used to expose variables
    // and native functions.
    class SymbolReference
    {
    public:
        explicit SymbolReference(ExprJIT &exprjit, const std::string &name);
        SymbolReference& operator =(const ExprJIT::Real value);
        SymbolReference& operator =(Function1Ptr func);
        SymbolReference& operator =(Function2Ptr func);
        SymbolReference& operator =(Function3Ptr func);
    private:
        ExprJIT& m_exprjit;
        std::string m_name;
    };

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
    SymbolReference operator[](const std::string &name);

private:

    friend class ExprJIT::SymbolReference;

    ExprJIT(const ExprJIT&) = delete;
    ExprJIT& operator =(const ExprJIT&) = delete;

    void setSymbol(const std::string &name, const ExprJIT::Real value);
    void setSymbol(const std::string &name, Function1Ptr func);
    void setSymbol(const std::string &name, Function2Ptr func);
    void setSymbol(const std::string &name, Function3Ptr func);

    // Implementation details.
    struct Impl;
    std::unique_ptr<Impl> d;
};

#endif // EXPRJIT_H
