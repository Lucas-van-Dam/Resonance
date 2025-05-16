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
        meshJson["tangents"] = ConvertVec3Array(tangents);
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
        tangents = ConvertJsonToVec3Array(meshJson["tangents"]);
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

    void Mesh::calculateTangents() {
        // https://learnopengl.com/Advanced-Lighting/Normal-Mapping
        tangents.resize(normals.size());

        // Iterate over all triangles of each sub-mesh.
        for (auto& submesh : subMeshes)
            for (size_t i = submesh.indexOffset; i < submesh.indexOffset + submesh.indexCount; i += 3)
            {
                if (i + 2 > indices.size())
                    break;

                glm::vec3 indices{ indices[i], indices[i + 1], indices[i + 2] };

                if (indices[0] > positions.size() || indices[1] > positions.size() || indices[2] > positions.size())
                    continue;

                // Get vertices of the triangle.
                glm::vec3 vtx0 = positions[indices[0]];
                glm::vec3 vtx1 = positions[indices[1]];
                glm::vec3 vtx2 = positions[indices[2]];

                // Get UVs of the triangle.
                glm::vec2 uv0 = texCoords[indices[0]];
                glm::vec2 uv1 = texCoords[indices[1]];
                glm::vec2 uv2 = texCoords[indices[2]];

                // Get primary edges
                glm::vec3 edge0 = vtx1 - vtx0;
                glm::vec3 edge1 = vtx2 - vtx0;

                // Get difference in uv over the two primary edges.
                glm::vec2 deltaUV0 = uv1 - uv0;
                glm::vec2 deltaUV1 = uv2 - uv0;

                // Get inverse of the determinant of the UV tangent matrix.
                float inverseUVDeterminant = 1.0f / (deltaUV0.x * deltaUV1.y - deltaUV1.x * deltaUV0.y);

                // T = tangent
                // B = bi-tangent
                // E0 = first primary edge
                // E1 = second primary edge
                // dU0 = delta of x texture coordinates of the first primary edge
                // dV0 = delta of y texture coordinates of the first primary edge
                // dU1 = delta of x texture coordinates of the second primary edge
                // dV1 = delta of y texture coordinates of the second primary edge
                // ┌          ┐          1        ┌           ┐ ┌             ┐
                // │ Tx Ty Tz │ _ ─────────────── │  dV1 -dV0 │ │ E0x E0y E0z │
                // │ Bx By Bz │ ─ dU0ΔV1 - dU1ΔV0 │ -dU1  dU0 │ │ E1x E1y E1z │
                // └          ┘                   └           ┘ └             ┘
                glm::vec3 tangent;
                tangent.x = inverseUVDeterminant * ((deltaUV1.y * edge0.x) - (deltaUV0.y * edge1.x));
                tangent.y = inverseUVDeterminant * ((deltaUV1.y * edge0.y) - (deltaUV0.y * edge1.y));
                tangent.z = inverseUVDeterminant * ((deltaUV1.y * edge0.z) - (deltaUV0.y * edge1.z));

                // Check if the tangent is valid.
                if (tangent == glm::vec3(0, 0, 0) || tangent != tangent)
                    continue;

                // Normalize the tangent.
                tangent = glm::normalize(tangent);

                // Accumulate the tangent in order to be able to smooth it later.
                tangents[indices[0]] += tangent;
                tangents[indices[1]] += tangent;
                tangents[indices[2]] += tangent;
            }

        // Smooth all tangents.
        for (size_t i = 0; i < tangents.size(); i++)
            if (tangents[i] != glm::vec3(0.0f,0.0f,0.0f))
                tangents[i] = glm::normalize(tangents[i]);
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
            vertex.Tangent = tangents.size() > i ? tangents[i] : glm::vec3(0.0f,0.0f,0.0f); //TODO: Calculate tangents when loading

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