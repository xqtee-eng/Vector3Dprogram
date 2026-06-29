#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include "CameraController.h"
#include "GlutApp.h"
#include "HudRenderer.h"
#include "InputController.h"

#include "OperationEngine.h"
#include "PresetLibrary.h"
#include "SceneController.h"
#include "SceneRenderer.h"
#include "SceneTypes.h"
#include "TextRenderer.h"
#include "VectorMathSelfTest.h"

#include <string>

void showMessage(const std::string& message, double seconds);

SceneModel scene;
CameraController camera;
OperationEngine operationEngine(scene);
TextRenderer textRenderer(scene);
SceneRenderer sceneRenderer(scene, camera, [](const Vec3& p, const std::string& text, const Color& color) {
    textRenderer.draw3D(p, text, color);
});

// Presets are small computer-graphics scenarios, not just random test data.
const PresetLibrary& presetLibrary = PresetLibrary::defaults();
HudRenderer hudRenderer(scene, camera, presetLibrary);
SceneController sceneController(scene, camera, operationEngine, presetLibrary, SceneController::Callbacks{
    showMessage,
    GlutApp::requestRedisplay
});

InputController inputController(scene, camera, InputController::Callbacks{
    [](OperationMode mode) { sceneController.setOperation(mode); },
    [](int index) { sceneController.loadPreset(index); },
    [](int first, int second) { sceneController.selectVectorPair(first, second); },
    []() { sceneController.clearAllVectors(); },
    []() { sceneController.rebuildOperation(); },
    []() { sceneController.resetAnimation(); },
    []() { VectorMathSelfTest::run(); },
    showMessage
});

GlutApp app(scene, camera, sceneController, sceneRenderer, hudRenderer, textRenderer,
    inputController, presetLibrary);

void showMessage(const std::string& message, double seconds = 3.0) {
    scene.ui.hudMessage = message;
    scene.ui.hudMessageTimer = seconds;
}

int main(int argc, char** argv) {
    SetProcessDPIAware();
    return app.run(argc, argv);
}