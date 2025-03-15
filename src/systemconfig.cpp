#include "gs2.hpp"
#include "systemconfig.hpp"


DeviceMap_t DeviceMap_II[] = {
    {DEVICE_ID_KEYBOARD_IIPLUS, 0},
    {DEVICE_ID_END, 0}
};

DeviceMap_t DeviceMap_IIPLUS[] = {
    {DEVICE_ID_KEYBOARD_IIPLUS, 0},
    {DEVICE_ID_SPEAKER, 0},
    {DEVICE_ID_DISPLAY, 0},
    {DEVICE_ID_GAMECONTROLLER, 0},
    {DEVICE_ID_LANGUAGE_CARD, 0},
    {DEVICE_ID_PRODOS_BLOCK, 5},
    {DEVICE_ID_PRODOS_CLOCK, 2},
    {DEVICE_ID_DISK_II, 6},
    {DEVICE_ID_MEM_EXPANSION, 4},
    {DEVICE_ID_END, 0}
};

DeviceMap_t DeviceMap_IIE[] = {
    {DEVICE_ID_KEYBOARD_IIE, 0},
    {DEVICE_ID_END, 0}
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
