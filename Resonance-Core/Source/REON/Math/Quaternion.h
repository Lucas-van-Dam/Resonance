#pragma once

//#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"

namespace REON {

    class Quaternion : public glm::quat {
    public:
        Quaternion() : glm::quat() {}
        Quaternion(float w, float x, float y, float z) : glm::quat(w, x, y, z) {}
        // Set the quaternion using Euler angles (in radians)
        void setFromEulerAngles(float roll, float pitch, float yaw);
        void setFromEulerAngles(glm::vec3 euler);

        /// <summary>
        /// Get euler angles from quaternion
        /// </summary>
        /// <returns>Euler angles in Radians</returns>
        glm::vec3 getEulerAngles() const;

        Quaternion& operator*=(const glm::quat other) {
            glm::quat::operator*=(other);
            return *this;
        }

        Quaternion& operator=(const glm::quat& other) {
            this->x = other.x;
            this->y = other.y;
            this->z = other.z;
            this->w = other.w;
            return *this;
        }

        void Normalize() {
            float length = std::sqrt(x * x + y * y + z * z + w * w);
            if (length > 0.0f) {
                float invLength = 1.0f / length;
                x *= invLength;
                y *= invLength;
                z *= invLength;
                w *= invLength;
            }
        }
    };

}

