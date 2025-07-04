#include "reonpch.h"
#include "Camera.h"
#include <glm/ext/matrix_transform.hpp>
#include <REON/GameHierarchy/GameObject.h>
#include "Transform.h"
#include <REON/GameHierarchy/SceneManager.h>

namespace REON {
	void Camera::Update(float deltaTime)
	{
		glm::vec3 position = GetOwner()->GetTransform()->GetWorldPosition();
		Quaternion rotation = GetOwner()->GetTransform()->GetWorldRotation();

		glm::vec3 front = rotation * glm::vec3(0, 0, -1);
		glm::vec3 up = rotation * glm::vec3(0, 1, 0);
		m_ViewMatrix = glm::lookAt(position, position + front, up);
		m_ProjectionMatrix = glm::perspective(glm::radians(fov),
			(float)viewportSize.x /
			(float)viewportSize.y,
			nearPlane,
			farPlane);
	}

	void Camera::cleanup()
	{
	}

	void Camera::OnGameObjectAddedToScene()
	{
		GetOwner()->GetScene()->cameras.push_back(shared_from_this());
	}

	void Camera::OnComponentDetach()
	{
	}

	nlohmann::ordered_json Camera::Serialize() const
	{
		nlohmann::ordered_json json;

		json["Type"] = GetTypeName();
		json["fov"] = fov;
		json["nearPlane"] = nearPlane;
		json["farPlane"] = farPlane;
		json["primary"] = primary;

		return json;
	}
	void Camera::Deserialize(const nlohmann::ordered_json& json, std::filesystem::path basePath)
	{
		if (json.contains("fov")) {
			fov = json["fov"];
		}
		if (json.contains("nearPlane")) {
			nearPlane = json["nearPlane"];
		}
		if (json.contains("farPlane")) {
			farPlane = json["farPlane"];
		}
		if (json.contains("primary")) {
			primary = json["primary"];
		}
	}
}