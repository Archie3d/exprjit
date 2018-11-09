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

TEST_CASE("Test computed values")
{
    ExprJIT expr;
    expr["x"] = 0.0;

    REQUIRE(expr("sin(x)/x"));

    double x = 1.0;
    while (x < 1.0) {
        expr["x"] = x;

        auto y = expr();
        auto z = sin(x) / x;
        REQUIRE(y == Approx(z));

        x += 0.1;
    }
}

