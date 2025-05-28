#include "reonpch.h"
#include "Material.h"
#include "REON/ResourceManagement/ResourceManager.h"
#include <REON/Platform/Vulkan/VulkanContext.h>
#include "REON/Application.h"

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
		const VulkanContext* context = static_cast<const VulkanContext*>(Application::Get().GetRenderContext());

		VkDeviceSize bufferSize = sizeof(FlatData);

		flatDataBuffers.resize(context->MAX_FRAMES_IN_FLIGHT);
		m_FlatDataBufferAllocations.resize(context->MAX_FRAMES_IN_FLIGHT);
		flatDataBuffersMapped.resize(context->MAX_FRAMES_IN_FLIGHT);

		for (size_t i = 0; i < context->MAX_FRAMES_IN_FLIGHT; i++) {
			// TODO: use the VMA_ALLOCATION_CREATE_MAPPED_BIT along with VmaAllocationInfo object instead
			context->createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, flatDataBuffers[i], m_FlatDataBufferAllocations[i]);

			vmaMapMemory(context->getAllocator(), m_FlatDataBufferAllocations[i], &flatDataBuffersMapped[i]);
		}
	}

	void Material::Unload()
	{
		const VulkanContext* context = static_cast<const VulkanContext*>(Application::Get().GetRenderContext());

		for (size_t i = 0; i < context->MAX_FRAMES_IN_FLIGHT; i++) {
			vkDestroyBuffer(context->getDevice(), flatDataBuffers[i], nullptr);
			vmaUnmapMemory(context->getAllocator(), m_FlatDataBufferAllocations[i]);
			vmaFreeMemory(context->getAllocator(), m_FlatDataBufferAllocations[i]);
		}
	}

	std::filesystem::path Material::Serialize(std::filesystem::path path) {
		nlohmann::ordered_json json;
		json["GUID"] = GetID();
		json["Name"] = GetName();

		json["Albedo"] = { flatData.albedo.r, flatData.albedo.g, flatData.albedo.b, flatData.albedo.a };
		if (albedoTexture.Get<Texture>())
			json["AlbedoTexture"] = albedoTexture.Get<Texture>()->GetID();
		if (roughnessMetallicTexture.Get<Texture>())
			json["RoughnessMetallicTexture"] = roughnessMetallicTexture.Get<Texture>()->GetID();

		json["Roughness"] = flatData.roughness;
		json["Metallic"] = flatData.metallic;

		if (normalTexture.Get<Texture>())
			json["NormalTexture"] = normalTexture.Get<Texture>()->GetID();

		if (shader.Get<Shader>())
			json["Shader"] = shader.Get<Shader>()->GetID();

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

	//basePath is the project path, used for loading textures
	void Material::Deserialize(std::filesystem::path path, std::filesystem::path basePath) {
		std::ifstream file(path);
		if (!file.is_open()) {
			REON_ERROR("Failed to open material file for reading: {}", path.string());
			return;
		}

		nlohmann::ordered_json json;
		file >> json;
		file.close();

		// Extract UID
		if (json.contains("GUID")) {
			m_ID = (json["GUID"].get<std::string>());
		}

		if (json.contains("Name")) {
			SetName(json["Name"].get<std::string>());
		}

		// Extract Albedo Color
		if (json.contains("Albedo")) {
			auto albedoArray = json["Albedo"];
			flatData.albedo = glm::vec4(albedoArray[0], albedoArray[1], albedoArray[2], albedoArray[3]);
		}

		// Extract Albedo Texture
		if (json.contains("AlbedoTexture")) {
			std::string albedoTextureID = json["AlbedoTexture"].get<std::string>();
			albedoTexture = Texture::getTextureFromId(albedoTextureID, basePath.string());
			if (albedoTexture.Get<Texture>()) {
				flatData.useAlbedoTexture = true;
			}
			else {
				REON_CORE_ERROR("Albedo texture load went wrong");
			}
		}

		// Extract Roughness and Roughness Texture
		if (json.contains("Roughness")) {
			flatData.roughness = json["Roughness"].get<float>();
		}

		if (json.contains("RoughnessMetallicTexture")) {
			std::string roughnessMetallicTextureID = json["RoughnessMetallicTexture"].get<std::string>();
			flatData.useRoughnessTexture = true;
			flatData.useMetallicTexture = true;
			roughnessMetallicTexture = Texture::getTextureFromId(roughnessMetallicTextureID, basePath.string());
		}

		// Extract Metallic
		if (json.contains("Metallic")) {
			flatData.metallic = json["Metallic"].get<float>();
		}

		// Extract Normal Texture
		if (json.contains("NormalTexture")) {
			std::string normalTextureID = json["NormalTexture"].get<std::string>();
			flatData.useNormalTexture = true;
			normalTexture = Texture::getTextureFromId(normalTextureID, basePath.string());
		}

		// Extract Shader
		if (json.contains("Shader")) {
			std::string shaderID = json["Shader"].get<std::string>();
			shader = ResourceManager::GetInstance().LoadResource<Shader>("DefaultLit", std::make_tuple("PBR.vert", "PBR.frag", std::optional<std::string>{}));
		}
	}

	
}