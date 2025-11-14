#include <iostream>
#include <iomanip>
#include <random>
#include <cmath>
#include <algorithm> 

struct Circle {
    double x;
    double y;
    double r;
};

bool in_all_circles(double x, double y,
                    const Circle& c1,
                    const Circle& c2,
                    const Circle& c3)
{
    double dx1 = x - c1.x;
    double dy1 = y - c1.y;
    if (dx1 * dx1 + dy1 * dy1 > c1.r * c1.r) {
        return false;
    }
    double dx2 = x - c2.x;
    double dy2 = y - c2.y;
    if (dx2 * dx2 + dy2 * dy2 > c2.r * c2.r) {
        return false;
    }
    double dx3 = x - c3.x;
    double dy3 = y - c3.y;
    if (dx3 * dx3 + dy3 * dy3 > c3.r * c3.r) {
        return false;
    }
    return true;
}

double estimate_area_monte_carlo(double xmin, double xmax,
                                 double ymin, double ymax,
                                 int N,
                                 std::mt19937_64& gen,
                                 const Circle& c1,
                                 const Circle& c2,
                                 const Circle& c3)
{
    std::uniform_real_distribution<double> distX(xmin, xmax);
    std::uniform_real_distribution<double> distY(ymin, ymax);

    int M = 0;
    for (int i = 0; i < N; ++i) {
        double x = distX(gen);
        double y = distY(gen);

        if (in_all_circles(x, y, c1, c2, c3)) {
            ++M;
        }
    }

    double Srect = (xmax - xmin) * (ymax - ymin);
    double area  = (static_cast<double>(M) / static_cast<double>(N)) * Srect;
    return area;
}

int main() {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    Circle c1{1.0, 1.0, 1.0};
    Circle c2{1.5, 2.0, std::sqrt(5.0) / 2.0};
    Circle c3{2.0, 1.5, std::sqrt(5.0) / 2.0};

    // S = 0.25 * pi + 1.25 * arcsin(0.8) - 1
    const double PI = std::acos(-1.0);
    const double S_exact = 0.25 * PI + 1.25 * std::asin(0.8) - 1.0;

    // Широкий прямоугольник
    const double wide_xmin = std::min({c1.x - c1.r, c2.x - c2.r, c3.x - c3.r});
    const double wide_xmax = std::max({c1.x + c1.r, c2.x + c2.r, c3.x + c3.r});
    const double wide_ymin = std::min({c1.y - c1.r, c2.y - c2.r, c3.y - c3.r});
    const double wide_ymax = std::max({c1.y + c1.r, c2.y + c2.r, c3.y + c3.r});

    // Узкий прямоугольник
    double narrow_xmin = std::max({c1.x - c1.r, c2.x - c2.r, c3.x - c3.r});
    double narrow_xmax = std::min({c1.x + c1.r, c2.x + c2.r, c3.x + c3.r});
    double narrow_ymin = std::max({c1.y - c1.r, c2.y - c2.r, c3.y - c3.r});
    double narrow_ymax = std::min({c1.y + c1.r, c2.y + c2.r, c3.y + c3.r});

    std::mt19937_64 gen(42);

    std::cout << std::fixed << std::setprecision(10);

    std::cout << "# Точная площадь = " << S_exact << "\n";

    std::cout << "# Широкий прямоугольник: "
          << "xmin=" << wide_xmin << " xmax=" << wide_xmax
          << " ymin=" << wide_ymin << " ymax=" << wide_ymax << "\n";

    std::cout << "# Узкий прямоугольник:   "
          << "xmin=" << narrow_xmin << " xmax=" << narrow_xmax
          << " ymin=" << narrow_ymin << " ymax=" << narrow_ymax << "\n";

    std::cout << "# N   S(широкий)   Ошибка(широкий)   S(узкий)   Ошибка(узкий)\n";
    for (int N = 100; N <= 100000; N += 500) {
        double S_wide = estimate_area_monte_carlo(
            wide_xmin, wide_xmax, wide_ymin, wide_ymax,
            N, gen, c1, c2, c3
        );
        double S_narrow = estimate_area_monte_carlo(
            narrow_xmin, narrow_xmax, narrow_ymin, narrow_ymax,
            N, gen, c1, c2, c3
        );

        double err_wide   = std::fabs(S_wide   - S_exact);
        double err_narrow = std::fabs(S_narrow - S_exact);

        std::cout << N << " "
                  << S_wide   << " " << err_wide   << " "
                  << S_narrow << " " << err_narrow << "\n";
    }

    return 0;
}
