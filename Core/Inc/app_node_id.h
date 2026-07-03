/**
 * @file app_node_id.h
 * @author k-omura
 * @brief Node ID definitions and utilities
 * @version 0.1
 * @date 2026-07-02
 *
 */

#ifndef APP_NODE_ID_H
#define APP_NODE_ID_H

#include <stdint.h>

#define APP_NODE_GROUP_RE1 0x0100U  //!< Node group identifier for RE1 controls
#define APP_NODE_GROUP_RE2 0x0200U  //!< Node group identifier for RE2 controls
#define APP_NODE_GROUP_MASK 0xFF00U //!< Mask to extract the node group
#define APP_NODE_VALUE_MASK 0x00FFU //!< Mask to extract the node value

#define APP_NODE_RE1(ctrl) (uint16_t)(APP_NODE_GROUP_RE1 | ((uint16_t)(ctrl) & APP_NODE_VALUE_MASK)) //!< Macro to create a node ID for RE1 controls
#define APP_NODE_RE2(ctrl) (uint16_t)(APP_NODE_GROUP_RE2 | ((uint16_t)(ctrl) & APP_NODE_VALUE_MASK)) //!< Macro to create a node ID for RE2 controls
#define APP_NODE_GROUP(node) (uint16_t)((uint16_t)(node) & APP_NODE_GROUP_MASK)                      //!< Macro to extract the node group from a node ID
#define APP_NODE_VALUE(node) (uint16_t)((uint16_t)(node) & APP_NODE_VALUE_MASK)                      //!< Macro to extract the node value from a node ID

typedef enum
{
    APP_NODE_ID_NONE = 0,
    APP_NODE_ID_RE1_MODE = APP_NODE_RE1(0U),
    APP_NODE_ID_RE1_VOLUME = APP_NODE_RE1(1U),
    APP_NODE_ID_RE1_PROGRAM = APP_NODE_RE1(2U),
    APP_NODE_ID_RE2_CHANNEL = APP_NODE_RE2(0U),
    APP_NODE_ID_RE2_REVERB = APP_NODE_RE2(1U),
} app_node_id_t;

#endif
