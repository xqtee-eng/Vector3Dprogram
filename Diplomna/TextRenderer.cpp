#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <GL/glut.h>

#include "TextRenderer.h"

static void setColor(const Color& c) {
    glColor4f(c.r, c.g, c.b, c.a);
}

TextRenderer::TextRenderer(SceneModel& scene)
    : scene_(scene) {
}

bool TextRenderer::createFont(Font& out, const char* face, int pixelHeight, int weight) {
    HDC hdc = wglGetCurrentDC();
    if (!hdc) return false;

    out.base = glGenLists(96);
    if (out.base == 0) return false;

    HFONT font = CreateFontA(
        -pixelHeight, 0, 0, 0, weight, FALSE, FALSE, FALSE,
        ANSI_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, FF_DONTCARE | DEFAULT_PITCH, face);

    if (!font) return false;

    out.handle = font;
    HFONT oldFont = static_cast<HFONT>(SelectObject(hdc, font));
    out.ready = wglUseFontBitmapsA(hdc, 32, 96, out.base) == TRUE;
    SelectObject(hdc, oldFont);
    return out.ready;
}

void TextRenderer::initFonts() {
    createFont(titleFont_, "Segoe UI Semibold", 19, FW_SEMIBOLD);
}

void TextRenderer::draw3D(const Vec3& p, const std::string& text, const Color& color) const {
    if (!scene_.ui.showLabels) return;

    setColor(color);
    glRasterPos3d(p.x, p.y, p.z);
    if (titleFont_.ready) {
        std::string printable = text;
        for (char& ch : printable) {
            unsigned char c = static_cast<unsigned char>(ch);
            if (c < 32 || c > 127) ch = '?';
        }
        glPushAttrib(GL_LIST_BIT);
        glListBase(titleFont_.base - 32);
        glCallLists(static_cast<GLsizei>(printable.size()), GL_UNSIGNED_BYTE, printable.c_str());
        glPopAttrib();
    }
    else {
        for (unsigned char ch : text) {
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, ch);
        }
    }
}
