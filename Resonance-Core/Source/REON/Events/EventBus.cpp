#include "reonpch.h"
#include "EventBus.h"

#include "REON/Events/Event.h"

void REON::EventBus::publishDynamic(const Event& event) const {
    std::type_index idx(event.getDynamicType());
    auto it = subscribers.find(idx);
    if (it != subscribers.end()) {
        for (const auto& [id, callback] : it->second) {
            callback(event);
        }
    }
}
