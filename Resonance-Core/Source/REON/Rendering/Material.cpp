#include "reonpch.h"
#include "Material.h"
#include "REON/ResourceManagement/ResourceManager.h"

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

	void Material::Load()
	{

	}

	void Material::Unload()
	{
		shader.reset();
	}

	std::filesystem::path Material::Serialize(std::filesystem::path path) {
		nlohmann::ordered_json json;
		json["GUID"] = GetID();

		json["Albedo"] = { albedoColor.r, albedoColor.g, albedoColor.b, albedoColor.a };
		if (albedoTexture)
			json["AlbedoTexture"] = albedoTexture->GetID();
		if (roughnessTexture)
			json["RoughnessTexture"] = roughnessTexture->GetID();

		json["Roughness"] = roughness;
		json["Metallic"] = metallic;

		if (normalTexture)
			json["NormalTexture"] = normalTexture->GetID();

		if (shader)
			json["Shader"] = shader->GetID();

		std::string writePath = path.string() + "\\" + GetName() + ".material";

		std::ofstream file(writePath);
		if (file.is_open())
		{
			file << json.dump(4); // Pretty print with an indent of 4 spaces.
			file.close();
		}
		else
		{
			REON_ERROR("Failed to open material file for writing: {}", writePath);
		}

		return writePath;
	}

	void Material::Deserialize(std::filesystem::path path) {
		std::ifstream file(path);
		if (!file.is_open()) {
			REON_ERROR("Failed to open material file for reading: {}", path.string());
			return;
		}

		nlohmann::ordered_json json;
		file >> json;
		file.close();

		// Extract UID
		if (json.contains("UID")) {
			m_ID = (json["UID"].get<std::string>());
		}

		// Extract Albedo Color
		if (json.contains("Albedo")) {
			auto albedoArray = json["Albedo"];
			albedoColor = glm::vec4(albedoArray[0], albedoArray[1], albedoArray[2], albedoArray[3]);
		}

		// Extract Albedo Texture
		if (json.contains("AlbedoTexture")) {
			std::string albedoTextureID = json["AlbedoTexture"].get<std::string>();
			//albedoTexture = LoadTextureByID(albedoTextureID); // Replace with your actual texture loading logic
		}

		// Extract Roughness and Roughness Texture
		if (json.contains("Roughness")) {
			roughness = json["Roughness"].get<float>();
		}
		if (json.contains("RoughnessTexture")) {
			std::string roughnessTextureID = json["RoughnessTexture"].get<std::string>();
			//roughnessTexture = LoadTextureByID(roughnessTextureID);
		}

		// Extract Metallic
		if (json.contains("Metallic")) {
			metallic = json["Metallic"].get<float>();
		}

		// Extract Normal Texture
		if (json.contains("NormalTexture")) {
			std::string normalTextureID = json["NormalTexture"].get<std::string>();
			//normalTexture = LoadTextureByID(normalTextureID);
		}

		// Extract Shader
		if (json.contains("Shader")) {
			std::string shaderID = json["Shader"].get<std::string>();
			shader = ResourceManager::GetInstance().LoadResource<Shader>("DefaultLit", std::make_tuple("PBR.vert", "PBR.frag", std::optional<std::string>{}));
		}
	}
}