#include "reonpch.h"

#include "MeshLoader.h"

#include "REON/AssetManagement/ModelBinFormat.h"

namespace REON
{

std::shared_ptr<ResourceBase> MeshLoader::Load(const AssetKey& key, const ArtifactRef& ref, IBlobReader& reader)
{
    std::vector<std::byte> bytes;
    if (!reader.ReadRange(ref.uri, ref.offset, ref.size, bytes))
        return {};

    if (bytes.size() < sizeof(MeshHeader))
        return {};

    const MeshHeader* mh = reinterpret_cast<const MeshHeader*>(bytes.data());
    if (mh->magic != MESH_MAGIC || mh->version != 1)
        return {};
    const uint32_t vtx = mh->vertexCount;
    const uint32_t idx = mh->indexCount;

    auto inRange = [&](uint32_t off, uint64_t len) -> bool { return uint64_t(off) + len <= bytes.size(); };

    const uint64_t posBytes = uint64_t(vtx) * 3ull * sizeof(float);
    const uint64_t nrmBytes = uint64_t(vtx) * 3ull * sizeof(float);
    const uint64_t tanBytes = uint64_t(vtx) * 4ull * sizeof(float);
    const uint64_t uvBytes = uint64_t(vtx) * 2ull * sizeof(float);
    const uint64_t idxBytes = uint64_t(idx) * sizeof(uint32_t);

    if (!inRange(mh->posOffset, posBytes) || !inRange(mh->idxOffset, idxBytes))
        return {};

    DecodedMeshData meshData;

    const std::byte* base = bytes.data();

    {
        const float* p = reinterpret_cast<const float*>(base + mh->posOffset);
        meshData.positions.resize(vtx);
        for (uint32_t i = 0; i < vtx; ++i)
            meshData.positions[i] = glm::vec3(p[i * 3 + 0], p[i * 3 + 1], p[i * 3 + 2]);
    }

    if (mh->nrmOffset != 0)
    {
        if (!inRange(mh->nrmOffset, nrmBytes))
            return {};
        const float* n = reinterpret_cast<const float*>(base + mh->nrmOffset);
        meshData.normals.resize(vtx);
        for (uint32_t i = 0; i < vtx; ++i)
            meshData.normals[i] = glm::vec3(n[i * 3 + 0], n[i * 3 + 1], n[i * 3 + 2]);
    }

    if (mh->tanOffset != 0)
    {
        if (!inRange(mh->tanOffset, tanBytes))
            return {};
        const float* t = reinterpret_cast<const float*>(base + mh->tanOffset);
        meshData.tangents.resize(vtx);
        for (uint32_t i = 0; i < vtx; ++i)
            meshData.tangents[i] = glm::vec4(t[i * 4 + 0], t[i * 4 + 1], t[i * 4 + 2], t[i * 4 + 3]);
    }

    if (mh->uv0Offset != 0)
    {
        if (!inRange(mh->uv0Offset, uvBytes))
            return {};
        const float* u = reinterpret_cast<const float*>(base + mh->uv0Offset);
        meshData.texCoords.resize(vtx);
        for (uint32_t i = 0; i < vtx; ++i)
            meshData.texCoords[i] = glm::vec2(u[i * 2 + 0], u[i * 2 + 1]);
    }

    {
        const uint32_t* ix = reinterpret_cast<const uint32_t*>(base + mh->idxOffset);
        meshData.indices.assign(ix, ix + idx);
    }

    if (mh->colorOffset != 0)
    {
        if (!inRange(mh->colorOffset, uint64_t(vtx) * 4ull * sizeof(float)))
            return {};
        const float* c = reinterpret_cast<const float*>(base + mh->colorOffset);
        meshData.colors.resize(vtx);
        for (uint32_t i = 0; i < vtx; ++i)
            meshData.colors[i] = glm::vec4(c[i * 4 + 0], c[i * 4 + 1], c[i * 4 + 2], c[i * 4 + 3]);
    }

    auto mesh = std::make_shared<Mesh>(meshData);

    for (int i = 0; i < mh->subMeshCount; ++i)
    {
        if (!inRange(mh->subMeshOffset + (i * sizeof(SubMeshEntry)), sizeof(SubMeshEntry))) {
            return {};
        }

        const SubMeshEntry* entry =
            reinterpret_cast<const SubMeshEntry*>(base + mh->subMeshOffset + (i * sizeof(SubMeshEntry)));

        SubMesh subm;
        subm.indexCount = entry->indexCount;
        subm.indexOffset = entry->indexOffset;
        subm.materialIndex = entry->materialId;

        mesh->subMeshes.push_back(subm);
    }


    return mesh;
}
} // namespace REON