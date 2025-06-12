#pragma once

#include "Message.hpp"
#include <unordered_map>
#include <cstdint>

class MessageBus {
    public:
        MessageBus();
        void send(Message *message);
        Message *read(uint64_t address);
    
    private:
        std::unordered_map<uint64_t, Message*> messages;
};
