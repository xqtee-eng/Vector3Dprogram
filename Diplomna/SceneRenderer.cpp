#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <GL/glut.h>

#include "SceneRenderer.h"
#include "VectorMath.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <utility>

static void setColor(const Color& c) {
    glColor4f(c.r, c.g, c.b, c.a);
}

double smoothStep(double t) {
    t = std::clamp(t, 0.0, 1.0);
    return t * t * (3.0 - 2.0 * t);
}

static bool samePoint(const Vec3& a, const Vec3& b) {
    return (a - b).length() < EPS;
}

SceneRenderer::SceneRenderer(SceneModel& scene, CameraController& camera, Text3DCallback drawText3D)
    : scene_(scene), camera_(camera), drawText3D_(std::move(drawText3D)) {
}

double SceneRenderer::sceneLimit() const {
    double limit = 6.0;
    auto absorb = [&](const Vec3& p) {
        limit = std::max(limit, std::fabs(p.x));
        limit = std::max(limit, std::fabs(p.y));
        limit = std::max(limit, std::fabs(p.z));
    };

    for (const auto& v : scene_.vectors) {
        if (v.hasStart) absorb(v.start);
        if (v.hasEnd) absorb(v.end);
    }
    for (const auto& a : scene_.resultArrows) {
        absorb(a.start);
        absorb(a.end);
    }
    for (const auto& s : scene_.resultSurfaces) {
        for (const auto& p : s.vertices) absorb(p);
    }
    return std::clamp(std::ceil(limit + 2.0), 6.0, 40.0);
}

bool SceneRenderer::sameArrowForLabel(const Arrow3D& a, const Arrow3D& b) const {
    return samePoint(a.start, b.start) && samePoint(a.end, b.end);
}

void SceneRenderer::assignLabelLanes(std::vector<Arrow3D>& arrows) const {
    for (size_t i = 0; i < arrows.size(); ++i) {
        if (arrows[i].label.empty()) continue;

        int groupSize = 0;
        int indexInGroup = 0;
        for (size_t j = 0; j < arrows.size(); ++j) {
            if (arrows[j].label.empty() || !sameArrowForLabel(arrows[i], arrows[j])) continue;
            if (j < i) ++indexInGroup;
            ++groupSize;
        }

        if (groupSize <= 1) {
            arrows[i].labelLane = 0;
        }
        else {
            arrows[i].labelLane = indexInGroup * 2 - (groupSize - 1);
        }
    }
}

void SceneRenderer::drawArrow(const Arrow3D& arrow, double progress) const {
    Vec3 delta = arrow.end - arrow.start;
    Vec3 partialEnd = arrow.start + delta * progress;
    Vec3 dir = partialEnd - arrow.start;
    double length = dir.length();
    if (length < EPS) {
        glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT);
        glDisable(GL_LIGHTING);
        setColor(arrow.color);
        glPushMatrix();
        glTranslated(arrow.start.x, arrow.start.y, arrow.start.z);
        glutSolidSphere(0.13, 18, 12);
        glPopMatrix();

        if (!arrow.label.empty() && drawText3D_) {
            int signature = 0;
            for (unsigned char ch : arrow.label) signature += ch;
            double lane = static_cast<double>((signature % 5) - 2) * 0.45
                + static_cast<double>(arrow.labelLane) * 1.20;
            double lift = static_cast<double>(((signature / 5) % 3) - 1);
            Vec3 right = camera_.right();
            Vec3 up = right.cross(camera_.forward()).normalized();
            Vec3 labelPos = arrow.start
                + right * (0.24 * lane)
                + up * (0.30 + 0.10 * lift + 0.08 * std::abs(arrow.labelLane));
            drawText3D_(labelPos, arrow.label + " = 0", arrow.color);
        }

        glPopAttrib();
        return;
    }

    Vec3 unit = dir / length;

    glPushAttrib(GL_LINE_BIT | GL_ENABLE_BIT | GL_CURRENT_BIT);
    glDisable(GL_LIGHTING);
    if (arrow.dashed) {
        glEnable(GL_LINE_STIPPLE);
        glLineStipple(1, 0x00FF);
    }

    glLineWidth(arrow.width);
    setColor(arrow.color);
    glBegin(GL_LINES);
    glVertex3d(arrow.start.x, arrow.start.y, arrow.start.z);
    glVertex3d(partialEnd.x, partialEnd.y, partialEnd.z);
    glEnd();

    double coneHeight = std::clamp(length * 0.13, 0.18, 0.48);
    double coneRadius = std::clamp(length * 0.045, 0.07, 0.22);
    Vec3 coneBase = partialEnd - unit * coneHeight;

    glPushMatrix();
    glTranslated(coneBase.x, coneBase.y, coneBase.z);

    Vec3 base(0, 0, 1);
    Vec3 axis = base.cross(unit);
    double axisLen = axis.length();
    if (axisLen > EPS) {
        axis = axis / axisLen;
        double angle = std::acos(std::clamp(base.dot(unit), -1.0, 1.0)) * 180.0 / PI;
        glRotated(angle, axis.x, axis.y, axis.z);
    }
    else if (unit.z < 0) {
        glRotated(180.0, 1.0, 0.0, 0.0);
    }

    glutSolidCone(coneRadius, coneHeight, 24, 12);
    glPopMatrix();

    if (!arrow.label.empty() && drawText3D_) {
        int signature = 0;
        for (unsigned char ch : arrow.label) signature += ch;
        double lane = static_cast<double>((signature % 5) - 2) * 0.45
            + static_cast<double>(arrow.labelLane) * 1.20;
        double lift = static_cast<double>(((signature / 5) % 3) - 1);
        double labelAt = (arrow.label == "X" || arrow.label == "Y" || arrow.label == "Z") ? 0.98 : 0.68;
        Vec3 right = camera_.right();
        Vec3 up = right.cross(camera_.forward()).normalized();
        Vec3 labelPos = arrow.start + (partialEnd - arrow.start) * labelAt
            + right * (0.22 * lane)
            + up * (0.24 + 0.09 * lift + 0.06 * std::abs(arrow.labelLane));
        drawText3D_(labelPos, arrow.label, arrow.color);
    }

    glPopAttrib();
}

void SceneRenderer::drawVectorSlot(const VectorSlot& slot, double progress, int labelLane) const {
    if (!slot.complete()) return;
    Arrow3D arrow{ slot.start, slot.end, slot.color, slot.name, 4.0f, false };
    arrow.labelLane = labelLane;
    drawArrow(arrow, progress);

    if (vectorLength(slot.vec()) < EPS) return;

    glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT);
    glDisable(GL_LIGHTING);
    setColor(Color{slot.color.r, slot.color.g, slot.color.b, 0.95f});
    glPushMatrix();
    glTranslated(slot.start.x, slot.start.y, slot.start.z);
    glutSolidSphere(0.08, 12, 8);
    glPopMatrix();
    glPopAttrib();
}

void SceneRenderer::drawSurface(const Surface3D& surface) const {
    if (surface.vertices.size() < 3) return;

    glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT | GL_LINE_BIT);
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    setColor(surface.color);

    glBegin(GL_POLYGON);
    for (const auto& p : surface.vertices) {
        glVertex3d(p.x, p.y, p.z);
    }
    glEnd();

    glDepthMask(GL_TRUE);

    if (surface.outline) {
        Color edge = surface.color;
        edge.a = 0.75f;
        setColor(edge);
        glLineWidth(1.5f);
        glBegin(GL_LINE_LOOP);
        for (const auto& p : surface.vertices) {
            glVertex3d(p.x, p.y, p.z);
        }
        glEnd();
    }

    glPopAttrib();
}

void SceneRenderer::drawArc(const Arc3D& arc) const {
    glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT | GL_LINE_BIT);
    glDisable(GL_LIGHTING);
    setColor(arc.color);
    glLineWidth(3.0f);

    int steps = 64;
    glBegin(GL_LINE_STRIP);
    for (int i = 0; i <= steps; ++i) {
        double t = arc.angle * static_cast<double>(i) / static_cast<double>(steps);
        Vec3 p = arc.origin + (arc.u * std::cos(t) + arc.v * std::sin(t)) * arc.radius;
        glVertex3d(p.x, p.y, p.z);
    }
    glEnd();

    double mid = arc.angle * 0.5;
    Vec3 labelPos = arc.origin + (arc.u * std::cos(mid) + arc.v * std::sin(mid)) * (arc.radius + 0.25);
    if (drawText3D_) drawText3D_(labelPos, arc.label, arc.color);

    glPopAttrib();
}

void SceneRenderer::drawGridPlane(double limit, double step, int plane) const {
    glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT | GL_LINE_BIT);
    glDisable(GL_LIGHTING);
    glLineWidth(plane == 0 ? 1.0f : 0.95f);

    if (plane == 0) setColor(Color{0.18f, 0.22f, 0.28f, 0.48f});
    if (plane == 1) setColor(Color{0.18f, 0.24f, 0.32f, 0.22f});
    if (plane == 2) setColor(Color{0.17f, 0.28f, 0.22f, 0.20f});

    glBegin(GL_LINES);
    for (double i = -limit; i <= limit + EPS; i += step) {
        if (plane == 0) {
            glVertex3d(i, 0, -limit);
            glVertex3d(i, 0, limit);
            glVertex3d(-limit, 0, i);
            glVertex3d(limit, 0, i);
        }
        else if (plane == 1) {
            glVertex3d(i, -limit, 0);
            glVertex3d(i, limit, 0);
            glVertex3d(-limit, i, 0);
            glVertex3d(limit, i, 0);
        }
        else {
            glVertex3d(0, i, -limit);
            glVertex3d(0, i, limit);
            glVertex3d(0, -limit, i);
            glVertex3d(0, limit, i);
        }
    }
    glEnd();

    glLineWidth(plane == 0 ? 1.45f : 1.2f);
    if (plane == 0) setColor(Color{0.32f, 0.38f, 0.48f, 0.66f});
    if (plane == 1) setColor(Color{0.30f, 0.42f, 0.58f, 0.36f});
    if (plane == 2) setColor(Color{0.28f, 0.48f, 0.34f, 0.32f});

    glBegin(GL_LINES);
    for (double i = -limit; i <= limit + EPS; i += 5.0) {
        if (plane == 0) {
            glVertex3d(i, 0, -limit);
            glVertex3d(i, 0, limit);
            glVertex3d(-limit, 0, i);
            glVertex3d(limit, 0, i);
        }
        else if (plane == 1) {
            glVertex3d(i, -limit, 0);
            glVertex3d(i, limit, 0);
            glVertex3d(-limit, i, 0);
            glVertex3d(limit, i, 0);
        }
        else {
            glVertex3d(0, i, -limit);
            glVertex3d(0, i, limit);
            glVertex3d(0, -limit, i);
            glVertex3d(0, limit, i);
        }
    }
    glEnd();

    glPopAttrib();
}

void SceneRenderer::drawAxes(double limit) const {
    if (!scene_.ui.showAxes) return;

    glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT | GL_LINE_BIT);
    glDisable(GL_LIGHTING);

    const Color xColor{1.0f, 0.24f, 0.22f, 1.0f};
    const Color yColor{0.28f, 1.0f, 0.40f, 1.0f};
    const Color zColor{0.30f, 0.62f, 1.0f, 1.0f};

    auto drawAxisSegment = [&](const Vec3& from, const Vec3& to, const Color& color, float width, float alpha) {
        Color c = color;
        c.a = alpha;
        setColor(c);
        glLineWidth(width);
        glBegin(GL_LINES);
        glVertex3d(from.x, from.y, from.z);
        glVertex3d(to.x, to.y, to.z);
        glEnd();
    };

    drawAxisSegment(Vec3(-limit, 0, 0), Vec3(0, 0, 0), xColor, 1.45f, 0.36f);
    drawAxisSegment(Vec3(0, -limit, 0), Vec3(0, 0, 0), yColor, 1.45f, 0.36f);
    drawAxisSegment(Vec3(0, 0, -limit), Vec3(0, 0, 0), zColor, 1.45f, 0.36f);
    drawAxisSegment(Vec3(0, 0, 0), Vec3(limit, 0, 0), xColor, 2.4f, 0.88f);
    drawAxisSegment(Vec3(0, 0, 0), Vec3(0, limit, 0), yColor, 2.4f, 0.88f);
    drawAxisSegment(Vec3(0, 0, 0), Vec3(0, 0, limit), zColor, 2.4f, 0.88f);

    auto axisArrow = [&](const Vec3& end, const Color& color, const std::string& label) {
        drawArrow(Arrow3D{ Vec3(0, 0, 0), end, color, label, 3.2f, false }, 1.0);
    };

    axisArrow(Vec3(limit, 0, 0), xColor, "X");
    axisArrow(Vec3(0, limit, 0), yColor, "Y");
    axisArrow(Vec3(0, 0, limit), zColor, "Z");

    glPushMatrix();
    glTranslated(0, 0, 0);
    setColor(Color{0.94f, 0.96f, 1.0f, 1.0f});
    glutSolidSphere(0.10, 16, 10);
    glPopMatrix();
    if (drawText3D_) {
        drawText3D_(Vec3(0.22, 0.22, 0.22), "O", Color{0.94f, 0.96f, 1.0f, 1.0f});
    }

    for (int i = static_cast<int>(-limit); i <= static_cast<int>(limit); ++i) {
        if (i == 0) continue;
        bool major = i % 5 == 0;
        double tick = major ? 0.18 : 0.08;
        glLineWidth(major ? 2.0f : 1.0f);
        glBegin(GL_LINES);
        setColor(xColor);
        glVertex3d(i, -tick, 0);
        glVertex3d(i, tick, 0);
        setColor(yColor);
        glVertex3d(-tick, i, 0);
        glVertex3d(tick, i, 0);
        setColor(zColor);
        glVertex3d(0, -tick, i);
        glVertex3d(0, tick, i);
        glEnd();
    }

    glPopAttrib();
}

void SceneRenderer::draw() const {
    double limit = sceneLimit();

    glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT | GL_LINE_BIT);
    glEnable(GL_DEPTH_TEST);

    if (scene_.ui.gridMode != GridMode::Off) {
        drawGridPlane(limit, 1.0, 0);
    }
    if (scene_.ui.gridMode == GridMode::Full) {
        drawGridPlane(limit, 2.0, 1);
        drawGridPlane(limit, 2.0, 2);
    }

    drawAxes(limit);

    for (const auto& surface : scene_.resultSurfaces) {
        drawSurface(surface);
    }

    double t = smoothStep(scene_.ui.animationT);

    std::vector<Arrow3D> labelArrows;
    std::array<int, 3> vectorLabelLanes = { 0, 0, 0 };

    for (const auto& slot : scene_.vectors) {
        if (!slot.complete()) continue;
        labelArrows.push_back(Arrow3D{ slot.start, slot.end, slot.color, slot.name, 4.0f, false });
    }

    if (scene_.ui.showGhosts) {
        labelArrows.insert(labelArrows.end(), scene_.resultArrows.begin(), scene_.resultArrows.end());
    }

    assignLabelLanes(labelArrows);

    size_t laneIndex = 0;
    for (size_t i = 0; i < scene_.vectors.size(); ++i) {
        if (!scene_.vectors[i].complete()) continue;
        vectorLabelLanes[i] = labelArrows[laneIndex++].labelLane;
        drawVectorSlot(scene_.vectors[i], 1.0, vectorLabelLanes[i]);
    }

    if (scene_.ui.showGhosts) {
        for (size_t i = 0; i < scene_.resultArrows.size(); ++i) {
            Arrow3D arrow = scene_.resultArrows[i];
            if (laneIndex + i < labelArrows.size()) {
                arrow.labelLane = labelArrows[laneIndex + i].labelLane;
            }
            drawArrow(arrow, t);
        }
    }

    for (const auto& arc : scene_.resultArcs) {
        drawArc(arc);
    }

    glPopAttrib();
}
