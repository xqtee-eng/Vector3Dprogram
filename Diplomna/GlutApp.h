#pragma once

#include "CameraController.h"
#include "HudRenderer.h"
#include "InputController.h"

#include "PresetLibrary.h"
#include "SceneController.h"
#include "SceneRenderer.h"
#include "SceneTypes.h"
#include "TextRenderer.h"

// Owns GLUT window setup, callback registration, frame timing, and draw loop.
class GlutApp {
public:
    GlutApp(SceneModel& scene, CameraController& camera, SceneController& sceneController,
        SceneRenderer& sceneRenderer, HudRenderer& hudRenderer, TextRenderer& textRenderer,
        InputController& inputController, const PresetLibrary& presets);

    int run(int argc, char** argv);
    static void requestRedisplay();

private:
    static constexpr int kMinWindowWidth = 1600;
    static constexpr int kMinWindowHeight = 900;
    static GlutApp* activeApp_;

    SceneModel& scene_;
    CameraController& camera_;
    SceneController& sceneController_;
    SceneRenderer& sceneRenderer_;
    HudRenderer& hudRenderer_;
    TextRenderer& textRenderer_;
    InputController& inputController_;

    const PresetLibrary& presets_;

    int width_ = 1600;
    int height_ = 900;

    void initOpenGL();
    void setupCamera();
    void display();
    void reshape(int width, int height);
    void timer();
    void printConsoleHelp() const;

    static void displayCallback();
    static void reshapeCallback(int width, int height);
    static void timerCallback(int value);
    static void keyboardCallback(unsigned char key, int x, int y);
    static void specialKeyboardCallback(int key, int x, int y);
    static void mouseButtonCallback(int button, int state, int x, int y);
    static void mouseMotionCallback(int x, int y);
};
