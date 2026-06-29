#pragma once

#include "SceneTypes.h"

#include <algorithm>
#include <cmath>

// Pure vector algebra service. It mirrors lecture formulas and has no OpenGL dependencies,
// so operation code and self-tests use the same mathematical implementation.
class VectorMath {
public:
    static double length(const Vec3& v) {
        return v.length();
    }

    static double dot(const Vec3& a, const Vec3& b) {
        return a.dot(b);
    }

    static Vec3 cross(const Vec3& a, const Vec3& b) {
        return a.cross(b);
    }

    static double mixed(const Vec3& a, const Vec3& b, const Vec3& c) {
        return dot(a, cross(b, c));
    }

    static bool angle(const Vec3& a, const Vec3& b, double& radians) {
        double lenA = length(a);
        double lenB = length(b);
        if (lenA < EPS || lenB < EPS) return false;

        double cosine = std::clamp(dot(a, b) / (lenA * lenB), -1.0, 1.0);
        radians = std::acos(cosine);
        return true;
    }

    static Vec3 projectOntoUnitAxis(const Vec3& v, const Vec3& unitAxis) {
        return unitAxis * dot(v, unitAxis);
    }

    static bool scalarProjection(const Vec3& v, const Vec3& axis, double& scalar) {
        double axisLength = length(axis);
        if (axisLength < EPS) return false;

        scalar = dot(v, axis) / axisLength;
        return true;
    }

    static bool projectOntoAxis(const Vec3& v, const Vec3& axis, Vec3& projection) {
        double axisLengthSq = dot(axis, axis);
        if (axisLengthSq < EPS) return false;

        projection = axis * (dot(v, axis) / axisLengthSq);
        return true;
    }
};

inline double vectorLength(const Vec3& v) {
    return VectorMath::length(v);
}

inline double dotProduct(const Vec3& a, const Vec3& b) {
    return VectorMath::dot(a, b);
}

inline Vec3 crossProduct(const Vec3& a, const Vec3& b) {
    return VectorMath::cross(a, b);
}

inline double mixedProduct(const Vec3& a, const Vec3& b, const Vec3& c) {
    return VectorMath::mixed(a, b, c);
}

inline bool angleBetween(const Vec3& a, const Vec3& b, double& radians) {
    return VectorMath::angle(a, b, radians);
}

inline Vec3 projectionOntoUnitAxis(const Vec3& v, const Vec3& unitAxis) {
    return VectorMath::projectOntoUnitAxis(v, unitAxis);
}

inline bool scalarProjection(const Vec3& v, const Vec3& axis, double& scalar) {
    return VectorMath::scalarProjection(v, axis, scalar);
}

inline bool projectOntoAxis(const Vec3& v, const Vec3& axis, Vec3& projection) {
    return VectorMath::projectOntoAxis(v, axis, projection);
}
