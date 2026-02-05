#pragma once

#include <glm/glm.hpp>

namespace REON {

	// GPU Particle structure
	struct Particle {
		glm::vec3 position;
		float age;

		glm::vec3 velocity;
		float lifetime;

		glm::vec4 color;

		glm::vec3 acceleration;
		float size;

		glm::vec2 uvOffset;
		float rotation;
		float angularVelocity;
	};

	struct EmitterProperties {
		glm::vec3 position;
		uint32_t maxParticles;

		glm::vec3 direction;
		float emissionRate;

		glm::vec3 gravity;
		float particleLifeTime;

		glm::vec4 startColor;
		glm::vec4 endColor;

		float startSize;
		float endSize;
		float spreadAngle;
		float speedMin;

		float speedMax;
		float deltaTime;
		uint32_t emitCount;
		uint32_t totalAlive;
	};

	struct DrawIndirectCommand {
		uint32_t vertexCount;
		uint32_t instanceCount;
		uint32_t firstVertex;
		uint32_t firstInstance;
	};
}