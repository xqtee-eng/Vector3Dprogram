#pragma once

#include "CameraController.h"
#include "OperationEngine.h"
#include "PresetLibrary.h"
#include "SceneTypes.h"

#include <functional>
#include <string>

// Coordinates scene-state changes: editable vectors, presets, active operation,
// operation rebuilds, animation reset, and camera fitting.
class SceneController {
public:
    struct Callbacks {
        std::function<void(const std::string&, double)> showMessage;
        std::function<void()> requestRedisplay;
    };

    SceneController(SceneModel& scene, CameraController& camera, OperationEngine& operations,
        const PresetLibrary& presets, Callbacks callbacks);

    void clearResults();
    int completeVectorCount() const;

    void setVector(int index, const Vec3& start, const Vec3& end, bool keepPreset = false);
    void clearVector(int index, bool keepPreset = false);
    void clearAllVectors();

    std::string pairLabel(const std::string& separator = ", ") const;
    std::string pairFormulaName(const std::string& op) const;

    void selectVectorPair(int first, int second);
    void setOperation(OperationMode mode);
    void rebuildOperation();
    void resetAnimation();
    void loadPreset(int index);
    void fitCameraToScene();

private:
    SceneModel& scene_;
    CameraController& camera_;
    OperationEngine& operations_;
    const PresetLibrary& presets_;
    Callbacks callbacks_;

    void showMessage(const std::string& message, double seconds = 3.0) const;
    void requestRedisplay() const;
    void absorbBounds(const Vec3& p, Vec3& minP, Vec3& maxP, bool& any) const;
    std::string operationName(OperationMode mode) const;
};
