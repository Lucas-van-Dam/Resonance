#pragma once

#include "Event.h"
#include <memory>
#include <string>
#include <sstream>

namespace REON {
	class Material;
	class Renderer;

	class  MaterialChangedEvent : public Event {
	public:
		MaterialChangedEvent(std::shared_ptr<Material> mat, std::shared_ptr<Material> oldMat, std::shared_ptr<Renderer> ren)
			: material(mat), renderer(ren), oldMaterial(oldMat) {
		}

		inline std::shared_ptr<Material> GetMaterial() const { if (auto lockedMat = material.lock()) return lockedMat; }
		inline std::shared_ptr<Material> GetOldMaterial() const { if (auto lockedMat = oldMaterial.lock()) return lockedMat; }
		inline std::shared_ptr<Renderer> GetRenderer() const { if(auto lockedRen = renderer.lock()) return lockedRen; }

		std::string ToString() const override {
			std::stringstream ss;
			ss << "MaterialChangedEvent";
			return ss.str();
		}

		EVENT_CLASS_NAME(MaterialChanged)

	private:
		std::weak_ptr<Renderer> renderer;
		std::weak_ptr<Material> material;
		std::weak_ptr<Material> oldMaterial;
	};
}