#pragma once

#include "gs2.hpp"
#include "platforms.hpp"
#include "devices.hpp"
/**
 * a System Configuraiton is a platform, and, a list of devices and their slots.
 */

struct SystemConfig_t {
    const char *name;
    PlatformId_t platform_id;
    DeviceMap_t *device_map;
    bool builtin;
};

extern SystemConfig_t BuiltinSystemConfigs[];

SystemConfig_t *get_system_config(int index);
