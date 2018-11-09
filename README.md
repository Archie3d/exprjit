# exprjit
Mathematical expressions evaluation using JIT compiler.

This is a simple parser that wraps [NativeJIT](https://github.com/BitFunnel/NativeJIT) library to compile and evaluate mathematical expressions.

Example:
```cpp
ExprJIT expr;

// Compile expression
bool ok = expr("0.25 + 1.2e-3 * 5");
if (ok) {
    // Evaluate compiled expression
    auto res = expr();
    std::cout << "Result: " << res << "\n";
} else {
    std::cerr << "Error: " << expr.error() << "\n";
}
```

It is possible to use custom variables in expressions:
```cpp
ExprJIT expr;

// Define variable x
expr["x"] = 0.0;

// Compile expression
bool ok = expr("3*x - 2");

// Evaluate compiled expression for a range of values of x
double x = 0.0;
while (x < 10.0) {
    expr["x"] = x;
    std::cout << x << " : " << expr() << "\n";
    x += 1.0;
}
```

There are some predefined functions available, like `sqrt`, `sin`, and `cos`:
```cpp
ExprJIT expr;

expr["pi"] = 3.1415926;
bool ok = expr("sin(pi/4) + cos(pi/6) / sqrt(2)");
auto res = expr();
std::cout << "Result: " << res << "\n";
```
All values in expression are treated and evaluated as `double` type.

The parser will evaluate constant expressions on the fly in order to minimize the generated JIT-compiled code as much as possible.
