/**
 * @file domain_parameter_registry.h
 * @author k-omura
 * @brief Parameter registry definitions and utilities
 * @version 0.1
 * @date 2026-07-02
 *
 */

#ifndef DOMAIN_PARAMETER_REGISTRY_H
#define DOMAIN_PARAMETER_REGISTRY_H

#include <stdint.h>

#define DOMAIN_PARAM_REGISTRY_CAPACITY (64)

typedef struct
{
    uint16_t param_id;
    int32_t value;
    int32_t min;
    int32_t max;
    int32_t step;
} domain_parameter_entry_t;

void domain_parameter_registry_init(void);
int32_t domain_parameter_registry_get_value(uint16_t param_id);
void domain_parameter_registry_set_value(uint16_t param_id, int32_t value);
void domain_parameter_registry_apply_delta(uint16_t param_id, int32_t delta);

#endif
