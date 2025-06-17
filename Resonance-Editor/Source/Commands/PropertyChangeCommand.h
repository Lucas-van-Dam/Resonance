#pragma once  
#include <memory>
#include "ICommand.h"  
#include <REON/GameHierarchy/GameObject.h>  
#include <REON/GameHierarchy/Components/Transform.h>  

namespace REON::EDITOR {  
	template<typename T>
    class PropertyChangeCommand : public ICommand {
    public:  
        PropertyChangeCommand(
            std::function<void(const T&)> setter,
            std::function<T()> getter,
            const T& newValue,
            void* valuePtr)
            : m_Setter(std::move(setter)), m_Getter(std::move(getter)),
              m_OldValue(m_Getter()), m_NewValue(newValue), valuePtr(valuePtr)
        {}  

        bool MergeWith(const ICommand* other) override {
			auto cmd = dynamic_cast<const PropertyChangeCommand<T>*>(other);
            if (cmd && cmd->valuePtr == valuePtr
                && std::abs(std::chrono::duration_cast<std::chrono::milliseconds>(cmd->time - time).count()) < 200) {
                m_NewValue = cmd->m_NewValue;
                return true;
            } else {
				return false;
            }
        }

        void UpdateValue(const T& newValue)
        {
            m_NewValue = newValue;
		}

        void execute() override {
            m_Setter(m_NewValue);
            time = std::chrono::high_resolution_clock::now();
        }

        void undo() override {
            m_Setter(m_OldValue);
        }

    private:  
        std::function<void(const T&)> m_Setter;  
        std::function<T()> m_Getter;  
        T m_NewValue;  
		T m_OldValue;
		void* valuePtr = nullptr;
        std::chrono::high_resolution_clock::time_point time;
    };  
}