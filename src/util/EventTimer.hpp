#pragma once

#include <vector>
#include "gs2.hpp"

class EventTimer {
public:
    struct Event {
        uint64_t triggerCycles;
        void (*triggerCallback)(uint64_t, void*);
        uint64_t instanceID;
        void* userData;
    };

    EventTimer();
    ~EventTimer();

    void scheduleEvent(uint64_t triggerCycles, void (*callback)(uint64_t, void*), uint64_t instanceID, void* userData = nullptr);
    void processEvents(uint64_t currentCycles);
    void cancelEvents(uint64_t instanceID);
    bool hasPendingEvents() const;
    uint64_t getNextEventCycle() const;

private:
    std::vector<Event> events;
};
