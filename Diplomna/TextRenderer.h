#pragma once

#include "SceneTypes.h"

#include <string>

// Owns 3D label text drawing for the OpenGL scene.
class TextRenderer {
public:
    explicit TextRenderer(SceneModel& scene);

    void initFonts();
    void draw3D(const Vec3& p, const std::string& text, const Color& color = Color{1, 1, 1, 1}) const;

private:
    struct Font {
        void* handle = nullptr;
        unsigned int base = 0;
        bool ready = false;
    };

    SceneModel& scene_;
    Font titleFont_;

    bool createFont(Font& out, const char* face, int pixelHeight, int weight);
};
