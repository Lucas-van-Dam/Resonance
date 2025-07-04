#pragma once
#include "Component.h"
#include <glm/ext/matrix_float4x4.hpp>

namespace REON {
	class [[clang::annotate("serialize")]] Camera : public ComponentBase<Camera>, public std::enable_shared_from_this<Camera>
	{
	public:
		Camera() {}
		void Update(float deltaTime) override;
		void cleanup() override;
		void OnGameObjectAddedToScene() override;
		void OnComponentDetach() override;

		virtual glm::mat4 GetViewMatrix() const { return m_ViewMatrix; }
		virtual glm::mat4 GetProjectionMatrix() const { return m_ProjectionMatrix; }

		nlohmann::ordered_json Serialize() const override;
		void Deserialize(const nlohmann::ordered_json& json, std::filesystem::path basePath) override;

	public:
		float fov = 60.0f;
		float nearPlane = 0.1f;
		float farPlane = 100.0f;

		bool primary = false;

		glm::uvec2 viewportSize = { 800, 600 };

	private:
		glm::mat4 m_ProjectionMatrix;
		glm::mat4 m_ViewMatrix;
	};

}