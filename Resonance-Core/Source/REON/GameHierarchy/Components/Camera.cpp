#include "reonpch.h"

#include "Camera.h"

#include "Transform.h"

#include <REON/GameHierarchy/GameObject.h>
#include <REON/GameHierarchy/SceneManager.h>
#include <REON/Math/Quaternion.h>
#include <glm/ext/matrix_transform.hpp>

namespace REON
{
void Camera::update(float deltaTime)
{
    glm::vec3 position = get_owner()->GetTransform()->GetWorldPosition();
    Quaternion rotation = get_owner()->GetTransform()->GetWorldRotation();

    glm::vec3 front = rotation * glm::vec3(0, 0, -1);
    glm::vec3 up = rotation * glm::vec3(0, 1, 0);
    m_ViewMatrix = glm::lookAtRH(position, position + front, up);
    m_ProjectionMatrix =
        glm::perspectiveRH(glm::radians(fov), (float)viewportSize.x / (float)viewportSize.y, nearPlane, farPlane);
}

void Camera::cleanup() {}

void Camera::on_game_object_added_to_scene()
{
    get_owner()->GetScene()->cameras.push_back(shared_from_this());
}

void Camera::on_component_detach() {}

} // namespace REON