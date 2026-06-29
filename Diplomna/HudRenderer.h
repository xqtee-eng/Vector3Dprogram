#pragma once

#include "CameraController.h"
#include "PresetLibrary.h"
#include "SceneTypes.h"

// Owns the 2D OpenGL HUD drawing path: panels, controls, result text, status,
// and the active input field. It intentionally does not handle keyboard/mouse
// events; those remain in the GLUT entry file for now.
class HudRenderer {
public:
    HudRenderer(SceneModel& scene, CameraController& camera, const PresetLibrary& presets);

    void initFonts();
    void draw(int width, int height);

private:
    SceneModel& scene_;
    CameraController& camera_;
    const PresetLibrary& presets_;

    std::string formatDouble(double value, int precision = 2) const;
    std::string vectorText(const Vec3& v) const;
    std::string operationName(OperationMode mode) const;
    std::string gridModeName() const;
    std::string pairLabel(const std::string& separator) const;
    int displayAngle(double degrees) const;
};
