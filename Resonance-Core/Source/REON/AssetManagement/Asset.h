#pragma once

#include <any>
#include <string>

namespace REON {

	class Asset
	{
    public:
        virtual ~Asset() = default;
        virtual void Load(const std::string& filePath, std::any metadata = {}) = 0; // Pure virtual function for loading assets
        virtual void Unload() = 0;
        const std::string& GetPath() const { return m_Path; }

    protected:
        std::string m_Path;
	};

}

