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

		json["Rendering Mode"] = renderingMode;
		json["Blend Mode"] = blendingMode;

		json["Double Sided"] = m_DoubleSided;

		json["Albedo"] = { flatData.albedo.r, flatData.albedo.g, flatData.albedo.b, flatData.albedo.a };
		if (albedoTexture.Get<Texture>())
			json["AlbedoTexture"] = albedoTexture.Get<Texture>()->GetID();
		if (metallicRoughnessTexture.Get<Texture>())
			json["MetallicRoughnessTexture"] = metallicRoughnessTexture.Get<Texture>()->GetID();
		if (emissiveTexture.Get<Texture>())
			json["EmissiveTexture"] = emissiveTexture.Get<Texture>()->GetID();

		json["Roughness"] = flatData.roughness;
		json["Metallic"] = flatData.metallic;

		if (normalTexture.Get<Texture>())
			json["NormalTexture"] = normalTexture.Get<Texture>()->GetID();

		json["NormalScalar"] = flatData.normalScalar;

		json["AlphaCutoff"] = flatData.emissiveFactor.w;
		
		json["EmissiveFactor"] = { flatData.emissiveFactor.r, flatData.emissiveFactor.g, flatData.emissiveFactor.b };

		json["preCompF0"] = flatData.preCompF0;

		json["Flags"] = materialFlags;

		if (shader.Get<Shader>())
			json["Shader"] = shader.Get<Shader>()->GetID();

		std::string name = GetName();

		std::replace_if(name.begin(), name.end(), [](char c) {    static std::string forbiddenChars("\\/:?\"<>|");  return std::string::npos != forbiddenChars.find(c); }, '_');

		SetName(name);
		json["Name"] = GetName();

		std::string writePath = path.string() + "\\" + name + ".material";

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

		if (json.contains("Rendering Mode")) {
			renderingMode = json["Rendering Mode"].get<RenderingModes>();
		}

		if (json.contains("Blend Mode")) {
			blendingMode = json["Blend Mode"].get<BlendingModes>();
		}

		if (json.contains("Double Sided")) {
			m_DoubleSided = json["Double Sided"].get<bool>();
		}

		// Extract Albedo Color
		if (json.contains("Albedo")) {
			auto albedoArray = json["Albedo"];
			flatData.albedo = glm::vec4(albedoArray[0], albedoArray[1], albedoArray[2], albedoArray[3]);
		}

		materialFlags = 0;

		if (json.contains("Flags")) {
			materialFlags = json["Flags"].get<int>();
		}

		if (json.contains("AlphaCutoff")) {
			flatData.emissiveFactor.w = json["AlphaCutoff"].get<float>();
		}

		if (json.contains("EmissiveFactor")) {
			auto emissiveArray = json["EmissiveFactor"];
			flatData.emissiveFactor.r = emissiveArray[0];
			flatData.emissiveFactor.g = emissiveArray[1];
			flatData.emissiveFactor.b = emissiveArray[2];
		}

		// Extract Albedo Texture
		if (json.contains("AlbedoTexture")) {
			std::string albedoTextureID = json["AlbedoTexture"].get<std::string>();
			albedoTexture = Texture::getTextureFromId(albedoTextureID, basePath.string());
			if (albedoTexture.Get<Texture>()) {
				materialFlags |= AlbedoTexture;
			}
			else {
				REON_CORE_ERROR("Albedo texture load went wrong");
			}
		}

		// Extract Roughness and Roughness Texture
		if (json.contains("Roughness")) {
			flatData.roughness = json["Roughness"].get<float>();
		}

		if (json.contains("MetallicRoughnessTexture")) {
			std::string roughnessMetallicTextureID = json["MetallicRoughnessTexture"].get<std::string>();
			metallicRoughnessTexture = Texture::getTextureFromId(roughnessMetallicTextureID, basePath.string());
			if (metallicRoughnessTexture.Get<Texture>()) {
				materialFlags |= MetallicRoughnessTexture;
			}
		}

		// Extract Metallic
		if (json.contains("Metallic")) {
			flatData.metallic = json["Metallic"].get<float>();
		}

		// Extract Normal Texture
		if (json.contains("NormalTexture")) {
			std::string normalTextureID = json["NormalTexture"].get<std::string>();
			normalTexture = Texture::getTextureFromId(normalTextureID, basePath.string());
			if (normalTexture.Get<Texture>()) {
				materialFlags |= NormalTexture;
			}
		}

		flatData.normalScalar = json["NormalScalar"];

		if (json.contains("EmissiveTexture")) {
			std::string emissiveTextureID = json["EmissiveTexture"].get<std::string>();
			emissiveTexture = Texture::getTextureFromId(emissiveTextureID, basePath.string());
			if (emissiveTexture.Get<Texture>()) {
				materialFlags |= EmissiveTexture;
			}
		}

		if (json.contains("preCompF0")) {
			flatData.preCompF0 = json["preCompF0"];
		}

		// Extract Shader
		if (json.contains("Shader")) {
			std::string shaderID = json["Shader"].get<std::string>();
			shader = ResourceManager::GetInstance().LoadResource<Shader>("DefaultLit", std::make_tuple("PBR.vert", "PBR.frag", std::optional<std::string>{}));
		}
	}

	
}