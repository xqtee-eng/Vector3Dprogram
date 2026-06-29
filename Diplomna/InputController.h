#pragma once

#include "CameraController.h"
#include "SceneTypes.h"

#include <functional>
#include <string>

// Handles keyboard and mouse input. It mutates input/camera state directly and
// delegates operation rebuilds, presets, and self-tests back to the application.
class InputController {
public:
    struct Callbacks {
        std::function<void(OperationMode)> setOperation;
        std::function<void(int)> loadPreset;
        std::function<void(int, int)> selectVectorPair;
        std::function<void()> clearAllVectors;
        std::function<void()> rebuildOperation;
        std::function<void()> resetAnimation;
        std::function<void()> runVectorMathSelfTest;
        std::function<void(const std::string&, double)> showMessage;
    };

    InputController(SceneModel& scene, CameraController& camera, Callbacks callbacks);

    void keyboard(unsigned char key);
    void specialKeyboard(int key);
    void mouseButton(int button, int state, int x, int y);
    void mouseMotion(int x, int y);

    void beginVectorInput(int vectorIndex, InputMode mode, bool fullVector = false);
    void beginScalarInput();
    void beginScalarValueInput(int vectorIndex);
    void cancelInput();
    void commitInput();

private:
    SceneModel& scene_;
    CameraController& camera_;
    Callbacks callbacks_;

    void handleInputKey(unsigned char key);
    void beginOperationVectorInput(OperationMode mode);
    void beginOperationPairInput(OperationMode mode);
    void applyPendingVectorOperation(int vectorIndex);
    void applyPendingPairOperation(int first, int second);
    void showMessage(const std::string& message, double seconds = 3.0) const;
    void postRedisplay() const;
    bool parseVec3Input(std::string text, Vec3& out) const;
    bool parseScalarInput(const std::string& text, double& out) const;
    std::string formatDouble(double value, int precision = 2) const;
};
