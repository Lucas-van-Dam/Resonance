#include "reonpch.h"
#include "Mesh.h"
#include <REON/ResourceManagement/ResourceInfo.h>
#include <REON/Platform/Vulkan/VulkanContext.h>
#include <REON/Application.h>

namespace REON {

    
    void Mesh::Load(const std::string& name, std::any metadata)
    {
        try {
            Mesh mesh = std::any_cast<Mesh>(metadata);
            return;
        }
        catch (const std::bad_any_cast&) {}
        m_Path = name;
        auto data = std::any_cast<std::tuple<std::vector<Vertex>, std::vector<unsigned int>>>(metadata);
        this->m_Vertices = std::get<0>(data);
        this->indices = std::get<1>(data);

        setupMesh();
    }

    void Mesh::Load()
    {
        setupMesh2();
    }

    void Mesh::Unload()
    {
        const VulkanContext* context = static_cast<const VulkanContext*>(Application::Get().GetRenderContext());
        vkDestroyBuffer(context->getDevice(), m_VertexBuffer, nullptr);
        vmaFreeMemory(context->getAllocator(), m_VertexBufferAllocation);
        vkDestroyBuffer(context->getDevice(), m_IndexBuffer, nullptr);
        vmaFreeMemory(context->getAllocator(), m_IndexBufferAllocation);
    }

    nlohmann::ordered_json Mesh::Serialize() const
    {
        nlohmann::ordered_json meshJson;
        meshJson["GUID"] = GetID();
        meshJson["vertices"] = ConvertVec3Array(positions); // Convert glm::vec3 to JSON
        meshJson["normals"] = ConvertVec3Array(normals);
        meshJson["uvs"] = ConvertVec2Array(texCoords);
        meshJson["tangents"] = ConvertVec4Array(tangents);
        meshJson["colors"] = ConvertVec4Array(colors);
        meshJson["indices"] = indices;

        // Serialize submeshes
        nlohmann::ordered_json subMeshArray = nlohmann::ordered_json::array();
        for (const auto& subMesh : subMeshes)
        {
            nlohmann::ordered_json subMeshJson;
            subMeshJson["indexCount"] = subMesh.indexCount;
            subMeshJson["indexOffset"] = subMesh.indexOffset;
            subMeshJson["materialIndex"] = subMesh.materialIndex;
            subMeshArray.push_back(subMeshJson);
        }
        meshJson["subMeshes"] = subMeshArray;
        
        return meshJson;
    }

    void Mesh::DeSerialize(const nlohmann::ordered_json& meshJson)
    {
        positions = ConvertJsonToVec3Array(meshJson["vertices"]);
        normals = ConvertJsonToVec3Array(meshJson["normals"]);
        texCoords = ConvertJsonToVec2Array(meshJson["uvs"]);
        tangents = ConvertJsonToVec4Array(meshJson["tangents"]);
        colors = ConvertJsonToVec4Array(meshJson["colors"]);
        indices = meshJson["indices"].get<std::vector<uint32_t>>();

        // Deserialize subMeshes
        for (const auto& subMeshJson : meshJson["subMeshes"]) {
            SubMesh subMesh;
            subMesh.indexCount = subMeshJson["indexCount"].get<int>();
            subMesh.indexOffset = subMeshJson["indexOffset"].get<int>();
            subMesh.materialIndex = subMeshJson["materialIndex"].get<int>();
            subMeshes.push_back(subMesh);
        }
    }

    nlohmann::ordered_json Mesh::ConvertVec3Array(const std::vector<glm::vec3>& vecs) const{
        nlohmann::ordered_json arr = nlohmann::ordered_json::array();
        for (const auto& v : vecs) {
            arr.push_back({ v.x, v.y, v.z });
        }
        return arr;
    }

    nlohmann::ordered_json Mesh::ConvertVec2Array(const std::vector<glm::vec2>& vecs) const {
        nlohmann::ordered_json arr = nlohmann::ordered_json::array();
        for (const auto& v : vecs) {
            arr.push_back({ v.x, v.y });
        }
        return arr;
    }

    nlohmann::ordered_json Mesh::ConvertVec4Array(const std::vector<glm::vec4>& vecs) const {
        nlohmann::ordered_json arr = nlohmann::ordered_json::array();
        for (const auto& v : vecs) {
            arr.push_back({ v.x, v.y, v.z, v.w });
        }
        return arr;
    }

    std::vector<glm::vec3> Mesh::ConvertJsonToVec3Array(const nlohmann::json& jsonArray) {
        std::vector<glm::vec3> vec;
        for (const auto& item : jsonArray) {
            vec.emplace_back(item[0], item[1], item[2]);
        }
        return vec;
    }

    std::vector<glm::vec2> Mesh::ConvertJsonToVec2Array(const nlohmann::json& jsonArray) {
        std::vector<glm::vec2> vec;
        for (const auto& item : jsonArray) {
            vec.emplace_back(item[0], item[1]);
        }
        return vec;
    }

    std::vector<glm::vec4> Mesh::ConvertJsonToVec4Array(const nlohmann::json& jsonArray) {
        std::vector<glm::vec4> vec;
        for (const auto& item : jsonArray) {
            vec.emplace_back(item[0], item[1], item[2], item[3]);
        }
        return vec;
    }

    void Mesh::calculateTangents(std::shared_ptr<Mesh> mesh) {
        mesh->tangents.resize(mesh->indices.size());
        SMikkTSpaceMeshContext userContext{};
        userContext.mesh = mesh;

        SMikkTSpaceInterface mikkInterface{};
        mikkInterface.m_getNumFaces = getNumFaces;
        mikkInterface.m_getNumVerticesOfFace = getNumVerticesOfFace;
        mikkInterface.m_getPosition = getPosition;
        mikkInterface.m_getNormal = getNormal;
        mikkInterface.m_getTexCoord = getTexCoord;
        mikkInterface.m_setTSpaceBasic = setTSpaceBasic;

        SMikkTSpaceContext context{};
        context.m_pUserData = &userContext;
        context.m_pInterface = &mikkInterface;

        genTangSpaceDefault(&context);
    }


    // initializes all the buffer objects/arrays
    void Mesh::setupMesh()
    {
        REON_CORE_CRITICAL("THIS SHOULD NOT BE CALLED ANYMORE");
    }

    void Mesh::setupMesh2()
    {
        setupBuffers();
    }

    void Mesh::setupBuffers()
    {
        m_Vertices.resize(positions.size());

        for (size_t i = 0; i < positions.size(); ++i) {
            Vertex vertex{};
            vertex.Position = positions[i];
            vertex.Color = colors[i];
            vertex.Normal = normals[i];
            vertex.TexCoords = texCoords[i];
            vertex.Tangent = tangents.size() > i ? tangents[i] : glm::vec4(0.0f,0.0f,0.0f,0.0f); //TODO: Calculate tangents when loading

            m_Vertices[i] = vertex;
        }

        VkDeviceSize bufferSize = sizeof(m_Vertices[0]) * m_Vertices.size();

        VkBuffer stagingBuffer;
        VmaAllocation stagingBufferAllocation;
        const VulkanContext* context = static_cast<const VulkanContext*>(Application::Get().GetRenderContext());
        context->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, stagingBuffer, stagingBufferAllocation);

        void* data;
        vmaMapMemory(context->getAllocator(), stagingBufferAllocation, &data);
        memcpy(data, m_Vertices.data(), (size_t)bufferSize);
        vmaUnmapMemory(context->getAllocator(), stagingBufferAllocation);

        context->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 0, m_VertexBuffer, m_VertexBufferAllocation);

        context->copyBuffer(stagingBuffer, m_VertexBuffer, bufferSize);

        vkDestroyBuffer(context->getDevice(), stagingBuffer, nullptr);
        vmaFreeMemory(context->getAllocator(), stagingBufferAllocation);

        bufferSize = sizeof(indices[0]) * indices.size();

        context->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, stagingBuffer, stagingBufferAllocation);

        vmaMapMemory(context->getAllocator(), stagingBufferAllocation, &data);
        memcpy(data, indices.data(), (size_t)bufferSize);
        vmaUnmapMemory(context->getAllocator(), stagingBufferAllocation);

        context->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, 0, m_IndexBuffer, m_IndexBufferAllocation);

        context->copyBuffer(stagingBuffer, m_IndexBuffer, bufferSize);

        vkDestroyBuffer(context->getDevice(), stagingBuffer, nullptr);
        vmaFreeMemory(context->getAllocator(), stagingBufferAllocation);
    }

    Mesh::Mesh(const Mesh& mesh)
    {
        
    }

    // render the mesh
    void Mesh::Draw(Material& material, std::vector<LightData> lightData) const
    {
        /*
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, 0);
        //load textures or flat values
        //diffuse
        if (material.albedoTexture == nullptr) {
            //material.shader->setBool("useAlbedoTexture", false);
            //material.shader->setVec4("albedo", material.albedoColor);
        }
        else {
            //material.shader->setBool("useAlbedoTexture", true);
            glActiveTexture(GL_TEXTURE0);
            glUniform1i(glGetUniformLocation(material.shader->ID, "texture_albedo"), 0);
            //glBindTexture(GL_TEXTURE_2D, material.albedoTexture->GetTextureId());
        }
        //normal
        if (material.normalTexture == nullptr) {
            //material.shader->setBool("useNormalTexture", false);
        }
        else {
            //material.shader->setBool("useNormalTexture", true);
            glActiveTexture(GL_TEXTURE1);
            glUniform1i(glGetUniformLocation(material.shader->ID, "texture_normal"), 1);
            //glBindTexture(GL_TEXTURE_2D, material.normalTexture->GetTextureId());
        }
        //roughness
        if (material.roughnessTexture == nullptr) {
            //material.shader->setBool("useRoughnessTexture", false);
            //material.shader->setFloat("roughness", material.roughness);
        }
        else {
            //material.shader->setBool("useRoughnessTexture", true);
            glActiveTexture(GL_TEXTURE2);
            glUniform1i(glGetUniformLocation(material.shader->ID, "texture_roughness"), 2);
            //glBindTexture(GL_TEXTURE_2D, material.roughnessTexture->GetTextureId());
        }
        //metallic
        if (material.metallicTexture == nullptr) {
            //material.shader->setBool("useMetallicTexture", false);
            //material.shader->setFloat("metallic", material.metallic);
        }
        else {
            //material.shader->setBool("useMetallicTexture", true);
            glActiveTexture(GL_TEXTURE3);
            //glBindTexture(GL_TEXTURE_2D, material.metallicTexture->GetTextureId());
        }

        if (!lightData.empty()) {
            LightData* lightDataStatic = &lightData.front();

            glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_SSBO);
            size_t totalSize = sizeof(LightData) * lightData.size();
            glBufferData(GL_SHADER_STORAGE_BUFFER, totalSize, lightDataStatic,
                GL_DYNAMIC_DRAW);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_SSBO);
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0); // unbind
        }

        // draw mesh
        glBindVertexArray(m_VAO);
        glDrawElements(GL_TRIANGLES, static_cast<unsigned int>(indices.size()), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        // always good practice to set everything back to defaults once configured.
        glActiveTexture(GL_TEXTURE0);
        */
    }
}