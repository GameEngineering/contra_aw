#ifndef CONTRA_COMPONENT_H
#define CONTRA_COMPONENT_H

#include <gs.h>

typedef enum component_state
{
	component_state_inactive,
	component_state_active,
	component_state_count
} component_state;

typedef struct component_t
{
	component_state state;
} component_t;

#endif