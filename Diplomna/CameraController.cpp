#include "CameraController.h"

#include <algorithm>
#include <cmath>

double normalizeTurnAngle(double degrees) {
    double normalized = std::fmod(degrees, 360.0);
    if (std::fabs(normalized) < 1e-7) return 0.0;
    return normalized;
}

double approach(double current, double target, double factor) {
    return current + (target - current) * factor;
}

Vec3 approachVec(const Vec3& current, const Vec3& target, double factor) {
    return Vec3(
        approach(current.x, target.x, factor),
        approach(current.y, target.y, factor),
        approach(current.z, target.z, factor)
    );
}

// Converts the orbit camera state into an eye position.
Vec3 CameraController::eye() const {
    double yawRad = yaw * PI / 180.0;
    double pitchRad = std::clamp(pitch, -86.0, 86.0) * PI / 180.0;
    double cp = std::cos(pitchRad);
    return Vec3(
        target.x + distance * cp * std::sin(yawRad),
        target.y + distance * std::sin(pitchRad),
        target.z + distance * cp * std::cos(yawRad)
    );
}

// Returns normalized view direction from camera eye to target.
Vec3 CameraController::forward() const {
    return (target - eye()).normalized();
}

// Returns horizontal right vector used for panning and label offsets.
Vec3 CameraController::right() const {
    Vec3 rightVector = forward().cross(Vec3(0, 1, 0));
    if (rightVector.length() < EPS) return Vec3(1, 0, 0);
    return rightVector.normalized();
}

// Restores default camera target, yaw, pitch, and distance.
void CameraController::reset() {
    yaw = -42.0;
    pitch = 24.0;
    distance = 18.0;
    target = Vec3(0, 0, 0);
    targetYaw = yaw;
    targetPitch = pitch;
    targetDistance = distance;
    targetPoint = target;
}

// Updates target yaw/pitch for smooth orbit camera movement.
void CameraController::orbit(double yawDelta, double pitchDelta) {
    targetYaw += yawDelta;
    targetPitch = std::clamp(targetPitch + pitchDelta, -86.0, 86.0);

    double normalizedTargetYaw = normalizeTurnAngle(targetYaw);
    double wrapOffset = normalizedTargetYaw - targetYaw;
    if (std::fabs(wrapOffset) > EPS) {
        targetYaw = normalizedTargetYaw;
        yaw = normalizeTurnAngle(yaw + wrapOffset);
    }
}

// Zooms camera by scaling the orbit distance.
void CameraController::dolly(double factor) {
    targetDistance = std::clamp(targetDistance * factor, 3.0, 80.0);
}

// Moves camera target in camera-relative right/up/forward directions.
void CameraController::pan(double rightAmount, double upAmount, double forwardAmount) {
    Vec3 rightVector = right();
    Vec3 forwardVector = forward();
    forwardVector.y = 0.0;
    if (forwardVector.length() < EPS) forwardVector = Vec3(0, 0, -1);
    forwardVector = forwardVector.normalized();

    targetPoint = targetPoint + rightVector * rightAmount + Vec3(0, 1, 0) * upAmount + forwardVector * forwardAmount;
}

// Advances camera smoothing toward target state; returns true while redraw is needed.
bool CameraController::update(double factor) {
    bool moving =
        std::fabs(yaw - targetYaw) > 0.01 ||
        std::fabs(pitch - targetPitch) > 0.01 ||
        std::fabs(distance - targetDistance) > 0.01 ||
        (target - targetPoint).length() > 0.01;

    if (moving) {
        yaw = approach(yaw, targetYaw, factor);
        pitch = approach(pitch, targetPitch, factor);
        distance = approach(distance, targetDistance, factor);
        target = approachVec(target, targetPoint, factor);
        return true;
    }

    yaw = targetYaw;
    pitch = targetPitch;
    distance = targetDistance;
    target = targetPoint;
    return false;
}
