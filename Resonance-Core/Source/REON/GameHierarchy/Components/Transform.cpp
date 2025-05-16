#include "reonpch.h"
#include "Transform.h"

#include "glm/glm.hpp"
#include "REON/GameHierarchy/GameObject.h"
#include <glm/gtx/quaternion.hpp>

namespace REON {

    void Transform::Update(float deltaTime) {

    }

    // Set the world transform based on the provided matrix
    void Transform::SetWorldTransform(const glm::mat4& worldTransform) {
        // Calculate the local transform from the world transform
        glm::mat4 parentWorldTransform = GetOwner()->GetParent() ? GetOwner()->GetParent()->GetTransform()->GetWorldTransform() : glm::mat4(1.0f);
        glm::mat4 localTransform = glm::inverse(parentWorldTransform) * worldTransform;

        // Extract translation
        localPosition = glm::vec3(localTransform[3]); // Assuming 4th column contains translation

        // Extract scale
        localScale = glm::vec3(
            glm::length(localTransform[0]),
            glm::length(localTransform[1]),
            glm::length(localTransform[2])
        );

        const float minScale = 0.001f;
        localScale = glm::max(localScale, glm::vec3(minScale));

        // Remove scale from localTransform to isolate rotation
        localTransform[0] /= localScale.x;
        localTransform[1] /= localScale.y;
        localTransform[2] /= localScale.z;

        // Extract rotation using the upper-left 3x3 part
        glm::mat3 rotationMatrix = glm::mat3(localTransform);
        glm::quat quat = glm::normalize(glm::quat_cast(rotationMatrix)); // Convert to quaternion and normalize

        localRotation.x = quat.x;
        localRotation.y = quat.y;
        localRotation.z = quat.z;
        localRotation.w = quat.w;
        // Optionally, update the local matrix
        //UpdateLocalMatrix();
    }

    void Transform::SetFromMatrix(const std::vector<float>& matrixData)
    {
        DecomposeMatrix(matrixData, localPosition, localRotation, localScale);
    }

    void Transform::UpdateLocalMatrix() {
        glm::mat4 translation = glm::translate(glm::mat4(1.0f), localPosition);
        glm::mat4 rotation = glm::toMat4(localRotation);
        glm::mat4 scale = glm::scale(glm::mat4(1.0f), localScale);

        m_LocalMatrix = translation * rotation * scale;
    }

    void Transform::cleanup()
    {
    }

    void Transform::OnGameObjectAddedToScene()
    {
    }

    void Transform::OnComponentDetach()
    {
    }

    void Transform::DecomposeMatrix(const std::vector<float>& matData, glm::vec3& position, glm::quat& rotation, glm::vec3& scale) {
        // Convert vector to glm::mat4
        glm::mat4 mat = glm::make_mat4(matData.data());

        // Extract translation (position)
        position = glm::vec3(mat[3]);

        // Extract scale (length of each basis vector)
        scale = glm::vec3(
            glm::length(glm::vec3(mat[0])),
            glm::length(glm::vec3(mat[1])),
            glm::length(glm::vec3(mat[2]))
        );

        // Normalize basis vectors to extract rotation
        glm::mat3 rotationMatrix = glm::mat3(mat);
        rotationMatrix[0] /= scale.x;
        rotationMatrix[1] /= scale.y;
        rotationMatrix[2] /= scale.z;

        // Convert rotation matrix to quaternion
        rotation = glm::quat_cast(rotationMatrix);
    }

    Transform::~Transform()
    {
    }

    glm::mat4 Transform::GetTransformationMatrix() const {
        glm::mat4 translation = glm::translate(glm::mat4(1.0f), localPosition);
        glm::mat4 rotation = glm::toMat4(localRotation);
        glm::mat4 scale = glm::scale(glm::mat4(1.0f), localScale);
        return translation * rotation * scale;
    }

    glm::mat4 Transform::GetWorldTransform() const {
        glm::mat4 translation = glm::translate(glm::mat4(1.0f), localPosition);
        glm::mat4 rotation = glm::toMat4(localRotation); // Convert quaternion to matrix
        glm::mat4 scale = glm::scale(glm::mat4(1.0f), localScale);

        return (GetOwner()->GetParent() ? GetOwner()->GetParent()->GetTransform()->GetWorldTransform() : glm::mat4(1.0f)) * translation * rotation * scale;
    }

    glm::vec3 Transform::GetForwardVector() const {
        // Assuming your rotation quaternion is normalized
        glm::vec3 forward = glm::mat3_cast(glm::normalize(localRotation)) * glm::vec3(0.0f, 0.0f, -1.0f);
        return glm::normalize(forward);
    }

    glm::vec3 Transform::GetWorldPosition() const {
        glm::mat4 worldTransform = GetWorldTransform();
        return glm::vec3(worldTransform[3]);  // Extract translation from the world transform
    }

    Quaternion Transform::GetWorldRotation() const {
        Quaternion worldRotation = localRotation;
        std::shared_ptr<GameObject> parent = GetOwner()->GetParent();
        while (parent != nullptr) {
            worldRotation *= parent->GetTransform()->localRotation;
            parent = parent->GetParent();
        }
        return worldRotation;
    }

    glm::vec3 Transform::GetWorldScale() const {
        glm::vec3 worldScale = localScale;

        std::shared_ptr<GameObject> parent = GetOwner()->GetParent();
        while (parent != nullptr) {
            worldScale *= parent->GetTransform()->localScale;
            parent = parent->GetParent();
        }

        return worldScale;
    }

}