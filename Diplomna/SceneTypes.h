#pragma once

#include <array>
#include <cmath>
#include <string>
#include <vector>

inline constexpr double PI = 3.1415926535897932384626433832795;
inline constexpr double EPS = 1e-8;

// Minimal 3D vector type used by both math operations and OpenGL geometry.
struct Vec3 {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;

    Vec3() = default;
    Vec3(double px, double py, double pz) : x(px), y(py), z(pz) {}

    Vec3 operator+(const Vec3& v) const { return Vec3(x + v.x, y + v.y, z + v.z); }
    Vec3 operator-(const Vec3& v) const { return Vec3(x - v.x, y - v.y, z - v.z); }
    Vec3 operator-() const { return Vec3(-x, -y, -z); }
    Vec3 operator*(double k) const { return Vec3(x * k, y * k, z * k); }
    Vec3 operator/(double k) const { return Vec3(x / k, y / k, z / k); }

    double dot(const Vec3& v) const { return x * v.x + y * v.y + z * v.z; }

    Vec3 cross(const Vec3& v) const {
        return Vec3(
            y * v.z - z * v.y,
            z * v.x - x * v.z,
            x * v.y - y * v.x
        );
    }

    double length() const { return std::sqrt(dot(*this)); }

    Vec3 normalized() const {
        double l = length();
        if (l < EPS) return Vec3();
        return *this / l;
    }
};

struct Color {
    float r = 1.0f;
    float g = 1.0f;
    float b = 1.0f;
    float a = 1.0f;
};

// One editable user vector. A vector becomes drawable only after both points
// are entered, which keeps incomplete input out of math operations.
struct VectorSlot {
    std::string name;
    Vec3 start;
    Vec3 end;
    Color color;
    bool visible = true;
    bool hasStart = false;
    bool hasEnd = false;

    double scalarK = 1.50;

    bool complete() const { return visible && hasStart && hasEnd; }
    Vec3 vec() const { return complete() ? end - start : Vec3(); }
};

// Render primitives produced by vector operations.
struct Arrow3D {
    Vec3 start;
    Vec3 end;
    Color color;
    std::string label;
    float width = 3.0f;
    bool dashed = false;
    int labelLane = 0;
};

struct Surface3D {
    std::vector<Vec3> vertices;
    Color color;
    bool outline = true;
};

struct Arc3D {
    Vec3 origin;
    Vec3 u;
    Vec3 v;
    double angle = 0.0;
    double radius = 1.0;
    Color color;
    std::string label;
};

enum class OperationMode {
    None,
    Lengths,
    AddParallelogram,
    AddTriangle,
    Subtract,
    Scalar,
    Collinearity,
    Angle,
    Dot,
    Orthogonality,
    Projection,
    Cross,
    Mixed,
    SurfaceNormal
};

enum class InputMode {
    None,
    VectorStart,
    VectorEnd,
    ScalarTarget,
    Scalar,
    OperationVector,
    OperationPair
};

enum class GridMode {
    Off,
    Floor,
    Full
};

struct SceneInputState {
    int activeVector = 0;
    InputMode mode = InputMode::None;
    std::string buffer;
    bool fullVector = false;
    OperationMode pendingOperation = OperationMode::None;

    bool leftMouseDown = false;
    bool rightMouseDown = false;
    int lastMouseX = 0;
    int lastMouseY = 0;
};

struct SceneUiState {
    std::string hudMessage = "No vectors yet. Press 1, 2, or 3 to enter a complete vector.";
    double hudMessageTimer = 4.0;

    bool showHud = true;
    GridMode gridMode = GridMode::Floor;
    bool showAxes = true;
    bool showLabels = true;
    bool showGhosts = true;
    bool animateScene = true;
    double animationT = 1.0;

    double leftHudScroll = 0.0;
    double leftHudMaxScroll = 0.0;
    int presetIndex = -1;
};

// Owns editable vectors and operation output; UI/input runtime state is grouped
// separately so rendering and interaction code no longer share one flat state bag.
class SceneModel {
public:
    std::array<VectorSlot, 3> vectors = {
        VectorSlot{ "a", Vec3(), Vec3(), Color{1.00f, 0.70f, 0.16f, 1.0f}, true, false, false },
        VectorSlot{ "b", Vec3(), Vec3(), Color{0.72f, 0.38f, 1.00f, 1.0f}, true, false, false },
        VectorSlot{ "c", Vec3(), Vec3(), Color{0.00f, 0.86f, 0.82f, 1.0f}, true, false, false }
    };

    std::vector<Arrow3D> resultArrows;
    std::vector<Surface3D> resultSurfaces;
    std::vector<Arc3D> resultArcs;
    std::vector<std::string> resultLines;

    OperationMode currentMode = OperationMode::None;
    std::string operationTitle;
    std::string formulaLine = "Enter a vector first: start point and end point.";
    std::string lessonLine = "A vector appears only after both coordinates are entered.";
    int activePairFirst = 0;
    int activePairSecond = 1;
    int scalarVectorIndex = 0;
    int operationVectorIndex = 0;

    SceneInputState input;
    SceneUiState ui;

    void clearResults() {
        resultArrows.clear();
        resultSurfaces.clear();
        resultArcs.clear();
        resultLines.clear();
    }

    int completeVectorCount() const {
        int count = 0;
        for (const auto& v : vectors) {
            if (v.complete()) ++count;
        }
        return count;
    }
};
