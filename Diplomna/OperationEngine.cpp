#include "OperationEngine.h"
#include "VectorMath.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>

static std::string trimNumber(std::string s) {
    if (s.find('.') != std::string::npos) {
        while (!s.empty() && s.back() == '0') s.pop_back();
        if (!s.empty() && s.back() == '.') s.pop_back();
    }
    if (s == "-0") s = "0";
    return s;
}

std::string fmt(double v, int precision = 2) {
    if (std::fabs(v) < EPS) v = 0.0;
    std::ostringstream out;
    out << std::fixed << std::setprecision(precision) << v;
    return trimNumber(out.str());
}

std::string vecToString(const Vec3& v) {
    return "(" + fmt(v.x) + ", " + fmt(v.y) + ", " + fmt(v.z) + ")";
}

static bool samePoint(const Vec3& a, const Vec3& b) {
    return (a - b).length() < EPS;
}

Vec3 perpendicularUnit(const Vec3& v) {
    Vec3 axis = std::fabs(v.y) < 0.9 ? Vec3(0, 1, 0) : Vec3(1, 0, 0);
    Vec3 p = axis - v * axis.dot(v);
    if (p.length() < EPS) return Vec3(0, 0, 1);
    return p.normalized();
}

OperationEngine::OperationEngine(SceneModel& scene)
    : scene_(scene) {
}

std::string OperationEngine::pairLabel(const std::string& separator) const {
    return scene_.vectors[scene_.activePairFirst].name + separator + scene_.vectors[scene_.activePairSecond].name;
}

std::string OperationEngine::pairFormulaName(const std::string& op) const {
    return scene_.vectors[scene_.activePairFirst].name + " " + op + " " + scene_.vectors[scene_.activePairSecond].name;
}

void OperationEngine::appendLine(const std::string& line) {
    scene_.resultLines.push_back(line);
}

void OperationEngine::addResultArrow(const Vec3& start, const Vec3& end, const Color& color, const std::string& label,
    float width, bool dashed) {
    scene_.resultArrows.push_back(Arrow3D{ start, end, color, label, width, dashed });
}

void OperationEngine::addSurface(const std::vector<Vec3>& vertices, const Color& color, bool outline) {
    scene_.resultSurfaces.push_back(Surface3D{ vertices, color, outline });
}

void OperationEngine::addParallelepiped(const Vec3& origin, const Vec3& a, const Vec3& b, const Vec3& c, const Color& color) {
    Vec3 A = origin + a;
    Vec3 B = origin + b;
    Vec3 C = origin + c;
    Vec3 AB = origin + a + b;
    Vec3 AC = origin + a + c;
    Vec3 BC = origin + b + c;
    Vec3 ABC = origin + a + b + c;

    addSurface({ origin, A, AB, B }, color);
    addSurface({ origin, B, BC, C }, color);
    addSurface({ origin, C, AC, A }, color);
    addSurface({ A, AB, ABC, AC }, color);
    addSurface({ B, AB, ABC, BC }, color);
    addSurface({ C, AC, ABC, BC }, color);
}

void OperationEngine::appendMissingVectorMessage(const std::string& names) {
    appendLine("Required vector data is missing: " + names + ".");
    appendLine("Press 1/2/3 to enter vector a/b/c as start and end coordinates.");
}

bool OperationEngine::requireAB() {
    if (scene_.vectors[scene_.activePairFirst].complete() && scene_.vectors[scene_.activePairSecond].complete()) return true;
    appendMissingVectorMessage(scene_.vectors[scene_.activePairFirst].name + " and " + scene_.vectors[scene_.activePairSecond].name);
    return false;
}

bool OperationEngine::requireABC() {
    if (scene_.vectors[0].complete() && scene_.vectors[1].complete() && scene_.vectors[2].complete()) return true;
    appendMissingVectorMessage("a, b and c");
    return false;
}

void OperationEngine::rebuild() {
    scene_.clearResults();

    const auto& vectors = scene_.vectors;
    const VectorSlot& A = vectors[0];
    const VectorSlot& B = vectors[1];
    const VectorSlot& C = vectors[2];
    const VectorSlot& U = vectors[scene_.activePairFirst];
    const VectorSlot& V = vectors[scene_.activePairSecond];
    Vec3 a = A.vec();
    Vec3 b = B.vec();
    Vec3 c = C.vec();
    Vec3 u = U.vec();
    Vec3 v = V.vec();
    Vec3 origin = U.hasStart ? U.start : Vec3();
    Vec3 uEnd = origin + u;
    Vec3 vEnd = origin + v;
    Vec3 mixedOrigin = A.hasStart ? A.start : Vec3();
    Vec3 bEnd = mixedOrigin + b;
    Vec3 cEnd = mixedOrigin + c;
    std::string firstName = U.name;
    std::string secondName = V.name;

    switch (scene_.currentMode) {
    case OperationMode::None:
        scene_.operationTitle.clear();
        scene_.formulaLine = "Enter a vector first: start point and end point.";
        scene_.lessonLine = "A vector appears only after both coordinates are entered.";
        if (scene_.completeVectorCount() == 0) {
            appendLine("No vectors entered yet.");
        }
        else {
            appendLine("Select F1-F11, V, or O to build a vector operation.");
        }
        break;

    case OperationMode::Lengths: {
        int index = std::clamp(scene_.operationVectorIndex, 0, static_cast<int>(vectors.size()) - 1);
        const VectorSlot& vector = vectors[index];
        scene_.operationTitle = "Vector length";
        scene_.formulaLine = "|" + vector.name + "| = sqrt(x^2 + y^2 + z^2)";
        scene_.lessonLine = "The length formula extends the Pythagorean theorem to three coordinates.";
        if (!vector.complete()) {
            appendMissingVectorMessage(vector.name);
            break;
        }
        appendLine(vector.name + " = " + vecToString(vector.vec()));
        appendLine("|" + vector.name + "| = " + fmt(vectorLength(vector.vec()), 4));
        break;
    }

    case OperationMode::AddParallelogram: {
        scene_.operationTitle = "Vector addition: parallelogram rule";
        scene_.formulaLine = pairFormulaName("+") + " = coordinate-wise sum";
        scene_.lessonLine = "Translated copies keep direction and length; the diagonal is the sum.";
        if (!requireAB()) break;
        Vec3 sum = u + v;
        appendLine(pairFormulaName("+") + " = " + vecToString(sum));
        appendLine("|" + pairFormulaName("+") + "| = " + fmt(vectorLength(sum), 4));

        if (!samePoint(V.start, origin)) {
            addResultArrow(origin, vEnd, V.color, secondName + "_O", 2.4f, true);
        }
        addResultArrow(origin, origin + sum, Color{0.96f, 0.88f, 0.18f, 1.0f}, pairFormulaName("+"), 4.0f);
        addResultArrow(origin + u, origin + u + v, V.color, secondName + "'", 2.5f, true);
        addResultArrow(origin + v, origin + v + u, U.color, firstName + "'", 2.5f, true);
        addSurface({ origin, uEnd, origin + u + v, vEnd }, Color{0.96f, 0.88f, 0.18f, 0.18f});
        break;
    }

    case OperationMode::AddTriangle: {
        scene_.operationTitle = "Vector addition: triangle rule";
        scene_.formulaLine = "Move " + secondName + " to the end of " + firstName + ".";
        scene_.lessonLine = "The triangle and parallelogram rules produce the same coordinate sum.";
        if (!requireAB()) break;
        Vec3 sum = u + v;
        appendLine(pairFormulaName("+") + " = " + vecToString(sum));
        appendLine("|" + pairFormulaName("+") + "| = " + fmt(vectorLength(sum), 4));

        addResultArrow(uEnd, uEnd + v, V.color, secondName + "'", 3.0f, true);
        addResultArrow(origin, origin + sum, Color{0.96f, 0.88f, 0.18f, 1.0f}, pairFormulaName("+"), 4.0f);
        addSurface({ origin, uEnd, origin + sum }, Color{0.96f, 0.88f, 0.18f, 0.15f});
        break;
    }

    case OperationMode::Subtract: {
        scene_.operationTitle = "Vector subtraction";
        scene_.formulaLine = pairFormulaName("-") + " = " + firstName + " + (-" + secondName + ")";
        scene_.lessonLine = "Subtracting a vector means adding the opposite vector.";
        if (!requireAB()) break;
        Vec3 diff = u - v;
        appendLine(pairFormulaName("-") + " = " + vecToString(diff));
        appendLine("|" + pairFormulaName("-") + "| = " + fmt(vectorLength(diff), 4));

        addResultArrow(uEnd, uEnd - v, Color{0.25f, 1.0f, 0.62f, 1.0f}, "-" + secondName + "'", 3.0f, true);
        addResultArrow(origin, origin + diff, Color{0.25f, 1.0f, 0.62f, 1.0f}, pairFormulaName("-"), 4.0f);
        addSurface({ origin, uEnd, origin + diff }, Color{0.25f, 1.0f, 0.62f, 0.14f});
        break;
    }

    case OperationMode::Scalar: {
        int index = std::clamp(scene_.scalarVectorIndex, 0, static_cast<int>(vectors.size()) - 1);
        const VectorSlot& vector = vectors[index];
        scene_.operationTitle = "Scalar on vector";
        scene_.formulaLine = "k*" + vector.name + " = (k*x, k*y, k*z), k = " + fmt(vector.scalarK);
        scene_.lessonLine = "Scalar multiplication changes the length of one selected vector; a negative k reverses direction.";
        if (!vector.complete()) {
            appendMissingVectorMessage(vector.name);
            break;
        }

        Vec3 scaled = vector.vec() * vector.scalarK;
        appendLine(vector.name + " = " + vecToString(vector.vec()));
        appendLine("k*" + vector.name + " = " + vecToString(scaled));
        addResultArrow(vector.start, vector.start + scaled,
            Color{vector.color.r, vector.color.g, vector.color.b, 1.0f}, "k*" + vector.name, 4.0f);
        break;
    }

    case OperationMode::Collinearity: {
        scene_.operationTitle = "Collinearity and direction";
        scene_.formulaLine = pairLabel(" and ") + " are collinear when |" + pairFormulaName("x") + "| = 0";
        scene_.lessonLine = "Dot sign separates same and opposite direction after collinearity is known.";
        if (!requireAB()) break;

        Vec3 cross = crossProduct(u, v);
        double dot = dotProduct(u, v);
        double crossLen = vectorLength(cross);
        appendLine(pairFormulaName("x") + " = " + vecToString(cross));
        appendLine("|" + pairFormulaName("x") + "| = " + fmt(crossLen, 6));
        if (vectorLength(u) < EPS || vectorLength(v) < EPS) {
            appendLine("Result: undefined because one vector is zero.");
            appendLine("A zero vector has no direction.");
        }
        else if (crossLen < EPS) {
            appendLine(dot > 0
                ? "Result: collinear, same direction."
                : "Result: collinear, opposite direction.");
        }
        else {
            appendLine("Result: not collinear.");
        }
        break;
    }

    case OperationMode::Angle: {
        scene_.operationTitle = "Angle between vectors";
        scene_.formulaLine = "cos(theta) = (" + pairFormulaName("*") + ") / (|" + firstName + "|*|" + secondName + "|)";
        scene_.lessonLine = "The dot product determines the angle between two non-zero vectors.";
        if (!requireAB()) break;
        double angle = 0.0;
        if (!angleBetween(u, v, angle)) {
            appendLine("Angle is undefined because one vector is zero.");
            break;
        }

        double degrees = angle * 180.0 / PI;
        appendLine(pairFormulaName("*") + " = " + fmt(dotProduct(u, v), 4));
        appendLine("theta = " + fmt(degrees, 4) + " deg");
        if (!samePoint(V.start, origin)) {
            appendLine(secondName + " is shown as a parallel copy from " + firstName + "'s start point.");
            addResultArrow(origin, vEnd, V.color, secondName + "'", 3.0f, true);
        }

        Vec3 unitU = u.normalized();
        Vec3 arcV = (v - projectionOntoUnitAxis(v, unitU)).normalized();
        if (vectorLength(arcV) < EPS) arcV = perpendicularUnit(unitU);
        double radius = std::min(vectorLength(u), vectorLength(v)) * 0.35;
        radius = std::clamp(radius, 0.8, 3.0);
        scene_.resultArcs.push_back(Arc3D{ origin, unitU, arcV, angle, radius, Color{1.0f, 0.62f, 0.12f, 1.0f}, "theta" });
        break;
    }

    case OperationMode::Dot: {
        scene_.operationTitle = "Dot product";
        scene_.formulaLine = pairFormulaName("*") + " = x1*x2 + y1*y2 + z1*z2";
        scene_.lessonLine = "Positive dot means an acute angle; negative dot means an obtuse angle.";
        if (!requireAB()) break;
        double dot = dotProduct(u, v);
        appendLine(pairFormulaName("*") + " = " + fmt(dot, 4));
        appendLine(pairFormulaName("*") + " = " + fmt(u.x) + "*" + fmt(v.x) + " + " + fmt(u.y) + "*" + fmt(v.y) + " + " + fmt(u.z) + "*" + fmt(v.z));
        if (vectorLength(u) < EPS || vectorLength(v) < EPS) {
            appendLine("Angle type: undefined because one vector is zero.");
        }
        else if (dot > EPS) appendLine("Angle type: acute.");
        else if (dot < -EPS) appendLine("Angle type: obtuse.");
        else appendLine("Angle type: right angle.");
        break;
    }

    case OperationMode::Orthogonality: {
        scene_.operationTitle = "Orthogonality check";
        scene_.formulaLine = pairLabel(" and ") + " are orthogonal when " + pairFormulaName("*") + " = 0";
        scene_.lessonLine = "For non-zero vectors, zero dot product is equivalent to a right angle.";
        if (!requireAB()) break;

        double dot = dotProduct(u, v);
        appendLine(pairFormulaName("*") + " = " + fmt(dot, 6));
        if (vectorLength(u) < EPS || vectorLength(v) < EPS) {
            appendLine("Result: undefined because one vector is zero.");
            appendLine("The zero vector has no fixed direction, so it is not treated as orthogonal here.");
        }
        else if (std::fabs(dot) < EPS) {
            appendLine("Result: orthogonal vectors.");
            appendLine("Angle = 90 deg.");
        }
        else {
            appendLine("Result: not orthogonal.");
        }
        break;
    }

    case OperationMode::Projection: {
        scene_.operationTitle = "Projection of vector " + secondName + " on vector " + firstName;
        scene_.formulaLine = "pr_" + firstName + "(" + secondName + ") = (" + pairFormulaName("*") + ") / |" + firstName + "|";
        scene_.lessonLine = "The scalar projection measures the signed component of one vector along another.";
        if (!requireAB()) break;

        double scalar = 0.0;
        Vec3 projection;
        if (!scalarProjection(v, u, scalar) || !projectOntoAxis(v, u, projection)) {
            appendLine("Projection is undefined because vector " + firstName + " is zero.");
            break;
        }

        Vec3 residual = v - projection;
        appendLine("pr_" + firstName + "(" + secondName + ") = " + fmt(scalar, 4));
        appendLine("proj_" + firstName + "(" + secondName + ") = " + vecToString(projection));
        appendLine(secondName + " - proj_" + firstName + "(" + secondName + ") = " + vecToString(residual));

        if (!samePoint(V.start, origin)) {
            addResultArrow(origin, vEnd, V.color, secondName + "_O", 2.4f, true);
        }
        addResultArrow(origin, origin + projection, Color{0.20f, 0.92f, 0.86f, 1.0f},
            "proj_" + firstName + "(" + secondName + ")", 4.0f);
        if (vectorLength(residual) > EPS) {
            addResultArrow(origin + projection, vEnd, Color{1.0f, 0.62f, 0.18f, 1.0f}, "perp", 2.8f, true);
            addSurface({ origin, origin + projection, vEnd }, Color{0.20f, 0.92f, 0.86f, 0.13f});
        }
        break;
    }

    case OperationMode::Cross: {
        scene_.operationTitle = "Cross product";
        scene_.formulaLine = pairFormulaName("x") + " is perpendicular to the selected pair.";
        scene_.lessonLine = "Direction follows the right-hand rule; reversing vector order flips the normal.";
        if (!requireAB()) break;
        Vec3 cross = crossProduct(u, v);
        appendLine(pairFormulaName("x") + " = " + vecToString(cross));
        appendLine("|" + pairFormulaName("x") + "| = " + fmt(vectorLength(cross), 4) + " (parallelogram area)");
        appendLine("Right-hand rule: " + firstName + " x " + secondName + " sets the normal direction.");

        if (!samePoint(V.start, origin)) {
            addResultArrow(origin, vEnd, V.color, secondName + "_O", 2.4f, true);
        }
        if (vectorLength(cross) > EPS) {
            addSurface({ origin, uEnd, origin + u + v, vEnd }, Color{1.0f, 0.44f, 0.15f, 0.18f});
            addResultArrow(origin, origin + cross, Color{1.0f, 0.52f, 0.12f, 1.0f}, pairFormulaName("x"), 4.0f);
        }
        else {
            appendLine("The cross product is zero: vectors are parallel or one is zero.");
        }
        break;
    }

    case OperationMode::SurfaceNormal: {
        scene_.operationTitle = "Normal on plane";
        scene_.formulaLine = "N = normalize(" + pairFormulaName("x") + "), w_plane = w - proj_N(w)";
        scene_.lessonLine = "Right-hand rule sets normal direction; the third vector is projected onto the plane.";
        if (!requireAB()) break;

        Vec3 normal = crossProduct(u, v);
        appendLine(pairFormulaName("x") + " = " + vecToString(normal));
        appendLine("|" + pairFormulaName("x") + "| = " + fmt(vectorLength(normal), 4));

        if (!samePoint(V.start, origin)) {
            addResultArrow(origin, vEnd, V.color, secondName + "_O", 2.4f, true);
        }
        if (vectorLength(normal) > EPS) {
            addSurface({ origin, uEnd, origin + u + v, vEnd }, Color{0.42f, 0.75f, 1.0f, 0.20f});
            Vec3 n = normal.normalized();
            double scale = std::max(1.8, std::min(4.0, std::sqrt(vectorLength(u) * vectorLength(v))));
            appendLine("N = " + vecToString(n));
            appendLine("Right-hand rule: " + firstName + " x " + secondName + " defines N.");
            addResultArrow(origin, origin + n * scale, Color{1.0f, 0.92f, 0.25f, 1.0f}, "N", 4.0f);

            int projectionIndex = 3 - scene_.activePairFirst - scene_.activePairSecond;
            const VectorSlot& W = vectors[projectionIndex];
            if (W.complete()) {
                Vec3 w = W.vec();

                Vec3 normalProjection = projectionOntoUnitAxis(w, n);
                Vec3 planeProjection = w - normalProjection;

                appendLine("Projection source: vector " + W.name + " = " + vecToString(w));
                appendLine("Third vector " + W.name + " is projected onto the surface plane.");
                appendLine("proj_N(" + W.name + ") = " + vecToString(normalProjection));
                appendLine(W.name + "_plane = " + vecToString(planeProjection));

                addResultArrow(origin, origin + w, Color{W.color.r, W.color.g, W.color.b, 0.72f}, W.name + "'", 2.5f, true);
                addResultArrow(origin, origin + planeProjection, Color{0.20f, 0.92f, 0.86f, 1.0f}, W.name + "_plane", 3.8f);
                addResultArrow(origin + planeProjection, origin + w, Color{1.0f, 0.62f, 0.18f, 1.0f}, "proj_N", 2.8f, true);
                addSurface({ origin, origin + planeProjection, origin + w }, Color{0.20f, 0.92f, 0.86f, 0.13f});
            }
            else {
                appendLine("Enter vector " + W.name + " to show its projection on this surface.");
            }
        }
        else {
            appendLine("Normal is undefined because the selected vectors are parallel or zero.");
        }
        break;
    }

    case OperationMode::Mixed: {
        scene_.operationTitle = "Mixed product and volume";
        scene_.formulaLine = "[a,b,c] = a*(b x c), uses all three vectors";
        scene_.lessonLine = "This operation always uses vectors a, b, and c, not only the active pair.";
        if (!requireABC()) break;

        Vec3 bxC = crossProduct(b, c);
        double mixed = mixedProduct(a, b, c);
        appendLine("Uses vectors a, b and c.");
        appendLine("b x c = " + vecToString(bxC));
        appendLine("[a,b,c] = " + fmt(mixed, 4));
        appendLine("Volume = |[a,b,c]| = " + fmt(std::fabs(mixed), 4));
        if (!samePoint(B.start, mixedOrigin)) {
            addResultArrow(mixedOrigin, bEnd, B.color, "b_O", 2.4f, true);
        }
        if (!samePoint(C.start, mixedOrigin)) {
            addResultArrow(mixedOrigin, cEnd, C.color, "c_O", 2.4f, true);
        }
        addParallelepiped(mixedOrigin, a, b, c, Color{0.35f, 0.72f, 1.0f, 0.12f});
        addResultArrow(mixedOrigin, mixedOrigin + bxC, Color{0.90f, 0.82f, 0.20f, 1.0f}, "b x c", 3.5f);
        addResultArrow(mixedOrigin, cEnd, C.color, "c", 3.0f);
        break;
    }
    }
}
