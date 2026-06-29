#include "VectorMathSelfTest.h"

#include "VectorMath.h"

#include <cmath>
#include <iostream>
#include <string>

void VectorMathSelfTest::run() {
    auto approx = [](double a, double b) {
        return std::fabs(a - b) < 1e-6;
    };
    auto approxVec = [&](const Vec3& a, const Vec3& b) {
        return approx(a.x, b.x) && approx(a.y, b.y) && approx(a.z, b.z);
    };
    int passed = 0;
    int total = 0;
    auto check = [&](const std::string& name, bool ok) {
        ++total;
        if (ok) ++passed;
        std::cout << "  [" << (ok ? "OK" : "FAIL") << "] " << name << "\n";
    };

    Vec3 a(3, 0, 0);
    Vec3 b(0, 4, 0);
    Vec3 c(0, 0, 5);
    Vec3 u(2, -1, 3);
    Vec3 v(-4, 2, -6);
    double angle = 0.0;
    double scalar = 0.0;
    Vec3 projection;

    std::cout << "\n===== VECTOR MATH SELF-TEST =====\n";
    check("a + b", approxVec(a + b, Vec3(3, 4, 0)));
    check("a - b", approxVec(a - b, Vec3(3, -4, 0)));
    check("dot product", approx(dotProduct(a, b), 0.0));
    check("cross product", approxVec(crossProduct(a, b), Vec3(0, 0, 12)));
    check("angle 90 deg", angleBetween(a, b, angle) && approx(angle * 180.0 / PI, 90.0));
    check("angle 180 deg", angleBetween(a, Vec3(-3, 0, 0), angle) && approx(angle * 180.0 / PI, 180.0));
    check("angle with zero vector undefined", !angleBetween(a, Vec3(0, 0, 0), angle));
    check("collinear opposite", vectorLength(crossProduct(u, v)) < EPS && dotProduct(u, v) < 0.0);
    check("zero vector has zero length", approx(vectorLength(Vec3(0, 0, 0)), 0.0));
    check("mixed product volume", approx(std::fabs(mixedProduct(a, b, c)), 60.0));
    check("projection onto unit normal", approxVec(projectionOntoUnitAxis(Vec3(2, 3, 4), Vec3(0, 1, 0)), Vec3(0, 3, 0)));
    check("scalar projection", scalarProjection(Vec3(3, 4, 0), Vec3(1, 0, 0), scalar) && approx(scalar, 3.0));
    check("vector projection", projectOntoAxis(Vec3(3, 4, 0), Vec3(2, 0, 0), projection) && approxVec(projection, Vec3(3, 0, 0)));
    std::cout << "Result: " << passed << "/" << total << " passed.\n";
    std::cout << "=================================\n";
}
