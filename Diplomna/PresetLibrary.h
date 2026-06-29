#pragma once

#include "SceneTypes.h"

#include <array>
#include <string>
#include <vector>

struct PresetVectorData {
    Vec3 start;
    Vec3 end;
};

struct PresetData {
    std::string name;
    std::string description;
    std::array<PresetVectorData, 3> vectorData;
    double scalar = 1.0;
    int pairFirst = 0;
    int pairSecond = 1;
    OperationMode mode = OperationMode::None;
};

// Stores reusable vector-scene presets separately from GLUT/UI code.
class PresetLibrary {
public:
    static const PresetLibrary& defaults();

    bool empty() const;
    int count() const;
    int normalizeIndex(int index) const;
    const PresetData& at(int index) const;
    const std::vector<PresetData>& all() const;

    std::string shortLabel(int currentIndex) const;
    std::string buttonLabel(int currentIndex) const;

private:
    explicit PresetLibrary(std::vector<PresetData> presets);

    std::vector<PresetData> presets_;
};
