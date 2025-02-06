#pragma once
#include <mutex>
#include <typeindex>
#include <unordered_map>
#include <functional>

namespace REON {

    class Event;

    using EventCallback = std::function<void(const Event&)>;
    using CallbackID = size_t;

    class EventBus
    {
    public:
        static EventBus& Get() {
            static EventBus instance;
            return instance;
        }

        template <typename EventType>
        CallbackID subscribe(std::function<void(const EventType&)> callback) {
            std::lock_guard<std::mutex> lock(mutex_);
            auto wrappedCallback = [callback](const Event& event) {
                if (const EventType* specificEvent = dynamic_cast<const EventType*>(&event)) {
                    callback(*specificEvent);
                }
                };
            subscribers[std::type_index(typeid(EventType))].emplace_back(nextCallbackID_, wrappedCallback);
            return nextCallbackID_++;
        }

        template <typename EventType>
        void unsubscribe(CallbackID id) {
            std::lock_guard<std::mutex> lock(mutex_);
            auto& vec = subscribers[std::type_index(typeid(EventType))];
            vec.erase(std::remove_if(vec.begin(), vec.end(),
                [id](const auto& pair) { return pair.first == id; }), vec.end());
        }

        

        template <typename EventType>
        void publish(const EventType& event) const {
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = subscribers.find(std::type_index(typeid(EventType)));
            if (it != subscribers.end()) {
                for (auto& [callbackID, callback] : it->second) {
                    callback(event);
                }
            }
        }

        // Expensive function since it uses RTTI, only use if absolutely needed
        void publishDynamic(const Event& event) const;

        // Delete copy constructor and assignment operator to ensure a single instance.
        EventBus(const EventBus&) = delete;
        EventBus& operator=(const EventBus&) = delete;

    private:
        // Private constructor to enforce singleton usage.
        EventBus() = default;
        ~EventBus() = default;

        mutable std::mutex mutex_;
        size_t nextCallbackID_ = 0;
        std::unordered_map<std::type_index, std::vector<std::pair<CallbackID, EventCallback>>> subscribers;
    };
}

