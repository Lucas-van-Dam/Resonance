#include "reonpch.h"
#include "Quaternion.h"

namespace REON {

    glm::vec3 Quaternion::getEulerAngles() const {
        // Convert quaternion to matrix
        glm::mat3 m = glm::mat3_cast(glm::quat(w, x, y, z));

        float eulerX, eulerY, eulerZ;

        // y = yaw (Y), x = pitch (X), z = roll (Z)
        eulerY = std::asin(glm::clamp(-m[0][2], -1.0f, 1.0f));

        if (std::abs(m[0][2]) < 0.999f) {
            eulerX = std::atan2(m[1][2], m[2][2]); // pitch
            eulerZ = std::atan2(m[0][1], m[0][0]); // roll
        }
        else {
            // Gimbal lock
            eulerX = std::atan2(-m[2][1], m[1][1]);
            eulerZ = 0.0f;
        }

        return glm::vec3(eulerX, eulerY, eulerZ); // pitch, yaw, roll
    }

    // roll, pitch and yaw in degrees
    void Quaternion::setFromEulerAngles(float roll, float pitch, float yaw) {
        // roll  = Z
        // pitch = X
        // yaw   = Y

        float halfX = glm::radians(pitch) * 0.5f; // X (pitch)
        float halfY = glm::radians(yaw) * 0.5f; // Y (yaw)
        float halfZ = glm::radians(roll) * 0.5f; // Z (roll)

        float cx = cos(halfX);
        float sx = sin(halfX);
        float cy = cos(halfY);
        float sy = sin(halfY);
        float cz = cos(halfZ);
        float sz = sin(halfZ);

        // Quaternion multiplication: q = qy * qx * qz
        this->w = cy * cx * cz + sy * sx * sz;
        this->x = sy * cx * cz - cy * sx * sz;
        this->y = cy * sx * cz + sy * cx * sz;
        this->z = cy * cx * sz - sy * sx * cz;
    }

    void Quaternion::setFromEulerAngles(glm::vec3 euler) {
        // roll  = Z
        // pitch = X
        // yaw   = Y

        float halfX = euler.x * 0.5f; // X (pitch)
        float halfY = euler.y * 0.5f; // Y (yaw)
        float halfZ = euler.z * 0.5f; // Z (roll)

        float cx = cos(halfX);
        float sx = sin(halfX);
        float cy = cos(halfY);
        float sy = sin(halfY);
        float cz = cos(halfZ);
        float sz = sin(halfZ);

        // Quaternion multiplication: q = qy * qx * qz
        this->w = cy * cx * cz + sy * sx * sz;
        this->x = sy * cx * cz - cy * sx * sz;
        this->y = cy * sx * cz + sy * cx * sz;
        this->z = cy * cx * sz - sy * sx * cz;
    }

}