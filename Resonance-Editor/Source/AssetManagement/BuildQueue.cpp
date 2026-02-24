#include "BuildQueue.h"

namespace REON::EDITOR
{
void BuildQueue::Enqueue(BuildJob item) 
{
	uint64_t hash = std::hash<AssetId>{}(item.sourceId) ^ std::hash<AssetTypeId>{}(item.type) ^ std::hash<int>{}(static_cast<int>(item.reason));
	if (m_Dedup.find(hash) == m_Dedup.end())
	{
		m_Items.push_back(std::move(item));
		m_Dedup.insert(hash);
    }
}
bool BuildQueue::TryDequeue(BuildJob& out)
{
	if (m_Items.empty())
		return false;
	out = std::move(m_Items.front());
	m_Items.pop_front();
	uint64_t hash = std::hash<AssetId>{}(out.sourceId) ^ std::hash<AssetTypeId>{}(out.type) ^ std::hash<int>{}(static_cast<int>(out.reason));
	m_Dedup.erase(hash);
    return true;
}
} // namespace REON::EDITOR