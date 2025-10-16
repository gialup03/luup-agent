/**
 * @file version.cpp
 * @brief Version information
 */

#include "../include/luup_agent.h"
#include <stdio.h>

extern "C" {

const char* luup_version(void) {
    static char version_str[32];
    snprintf(version_str, sizeof(version_str), "%d.%d.%d",
             LUUP_VERSION_MAJOR, LUUP_VERSION_MINOR, LUUP_VERSION_PATCH);
    return version_str;
}

void luup_version_components(int* major, int* minor, int* patch) {
    if (major) *major = LUUP_VERSION_MAJOR;
    if (minor) *minor = LUUP_VERSION_MINOR;
    if (patch) *patch = LUUP_VERSION_PATCH;
}

} // extern "C"

