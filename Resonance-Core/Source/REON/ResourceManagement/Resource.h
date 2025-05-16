#pragma once
#include <string>
#include <any>
#include <iostream>
#include "REON/Object.h"
namespace REON {

	class ResourceBase : public Object {
	public:
        virtual ~ResourceBase() = default;
        virtual void Load(const std::string& name, std::any metadata = {}) = 0; // Pure virtual function for loading resources
        virtual void Load() = 0;
        virtual void Unload() = 0;
        const std::string& GetPath() const { return m_Path; }

    protected:
        std::string m_Path;
	};

    class ResourceHandle {
    public:
        ResourceHandle() = default;

        // Move constructor
        ResourceHandle(ResourceHandle&& other) noexcept {
            std::cout << "Moving ResourceHandle: " << this << " <- " << &other << std::endl;
            resourceBase = std::move(other.resourceBase);
        }

        // Copy constructor (MUST BE CONST)
        ResourceHandle(const ResourceHandle& other) {
            std::cout << "Copying ResourceHandle: " << this << " <- " << &other << std::endl;
            resourceBase = other.resourceBase;  // shared_ptr allows copying
        }

        // Copy assignment operator (fixes the error)
        ResourceHandle& operator=(const ResourceHandle& other) {
            std::cout << "Copy Assigning ResourceHandle: " << this << " <- " << &other << std::endl;
            if (this != &other) {
                resourceBase = other.resourceBase;
            }
            return *this;
        }

        // Move assignment operator
        ResourceHandle& operator=(ResourceHandle&& other) noexcept {
            std::cout << "Move Assigning ResourceHandle: " << this << " <- " << &other << std::endl;
            if (this != &other) {
                resourceBase = std::move(other.resourceBase);
            }
            return *this;
        }

        template <typename T>
        ResourceHandle(std::shared_ptr<T> resource)
            : resourceBase(std::static_pointer_cast<ResourceBase>(resource)) {
            static_assert(std::is_base_of_v<ResourceBase, T>, "T must derive from ResourceBase");
        }

        template <typename T>
        std::shared_ptr<T> Get() const {
            static_assert(std::is_base_of_v<ResourceBase, T>, "T must derive from ResourceBase");
            if(auto sharedResource = resourceBase.lock())
                return std::dynamic_pointer_cast<T>(sharedResource);
            return nullptr;
        }



    private:
        std::weak_ptr<ResourceBase> resourceBase;
    };
}