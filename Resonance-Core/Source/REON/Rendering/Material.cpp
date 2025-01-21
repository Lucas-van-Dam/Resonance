#include "reonpch.h"
#include "Material.h"

namespace REON {

	Material::Material(std::shared_ptr<Shader> shader) : shader(std::move(shader)) {

	}

	Material::Material() {

	}

	void Material::Load(const std::string& name, std::any metadata)
	{
		auto shader = std::any_cast<std::shared_ptr<Shader>>(metadata);
		this->shader = std::move(shader);
		m_Path = name;
	}

	void Material::Unload()
	{
		shader.reset();
	}

}