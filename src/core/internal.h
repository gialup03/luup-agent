/**
 * @file internal.h
 * @brief Internal headers and utilities
 */

#ifndef LUUP_INTERNAL_H
#define LUUP_INTERNAL_H

#include "../../include/luup_agent.h"

// Internal error setter
extern void luup_set_error(luup_error_t code, const char* message);

#endif // LUUP_INTERNAL_H

