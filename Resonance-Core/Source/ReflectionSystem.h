#pragma once

#include <cstddef>
#include <string>
#include <unordered_map>


struct FieldInfo {
    const char* name;
    const char* type;
    size_t offset;
};


struct ReflectionClass {
    const char* name;
    const FieldInfo* fields;
    size_t field_count;
};


class ReflectionRegistry {
public:
    static ReflectionRegistry& Instance() {
        static ReflectionRegistry instance;
        return instance;
    }

    void RegisterClassForReflection(const std::string& className, const ReflectionClass& reflectionData) {
        registry[className] = &reflectionData;
    }

    const ReflectionClass* GetClass(const std::string& className) const {
        auto it = registry.find(className);
        return it != registry.end() ? it->second : nullptr;
    }

    const std::unordered_map<std::string, const ReflectionClass*>& GetAllClasses() const {
        return registry;
    }

private:
    std::unordered_map<std::string, const ReflectionClass*> registry;

    ReflectionRegistry() = default;
    ReflectionRegistry(const ReflectionRegistry&) = delete;
    ReflectionRegistry& operator=(const ReflectionRegistry&) = delete;
};