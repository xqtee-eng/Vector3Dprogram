#include "SceneController.h"

#include <algorithm>
#include <string>
#include <utility>

SceneController::SceneController(SceneModel& scene, CameraController& camera, OperationEngine& operations,
    const PresetLibrary& presets, Callbacks callbacks)
    : scene_(scene), camera_(camera), operations_(operations), presets_(presets), callbacks_(std::move(callbacks)) {
}

void SceneController::showMessage(const std::string& message, double seconds) const {
    if (callbacks_.showMessage) callbacks_.showMessage(message, seconds);
}

void SceneController::requestRedisplay() const {
    if (callbacks_.requestRedisplay) callbacks_.requestRedisplay();
}

void SceneController::clearResults() {
    scene_.clearResults();
}

int SceneController::completeVectorCount() const {
    return scene_.completeVectorCount();
}

void SceneController::setVector(int index, const Vec3& start, const Vec3& end, bool keepPreset) {
    if (index < 0 || index >= static_cast<int>(scene_.vectors.size())) return;

    scene_.vectors[index].start = start;
    scene_.vectors[index].end = end;
    scene_.vectors[index].hasStart = true;
    scene_.vectors[index].hasEnd = true;
    scene_.vectors[index].visible = true;
    if (!keepPreset) scene_.ui.presetIndex = -1;
}

void SceneController::clearVector(int index, bool keepPreset) {
    if (index < 0 || index >= static_cast<int>(scene_.vectors.size())) return;

    scene_.vectors[index].start = Vec3();
    scene_.vectors[index].end = Vec3();
    scene_.vectors[index].hasStart = false;
    scene_.vectors[index].hasEnd = false;
    scene_.vectors[index].visible = true;
    if (!keepPreset) scene_.ui.presetIndex = -1;
}

void SceneController::clearAllVectors() {
    for (int i = 0; i < static_cast<int>(scene_.vectors.size()); ++i) {
        clearVector(i, true);
    }
    scene_.ui.presetIndex = -1;
    scene_.currentMode = OperationMode::None;
    rebuildOperation();
    resetAnimation();
    showMessage("All vectors cleared. Press 1, 2, or 3 to enter a new vector.", 5.0);
    requestRedisplay();
}

std::string SceneController::pairLabel(const std::string& separator) const {
    return operations_.pairLabel(separator);
}

std::string SceneController::pairFormulaName(const std::string& op) const {
    return operations_.pairFormulaName(op);
}

void SceneController::selectVectorPair(int first, int second) {
    if (first == second) return;
    if (first < 0 || first >= static_cast<int>(scene_.vectors.size())) return;
    if (second < 0 || second >= static_cast<int>(scene_.vectors.size())) return;

    scene_.activePairFirst = first;
    scene_.activePairSecond = second;
    rebuildOperation();
    resetAnimation();
    showMessage("Active pair: " + pairLabel(" and "));
    requestRedisplay();
}

void SceneController::setOperation(OperationMode mode) {
    scene_.currentMode = mode;
    rebuildOperation();
    resetAnimation();
    showMessage(operationName(mode));
}

void SceneController::rebuildOperation() {
    operations_.rebuild();
}

void SceneController::resetAnimation() {
    scene_.ui.animationT = scene_.ui.animateScene ? 0.0 : 1.0;
}

void SceneController::loadPreset(int index) {
    if (presets_.empty()) return;

    int count = presets_.count();
    scene_.ui.presetIndex = presets_.normalizeIndex(index);

    const PresetData& preset = presets_.at(scene_.ui.presetIndex);
    for (int i = 0; i < static_cast<int>(scene_.vectors.size()); ++i) {
        setVector(i, preset.vectorData[i].start, preset.vectorData[i].end, true);
    }

    for (auto& vector : scene_.vectors) vector.scalarK = preset.scalar;
    scene_.scalarVectorIndex = 0;
    scene_.activePairFirst = preset.pairFirst;
    scene_.activePairSecond = preset.pairSecond;
    scene_.currentMode = preset.mode;

    rebuildOperation();
    resetAnimation();
    fitCameraToScene();
    showMessage("Preset " + std::to_string(scene_.ui.presetIndex + 1) + "/" + std::to_string(count) + ": " +
        preset.name + ". " + preset.description, 6.0);
    requestRedisplay();
}

void SceneController::absorbBounds(const Vec3& p, Vec3& minP, Vec3& maxP, bool& any) const {
    if (!any) {
        minP = p;
        maxP = p;
        any = true;
        return;
    }

    minP.x = std::min(minP.x, p.x);
    minP.y = std::min(minP.y, p.y);
    minP.z = std::min(minP.z, p.z);
    maxP.x = std::max(maxP.x, p.x);
    maxP.y = std::max(maxP.y, p.y);
    maxP.z = std::max(maxP.z, p.z);
}

void SceneController::fitCameraToScene() {
    Vec3 minP;
    Vec3 maxP;
    bool any = false;

    for (const auto& v : scene_.vectors) {
        if (v.hasStart) absorbBounds(v.start, minP, maxP, any);
        if (v.hasEnd) absorbBounds(v.end, minP, maxP, any);
    }
    for (const auto& arrow : scene_.resultArrows) {
        absorbBounds(arrow.start, minP, maxP, any);
        absorbBounds(arrow.end, minP, maxP, any);
    }
    for (const auto& surface : scene_.resultSurfaces) {
        for (const auto& p : surface.vertices) {
            absorbBounds(p, minP, maxP, any);
        }
    }

    if (!any) return;

    Vec3 center = (minP + maxP) * 0.5;
    Vec3 diagonal = maxP - minP;
    double radius = std::max(3.0, diagonal.length() * 0.55);

    camera_.targetPoint = center;
    camera_.targetDistance = std::clamp(radius * 2.25 + 2.5, 10.0, 55.0);
    camera_.targetYaw = -42.0;
    camera_.targetPitch = 24.0;
}

std::string SceneController::operationName(OperationMode mode) const {
    switch (mode) {
    case OperationMode::Lengths: return "Lengths";
    case OperationMode::AddParallelogram: return "Addition: parallelogram";
    case OperationMode::AddTriangle: return "Addition: triangle";
    case OperationMode::Subtract: return "Vector subtraction";
    case OperationMode::Scalar: return "Scalar on vector";
    case OperationMode::Collinearity: return "Collinearity and direction";
    case OperationMode::Angle: return "Angle between";
    case OperationMode::Dot: return "Dot product";
    case OperationMode::Orthogonality: return "Orthogonality";
    case OperationMode::Projection: return "Vector projection";
    case OperationMode::Cross: return "Cross product";
    case OperationMode::Mixed: return "Mixed product";
    case OperationMode::SurfaceNormal: return "Normal on plane";
    default: return "Scene";
    }
}
