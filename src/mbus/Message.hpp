#pragma once

#include <cstdint>

class Message {
    public:
        Message(uint32_t mtype, uint32_t instance);
        uint64_t get_address();
        uint32_t get_mtype();
        uint32_t get_instance();
    private:
        uint64_t address;
        uint32_t mtype;
        uint32_t instance;
};

enum message_class_t {
    MESSAGE_CLASS_NONE = 0,
    MESSAGE_CLASS_DISKII = 1,
};
