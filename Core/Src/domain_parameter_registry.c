/**
 * @file domain_parameter_registry.c
 * @author k-omura
 * @brief Parameter registry implementation
 * @version 0.1
 * @date 2026-07-02
 *
 */

#include "domain_parameter_registry.h"

#include <string.h>

static domain_parameter_entry_t g_params[DOMAIN_PARAM_REGISTRY_CAPACITY];

/**
 * @brief Finds a parameter entry by its ID. This function searches the parameter registry for the specified parameter ID.
 * @param param_id The ID of the parameter to find.
 * @return Pointer to the parameter entry if found, NULL otherwise.
 */
static domain_parameter_entry_t *domain_parameter_registry_find(uint16_t param_id)
{
    uint16_t i;

    for (i = 0; i < DOMAIN_PARAM_REGISTRY_CAPACITY; i++)
    {
        if (g_params[i].param_id == param_id)
        {
            return &g_params[i];
        }
    }

    return 0;
}

/**
 * @brief Initializes the parameter registry. This function clears all parameter entries.
 */
void domain_parameter_registry_init(void)
{
    memset(g_params, 0, sizeof(g_params));
}

/**
 * @brief Gets the value of a parameter. This function returns the current value of the specified parameter.
 * @param param_id The ID of the parameter to get.
 * @return The current value of the parameter, or 0 if the parameter is not found.
 */
int32_t domain_parameter_registry_get_value(uint16_t param_id)
{
    domain_parameter_entry_t *entry = domain_parameter_registry_find(param_id);
    if (entry == 0)
    {
        return 0;
    }

    return entry->value;
}

/**
 * @brief Sets the value of a parameter. This function updates the value of the specified parameter, ensuring it stays within the defined range.
 * @param param_id The ID of the parameter to set.
 * @param value The new value to set for the parameter.
 */
void domain_parameter_registry_set_value(uint16_t param_id, int32_t value)
{
    domain_parameter_entry_t *entry = domain_parameter_registry_find(param_id);

    if (entry == 0)
    {
        return;
    }

    if (value < entry->min)
    {
        entry->value = entry->min;
    }
    else if (value > entry->max)
    {
        entry->value = entry->max;
    }
    else
    {
        entry->value = value;
    }
}

/**
 * @brief Applies a delta to a parameter. This function updates the value of the specified parameter by adding a delta, ensuring it stays within the defined range.
 * @param param_id The ID of the parameter to update.
 * @param delta The delta to apply to the parameter value.
 */
void domain_parameter_registry_apply_delta(uint16_t param_id, int32_t delta)
{
    domain_parameter_entry_t *entry = domain_parameter_registry_find(param_id);
    int32_t next;

    if (entry == 0)
    {
        return;
    }

    next = entry->value + (delta * entry->step);
    domain_parameter_registry_set_value(param_id, next);
}
