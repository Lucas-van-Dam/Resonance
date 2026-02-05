#pragma once

#include <glm/glm.hpp>

#include <REON/Rendering/Structs/Texture.h>
#include "REON/GameHierarchy/Components/Component.h"

namespace REON {
	enum class EmitterShape {
		Point, Sphere, Box, Cone, Mesh
	};

	class ParticleEmitter : public ComponentBase<ParticleEmitter> {
	public:
		ParticleEmitter();
		~ParticleEmitter() override;

		//void Initialize() override;
		void Update(float deltaTime) override;
		void cleanup() override;
		void OnGameObjectAddedToScene() override;
		void OnComponentDetach() override;
		void Emit(uint32_t count);
		void Reset();
		nlohmann::ordered_json Serialize() const override;
		void Deserialize(const nlohmann::ordered_json& json, std::filesystem::path basePath) override;

		EmitterShape shape = EmitterShape::Point;
		uint32_t maxParticles = 1000;
		float emissionRate = 100.0f; // particles per second
		float particleLifeTime = 5.0f; // seconds

		glm::vec3 direction = glm::vec3(0.0f, 1.0f, 0.0f);
		float spreadAngle = 30.0f; // degrees
		float speedMin = 1.0f;
		float speedMax = 5.0f;

		glm::vec3 gravity = glm::vec3(0.0f, -9.81f, 0.0f);

		glm::vec4 startColor = glm::vec4(1.0f);
		glm::vec4 endColor = glm::vec4(1.0f, 1.0f, 1.0f, 0.0f);

		float startSize = 1.0f;
		float endSize = 0.1f;

		bool loop = true;
		bool playing = true;

		std::shared_ptr<Texture> particleTexture;

	private:
		float m_EmissionAccumulator = 0.0f;

		struct GPUResources {
			VkBuffer particleBuffer;
			VmaAllocation particleAllocation;

			VkBuffer aliveListBuffer;
			VmaAllocation aliveListAllocation;

			VkBuffer deadListBuffer;
			VmaAllocation deadListAllocation;

			VkBuffer indirectDrawBuffer;
			VmaAllocation indirectDrawAllocation;

			VkBuffer EmitterPropertiesBuffer;
			VmaAllocation EmitterPropertiesAllocation;
			void* emitterPropertiesMapped;

			VkDescriptorSet computeDescriptorSet;
			VkDescriptorSet renderDescriptorSet;
		};

		std::unique_ptr<GPUResources> m_GPUResources;

		friend class ParticleSystemManager;
	};
}