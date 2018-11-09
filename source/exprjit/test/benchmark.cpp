#include <cmath>
#include "catch.hpp"
#include "exprjit.h"

double cube(double x)
{
    return x*x*x;
}

double sinx_x(double x)
{
    return sin(x)/x;
}

double series(double x, double y)
{
    return x/y + x*x/y/y + x*x*x/y/y/y + x*x*x*x/y*y*y*y;
}

double constval(double x)
{
    return 0.1 + 0.2 * 0.3 / 0.4 - 0.5 + 0.6 + x;
}

//----------------------------------------------------------

TEST_CASE("Benchmarking x^3")
{
    double x = 2.0;

    ExprJIT expr;
    expr["x"] = x;
    REQUIRE(expr("x*x*x"));

    double r = 0.0;

    BENCHMARK("Native") {
        r = cube(x);
    }

    BENCHMARK("JIT-compiled") {
        r = expr();
    }
    REQUIRE(r == Approx(cube(x)));
}

//----------------------------------------------------------

TEST_CASE("Benchmarking sin(x)/x")
{
    double x = 0.5;

    ExprJIT expr;
    expr["x"] = x;
    REQUIRE(expr("sin(x)/x"));

    double r = 0.0;
    BENCHMARK("Native") {
        r = sinx_x(x);
    }

    BENCHMARK("JIT-compiled") {
        r = expr();
    }
    REQUIRE(r == Approx(sinx_x(x)));
}

//----------------------------------------------------------

TEST_CASE("Benchmarking series")
{
    double x = 0.5;
    double y = 0.2;

    ExprJIT expr;
    expr["x"] = x;
    expr["y"] = y;
    REQUIRE(expr("x/y + x*x/y/y + x*x*x/y/y/y + x*x*x*x/y*y*y*y"));

    double r = 0.0;
    BENCHMARK("Native") {
        r = series(x, y);
    }

    BENCHMARK("JIT-compiled") {
        r = expr();
    }
    REQUIRE(r == Approx(series(x, y)));
}

//----------------------------------------------------------

TEST_CASE("Benchmarking constexpr optimization")
{
    double x = 0.5;

    ExprJIT expr;
    expr["x"] = x;
    REQUIRE(expr("0.1 + 0.2 * 0.3 / 0.4 - 0.5 + 0.6 + x"));

    double r = 0.0;
    BENCHMARK("Native") {
        r = constval(x);
    }

    BENCHMARK("JIT-compiled") {
        r = expr();
    }
    REQUIRE(r == Approx(constval(x)));
}
