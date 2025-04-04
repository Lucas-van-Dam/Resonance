#pragma once

#include <cstddef>
#include <string>
#include <memory>
#include <functional>
#include <unordered_map>
#include "REON/Object.h"

namespace REON {

    enum AccessLevel {
        Public = 0,
        Protected = 1,
        Private = 2,
    };

    using setterFuncType = void (*)(void* instance, void* value);
    using getterFuncType = void* (*)(void* instance);

    struct FieldInfo {
        const char* name;
        const char* type;
        //void* memberPtr;
        getterFuncType getter;
        setterFuncType setter;
        //size_t size;
        //AccessLevel accessLevel;
    };


    struct ReflectionClass {
        const char* name;
        const FieldInfo* fields;
        size_t field_count;
    };

    //template <typename ClassType, typename FieldType, FieldType ClassType::* field>
    //struct ReflectionAccessor {
    //    static FieldType& Get(ClassType& instance) {
    //        return instance.*field;
    //    }
    //    static void Set(ClassType& instance, const FieldType& value) {
    //        instance.*field = value;
    //    }
    //};

    template <typename T, typename FieldType, FieldType T::* Field>
    void* genericGetter(void* instance) {
        T* obj = static_cast<T*>(instance);
        return static_cast<void*>(&(obj->*Field));
    }

    template <typename T, typename FieldType, FieldType T::* Field>
    void genericSetter(void* instance, void* value) {
        Object* object = static_cast<Object*>(instance);
        T* obj = dynamic_cast<T*>(object);
        REON_CORE_ASSERT(obj, "obj cant be null");
        obj->*Field = *static_cast<FieldType*>(value);
    }


    class ReflectionRegistry {
    public:
        using FactoryFunc = std::function<std::shared_ptr<REON::Object>()>;
        using FactoryFromVoidFunc = std::function<std::shared_ptr<REON::Object>(void*)>;
        static ReflectionRegistry& Instance() {
            static ReflectionRegistry instance;
            return instance;
        }

        void RegisterClassForReflection(const std::string& className, const ReflectionClass& reflectionData) {
            m_ReflectionRegistry[className] = &reflectionData;
        }

        void RegisterClassType(const std::string& className, FactoryFunc func) {
            m_TypeRegistry[className] = func;
        }

        void RegisterCastingFromVoid(const std::string& className, FactoryFromVoidFunc func) {
            m_CastingRegistry[className] = func;
        }

        const ReflectionClass* GetClass(const std::string& className) const {
            auto it = m_ReflectionRegistry.find(className);
            return it != m_ReflectionRegistry.end() ? it->second : nullptr;
        }

        const std::unordered_map<std::string, const ReflectionClass*>& GetAllClasses() const {
            return m_ReflectionRegistry;
        }

        std::shared_ptr<REON::Object> Create(const std::string& typeName) {
            auto it = m_TypeRegistry.find(typeName);
            if (it != m_TypeRegistry.end()) {
                return it->second();
            }
            return nullptr;
        }

        std::shared_ptr<REON::Object> Create(const std::string& typeName, void* data) {
            auto it = m_CastingRegistry.find(typeName);
            if (it != m_CastingRegistry.end()) {
                return it->second(data);
            }
            return std::make_shared<REON::Object>();
        }

    private:
        std::unordered_map<std::string, const ReflectionClass*> m_ReflectionRegistry;
        std::unordered_map<std::string, FactoryFunc> m_TypeRegistry;
        std::unordered_map<std::string, FactoryFromVoidFunc> m_CastingRegistry;

        ReflectionRegistry() = default;
        ReflectionRegistry(const ReflectionRegistry&) = delete;
        ReflectionRegistry& operator=(const ReflectionRegistry&) = delete;
    };
}