#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <GL/glut.h>

#include "InputController.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <sstream>

constexpr int kPanelMargin = 12;
constexpr int kLeftHudWidth = 410;

static std::string trimNumber(std::string text) {
    if (text.find('.') != std::string::npos) {
        while (!text.empty() && text.back() == '0') text.pop_back();
        if (!text.empty() && text.back() == '.') text.pop_back();
    }
    if (text == "-0") text = "0";
    return text;
}

InputController::InputController(SceneModel& scene, CameraController& camera, Callbacks callbacks)
    : scene_(scene), camera_(camera), callbacks_(std::move(callbacks)) {
}

void InputController::showMessage(const std::string& message, double seconds) const {
    if (callbacks_.showMessage) callbacks_.showMessage(message, seconds);
}

void InputController::postRedisplay() const {
    glutPostRedisplay();
}

std::string InputController::formatDouble(double value, int precision) const {
    if (std::fabs(value) < EPS) value = 0.0;
    std::ostringstream out;
    out << std::fixed << std::setprecision(precision) << value;
    return trimNumber(out.str());
}

bool InputController::parseVec3Input(std::string text, Vec3& out) const {
    for (char& ch : text) {
        if (ch == ',' || ch == ';' || ch == '(' || ch == ')' || ch == '[' || ch == ']') ch = ' ';
    }

    std::istringstream in(text);
    double x, y, z;
    char extra;
    if (!(in >> x >> y >> z)) return false;
    if (in >> extra) return false;
    out = Vec3(x, y, z);
    return true;
}

bool InputController::parseScalarInput(const std::string& text, double& out) const {
    std::istringstream in(text);
    double value;
    char extra;
    if (!(in >> value)) return false;
    if (in >> extra) return false;
    out = value;
    return true;
}

void InputController::beginVectorInput(int vectorIndex, InputMode mode, bool fullVector) {
    scene_.input.activeVector = std::clamp(vectorIndex, 0, static_cast<int>(scene_.vectors.size()) - 1);
    scene_.input.mode = mode;
    scene_.input.fullVector = fullVector;
    scene_.input.buffer.clear();

    std::string part = mode == InputMode::VectorStart ? "start" : "end";
    showMessage("Editing vector " + scene_.vectors[scene_.input.activeVector].name + " " + part +
        ". Type x y z and press Enter.", 8.0);
}

void InputController::beginScalarInput() {
    if (scene_.completeVectorCount() == 0) {
        showMessage("Enter at least one vector before scalar multiplication.", 5.0);
        return;
    }

    scene_.input.mode = InputMode::ScalarTarget;
    scene_.input.fullVector = false;
    scene_.input.buffer.clear();
    showMessage("Choose vector for scalar multiplication: press 1, 2, or 3.", 8.0);
}

void InputController::beginScalarValueInput(int vectorIndex) {
    int index = std::clamp(vectorIndex, 0, static_cast<int>(scene_.vectors.size()) - 1);
    if (!scene_.vectors[index].complete()) {
        showMessage("Vector " + scene_.vectors[index].name + " is not complete yet.", 5.0);
        return;
    }

    scene_.scalarVectorIndex = index;
    scene_.input.activeVector = index;
    scene_.input.mode = InputMode::Scalar;
    scene_.input.fullVector = false;
    scene_.input.buffer.clear();
    showMessage("Enter scalar k for vector " + scene_.vectors[index].name + ". Press Enter to apply.", 8.0);
}

void InputController::beginOperationVectorInput(OperationMode mode) {
    if (scene_.completeVectorCount() == 0) {
        showMessage("Enter at least one vector before this operation.", 5.0);
        return;
    }

    scene_.input.pendingOperation = mode;
    scene_.input.mode = InputMode::OperationVector;
    scene_.input.fullVector = false;
    scene_.input.buffer.clear();
    showMessage("Choose vector: type 1, 2, or 3, then press Enter.", 8.0);
}

void InputController::beginOperationPairInput(OperationMode mode) {
    if (scene_.completeVectorCount() < 2) {
        showMessage("Enter at least two vectors before this operation.", 5.0);
        return;
    }

    scene_.input.pendingOperation = mode;
    scene_.input.mode = InputMode::OperationPair;
    scene_.input.fullVector = false;
    scene_.input.buffer.clear();
    showMessage("Choose ordered pair: type 12, 21, 13, 31, 23, or 32, then press Enter.", 8.0);
}

void InputController::applyPendingVectorOperation(int vectorIndex) {
    int index = std::clamp(vectorIndex, 0, static_cast<int>(scene_.vectors.size()) - 1);
    if (!scene_.vectors[index].complete()) {
        scene_.input.buffer.clear();
        showMessage("Vector " + scene_.vectors[index].name + " is not complete yet.", 5.0);
        return;
    }

    scene_.operationVectorIndex = index;
    scene_.currentMode = scene_.input.pendingOperation;
    scene_.input.mode = InputMode::None;
    scene_.input.pendingOperation = OperationMode::None;
    scene_.input.buffer.clear();
    if (callbacks_.rebuildOperation) callbacks_.rebuildOperation();
    if (callbacks_.resetAnimation) callbacks_.resetAnimation();
    showMessage("Operation uses vector " + scene_.vectors[index].name + ".");
}

void InputController::applyPendingPairOperation(int first, int second) {
    int maxIndex = static_cast<int>(scene_.vectors.size()) - 1;
    first = std::clamp(first, 0, maxIndex);
    second = std::clamp(second, 0, maxIndex);
    if (first == second) {
        scene_.input.buffer.clear();
        showMessage("Choose two different vectors for this operation.", 5.0);
        return;
    }
    if (!scene_.vectors[first].complete() || !scene_.vectors[second].complete()) {
        scene_.input.buffer.clear();
        showMessage("Selected pair has incomplete vector data.", 5.0);
        return;
    }

    scene_.activePairFirst = first;
    scene_.activePairSecond = second;
    scene_.currentMode = scene_.input.pendingOperation;
    scene_.input.mode = InputMode::None;
    scene_.input.pendingOperation = OperationMode::None;
    scene_.input.buffer.clear();
    if (callbacks_.rebuildOperation) callbacks_.rebuildOperation();
    if (callbacks_.resetAnimation) callbacks_.resetAnimation();
    showMessage("Operation uses pair " + scene_.vectors[first].name + " and " + scene_.vectors[second].name + ".");
}

void InputController::cancelInput() {
    scene_.input.mode = InputMode::None;
    scene_.input.fullVector = false;
    scene_.input.pendingOperation = OperationMode::None;
    scene_.input.buffer.clear();
    showMessage("Input cancelled.");
    postRedisplay();
}

void InputController::commitInput() {
    if (scene_.input.mode == InputMode::VectorStart || scene_.input.mode == InputMode::VectorEnd) {
        Vec3 value;
        if (!parseVec3Input(scene_.input.buffer, value)) {
            showMessage("Invalid vector. Use exactly three numbers: x y z", 5.0);
            scene_.input.buffer.clear();
            postRedisplay();
            return;
        }
        scene_.ui.presetIndex = -1;

        if (scene_.input.mode == InputMode::VectorStart) {
            scene_.vectors[scene_.input.activeVector].start = value;
            scene_.vectors[scene_.input.activeVector].hasStart = true;
            if (scene_.input.fullVector) {
                scene_.input.mode = InputMode::VectorEnd;
                scene_.input.buffer.clear();
                showMessage("Now enter vector " + scene_.vectors[scene_.input.activeVector].name + " end point: x y z", 8.0);
                postRedisplay();
                return;
            }
        }
        else {
            scene_.vectors[scene_.input.activeVector].end = value;
            scene_.vectors[scene_.input.activeVector].hasEnd = true;
            scene_.input.fullVector = false;
        }
        showMessage(scene_.vectors[scene_.input.activeVector].complete()
            ? "Vector " + scene_.vectors[scene_.input.activeVector].name + " is complete."
            : "Vector " + scene_.vectors[scene_.input.activeVector].name + " updated, but it is not complete yet.");
    }
    else if (scene_.input.mode == InputMode::Scalar) {
        double value;
        if (!parseScalarInput(scene_.input.buffer, value)) {
            showMessage("Invalid scalar. Use one number.", 5.0);
            scene_.input.buffer.clear();
            postRedisplay();
            return;
        }

        int index = std::clamp(scene_.input.activeVector, 0, static_cast<int>(scene_.vectors.size()) - 1);
        scene_.ui.presetIndex = -1;
        scene_.vectors[index].scalarK = value;
        scene_.scalarVectorIndex = index;
        scene_.currentMode = OperationMode::Scalar;
        showMessage("Scalar k for vector " + scene_.vectors[index].name + " applied.");
    }

    scene_.input.mode = InputMode::None;
    scene_.input.fullVector = false;
    scene_.input.buffer.clear();
    if (callbacks_.rebuildOperation) callbacks_.rebuildOperation();
    if (callbacks_.resetAnimation) callbacks_.resetAnimation();
    postRedisplay();
}
void InputController::handleInputKey(unsigned char key) {
    if (scene_.input.mode == InputMode::ScalarTarget || scene_.input.mode == InputMode::OperationVector) {
        if (key == 27) {
            cancelInput();
            return;
        }
        if (key == 8 || key == 127) {
            scene_.input.buffer.clear();
            postRedisplay();
            return;
        }
        if (key == 13 || key == 10) {
            if (scene_.input.buffer.size() == 1 && scene_.input.buffer[0] >= '1' && scene_.input.buffer[0] <= '3') {
                int index = scene_.input.buffer[0] - '1';
                if (scene_.input.mode == InputMode::ScalarTarget) beginScalarValueInput(index);
                else applyPendingVectorOperation(index);
            }
            else {
                showMessage("Choose vector 1, 2, or 3, then press Enter.", 4.0);
            }
            postRedisplay();
            return;
        }
        if (key >= '1' && key <= '3') {
            scene_.input.buffer.assign(1, static_cast<char>(key));
            postRedisplay();
            return;
        }

        showMessage("Type 1, 2, or 3, then press Enter.", 4.0);
        postRedisplay();
        return;
    }

    if (scene_.input.mode == InputMode::OperationPair) {
        if (key == 27) {
            cancelInput();
            return;
        }
        if (key == 8 || key == 127) {
            if (!scene_.input.buffer.empty()) scene_.input.buffer.pop_back();
            postRedisplay();
            return;
        }
        if (key == 13 || key == 10) {
            if (scene_.input.buffer.size() == 2 &&
                scene_.input.buffer[0] >= '1' && scene_.input.buffer[0] <= '3' &&
                scene_.input.buffer[1] >= '1' && scene_.input.buffer[1] <= '3') {
                applyPendingPairOperation(scene_.input.buffer[0] - '1', scene_.input.buffer[1] - '1');
            }
            else {
                showMessage("Choose pair like 12, 21, 13, 31, 23, or 32, then press Enter.", 4.0);
            }
            postRedisplay();
            return;
        }
        if (key >= '1' && key <= '3') {
            if (scene_.input.buffer.size() >= 2) scene_.input.buffer.clear();
            scene_.input.buffer.push_back(static_cast<char>(key));
            postRedisplay();
            return;
        }

        showMessage("Type two vector numbers, then press Enter.", 4.0);
        postRedisplay();
        return;
    }

    if (key == 27) {
        cancelInput();
        return;
    }
    if (key == 8 || key == 127) {
        if (!scene_.input.buffer.empty()) scene_.input.buffer.pop_back();
        postRedisplay();
        return;
    }
    if (key == 13 || key == 10) {
        commitInput();
        return;
    }

    if (std::isdigit(key) || key == '-' || key == '+' || key == '.' || key == ' ' ||
        key == ',' || key == ';' || key == '(' || key == ')') {
        scene_.input.buffer.push_back(static_cast<char>(key));
        postRedisplay();
    }
}

void InputController::keyboard(unsigned char key) {
    if (scene_.input.mode != InputMode::None) {
        handleInputKey(key);
        return;
    }

    if (key == 27) std::exit(0);

    unsigned char lower = static_cast<unsigned char>(std::tolower(key));
    switch (lower) {
    case '1': beginVectorInput(0, InputMode::VectorStart, true); break;
    case '2': beginVectorInput(1, InputMode::VectorStart, true); break;
    case '3': beginVectorInput(2, InputMode::VectorStart, true); break;
    case '4':
        if (callbacks_.selectVectorPair) callbacks_.selectVectorPair(0, 1);
        return;
    case '5':
        if (callbacks_.selectVectorPair) callbacks_.selectVectorPair(0, 2);
        return;
    case '6':
        if (callbacks_.selectVectorPair) callbacks_.selectVectorPair(1, 2);
        return;
    case 'p':
        if (callbacks_.loadPreset) callbacks_.loadPreset(scene_.ui.presetIndex + 1);
        break;
    case 'v':
        beginOperationPairInput(OperationMode::Projection);
        break;
    case 'n':
        beginOperationPairInput(OperationMode::SurfaceNormal);
        break;
    case 'o':
        beginOperationPairInput(OperationMode::Orthogonality);
        break;
    case 'r':
        if (callbacks_.resetAnimation) callbacks_.resetAnimation();
        showMessage("Animation replayed.");
        break;
    case 't':
        if (callbacks_.runVectorMathSelfTest) callbacks_.runVectorMathSelfTest();
        showMessage("Vector math self-test printed to console.", 5.0);
        break;
    case 'w':
        camera_.pan(0.0, 0.0, camera_.distance * 0.04);
        showMessage("Camera pan forward.");
        break;
    case 's':
        camera_.pan(0.0, 0.0, -camera_.distance * 0.04);
        showMessage("Camera pan backward.");
        break;
    case 'a':
        camera_.pan(-camera_.distance * 0.04, 0.0);
        showMessage("Camera pan left.");
        break;
    case 'd':
        camera_.pan(camera_.distance * 0.04, 0.0);
        showMessage("Camera pan right.");
        break;
    case 'q':
        camera_.pan(0.0, -camera_.distance * 0.04);
        showMessage("Camera moves down.");
        break;
    case 'e':
        camera_.pan(0.0, camera_.distance * 0.04);
        showMessage("Camera moves up.");
        break;
    case 'g':
        if (scene_.ui.gridMode == GridMode::Floor) scene_.ui.gridMode = GridMode::Full;
        else if (scene_.ui.gridMode == GridMode::Full) scene_.ui.gridMode = GridMode::Off;
        else scene_.ui.gridMode = GridMode::Floor;
        showMessage(scene_.ui.gridMode == GridMode::Floor ? "Grid mode: FLOOR"
            : scene_.ui.gridMode == GridMode::Full ? "Grid mode: FULL" : "Grid mode: OFF");
        break;
    case 'x':
        scene_.ui.showAxes = !scene_.ui.showAxes;
        showMessage(std::string("Axes ") + (scene_.ui.showAxes ? "ON" : "OFF"));
        break;
    case 'l':
        scene_.ui.showLabels = !scene_.ui.showLabels;
        showMessage(std::string("Labels ") + (scene_.ui.showLabels ? "ON" : "OFF"));
        break;
    case 'h':
        scene_.ui.showHud = !scene_.ui.showHud;
        break;
    default:
        break;
    }
    postRedisplay();
}
void InputController::specialKeyboard(int key) {
    int modifiers = glutGetModifiers();
    double panStep = camera_.distance * 0.035;

    switch (key) {
    case GLUT_KEY_F1:
        beginOperationVectorInput(OperationMode::Lengths);
        break;
    case GLUT_KEY_F2:
        beginOperationPairInput(OperationMode::AddParallelogram);
        break;
    case GLUT_KEY_F3:
        beginOperationPairInput(OperationMode::AddTriangle);
        break;
    case GLUT_KEY_F4:
        beginOperationPairInput(OperationMode::Subtract);
        break;
    case GLUT_KEY_F5:
        beginScalarInput();
        break;
    case GLUT_KEY_F6:
        beginOperationPairInput(OperationMode::Collinearity);
        break;
    case GLUT_KEY_F7:
        beginOperationPairInput(OperationMode::Angle);
        break;
    case GLUT_KEY_F8:
        beginOperationPairInput(OperationMode::Dot);
        break;
    case GLUT_KEY_F9:
        beginOperationPairInput(OperationMode::Cross);
        break;
    case GLUT_KEY_F10:
        if (callbacks_.setOperation) callbacks_.setOperation(OperationMode::Mixed);
        break;
    case GLUT_KEY_F11:
        beginOperationPairInput(OperationMode::SurfaceNormal);
        break;
    case GLUT_KEY_INSERT:
        if (callbacks_.setOperation) callbacks_.setOperation(OperationMode::None);
        break;
    case GLUT_KEY_END:
        if (callbacks_.clearAllVectors) callbacks_.clearAllVectors();
        return;
    case GLUT_KEY_HOME:
        camera_.reset();
        showMessage("Camera reset.");
        break;
    case GLUT_KEY_LEFT:
        if (modifiers & GLUT_ACTIVE_SHIFT) camera_.pan(-panStep, 0.0);
        else camera_.orbit(-4.0, 0.0);
        showMessage("Camera adjusted.");
        break;
    case GLUT_KEY_RIGHT:
        if (modifiers & GLUT_ACTIVE_SHIFT) camera_.pan(panStep, 0.0);
        else camera_.orbit(4.0, 0.0);
        showMessage("Camera adjusted.");
        break;
    case GLUT_KEY_UP:
        if (modifiers & GLUT_ACTIVE_SHIFT) camera_.pan(0.0, panStep);
        else camera_.orbit(0.0, -3.0);
        showMessage("Camera adjusted.");
        break;
    case GLUT_KEY_DOWN:
        if (modifiers & GLUT_ACTIVE_SHIFT) camera_.pan(0.0, -panStep);
        else camera_.orbit(0.0, 3.0);
        showMessage("Camera adjusted.");
        break;
    case GLUT_KEY_PAGE_UP:
        camera_.dolly(0.92);
        showMessage("Zoom in.");
        break;
    case GLUT_KEY_PAGE_DOWN:
        camera_.dolly(1.08);
        showMessage("Zoom out.");
        break;
    default:
        break;
    }
    postRedisplay();
}

void InputController::mouseButton(int button, int state, int x, int y) {
    bool overLeftHud = scene_.ui.showHud && x >= kPanelMargin && x <= kPanelMargin + kLeftHudWidth;

    if (button == 3 && state == GLUT_DOWN) {
        if (overLeftHud && scene_.ui.leftHudMaxScroll > 0.5) {
            scene_.ui.leftHudScroll = std::clamp(scene_.ui.leftHudScroll - 44.0, 0.0, scene_.ui.leftHudMaxScroll);
            postRedisplay();
            return;
        }
        camera_.dolly(0.92);
        postRedisplay();
        return;
    }
    if (button == 4 && state == GLUT_DOWN) {
        if (overLeftHud && scene_.ui.leftHudMaxScroll > 0.5) {
            scene_.ui.leftHudScroll = std::clamp(scene_.ui.leftHudScroll + 44.0, 0.0, scene_.ui.leftHudMaxScroll);
            postRedisplay();
            return;
        }
        camera_.dolly(1.08);
        postRedisplay();
        return;
    }

    if (button == GLUT_LEFT_BUTTON) {
        scene_.input.leftMouseDown = state == GLUT_DOWN;
    }
    if (button == GLUT_RIGHT_BUTTON) {
        scene_.input.rightMouseDown = state == GLUT_DOWN;
    }

    scene_.input.lastMouseX = x;
    scene_.input.lastMouseY = y;
}

void InputController::mouseMotion(int x, int y) {
    int dx = x - scene_.input.lastMouseX;
    int dy = y - scene_.input.lastMouseY;
    scene_.input.lastMouseX = x;
    scene_.input.lastMouseY = y;

    if (scene_.input.leftMouseDown) {
        camera_.orbit(dx * 0.35, dy * 0.25);
        postRedisplay();
    }
    else if (scene_.input.rightMouseDown) {
        double scale = camera_.distance * 0.0018;
        camera_.pan(-dx * scale, dy * scale);
        postRedisplay();
    }
}
