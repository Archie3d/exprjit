#include "catch.hpp"
#include "exprjit.h"


TEST_CASE("Test basic expressions")
{
    ExprJIT expr;

    REQUIRE(expr("1 + 2*3"));
    REQUIRE(expr() == Approx(7.0));

    REQUIRE(expr("(1 + 2)*3"));
    REQUIRE(expr() == Approx(9.0));
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
