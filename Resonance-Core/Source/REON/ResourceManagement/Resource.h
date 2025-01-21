#pragma once
#include <string>
#include <any>
#include "REON/Object.h"


namespace REON {

	class Resource : public Object {
	public:
        virtual ~Resource() = default;
        virtual void Load(const std::string& name, std::any metadata = {}) = 0; // Pure virtual function for loading resources
        virtual void Unload() = 0;
        const std::string& GetPath() const { return m_Path; }

    protected:
        std::string m_Path;
	};
}