#include <cmath>
#include "catch.hpp"
#include "exprjit.h"


TEST_CASE("Test basic expressions")
{
    ExprJIT expr;

    REQUIRE(expr("1 + 2*3"));
    REQUIRE(expr() == Approx(7.0));

    REQUIRE(expr("(1 + 2)*3"));
    REQUIRE(expr() == Approx(9.0));

    REQUIRE(expr("(7 - 2)*(5 - 2)"));
    REQUIRE(expr() == Approx(15.0));

    REQUIRE(expr("8/2*0.5*1e-1"));
    REQUIRE(expr() == Approx(0.2));
}

//----------------------------------------------------------

TEST_CASE("Test external variables")
{
    ExprJIT expr;

    REQUIRE_FALSE(expr("x"));

    expr["x"] = 1.0;
    REQUIRE(expr("x"));

    REQUIRE(expr() == Approx(1.0));

    expr["x"] = 10.0;
    REQUIRE(expr() == Approx(10.0));

    expr["x"] = 2.0;

    REQUIRE(expr("x + x + x"));
    REQUIRE(expr() == Approx(6.0));
}

//----------------------------------------------------------

TEST_CASE("Test standard functions")
{
    ExprJIT expr;

    REQUIRE(expr("sqrt(16.0)"));
    REQUIRE(expr() == Approx(4.0));

    REQUIRE(expr("sin(0.0)"));
    REQUIRE(expr() == Approx(0.0));

    REQUIRE(expr("cos(0.0)"));
    REQUIRE(expr() == Approx(1.0));

    REQUIRE_FALSE(expr("undefined(0.0)"));
}

//----------------------------------------------------------

TEST_CASE("Test 2-argument functions")
{
    ExprJIT expr;

    REQUIRE(expr("min(5.0, 2.0)"));
    REQUIRE(expr() == Approx(2.0));

    REQUIRE(expr("min(2.0, 5.0)"));
    REQUIRE(expr() == Approx(2.0));

    REQUIRE(expr("max(5.0, 2.0)"));
    REQUIRE(expr() == Approx(5.0));

    REQUIRE(expr("max(2.0, 5.0)"));
    REQUIRE(expr() == Approx(5.0));
}

//----------------------------------------------------------

TEST_CASE("Test 3-argument functions")
{
    ExprJIT expr;
    expr["x"] = 0.0;

    REQUIRE(expr("clamp(x, -1, 1)"));

    REQUIRE(expr() == Approx(0.0));

    expr["x"] = 10.0;
    REQUIRE(expr() == Approx(1.0));

    expr["x"] = -10.0;
    REQUIRE(expr() == Approx(-1.0));
}

//----------------------------------------------------------

ExprJIT::Real func1(ExprJIT::Real x)
{
    return sqrt(x*x/2.0);
}

ExprJIT::Real func2(ExprJIT::Real x, ExprJIT::Real y)
{
    return sin(x*x)*cos(y*y);
}

ExprJIT::Real func3(ExprJIT::Real x, ExprJIT::Real y, ExprJIT::Real z)
{
    return sqrt(x*x + y*y + z*z);
}

TEST_CASE("Test custom functions")
{
    ExprJIT::Real x = 0.3;
    ExprJIT::Real y = 0.5;
    ExprJIT::Real z = 0.7;

    ExprJIT expr;
    expr["x"] = x;
    expr["y"] = y;
    expr["z"] = z;
    expr["func1"] = func1;
    expr["func2"] = func2;
    expr["func3"] = func3;

    REQUIRE(expr("func1(x)"));
    REQUIRE(expr() == Approx(func1(x)));
    REQUIRE(expr("func2(x, y)"));
    REQUIRE(expr() == Approx(func2(x, y)));
    REQUIRE(expr("func3(x, y, z)"));
    REQUIRE(expr() == Approx(func3(x, y, z)));
}

//----------------------------------------------------------

TEST_CASE("Test computed values")
{
    ExprJIT expr;
    double x = 0.1;
    expr["x"] = x;

    REQUIRE(expr("sin(x)/x"));

    while (x < 1.0) {
        expr["x"] = x;

        auto y = expr();
        auto z = sin(x) / x;
        REQUIRE(y == Approx(z));

        x += 0.1;
    }
}

//----------------------------------------------------------

TEST_CASE("Test multiple divisions")
{
    ExprJIT expr;
    double x = 2;
    expr["x"] = x;

    REQUIRE(expr("16/x/x/x/x"));
    REQUIRE(expr() == Approx(1.0));
}
