#include <gtest/gtest.h>
#include <common/integrator.hpp>

#include <cmath>

using namespace integration;

// ── Rectangle rule ────────────────────────────────────────────────────────────

TEST(IntegratorRectangle, SinZeroToPi) {
    // ∫₀^π sin(x) dx = 2
    double result = integrate([](double x) { return std::sin(x); },
                              0.0, M_PI, 1'000'000, Method::RECTANGLE);
    EXPECT_NEAR(result, 2.0, 1e-6);
}

TEST(IntegratorRectangle, ConstantOne) {
    // ∫₀¹ 1 dx = 1
    double result = integrate([](double) { return 1.0; },
                              0.0, 1.0, 100, Method::RECTANGLE);
    EXPECT_NEAR(result, 1.0, 1e-12);
}

TEST(IntegratorRectangle, LinearX) {
    // ∫₀¹ x dx = 0.5
    double result = integrate([](double x) { return x; },
                              0.0, 1.0, 1'000'000, Method::RECTANGLE);
    EXPECT_NEAR(result, 0.5, 1e-6);
}

// ── Trapezoid rule ────────────────────────────────────────────────────────────

TEST(IntegratorTrapezoid, SinZeroToPi) {
    double result = integrate([](double x) { return std::sin(x); },
                              0.0, M_PI, 1'000'000, Method::TRAPEZOID);
    EXPECT_NEAR(result, 2.0, 1e-6);
}

TEST(IntegratorTrapezoid, QuadraticX2) {
    // ∫₀¹ x² dx = 1/3
    double result = integrate([](double x) { return x * x; },
                              0.0, 1.0, 1'000'000, Method::TRAPEZOID);
    EXPECT_NEAR(result, 1.0 / 3.0, 1e-6);
}

// ── Simpson's rule ────────────────────────────────────────────────────────────

TEST(IntegratorSimpson, SinZeroToPi) {
    double result = integrate([](double x) { return std::sin(x); },
                              0.0, M_PI, 1'000'000, Method::SIMPSON);
    EXPECT_NEAR(result, 2.0, 1e-9);
}

TEST(IntegratorSimpson, CubicX3) {
    // ∫₀¹ x³ dx = 0.25
    double result = integrate([](double x) { return x * x * x; },
                              0.0, 1.0, 1'000'000, Method::SIMPSON);
    EXPECT_NEAR(result, 0.25, 1e-9);
}

TEST(IntegratorSimpson, OddNThrows) {
    EXPECT_THROW(
        integrate([](double x) { return x; }, 0.0, 1.0, 3, Method::SIMPSON),
        std::invalid_argument
    );
}

// ── Edge cases ────────────────────────────────────────────────────────────────

TEST(IntegratorEdge, NegativeNThrows) {
    EXPECT_THROW(
        integrate([](double x) { return x; }, 0.0, 1.0, -1, Method::RECTANGLE),
        std::invalid_argument
    );
}

TEST(IntegratorEdge, ZeroNThrows) {
    EXPECT_THROW(
        integrate([](double x) { return x; }, 0.0, 1.0, 0, Method::TRAPEZOID),
        std::invalid_argument
    );
}

TEST(IntegratorEdge, ZeroWidth) {
    // ∫ₐ^a f(x) dx = 0
    double result = integrate([](double x) { return std::sin(x); },
                              1.0, 1.0, 100, Method::RECTANGLE);
    EXPECT_NEAR(result, 0.0, 1e-12);
}
