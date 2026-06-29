#pragma once

#include "SceneTypes.h"

#include <string>

// Computes vector-operation results and fills the scene's render/HUD output buffers.
class OperationEngine {
public:
    explicit OperationEngine(SceneModel& scene);

    void rebuild();
    std::string pairLabel(const std::string& separator = ", ") const;
    std::string pairFormulaName(const std::string& op) const;

private:
    SceneModel& scene_;

    void appendLine(const std::string& line);
    void addResultArrow(const Vec3& start, const Vec3& end, const Color& color, const std::string& label,
        float width = 3.0f, bool dashed = false);
    void addSurface(const std::vector<Vec3>& vertices, const Color& color, bool outline = true);
    void addParallelepiped(const Vec3& origin, const Vec3& a, const Vec3& b, const Vec3& c, const Color& color);
    void appendMissingVectorMessage(const std::string& names);
    bool requireAB();
    bool requireABC();
};
