#pragma once

// A tiny, dependency-free publish/subscribe bus. Publishers and subscribers
// never reference each other — they only share the event type and this bus —
// so layers (NLP, VTK, future UI) stay decoupled. Header-only and generic:
// any copyable event type can be published; handlers are keyed by event type.

#include <functional>
#include <mutex>
#include <typeindex>
#include <unordered_map>
#include <vector>

namespace semanticad::core {

class EventBus {
public:
    // Register a handler invoked for every event of type E.
    template <class E>
    void subscribe(std::function<void(const E&)> handler) {
        std::lock_guard<std::mutex> lock(mutex_);
        handlers_[std::type_index(typeid(E))].push_back(
            [h = std::move(handler)](const void* e) { h(*static_cast<const E*>(e)); });
    }

    // Synchronously deliver `event` to every handler subscribed to type E.
    template <class E>
    void publish(const E& event) const {
        std::vector<Handler> snapshot;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = handlers_.find(std::type_index(typeid(E)));
            if (it == handlers_.end()) return;
            snapshot = it->second;  // copy so handlers can (un)subscribe safely
        }
        for (const auto& h : snapshot) h(&event);
    }

private:
    using Handler = std::function<void(const void*)>;
    mutable std::mutex mutex_;
    std::unordered_map<std::type_index, std::vector<Handler>> handlers_;
};

}  // namespace semanticad::core
