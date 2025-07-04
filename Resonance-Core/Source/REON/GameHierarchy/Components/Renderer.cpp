#include "reonpch.h"
#include "Renderer.h"

#include "REON/GameHierarchy/GameObject.h"
#include "REON/GameHierarchy/Scene.h"
#include "REON/GameHierarchy/Components/Transform.h"
#include "REON/GameHierarchy/Components/Light.h"
#include "REON/Application.h"
#include "REON/Platform/Vulkan/VulkanContext.h"
#include "REON/EditorCamera.h"

namespace REON {

	glm::mat4 Renderer::ViewProjMatrix;

	void Renderer::Draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, std::vector<VkDescriptorSet> descriptorSets) {
		const VulkanContext* context = static_cast<const VulkanContext*>(Application::Get().GetRenderContext());

		descriptorSets.push_back(objectDescriptorSets[context->getCurrentFrame()]);

		ObjectRenderData data{};
		data.model = m_ModelMatrix;
		data.transposeInverseModel = glm::transpose(glm::inverse(m_ModelMatrix));
		memcpy(objectDataBuffersMapped[context->getCurrentFrame()], &data, sizeof(data));

		VkBuffer vertexBuffers[] = { mesh.Get<Mesh>()->m_VertexBuffer };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

		vkCmdBindIndexBuffer(commandBuffer, mesh.Get<Mesh>()->m_IndexBuffer, 0, VK_INDEX_TYPE_UINT32);

		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, descriptorSets.size(), descriptorSets.data(), 0, nullptr);
		vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(mesh.Get<Mesh>()->indices.size()), 1, 0, 0, 0);
	}

	void Renderer::Draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, VkDescriptorSet perLightDescriptorSet) {
		const VulkanContext* context = static_cast<const VulkanContext*>(Application::Get().GetRenderContext());

		std::array<VkDescriptorSet, 2> descriptorSets{ perLightDescriptorSet, shadowObjectDescriptorSets[context->getCurrentFrame()] };

		memcpy(shadowObjectDataBuffersMapped[context->getCurrentFrame()], &m_ModelMatrix, sizeof(m_ModelMatrix));

		VkBuffer vertexBuffers[] = { mesh.Get<Mesh>()->m_VertexBuffer };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

		vkCmdBindIndexBuffer(commandBuffer, mesh.Get<Mesh>()->m_IndexBuffer, 0, VK_INDEX_TYPE_UINT32);

		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, descriptorSets.size(), descriptorSets.data(), 0, nullptr);
		vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(mesh.Get<Mesh>()->indices.size()), 1, 0, 0, 0);
	}

	void Renderer::Draw(glm::mat4 mainLightView, glm::mat4 mainLightProj, int skyboxId, int irradianceMapId, int prefilterMapId, int brdfLUTTextureId, std::vector<int> depthCubeId, int shadowMapId,
		const std::shared_ptr<Shader>& overrideShader) {
		/*
		auto data = GetLightingBuffer();
		if (overrideShader != nullptr) {
			overrideShader->use();
			//overrideShader->setMat4("model", m_ModelMatrix);
			//overrideShader->setMat4("view", m_ViewMatrix);
			//overrideShader->setMat4("projection", m_ProjectionMatrix);
			Material overrideMaterial(overrideShader);
			mesh.Get<Mesh>()->Draw(overrideMaterial, data);
			return;
		}
		std::shared_ptr<Material> mat = material.Get<Material>();
		//mat->shader->setMat4("model", m_ModelMatrix);
		//mat->shader->setMat4("view", m_ViewMatrix);
		//mat->shader->setMat4("projection", m_ProjectionMatrix);
		//mat->shader->setFloat("far_plane", 100.0f);
		glActiveTexture(GL_TEXTURE4);
		glUniform1i(glGetUniformLocation(mat->shader->ID, "shadowMap"), 4);
		glBindTexture(GL_TEXTURE_2D, shadowMapId);

		if (skyboxId >= 0) {
			glActiveTexture(GL_TEXTURE5);
			glUniform1i(glGetUniformLocation(mat->shader->ID, "skyboxCube"), 5);
			glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxId);

			glActiveTexture(GL_TEXTURE6);
			glUniform1i(glGetUniformLocation(mat->shader->ID, "irradianceMap"), 6);
			glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceMapId);

			glActiveTexture(GL_TEXTURE7);
			glUniform1i(glGetUniformLocation(mat->shader->ID, "preFilterMap"), 7);
			glBindTexture(GL_TEXTURE_CUBE_MAP, prefilterMapId);

			glActiveTexture(GL_TEXTURE8);
			glUniform1i(glGetUniformLocation(mat->shader.get->ID, "brdfLUT"), 8);
			glBindTexture(GL_TEXTURE_2D, brdfLUTTextureId);
		}

		for (int i = 0; i < depthCubeId.size(); i++) {
			glBindTextureUnit(9 + i, depthCubeId[i]);
		}

		mesh.Get<Mesh>()->Draw(*mat, data);
		*/
	}

	void Renderer::Update(float deltaTime) {
		if (m_Transform == nullptr) {
			m_Transform = GetOwner()->GetTransform();
		}
		m_ModelMatrix = m_Transform->GetWorldTransform();
		m_TransposeInverseModelMatrix = glm::transpose(glm::inverse(m_ModelMatrix));
	}

	void Renderer::SetOwner(std::shared_ptr<GameObject> owner)
	{
		Component::SetOwner(owner);
		GetOwner()->GetScene()->renderManager->AddRenderer(shared_from_this());
	}

	void Renderer::cleanup()
	{
		const VulkanContext* context = static_cast<const VulkanContext*>(Application::Get().GetRenderContext());


		//vkFreeDescriptorSets(context->getDevice(), context->getDescriptorPool(), objectDescriptorSets.size(), objectDescriptorSets.data());

		for (int i = 0; i < context->MAX_FRAMES_IN_FLIGHT; i++) {
			vkDestroyBuffer(context->getDevice(), objectDataBuffers[i], nullptr);
			vmaUnmapMemory(context->getAllocator(), objectDataBufferAllocations[i]);
			vmaFreeMemory(context->getAllocator(), objectDataBufferAllocations[i]);

			vkDestroyBuffer(context->getDevice(), shadowObjectDataBuffers[i], nullptr);
			vmaUnmapMemory(context->getAllocator(), shadowObjectDataBufferAllocations[i]);
			vmaFreeMemory(context->getAllocator(), shadowObjectDataBufferAllocations[i]);
		}
	}

	Renderer::Renderer(std::shared_ptr<Mesh> mesh, std::vector<ResourceHandle> materials) : mesh(std::move(mesh)), materials(materials)
	{
		const VulkanContext* context = static_cast<const VulkanContext*>(Application::Get().GetRenderContext());

		VkDeviceSize bufferSize = sizeof(ObjectRenderData);

		objectDataBuffers.resize(context->MAX_FRAMES_IN_FLIGHT);
		objectDataBufferAllocations.resize(context->MAX_FRAMES_IN_FLIGHT);
		objectDataBuffersMapped.resize(context->MAX_FRAMES_IN_FLIGHT);

		for (size_t i = 0; i < context->MAX_FRAMES_IN_FLIGHT; i++) {
			// TODO: use the VMA_ALLOCATION_CREATE_MAPPED_BIT along with VmaAllocationInfo object instead
			context->createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, objectDataBuffers[i], objectDataBufferAllocations[i]);

			vmaMapMemory(context->getAllocator(), objectDataBufferAllocations[i], &objectDataBuffersMapped[i]);
		}

		bufferSize = sizeof(glm::mat4);

		shadowObjectDataBuffers.resize(context->MAX_FRAMES_IN_FLIGHT);
		shadowObjectDataBufferAllocations.resize(context->MAX_FRAMES_IN_FLIGHT);
		shadowObjectDataBuffersMapped.resize(context->MAX_FRAMES_IN_FLIGHT);

		for (size_t i = 0; i < context->MAX_FRAMES_IN_FLIGHT; i++) {
			// TODO: use the VMA_ALLOCATION_CREATE_MAPPED_BIT along with VmaAllocationInfo object instead
			context->createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, shadowObjectDataBuffers[i], shadowObjectDataBufferAllocations[i]);

			vmaMapMemory(context->getAllocator(), shadowObjectDataBufferAllocations[i], &shadowObjectDataBuffersMapped[i]);
		}
	}

	Renderer::Renderer()
	{
		const VulkanContext* context = static_cast<const VulkanContext*>(Application::Get().GetRenderContext());

		VkDeviceSize bufferSize = sizeof(ObjectRenderData);

		objectDataBuffers.resize(context->MAX_FRAMES_IN_FLIGHT);
		objectDataBufferAllocations.resize(context->MAX_FRAMES_IN_FLIGHT);
		objectDataBuffersMapped.resize(context->MAX_FRAMES_IN_FLIGHT);

		for (size_t i = 0; i < context->MAX_FRAMES_IN_FLIGHT; i++) {
			// TODO: use the VMA_ALLOCATION_CREATE_MAPPED_BIT along with VmaAllocationInfo object instead
			context->createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, objectDataBuffers[i], objectDataBufferAllocations[i]);

			vmaMapMemory(context->getAllocator(), objectDataBufferAllocations[i], &objectDataBuffersMapped[i]);
		}

		bufferSize = sizeof(glm::mat4);

		shadowObjectDataBuffers.resize(context->MAX_FRAMES_IN_FLIGHT);
		shadowObjectDataBufferAllocations.resize(context->MAX_FRAMES_IN_FLIGHT);
		shadowObjectDataBuffersMapped.resize(context->MAX_FRAMES_IN_FLIGHT);

		for (size_t i = 0; i < context->MAX_FRAMES_IN_FLIGHT; i++) {
			// TODO: use the VMA_ALLOCATION_CREATE_MAPPED_BIT along with VmaAllocationInfo object instead
			context->createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, shadowObjectDataBuffers[i], shadowObjectDataBufferAllocations[i]);

			vmaMapMemory(context->getAllocator(), shadowObjectDataBufferAllocations[i], &shadowObjectDataBuffersMapped[i]);
		}
	}

	Renderer::~Renderer()
	{

	}

	void Renderer::OnGameObjectAddedToScene() {
		//GetOwner()->GetScene()->renderManager->AddRenderer(shared_from_this());
	}

	void Renderer::OnComponentDetach()
	{
		GetOwner()->GetScene()->renderManager->RemoveRenderer(shared_from_this());
	}

	void Renderer::RebuildDrawCommands()
	{
		drawCommands.clear();

		if (!mesh.Get<Mesh>())
			return;

		for (auto submesh : mesh.Get<Mesh>()->subMeshes) {
			if (submesh.materialIndex >= materials.size() || submesh.materialIndex < 0)
				continue;

			Material* mat = materials[submesh.materialIndex].Get<Material>().get();
			DrawCommand cmd;
			cmd.mesh = mesh.Get<Mesh>().get();
			cmd.material = mat;
			cmd.shader = mat->shader.Get<Shader>().get();
			cmd.startIndex = submesh.indexOffset;
			cmd.indexCount = submesh.indexCount;
			cmd.owner = this;

			drawCommands.push_back(cmd);
		}
	}
	void Renderer::MarkDrawCommandsDirty()
	{
		drawCommandsDirty = true;
	}
	void Renderer::SetMaterial(size_t index, const ResourceHandle& material)
	{
		if (index >= materials.size()) {
			REON_CORE_ERROR("Tried to set material of renderer {}, but index: {}, is outside of material list size: {}", GetName(), index, materials.size());
			return;
		}

		materials[index] = material;
		MarkDrawCommandsDirty();
	}
	nlohmann::ordered_json Renderer::Serialize() const
	{
		nlohmann::ordered_json json;

		json["Type"] = GetTypeName();
		json["Mesh"] = mesh.Get<Mesh>() ? mesh.Get<Mesh>()->GetID() : "";
		json["Materials"] = nlohmann::ordered_json::array();
		for (const auto& material : materials) {
			json["Materials"].push_back(material.Get<Material>()->GetID());
		}
		return json;
	}
	void Renderer::Deserialize(const nlohmann::ordered_json& json, std::filesystem::path basePath)
	{
		if (json.contains("Mesh")) {
			std::string meshId = json["Mesh"];
			if (!meshId.empty()) {
				mesh = ResourceManager::GetInstance().GetResource<Mesh>(meshId);
				if (!mesh.Get<Mesh>()) {
					if (auto meshInfo = AssetRegistry::Instance().GetAssetById(meshId)) {
						auto meshPtr = std::make_shared<Mesh>(meshId); // RETURNS NULLPTR FOR SOME REASON
						nlohmann::json meshJson;
						auto path = basePath / meshInfo->path;
						std::ifstream meshFile(path);
						if (meshFile.is_open()) {
							nlohmann::json modelJson;
							meshFile >> modelJson;
							meshFile.close();
							for (auto mesh : modelJson["meshes"]) {
								if (mesh["GUID"] == meshInfo->id) {
									meshJson = mesh;
								}
							}
							if (meshJson.empty()) {
								REON_CORE_ERROR("Mesh with ID: {} not found in file: {}", meshId, path.string());
							}
						}
						else {
							REON_CORE_ERROR("Failed to open mesh file: {}", (basePath / meshInfo->path).string());
						}
						meshPtr->DeSerialize(meshJson);
						ResourceManager::GetInstance().AddResource<Mesh>(meshPtr);
						mesh = meshPtr;
					}
					else {
						//REON_CORE_ERROR("Failed to load mesh with ID: {}", meshId);
					}
				}
			}
		}
		if (json.contains("Materials")) {
			materials.clear();
			for (const auto& materialId : json["Materials"]) {
				std::shared_ptr<Material> material;
				if (!(material = ResourceManager::GetInstance().GetResource<Material>(materialId))) {
					auto meshInfo = AssetRegistry::Instance().GetAssetById(materialId);
					if (meshInfo) {
						material = std::make_shared<Material>();
						material->Deserialize(basePath / meshInfo->path, basePath);
						ResourceManager::GetInstance().AddResource<Material>(material);
					}
					else {
						//REON_CORE_ERROR("Failed to find material with ID: {}", materialId);
					}
				}
				if (material) {
					materials.push_back(material);
				}
				else {
					//REON_CORE_ERROR("Failed to load material with ID: {}", materialId);
				}
			}
		}
	}
}