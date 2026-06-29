#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <GL/glut.h>

#include "HudRenderer.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

constexpr int kPanelMargin = 12;
constexpr int kPanelPad = 14;
constexpr int kLeftHudWidth = 410;
constexpr int kRightHudWidth = 280;
constexpr int kInfoLineH = 20;

struct UiFont {
    HFONT handle = nullptr;
    GLuint base = 0;
    int height = 12;
    int ascent = 9;
    bool ready = false;
};

UiFont fontBody;
UiFont fontSmall;
UiFont fontTitle;
UiFont fontMono;
UiFont fontMonoSmall;

static void setColor(const Color& color) {
    glColor4f(color.r, color.g, color.b, color.a);
}

UiFont* resolveFont(void* font) {
    if (font == GLUT_BITMAP_HELVETICA_18) return &fontTitle;
    if (font == GLUT_BITMAP_HELVETICA_10) return &fontSmall;
    if (font == GLUT_BITMAP_9_BY_15) return &fontMono;
    return &fontBody;
}

int fallbackTextWidth(const std::string& text, void* font) {
    int width = 0;
    for (unsigned char ch : text) {
        width += glutBitmapWidth(font, ch);
    }
    return width;
}

bool createUiFont(UiFont& out, const char* face, int pixelHeight, int weight = FW_NORMAL) {
    HDC hdc = wglGetCurrentDC();
    if (!hdc) return false;

    out.base = glGenLists(96);
    if (out.base == 0) return false;

    out.handle = CreateFontA(
        -pixelHeight, 0, 0, 0, weight, FALSE, FALSE, FALSE,
        ANSI_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, FF_DONTCARE | DEFAULT_PITCH, face);

    if (!out.handle) return false;

    HFONT oldFont = static_cast<HFONT>(SelectObject(hdc, out.handle));
    TEXTMETRICA tm{};
    GetTextMetricsA(hdc, &tm);
    out.height = tm.tmHeight;
    out.ascent = tm.tmAscent;
    out.ready = wglUseFontBitmapsA(hdc, 32, 96, out.base) == TRUE;
    SelectObject(hdc, oldFont);
    return out.ready;
}

void drawText2D(int x, int y, const std::string& text,
    const Color& color = Color{1, 1, 1, 1}, void* font = GLUT_BITMAP_HELVETICA_12) {
    setColor(color);
    glRasterPos2i(x, y);
    UiFont* uiFont = resolveFont(font);
    if (uiFont && uiFont->ready) {
        std::string printable = text;
        for (char& ch : printable) {
            unsigned char c = static_cast<unsigned char>(ch);
            if (c < 32 || c > 127) ch = '?';
        }
        glPushAttrib(GL_LIST_BIT);
        glListBase(uiFont->base - 32);
        glCallLists(static_cast<GLsizei>(printable.size()), GL_UNSIGNED_BYTE, printable.c_str());
        glPopAttrib();
    }
    else {
        for (unsigned char ch : text) {
            glutBitmapCharacter(font, ch);
        }
    }
}

int textWidth(const std::string& text, void* font = GLUT_BITMAP_HELVETICA_12) {
    UiFont* uiFont = resolveFont(font);
    if (uiFont && uiFont->ready && uiFont->handle) {
        HDC hdc = wglGetCurrentDC();
        if (hdc) {
            HFONT oldFont = static_cast<HFONT>(SelectObject(hdc, uiFont->handle));
            SIZE size{};
            GetTextExtentPoint32A(hdc, text.c_str(), static_cast<int>(text.size()), &size);
            SelectObject(hdc, oldFont);
            return size.cx;
        }
    }
    return fallbackTextWidth(text, font);
}

int fontHeight(void* font) {
    UiFont* uiFont = resolveFont(font);
    if (uiFont && uiFont->ready) return uiFont->height;
    if (font == GLUT_BITMAP_HELVETICA_18) return 18;
    if (font == GLUT_BITMAP_HELVETICA_10) return 10;
    if (font == GLUT_BITMAP_9_BY_15) return 15;
    return 12;
}

int fontDescent(void* font) {
    UiFont* uiFont = resolveFont(font);
    if (uiFont && uiFont->ready) return std::max(1, uiFont->height - uiFont->ascent);
    if (font == GLUT_BITMAP_HELVETICA_18) return 4;
    if (font == GLUT_BITMAP_HELVETICA_10) return 2;
    if (font == GLUT_BITMAP_9_BY_15) return 4;
    return 3;
}

int centeredTextBaselineY(int y, int h, void* font) {
    int fh = fontHeight(font);
    int descent = fontDescent(font);
    return y + std::max(0, (h - fh) / 2) + descent;
}

int infoTextBaselineY(int y, void* font = GLUT_BITMAP_HELVETICA_12) {
    return centeredTextBaselineY(y - kInfoLineH + 4, kInfoLineH, font);
}

void advanceInfoLine(int& y) {
    y -= kInfoLineH;
}

std::string fitText(std::string text, int maxWidth, void* font = GLUT_BITMAP_HELVETICA_12) {
    if (maxWidth <= 0) return "";
    if (textWidth(text, font) <= maxWidth) return text;

    const std::string suffix = "...";
    while (!text.empty() && textWidth(text + suffix, font) > maxWidth) {
        text.pop_back();
    }
    return text.empty() ? "" : text + suffix;
}

std::vector<std::string> wrapText(const std::string& text, int maxWidth) {
    std::vector<std::string> lines;
    std::istringstream words(text);
    std::string word;
    std::string line;

    while (words >> word) {
        std::string candidate = line.empty() ? word : line + " " + word;
        if (!line.empty() && textWidth(candidate) > maxWidth) {
            lines.push_back(line);
            line = word;
        }
        else {
            line = candidate;
        }
    }

    if (!line.empty()) lines.push_back(line);
    if (lines.empty()) lines.push_back("");
    return lines;
}

void drawWrappedText2D(int x, int& y, int maxWidth, const std::string& text,
    const Color& color = Color{1, 1, 1, 1}, int lineHeight = 16) {
    for (const auto& line : wrapText(text, maxWidth)) {
        drawText2D(x, y, line, color);
        y -= lineHeight;
    }
}

void drawRect2D(int x, int y, int w, int h, const Color& color) {
    setColor(color);
    glBegin(GL_QUADS);
    glVertex2i(x, y);
    glVertex2i(x + w, y);
    glVertex2i(x + w, y + h);
    glVertex2i(x, y + h);
    glEnd();
}

void drawRoundedRect2D(int x, int y, int w, int h, int radius, const Color& color) {
    if (w <= 0 || h <= 0) return;
    int r = std::max(0, std::min({ radius, w / 2, h / 2 }));
    if (r == 0) {
        drawRect2D(x, y, w, h, color);
        return;
    }

    setColor(color);
    glBegin(GL_POLYGON);
    constexpr int segments = 7;
    auto corner = [&](double cx, double cy, double startDeg, double endDeg) {
        for (int i = 0; i <= segments; ++i) {
            double t = startDeg + (endDeg - startDeg) * static_cast<double>(i) / segments;
            double rad = t * PI / 180.0;
            glVertex2d(cx + std::cos(rad) * r, cy + std::sin(rad) * r);
        }
    };
    corner(x + r, y + r, 180.0, 270.0);
    corner(x + w - r, y + r, 270.0, 360.0);
    corner(x + w - r, y + h - r, 0.0, 90.0);
    corner(x + r, y + h - r, 90.0, 180.0);
    glEnd();
}

void drawRoundedOutline2D(int x, int y, int w, int h, int radius, const Color& color) {
    if (w <= 0 || h <= 0) return;
    int r = std::max(0, std::min({ radius, w / 2, h / 2 }));
    if (r == 0) {
        setColor(color);
        glBegin(GL_LINE_LOOP);
        glVertex2i(x, y);
        glVertex2i(x + w, y);
        glVertex2i(x + w, y + h);
        glVertex2i(x, y + h);
        glEnd();
        return;
    }

    setColor(color);
    glBegin(GL_LINE_LOOP);
    constexpr int segments = 7;
    auto corner = [&](double cx, double cy, double startDeg, double endDeg) {
        for (int i = 0; i <= segments; ++i) {
            double t = startDeg + (endDeg - startDeg) * static_cast<double>(i) / segments;
            double rad = t * PI / 180.0;
            glVertex2d(cx + std::cos(rad) * r, cy + std::sin(rad) * r);
        }
    };
    corner(x + r, y + r, 180.0, 270.0);
    corner(x + w - r, y + r, 270.0, 360.0);
    corner(x + w - r, y + h - r, 0.0, 90.0);
    corner(x + r, y + h - r, 90.0, 180.0);
    glEnd();
}

void drawPanel2D(int x, int y, int w, int h, const Color& fill, const Color& accent) {
    drawRoundedRect2D(x + 8, y - 8, w, h, 14, Color{0.0f, 0.0f, 0.0f, 0.28f});
    drawRoundedRect2D(x - 1, y - 1, w + 2, h + 2, 14, Color{0.10f, 0.20f, 0.36f, 0.20f});
    drawRoundedRect2D(x, y, w, h, 10, fill);
    drawRoundedRect2D(x + 1, y + h - 78, w - 2, 76, 10, Color{0.080f, 0.105f, 0.150f, 0.28f});
    drawRoundedOutline2D(x, y, w, h, 10, Color{0.20f, 0.34f, 0.54f, 0.82f});
    drawRoundedRect2D(x + 2, y + h - 6, w - 4, 4, 3, accent);
}

void drawScrollBar2D(int x, int y, int h, double scroll, double maxScroll, int contentHeight) {
    if (h <= 0) return;

    int thumbH = h;
    int thumbY = y;
    if (maxScroll > 0.5 && contentHeight > 0) {
        double ratio = std::min(1.0, static_cast<double>(h) / static_cast<double>(contentHeight));
        thumbH = std::max(36, static_cast<int>(h * ratio));
        int thumbTravel = std::max(1, h - thumbH);
        thumbY = y + thumbTravel - static_cast<int>((scroll / maxScroll) * thumbTravel);
    }

    drawRoundedRect2D(x, y, 4, h, 2, Color{0.075f, 0.120f, 0.180f, 0.55f});
    drawRoundedRect2D(x - 1, thumbY, 6, thumbH, 3,
        maxScroll > 0.5 ? Color{0.38f, 0.66f, 1.0f, 0.90f} : Color{0.20f, 0.32f, 0.46f, 0.48f});
}

void drawKeyButton2D(int x, int y, int w, int h, const std::string& key, const std::string& label, bool active = false) {
    void* font = GLUT_BITMAP_HELVETICA_12;
    Color fill = active
        ? Color{0.10f, 0.17f, 0.31f, 0.98f}
        : Color{0.050f, 0.070f, 0.105f, 0.88f};
    Color border = active
        ? Color{0.50f, 0.70f, 1.0f, 1.0f}
        : Color{0.18f, 0.30f, 0.44f, 0.92f};
    Color keyColor = active
        ? Color{0.78f, 0.90f, 1.0f, 1.0f}
        : Color{0.42f, 0.78f, 1.0f, 1.0f};
    Color labelColor = active
        ? Color{0.96f, 0.98f, 1.0f, 1.0f}
        : Color{0.80f, 0.86f, 0.92f, 1.0f};

    drawRoundedRect2D(x + 3, y - 3, w, h, 7, Color{0.0f, 0.0f, 0.0f, active ? 0.24f : 0.15f});
    if (active) {
        drawRoundedRect2D(x - 1, y - 1, w + 2, h + 2, 7, Color{0.20f, 0.38f, 0.72f, 0.30f});
    }
    drawRoundedRect2D(x, y, w, h, 5, fill);
    drawRoundedOutline2D(x, y, w, h, 5, border);

    const int keyW = 34;
    setColor(Color{border.r, border.g, border.b, 0.72f});
    glBegin(GL_LINES);
    glVertex2i(x + keyW, y + 3);
    glVertex2i(x + keyW, y + h - 3);
    glEnd();

    int textY = centeredTextBaselineY(y, h, font);
    int keyX = x + std::max(4, (keyW - textWidth(key, font)) / 2);
    drawText2D(keyX, textY, key, keyColor, font);

    int labelW = w - keyW - 10;
    std::string text = fitText(label, labelW, font);
    int labelX = x + keyW + 5 + std::max(0, (labelW - textWidth(text, font)) / 2);
    drawText2D(labelX, textY, text, labelColor, font);
}

void drawPanelHeader(int x, int& y, const std::string& text, int width) {
    const int headerH = 24;
    int boxY = y - 17;
    drawRoundedRect2D(x - 3, boxY, width + 6, headerH, 8, Color{0.052f, 0.070f, 0.105f, 0.70f});
    drawRoundedRect2D(x + 2, boxY + 4, 4, headerH - 8, 2, Color{0.42f, 0.66f, 1.0f, 0.86f});
    drawRoundedOutline2D(x - 3, boxY, width + 6, headerH, 8, Color{0.17f, 0.27f, 0.40f, 0.64f});
    drawText2D(x + 12, centeredTextBaselineY(boxY, headerH, GLUT_BITMAP_HELVETICA_12),
        text, Color{0.86f, 0.90f, 0.98f, 1.0f}, GLUT_BITMAP_HELVETICA_12);
    y = boxY - 12;
}

void drawKeyValue(int x, int& y, const std::string& key, const std::string& value,
    const Color& keyColor, const Color& valueColor, int width) {
    const int rowH = 20;
    int rowY = y - rowH + 4;
    int textY = centeredTextBaselineY(rowY, rowH, GLUT_BITMAP_HELVETICA_12);
    drawRoundedRect2D(x - 3, rowY, width + 6, rowH, 5, Color{0.045f, 0.060f, 0.088f, 0.38f});
    drawText2D(x, textY, key, keyColor);
    drawText2D(x + 86, textY, fitText(value, width - 96), valueColor);
    y -= rowH;
}

static std::string trimNumber(std::string text) {
    if (text.find('.') != std::string::npos) {
        while (!text.empty() && text.back() == '0') text.pop_back();
        if (!text.empty() && text.back() == '.') text.pop_back();
    }
    if (text == "-0") text = "0";
    return text;
}

HudRenderer::HudRenderer(SceneModel& scene, CameraController& camera, const PresetLibrary& presets)
    : scene_(scene), camera_(camera), presets_(presets) {
}

void HudRenderer::initFonts() {
    createUiFont(fontBody, "Segoe UI", 14, FW_NORMAL);
    createUiFont(fontSmall, "Segoe UI", 11, FW_NORMAL);
    createUiFont(fontTitle, "Segoe UI Semibold", 19, FW_SEMIBOLD);
    createUiFont(fontMono, "Cascadia Mono", 14, FW_NORMAL);
    createUiFont(fontMonoSmall, "Cascadia Mono", 11, FW_NORMAL);
}

std::string HudRenderer::formatDouble(double value, int precision) const {
    if (std::fabs(value) < EPS) value = 0.0;
    std::ostringstream out;
    out << std::fixed << std::setprecision(precision) << value;
    return trimNumber(out.str());
}

std::string HudRenderer::vectorText(const Vec3& v) const {
    return "(" + formatDouble(v.x) + ", " + formatDouble(v.y) + ", " + formatDouble(v.z) + ")";
}

std::string HudRenderer::operationName(OperationMode mode) const {
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

std::string HudRenderer::gridModeName() const {
    switch (scene_.ui.gridMode) {
    case GridMode::Off: return "OFF";
    case GridMode::Floor: return "FLOOR";
    case GridMode::Full: return "FULL";
    default: return "FULL";
    }
}

std::string HudRenderer::pairLabel(const std::string& separator) const {
    int first = scene_.activePairFirst;
    int second = scene_.activePairSecond;
    if (first < 0 || first >= static_cast<int>(scene_.vectors.size())) return "?";
    if (second < 0 || second >= static_cast<int>(scene_.vectors.size())) return "?";
    return scene_.vectors[first].name + separator + scene_.vectors[second].name;
}

int HudRenderer::displayAngle(double degrees) const {
    double normalized = std::fmod(degrees, 360.0);
    if (std::fabs(normalized) < 1e-7) normalized = 0.0;

    return normalized >= 0.0
        ? static_cast<int>(std::floor(normalized))
        : static_cast<int>(std::ceil(normalized));
}

void HudRenderer::draw(int width, int height) {
    if (!scene_.ui.showHud) return;

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, width, 0, height);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT | GL_LINE_BIT);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    int panelMargin = kPanelMargin;
    int panelPad = kPanelPad;
    int leftW = kLeftHudWidth;
    int rightW = kRightHudWidth;
    int leftX = panelMargin;
    int rightX = width - rightW - panelMargin;
    int centerX = leftX + leftW + 16;
    int centerW = std::max(0, rightX - centerX - 16);
    bool showCenterMessage = centerW >= 300 && height >= 720;
    Color panelColor{0.030f, 0.040f, 0.060f, 0.86f};
    Color topAccent{0.34f, 0.55f, 1.0f, 0.92f};
    drawPanel2D(leftX, panelMargin, leftW, height - panelMargin * 2, panelColor, topAccent);
    drawPanel2D(rightX, panelMargin, rightW, height - panelMargin * 2, panelColor, topAccent);
    if (showCenterMessage) {
        drawRoundedRect2D(centerX + 6, height - 64, centerW, 44, 10, Color{0.0f, 0.0f, 0.0f, 0.20f});
        drawRoundedRect2D(centerX, height - 58, centerW, 44, 10, Color{0.040f, 0.055f, 0.085f, 0.72f});
        drawRoundedOutline2D(centerX, height - 58, centerW, 44, 10, Color{0.15f, 0.28f, 0.48f, 0.46f});
        drawRoundedRect2D(centerX + 2, height - 18, centerW - 4, 3, 2, topAccent);
    }

    int x = leftX + panelPad;
    int maxLeftText = leftW - panelPad * 2;
    int leftContentW = maxLeftText - 14;

    glEnable(GL_SCISSOR_TEST);
    glScissor(leftX, panelMargin + 2, leftW, height - panelMargin * 2 - 4);

    int y = height - 48 + static_cast<int>(scene_.ui.leftHudScroll);
    int leftStartY = y;
    drawRoundedRect2D(x - 3, y - 17, leftContentW + 6, 32, 10, Color{0.050f, 0.070f, 0.105f, 0.56f});
    drawRoundedRect2D(x + 12, y - 3, 8, 8, 4, Color{0.98f, 0.26f, 0.21f, 1.0f});
    drawRoundedRect2D(x + 26, y - 3, 8, 8, 4, Color{0.15f, 0.62f, 1.0f, 1.0f});
    drawRoundedRect2D(x + 40, y - 3, 8, 8, 4, Color{0.26f, 0.84f, 0.42f, 1.0f});
    drawText2D(x + 62, centeredTextBaselineY(y - 17, 32, GLUT_BITMAP_HELVETICA_18),
        "3D Vector Visualizer", Color{0.88f, 0.93f, 1.0f, 1.0f}, GLUT_BITMAP_HELVETICA_18);
    y -= 37;
    drawPanelHeader(x, y, "Vector data", leftContentW);

    for (const auto& v : scene_.vectors) {
        int lineY = infoTextBaselineY(y);
        drawText2D(x, lineY, v.name + ":", v.color);
        if (v.complete()) {
            drawText2D(x + 24, lineY, fitText("v = " + vectorText(v.vec()) + ", k = " + formatDouble(v.scalarK), leftContentW - 24),
                Color{0.82f, 0.92f, 1.0f, 1.0f});
            advanceInfoLine(y);

            lineY = infoTextBaselineY(y);
            drawText2D(x + 24, lineY, fitText("start " + vectorText(v.start) + " -> end " + vectorText(v.end), leftContentW - 24),
                Color{0.82f, 0.86f, 0.84f, 1.0f});
            advanceInfoLine(y);
        }
        else {
            drawText2D(x + 24, lineY, "not entered", Color{0.96f, 0.64f, 0.52f, 1.0f});
            advanceInfoLine(y);
        }
        y -= 4;
    }

    y -= 14;
    drawPanelHeader(x, y, "Edit", leftContentW);
    const int columnGap = 8;
    const int smallButtonW = (leftContentW - columnGap * 2) / 3;
    const int largeButtonW = (leftContentW - columnGap) / 2;
    const int controlH = 28;
    const int opH = 26;
    drawKeyButton2D(x, y - controlH, smallButtonW, controlH, "1", "Set a", false);
    drawKeyButton2D(x + smallButtonW + columnGap, y - controlH, smallButtonW, controlH, "2", "Set b", false);
    drawKeyButton2D(x + (smallButtonW + columnGap) * 2, y - controlH, smallButtonW, controlH, "3", "Set c", false);
    y -= 38;
    drawKeyButton2D(x, y - controlH, leftContentW, controlH,
        "P", presets_.buttonLabel(scene_.ui.presetIndex), false);
    y -= 58;

    drawPanelHeader(x, y, "Active pair", leftContentW);
    auto pairSelected = [&](int first, int second) {
        return (scene_.activePairFirst == first && scene_.activePairSecond == second) ||
            (scene_.activePairFirst == second && scene_.activePairSecond == first);
    };
    drawKeyButton2D(x, y - controlH, smallButtonW, controlH, "4", "a & b", pairSelected(0, 1));
    drawKeyButton2D(x + smallButtonW + columnGap, y - controlH, smallButtonW, controlH, "5", "a & c", pairSelected(0, 2));
    drawKeyButton2D(x + (smallButtonW + columnGap) * 2, y - controlH, smallButtonW, controlH, "6", "b & c", pairSelected(1, 2));
    y -= 58;

    drawPanelHeader(x, y, "Operations", leftContentW);
    y -= 2;
    struct OperationHelp {
        OperationMode mode;
        const char* key;
        const char* label;
    };
    auto drawOperationGroup = [&](const char* title, const std::vector<OperationHelp>& items) {
        int rows = static_cast<int>((items.size() + 1) / 2);
        int groupHeight = 25 + rows * 29 + 12;
        drawRoundedRect2D(x - 3, y - groupHeight + 12, leftContentW + 6, groupHeight, 9,
            Color{0.045f, 0.058f, 0.080f, 0.36f});
        drawRoundedOutline2D(x - 3, y - groupHeight + 12, leftContentW + 6, groupHeight, 9,
            Color{0.13f, 0.22f, 0.34f, 0.58f});
        drawText2D(x, infoTextBaselineY(y), title, Color{0.93f, 0.70f, 0.34f, 1.0f}, GLUT_BITMAP_HELVETICA_12);
        advanceInfoLine(y);
        y -= 3;

        int groupTop = y;
        for (size_t i = 0; i < items.size(); ++i) {
            int col = static_cast<int>(i) / rows;
            int row = static_cast<int>(i) % rows;
            const auto& item = items[i];
            drawKeyButton2D(x + col * (largeButtonW + columnGap), groupTop - row * 29 - opH,
                largeButtonW, opH, item.key, item.label, item.mode == scene_.currentMode);
        }
        y = groupTop - rows * 29 - 18;
    };

    drawOperationGroup("Basic operations", {
        OperationHelp{OperationMode::Lengths, "F1", "Vector length"},
        OperationHelp{OperationMode::AddParallelogram, "F2", "Parallelogram add"},
        OperationHelp{OperationMode::AddTriangle, "F3", "Triangle addition"},
        OperationHelp{OperationMode::Subtract, "F4", "Vector subtraction"},
        OperationHelp{OperationMode::Scalar, "F5", "Scalar on vector"},
        OperationHelp{OperationMode::Collinearity, "F6", "Collinearity"},
        OperationHelp{OperationMode::Angle, "F7", "Angle between"}
        });
    drawOperationGroup("Products", {
        OperationHelp{OperationMode::Dot, "F8", "Dot product"},
        OperationHelp{OperationMode::Orthogonality, "O", "Orthogonality"},
        OperationHelp{OperationMode::Projection, "V", "Vector projection"},
        OperationHelp{OperationMode::Cross, "F9", "Cross product"},
        OperationHelp{OperationMode::Mixed, "F10", "Mixed product"}
        });
    drawOperationGroup("Geometry", {
        OperationHelp{OperationMode::SurfaceNormal, "F11", "Normal on plane"}
    });

    y -= 5;
    drawPanelHeader(x, y, "View", leftContentW);
    auto drawControlLine = [&](const std::string& key, const std::string& action) {
        int lineY = infoTextBaselineY(y);
        drawText2D(x, lineY, fitText(key, 58), Color{0.42f, 0.78f, 1.0f, 1.0f});
        drawText2D(x + 62, lineY, "- " + fitText(action, leftContentW - 62), Color{0.80f, 0.86f, 0.92f, 1.0f});
        advanceInfoLine(y);
    };
    int viewBlockTop = y + 8;
    int viewBlockH = 6 * kInfoLineH + 8;
    drawRoundedRect2D(x - 3, viewBlockTop - viewBlockH, leftContentW + 6, viewBlockH, 7,
        Color{0.045f, 0.060f, 0.088f, 0.36f});
    drawControlLine("Mouse", "orbit / pan / wheel zoom");
    drawControlLine("WASD", "move on ground plane");
    drawControlLine("Q / E", "move down / up");
    drawControlLine("Arrows", "orbit, Shift - pan");
    drawControlLine("PgUp/Dn", "zoom in / out");
    drawControlLine("G", "grid off / floor / full");

    int leftEndY = y;
    int leftContentHeight = leftStartY - leftEndY;
    int leftVisibleHeight = height - panelMargin * 2 - 8;
    scene_.ui.leftHudMaxScroll = std::max(0.0, static_cast<double>(leftContentHeight - leftVisibleHeight + 24));
    scene_.ui.leftHudScroll = std::clamp(scene_.ui.leftHudScroll, 0.0, scene_.ui.leftHudMaxScroll);
    glDisable(GL_SCISSOR_TEST);

    int trackY = panelMargin + 16;
    int trackH = height - panelMargin * 2 - 32;
    drawScrollBar2D(leftX + leftW - 8, trackY, trackH,
        scene_.ui.leftHudScroll, scene_.ui.leftHudMaxScroll, leftContentHeight);

    int rx = rightX + panelPad;
    int maxRightText = rightW - panelPad * 2;
    int rightContentW = maxRightText - 12;

    int ry = height - 58;
    if (!scene_.operationTitle.empty()) {
        drawWrappedText2D(rx, ry, rightContentW, scene_.operationTitle, Color{0.91f, 0.94f, 1.0f, 1.0f});
        ry -= 25;
    }
    drawWrappedText2D(rx, ry, rightContentW, scene_.formulaLine, Color{0.58f, 0.74f, 1.0f, 1.0f});
    ry -= 5;
    drawWrappedText2D(rx, ry, rightContentW, scene_.lessonLine, Color{0.72f, 0.78f, 0.88f, 1.0f});
    if (scene_.ui.presetIndex >= 0 && scene_.ui.presetIndex < presets_.count()) {
        ry -= 4;
        drawWrappedText2D(rx, ry, rightContentW,
            std::string("Graphics context: ") + presets_.at(scene_.ui.presetIndex).description,
            Color{0.68f, 0.92f, 0.86f, 1.0f});
    }
    ry -= 30;

    drawPanelHeader(rx, ry, "Computed results", rightContentW);
    if (scene_.resultLines.empty()) {
        int lineY = infoTextBaselineY(ry);
        drawText2D(rx, lineY, "No computed result.", Color{0.78f, 0.84f, 0.91f, 1.0f});
        advanceInfoLine(ry);
    }
    else {
        for (const auto& line : scene_.resultLines) {
            for (const auto& wrappedLine : wrapText(line, rightContentW)) {
                int lineY = infoTextBaselineY(ry);
                drawText2D(rx, lineY, wrappedLine, Color{0.84f, 0.88f, 0.94f, 1.0f});
                advanceInfoLine(ry);
            }
            ry -= 2;
        }
    }

    ry -= 12;
    drawPanelHeader(rx, ry, "Scene status", rightContentW);
    drawKeyValue(rx, ry, "Mode", operationName(scene_.currentMode),
        Color{0.48f, 0.76f, 1.0f, 1.0f}, Color{0.84f, 0.88f, 0.94f, 1.0f}, rightContentW);
    drawKeyValue(rx, ry, "Pair", pairLabel(" & "),
        Color{0.48f, 0.76f, 1.0f, 1.0f}, Color{0.84f, 0.88f, 0.94f, 1.0f}, rightContentW);
    drawKeyValue(rx, ry, "Vectors", std::to_string(scene_.completeVectorCount()) + " complete",
        Color{0.48f, 0.76f, 1.0f, 1.0f}, Color{0.84f, 0.88f, 0.94f, 1.0f}, rightContentW);
    drawKeyValue(rx, ry, "Preset", presets_.shortLabel(scene_.ui.presetIndex),
        Color{0.48f, 0.76f, 1.0f, 1.0f}, Color{0.84f, 0.88f, 0.94f, 1.0f}, rightContentW);
    if (scene_.ui.presetIndex >= 0 && scene_.ui.presetIndex < presets_.count()) {
        drawKeyValue(rx, ry, "Scene", presets_.at(scene_.ui.presetIndex).name,
            Color{0.48f, 0.76f, 1.0f, 1.0f}, Color{0.84f, 0.88f, 0.94f, 1.0f}, rightContentW);
    }
    drawKeyValue(rx, ry, "Grid", gridModeName(),
        Color{0.48f, 0.76f, 1.0f, 1.0f}, Color{0.84f, 0.88f, 0.94f, 1.0f}, rightContentW);
    drawKeyValue(rx, ry, "Axes", scene_.ui.showAxes ? "ON" : "OFF",
        Color{0.48f, 0.76f, 1.0f, 1.0f}, Color{0.84f, 0.88f, 0.94f, 1.0f}, rightContentW);
    drawKeyValue(rx, ry, "Labels", scene_.ui.showLabels ? "ON" : "OFF",
        Color{0.48f, 0.76f, 1.0f, 1.0f}, Color{0.84f, 0.88f, 0.94f, 1.0f}, rightContentW);
    drawKeyValue(rx, ry, "Yaw", std::to_string(displayAngle(camera_.yaw)),
        Color{0.48f, 0.76f, 1.0f, 1.0f}, Color{0.84f, 0.88f, 0.94f, 1.0f}, rightContentW);
    drawKeyValue(rx, ry, "Pitch", std::to_string(displayAngle(camera_.pitch)),
        Color{0.48f, 0.76f, 1.0f, 1.0f}, Color{0.84f, 0.88f, 0.94f, 1.0f}, rightContentW);
    drawKeyValue(rx, ry, "Distance", formatDouble(camera_.distance, 1),
        Color{0.48f, 0.76f, 1.0f, 1.0f}, Color{0.84f, 0.88f, 0.94f, 1.0f}, rightContentW);

    ry -= 28;
    drawPanelHeader(rx, ry, "Shortcuts", rightContentW);
    auto drawShortcutLine = [&](const std::string& key, const std::string& action) {
        int lineY = infoTextBaselineY(ry);
        drawText2D(rx, lineY, fitText(key, 76), Color{0.42f, 0.78f, 1.0f, 1.0f});
        drawText2D(rx + 80, lineY, "- " + fitText(action, rightContentW - 80), Color{0.80f, 0.86f, 0.92f, 1.0f});
        advanceInfoLine(ry);
    };
    int shortcutsBlockTop = ry + 8;
    int shortcutsBlockH = 7 * kInfoLineH + 8;
    drawRoundedRect2D(rx - 3, shortcutsBlockTop - shortcutsBlockH, rightContentW + 6, shortcutsBlockH, 7,
        Color{0.045f, 0.060f, 0.088f, 0.36f});
    drawShortcutLine("P", "next preset");
    drawShortcutLine("G / X / L", "grid mode / axes / labels");
    drawShortcutLine("R", "replay animation");
    drawShortcutLine("Home", "reset camera");
    drawShortcutLine("Ins / End", "clear result / all");
    drawShortcutLine("T", "math self-test");
    drawShortcutLine("H / Esc", "HUD / exit");

    int bx = centerX + 18;
    int by = height - 64;
    int msgH = 44;
    if (showCenterMessage) {
        drawText2D(bx, centeredTextBaselineY(by, msgH, GLUT_BITMAP_HELVETICA_12),
            fitText(scene_.ui.hudMessage, centerW - 36), Color{0.98f, 0.78f, 0.26f, 1.0f});
    }

    if (scene_.input.mode != InputMode::None && centerW >= 260) {
        std::string prompt;
        std::string hint;
        if (scene_.input.mode == InputMode::ScalarTarget || scene_.input.mode == InputMode::OperationVector) {
            prompt = "Choose vector";
            hint = "Type 1, 2, or 3, then press Enter. Esc - cancel.";
        }
        else if (scene_.input.mode == InputMode::OperationPair) {
            prompt = "Choose pair";
            hint = "Type ordered pair, e.g. 12 or 21, then press Enter. Esc - cancel.";
        }
        else if (scene_.input.mode == InputMode::Scalar) {
            prompt = "Vector " + scene_.vectors[scene_.input.activeVector].name + " scalar k";
            hint = "Type one number. Example: 1.5. Enter - apply, Esc - cancel.";
        }
        else {
            std::string pointName = scene_.input.mode == InputMode::VectorStart ? "start point" : "end point";
            prompt = "Vector " + scene_.vectors[scene_.input.activeVector].name + ": " + pointName;
            hint = "Type coordinates as x y z. Example: 3 -1 2.5. Enter - confirm, Esc - cancel.";
        }
        int inputY = 22;
        int inputH = 78;
        int labelW = std::min(190, std::max(148, centerW / 4));
        int fieldY = inputY + 25;
        int fieldH = 42;
        drawRoundedRect2D(centerX + 5, inputY - 5, centerW, inputH, 12, Color{0.0f, 0.0f, 0.0f, 0.28f});
        drawRoundedRect2D(centerX, inputY, centerW, inputH, 12, Color{0.050f, 0.064f, 0.088f, 0.97f});
        drawRoundedOutline2D(centerX, inputY, centerW, inputH, 12, Color{0.30f, 0.42f, 0.62f, 0.96f});
        drawRoundedRect2D(centerX + 10, fieldY, labelW, fieldH, 9, Color{0.070f, 0.090f, 0.125f, 0.96f});
        drawRoundedRect2D(centerX + labelW + 18, fieldY, centerW - labelW - 28, fieldH, 9,
            Color{0.030f, 0.040f, 0.058f, 0.96f});
        std::string promptText = fitText(prompt, labelW - 18);
        drawText2D(centerX + 10 + std::max(8, (labelW - textWidth(promptText)) / 2),
            centeredTextBaselineY(fieldY, fieldH, GLUT_BITMAP_HELVETICA_12),
            promptText, Color{0.62f, 0.82f, 1.0f, 1.0f});
        drawText2D(centerX + labelW + 32, centeredTextBaselineY(fieldY, fieldH, GLUT_BITMAP_HELVETICA_12),
            fitText(scene_.input.buffer + "_", centerW - labelW - 50), Color{0.94f, 0.97f, 1.0f, 1.0f});
        drawText2D(centerX + 18, inputY + 10, fitText(hint, centerW - 36, GLUT_BITMAP_HELVETICA_10),
            Color{0.67f, 0.74f, 0.84f, 1.0f}, GLUT_BITMAP_HELVETICA_10);
    }

    glPopAttrib();

    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}
