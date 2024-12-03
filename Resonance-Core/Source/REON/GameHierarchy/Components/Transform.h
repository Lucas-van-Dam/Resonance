#pragma once

#include "glm/glm.hpp"
#include "REON/Math/Quaternion.h"
#include "REON/GameHierarchy/Components/Component.h"

class GameObject;

namespace REON {

    class Transform : public ComponentBase<Transform> {
    public:
        Transform()
            : localPosition(glm::vec3(0.0f)), localRotation(1.0f, 0.0f, 0.0f, 0.0f), localScale(glm::vec3(1.0f)), m_LocalMatrix(1.0f) {}

        ~Transform();

        // Get the transformation matrix
        glm::mat4 GetTransformationMatrix() const;

        glm::mat4 GetWorldTransform() const;

        glm::vec3 GetForwardVector() const;

        glm::vec3 GetWorldPosition() const;
        Quaternion GetWorldRotation() const;
        glm::vec3 GetWorldScale() const;

        void Update(float deltaTime) override;

        void SetWorldTransform(const glm::mat4& matrix);

        void UpdateLocalMatrix();

    public:
        glm::vec3 localPosition; // Position (translation)
        Quaternion localRotation; // Quaternion for rotation
        glm::vec3 localScale;    // Scale factors (x, y, z)

    private:
        // Create the rotation matrix from a quaternion
        static glm::mat4 CreateRotationMatrix(const Quaternion& quat) {
            return glm::mat4_cast(quat);
        }

        // Inherited via Component
        void OnGameObjectAddedToScene() override;
        void OnComponentDetach() override;

    private:
        glm::mat4 m_LocalMatrix;
    };

}

