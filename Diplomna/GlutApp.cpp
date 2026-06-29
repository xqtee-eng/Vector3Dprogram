#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <GL/glut.h>

#ifndef GL_MULTISAMPLE
#define GL_MULTISAMPLE 0x809D
#endif

#include "GlutApp.h"

#include <algorithm>
#include <iostream>

GlutApp* GlutApp::activeApp_ = nullptr;

GlutApp::GlutApp(SceneModel& scene, CameraController& camera, SceneController& sceneController,
    SceneRenderer& sceneRenderer, HudRenderer& hudRenderer, TextRenderer& textRenderer,
    InputController& inputController, const PresetLibrary& presets)
    : scene_(scene),
      camera_(camera),
      sceneController_(sceneController),
      sceneRenderer_(sceneRenderer),
      hudRenderer_(hudRenderer),
      textRenderer_(textRenderer),
      inputController_(inputController),
      presets_(presets) {
}

void GlutApp::requestRedisplay() {
    glutPostRedisplay();
}

int GlutApp::run(int argc, char** argv) {
    activeApp_ = this;

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH | GLUT_MULTISAMPLE);
    glutInitWindowSize(width_, height_);
    glutCreateWindow("3D Vector Visualizer");

    initOpenGL();

    sceneController_.rebuildOperation();
    sceneController_.resetAnimation();

    glutDisplayFunc(displayCallback);
    glutReshapeFunc(reshapeCallback);
    glutKeyboardFunc(keyboardCallback);
    glutSpecialFunc(specialKeyboardCallback);
    glutMouseFunc(mouseButtonCallback);
    glutMotionFunc(mouseMotionCallback);
	glutTimerFunc(10, timerCallback, 0); // зміна частота таймера на 10 мс для більш плавної анімації

    printConsoleHelp();
    glutMainLoop();
    return 0;
}

void GlutApp::initOpenGL() {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_COLOR_MATERIAL);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHT1);
    glEnable(GL_MULTISAMPLE);

    GLfloat lightPos[] = { 6.0f, 12.0f, 8.0f, 1.0f };
    GLfloat ambient[] = { 0.18f, 0.20f, 0.24f, 1.0f };
    GLfloat diffuse[] = { 0.85f, 0.88f, 0.92f, 1.0f };
    GLfloat specular[] = { 0.8f, 0.8f, 0.8f, 1.0f };
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, specular);

    GLfloat light1Pos[] = { -6.0f, -8.0f, -8.0f, 1.0f };
    GLfloat light1Ambient[] = { 0.05f, 0.05f, 0.08f, 1.0f };
    GLfloat light1Diffuse[] = { 0.25f, 0.25f, 0.30f, 1.0f };
    GLfloat light1Specular[] = { 0.10f, 0.10f, 0.15f, 1.0f };
    glLightfv(GL_LIGHT1, GL_POSITION, light1Pos);
    glLightfv(GL_LIGHT1, GL_AMBIENT, light1Ambient);
    glLightfv(GL_LIGHT1, GL_DIFFUSE, light1Diffuse);
    glLightfv(GL_LIGHT1, GL_SPECULAR, light1Specular);

    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    GLfloat matSpecular[] = { 0.4f, 0.4f, 0.4f, 1.0f };
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, matSpecular);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 32.0f);

    textRenderer_.initFonts();
    hudRenderer_.initFonts();
}

void GlutApp::setupCamera() {
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    double aspect = height_ == 0 ? 1.0 : static_cast<double>(width_) / static_cast<double>(height_);
    gluPerspective(55.0, aspect, 0.05, 500.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    Vec3 eye = camera_.eye();
    gluLookAt(eye.x, eye.y, eye.z, camera_.target.x, camera_.target.y, camera_.target.z, 0, 1, 0);
}

    void GlutApp::display() {
        glViewport(0, 0, width_, height_);
        glClearColor(0.008f, 0.012f, 0.020f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glEnable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_LINE_SMOOTH);
        glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

        setupCamera();
        sceneRenderer_.draw();
        hudRenderer_.draw(width_, height_);

        glutSwapBuffers();
    }

void GlutApp::reshape(int width, int height) {
    int clampedW = std::max(width, kMinWindowWidth);
    int clampedH = std::max(height, kMinWindowHeight);
    if (width != clampedW || height != clampedH) {
        width_ = clampedW;
        height_ = clampedH;
        glViewport(0, 0, width_, height_);
        glutReshapeWindow(clampedW, clampedH);
        requestRedisplay();
        return;
    }

    width_ = clampedW;
    height_ = clampedH;
    glViewport(0, 0, width_, height_);
    requestRedisplay();
}

void GlutApp::timer() {
    if (camera_.update(0.18)) {
        requestRedisplay();
    }

    if (scene_.ui.animationT < 1.0) {
        scene_.ui.animationT = std::min(1.0, scene_.ui.animationT + 0.018);
        requestRedisplay();
    }
    if (scene_.ui.hudMessageTimer > 0.0) {
        scene_.ui.hudMessageTimer -= 0.016;
        if (scene_.ui.hudMessageTimer <= 0.0 && scene_.input.mode == InputMode::None) {
            scene_.ui.hudMessage = "Ready. Press H to hide/show help panels.";
            requestRedisplay();
        }
    }
	glutTimerFunc(10, timerCallback, 0); // зміна частота таймера на 10 мс для більш плавної анімації
}

void GlutApp::printConsoleHelp() const {
    std::cout << "3D Vector Visualizer started.\n";
    std::cout << "Keys:\n";
    std::cout << "  1/2/3 enter complete vectors a/b/c: start point, then end point.\n";
    std::cout << "  4/5/6 selects active pair a & b, a & c, b & c.\n";
    std::cout << "  F1 asks for one vector; F2/F3/F4/F6/F7/F8/F9/F11 ask for an ordered pair.\n";
    std::cout << "  F5 asks for one vector and scalar k. F10 uses all three vectors.\n";
    std::cout << "  Pair input accepts 12, 21, 13, 31, 23, or 32, then Enter.\n";
    std::cout << "  F10 mixed product, F11 normal on plane with vector projection.\n";
    std::cout << "  V projection of active pair, O orthogonality check.\n";
    std::cout << "  F5 asks for vector 1/2/3 and scalar k; P cycles "
              << presets_.count() << " presets.\n";
    std::cout << "  Camera: LMB orbit, RMB pan, wheel zoom, W/A/S/D pan, Q/E vertical move.\n";
    std::cout << "  Arrows orbit, Shift+arrows pan, PageUp/PageDown zoom, Home resets camera.\n";
    std::cout << "  G cycles grid mode, X axes, L labels, H HUD, R replay, Insert clear result, End clear all.\n";
    std::cout << "  T runs vector math self-test in the console. Esc exits.\n";

}

void GlutApp::displayCallback() {
    if (activeApp_) activeApp_->display();
}

void GlutApp::reshapeCallback(int width, int height) {
    if (activeApp_) activeApp_->reshape(width, height);
}

void GlutApp::timerCallback(int) {
    if (activeApp_) activeApp_->timer();
}

void GlutApp::keyboardCallback(unsigned char key, int, int) {
    if (activeApp_) activeApp_->inputController_.keyboard(key);
}

void GlutApp::specialKeyboardCallback(int key, int, int) {
    if (activeApp_) activeApp_->inputController_.specialKeyboard(key);
}

void GlutApp::mouseButtonCallback(int button, int state, int x, int y) {
    if (activeApp_) activeApp_->inputController_.mouseButton(button, state, x, y);
}

void GlutApp::mouseMotionCallback(int x, int y) {
    if (activeApp_) activeApp_->inputController_.mouseMotion(x, y);
}
