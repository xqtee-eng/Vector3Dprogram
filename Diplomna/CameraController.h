#pragma once

#include "SceneTypes.h"

// Camera state is separated from vector algebra state because camera movement is only presentation logic.
class CameraController {
public:
    double yaw = -42.0;
    double pitch = 24.0;
    double distance = 18.0;
    Vec3 target = Vec3(0, 0, 0);

    double targetYaw = -42.0;
    double targetPitch = 24.0;
    double targetDistance = 18.0;
    Vec3 targetPoint = Vec3(0, 0, 0);

    Vec3 eye() const;
    Vec3 forward() const;
    Vec3 right() const;
    void reset();
    void orbit(double yawDelta, double pitchDelta);
    void dolly(double factor);
    void pan(double rightAmount, double upAmount, double forwardAmount = 0.0);
    bool update(double factor);
};
