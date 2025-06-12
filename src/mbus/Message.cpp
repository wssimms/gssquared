#include "Message.hpp"

Message::Message(uint32_t mtype, uint32_t instance) {
    address = ((uint64_t)mtype << 32 | instance);
    this->mtype = mtype;
    this->instance = instance;
}