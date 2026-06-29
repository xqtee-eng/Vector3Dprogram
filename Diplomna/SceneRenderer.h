#pragma once

#include "CameraController.h"
#include "SceneTypes.h"

#include <functional>
#include <string>
#include <vector>

// Renders the 3D part of the vector scene. HUD, input, and font ownership stay
// in the GLUT entry file; this class only draws scene geometry.
class SceneRenderer {
public:
    using Text3DCallback = std::function<void(const Vec3&, const std::string&, const Color&)>;

    SceneRenderer(SceneModel& scene, CameraController& camera, Text3DCallback drawText3D);

    void draw() const;

private:
    SceneModel& scene_;
    CameraController& camera_;
    Text3DCallback drawText3D_;

    double sceneLimit() const;
    bool sameArrowForLabel(const Arrow3D& a, const Arrow3D& b) const;
    void assignLabelLanes(std::vector<Arrow3D>& arrows) const;

    void drawArrow(const Arrow3D& arrow, double progress = 1.0) const;
    void drawVectorSlot(const VectorSlot& slot, double progress, int labelLane = 0) const;
    void drawSurface(const Surface3D& surface) const;
    void drawArc(const Arc3D& arc) const;
    void drawGridPlane(double limit, double step, int plane) const;
    void drawAxes(double limit) const;
};
