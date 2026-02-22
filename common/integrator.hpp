#pragma once

#include <functional>
#include <stdexcept>
#include <cmath>

namespace integration {

/// Available numerical integration methods.
enum class Method {
    RECTANGLE = 0, ///< Midpoint (rectangle) rule
    TRAPEZOID = 1, ///< Trapezoid rule
    SIMPSON   = 2  ///< Simpson's 1/3 rule (n must be even)
};

/**
 * Numerically integrates f over [a, b] using n sub-intervals and the
 * chosen Method.
 *
 * @throws std::invalid_argument when n <= 0 or (Simpson and n is odd).
 */
inline double integrate(std::function<double(double)> f,
                        double a, double b,
                        int    n,
                        Method method)
{
    if (n <= 0) {
        throw std::invalid_argument("n must be positive");
    }

    const double h = (b - a) / n;
    double result  = 0.0;

    switch (method) {
    case Method::RECTANGLE:
        for (int i = 0; i < n; ++i) {
            result += f(a + (i + 0.5) * h);
        }
        result *= h;
        break;

    case Method::TRAPEZOID:
        result = (f(a) + f(b)) * 0.5;
        for (int i = 1; i < n; ++i) {
            result += f(a + i * h);
        }
        result *= h;
        break;

    case Method::SIMPSON:
        if (n % 2 != 0) {
            throw std::invalid_argument(
                "Simpson's rule requires an even number of intervals");
        }
        result = f(a) + f(b);
        for (int i = 1; i < n; ++i) {
            result += f(a + i * h) * (i % 2 == 0 ? 2.0 : 4.0);
        }
        result *= h / 3.0;
        break;

    default:
        throw std::invalid_argument("Unknown integration method");
    }

    return result;
}

} // namespace integration
