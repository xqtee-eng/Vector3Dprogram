#include "PresetLibrary.h"

#include <stdexcept>
#include <utility>

std::vector<PresetData> createDefaultPresets() {
    return {
        PresetData{
            "3D scene vectors",
            "Base object-space directions for addition, products, and scene construction.",
            {
                PresetVectorData{Vec3(0, 0, 0), Vec3(4, 2, 1)},
                PresetVectorData{Vec3(0, 0, 0), Vec3(1, 4, 3)},
                PresetVectorData{Vec3(0, 0, 0), Vec3(2, -1, 4)}
            },
            1.5, 0, 1, OperationMode::AddParallelogram
        },
        PresetData{
            "Direction alignment",
            "Collinearity check for aligned edges, paths, or object directions.",
            {
                PresetVectorData{Vec3(0, 0, 0), Vec3(3, 1, 2)},
                PresetVectorData{Vec3(0, 0, 0), Vec3(6, 2, 4)},
                PresetVectorData{Vec3(0, 0, 0), Vec3(-2, 3, 1)}
            },
            0.5, 0, 1, OperationMode::Collinearity
        },
        PresetData{
            "Reversed direction",
            "Detects opposite directions, useful for path orientation and flipped normals.",
            {
                PresetVectorData{Vec3(-1, 0, 1), Vec3(4, 0, 1)},
                PresetVectorData{Vec3(1, 2, -1), Vec3(-4, 2, -1)},
                PresetVectorData{Vec3(0, 0, 0), Vec3(0, 3, 3)}
            },
            -1.0, 0, 1, OperationMode::Collinearity
        },
        PresetData{
            "Local basis volume",
            "Orthogonal basis vectors model a local coordinate frame and its volume.",
            {
                PresetVectorData{Vec3(0, 0, 0), Vec3(3, 0, 0)},
                PresetVectorData{Vec3(0, 0, 0), Vec3(0, 3, 0)},
                PresetVectorData{Vec3(0, 0, 0), Vec3(0, 0, 3)}
            },
            2.0, 0, 1, OperationMode::Mixed
        },
        PresetData{
            "Translated motion",
            "Parallel vector transfer shows the same displacement at another object position.",
            {
                PresetVectorData{Vec3(1, 1, 0), Vec3(5, 3, 2)},
                PresetVectorData{Vec3(-2, 0, 1), Vec3(1, 4, 3)},
                PresetVectorData{Vec3(0, -1, 0), Vec3(2, 2, 5)}
            },
            2.5, 0, 1, OperationMode::AddTriangle
        },
        PresetData{
            "View angle",
            "Angle between direction vectors for view, aim, or orientation analysis.",
            {
                PresetVectorData{Vec3(0, 0, 0), Vec3(4, 1, 0)},
                PresetVectorData{Vec3(0, 0, 0), Vec3(2, 3, 1)},
                PresetVectorData{Vec3(0, 0, 0), Vec3(-1, 2, 4)}
            },
            1.25, 0, 1, OperationMode::Angle
        },
        PresetData{
            "Dot visibility",
            "Dot product sign separates front-facing and back-facing directions.",
            {
                PresetVectorData{Vec3(0, 0, 0), Vec3(4, 0, 1)},
                PresetVectorData{Vec3(0, 0, 0), Vec3(-2, 1, 0)},
                PresetVectorData{Vec3(0, 0, 0), Vec3(1, 3, -2)}
            },
            1.0, 0, 1, OperationMode::Dot
        },
        PresetData{
            "Polygon normal",
            "Cross product of two surface edges builds a perpendicular normal vector.",
            {
                PresetVectorData{Vec3(0, 0, 0), Vec3(4, 1, 0)},
                PresetVectorData{Vec3(0, 0, 0), Vec3(1, 0, 4)},
                PresetVectorData{Vec3(0, 0, 0), Vec3(2, 3, -1)}
            },
            1.75, 0, 1, OperationMode::Cross
        },
        PresetData{
            "Plane projection",
            "Vector c is decomposed into surface-plane and normal components.",
            {
                PresetVectorData{Vec3(0, 0, 0), Vec3(4, 0, 0)},
                PresetVectorData{Vec3(0, 0, 0), Vec3(0, 0, 4)},
                PresetVectorData{Vec3(0, 0, 0), Vec3(2, 3, 2)}
            },
            1.5, 0, 1, OperationMode::SurfaceNormal
        },
        PresetData{
            "3D volume test",
            "Mixed product measures parallelepiped volume and spatial orientation.",
            {
                PresetVectorData{Vec3(0, 0, 0), Vec3(3, 0, 1)},
                PresetVectorData{Vec3(0, 0, 0), Vec3(1, 3, 0)},
                PresetVectorData{Vec3(0, 0, 0), Vec3(1, 1, 4)}
            },
            1.5, 0, 1, OperationMode::Mixed
        },
        PresetData{
            "Scale transform",
            "Scalar multiplication models stretching and direction reversal.",
            {
                PresetVectorData{Vec3(0, 0, 0), Vec3(2, 1, 0)},
                PresetVectorData{Vec3(1, -1, 0), Vec3(3, 0, 2)},
                PresetVectorData{Vec3(0, 0, 0), Vec3(-2, 1, 3)}
            },
            -1.5, 0, 1, OperationMode::Scalar
        },
        PresetData{
            "Displacement delta",
            "Subtraction computes a difference vector between two directions.",
            {
                PresetVectorData{Vec3(0, 0, 0), Vec3(5, 2, 1)},
                PresetVectorData{Vec3(0, 0, 0), Vec3(1, 4, 2)},
                PresetVectorData{Vec3(0, 0, 0), Vec3(2, -2, 3)}
            },
            1.5, 0, 1, OperationMode::Subtract
        }
    };
}

PresetLibrary::PresetLibrary(std::vector<PresetData> presets)
    : presets_(std::move(presets)) {
}

const PresetLibrary& PresetLibrary::defaults() {
    static const PresetLibrary library(createDefaultPresets());
    return library;
}

bool PresetLibrary::empty() const {
    return presets_.empty();
}

int PresetLibrary::count() const {
    return static_cast<int>(presets_.size());
}

int PresetLibrary::normalizeIndex(int index) const {
    if (presets_.empty()) return -1;
    int normalized = index % count();
    if (normalized < 0) normalized += count();
    return normalized;
}

const PresetData& PresetLibrary::at(int index) const {
    if (index < 0 || index >= count()) {
        throw std::out_of_range("Preset index is out of range.");
    }
    return presets_[index];
}

const std::vector<PresetData>& PresetLibrary::all() const {
    return presets_;
}

std::string PresetLibrary::shortLabel(int currentIndex) const {
    if (currentIndex < 0 || currentIndex >= count()) return "--";
    return std::to_string(currentIndex + 1) + "/" + std::to_string(count()) + " " + presets_[currentIndex].name;
}

std::string PresetLibrary::buttonLabel(int currentIndex) const {
    if (currentIndex < 0 || currentIndex >= count()) return "Preset";
    return "Preset " + std::to_string(currentIndex + 1) + "/" + std::to_string(count());
}
