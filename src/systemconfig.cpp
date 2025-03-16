#include "gs2.hpp"
#include "systemconfig.hpp"


DeviceMap_t DeviceMap_II[] = {
    {DEVICE_ID_KEYBOARD_IIPLUS, SLOT_NONE},
    {DEVICE_ID_END, SLOT_NONE}
};

DeviceMap_t DeviceMap_IIPLUS[] = {
    {DEVICE_ID_KEYBOARD_IIPLUS, SLOT_NONE},
    {DEVICE_ID_SPEAKER, SLOT_NONE},
    {DEVICE_ID_DISPLAY, SLOT_NONE},
    {DEVICE_ID_GAMECONTROLLER, SLOT_NONE},
    {DEVICE_ID_LANGUAGE_CARD, SLOT_0},
    {DEVICE_ID_PD_BLOCK2, SLOT_5},
    {DEVICE_ID_PRODOS_CLOCK, SLOT_2},
    {DEVICE_ID_DISK_II, SLOT_6},
    {DEVICE_ID_MEM_EXPANSION, SLOT_4},
    {DEVICE_ID_END, SLOT_NONE}
};

DeviceMap_t DeviceMap_IIE[] = {
    {DEVICE_ID_KEYBOARD_IIE, SLOT_NONE},
    {DEVICE_ID_END, SLOT_NONE}
};

SystemConfig_t BuiltinSystemConfigs[] = {
    {
        "Apple ][", 
        PLATFORM_APPLE_II_PLUS, 
        DeviceMap_II,
        true
    },
    {
        "Apple ][+", 
        PLATFORM_APPLE_II_PLUS, 
        DeviceMap_IIPLUS,
        true
    },
    {
        "Apple IIe",
        PLATFORM_APPLE_IIE,
        DeviceMap_IIE,
        true
    },
/*     {
        "Apple IIc",
        PLATFORM_APPLE_IIC,
        DeviceMap_IIC,
        true
    }, */
    // Add more built-in configurations as needed
};

SystemConfig_t *get_system_config(int index) {
    return &BuiltinSystemConfigs[index];
}
