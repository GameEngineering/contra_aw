#ifndef CONTRA_PLAYER_H
#define CONTRA_PLAYER_H

#include <gs.h>

#include "component.h"
#include "asset_manager.h"
#include "sprite.h"
#include "aabb.h"
#include "entity_groups.h"

// Forward Decls.
struct asset_manager_t;
struct game_context_t;

#define player_state( lower, upper, gun )\
	player_state_##lower##_##upper##_##gun

typedef enum player_state_t
{
	player_state(running, gun_forward, not_firing),
	player_state(running, gun_up, not_firing),
	player_state(running, gun_down, not_firing),
	player_state(idle, gun_forward, not_firing),
	player_state(idle, gun_up, not_firing),
	player_state(idle_prone, gun_forward, not_firing),
	player_state(jumping, null, null),
	player_state_count
} player_state_t;

typedef struct player_t
{
	transform_component_t 			xform_comp;
	rigid_body_component_t 			rigid_body_comp;
	sprite_animation_component_t 	animation_comp;

	gs_vqs transform;
	gs_vec2 velocity;
	aabb_t aabb;
	sprite_frame_animation_t animations[ player_state_count ];	// Animations player states
	f32 heading;
	player_state_t state;
	f32 speed;
} player_t;

#define player_set_state( player, lower, upper, gun )\
do {\
	(player).state = player_state( lower, upper, gun );\
} while (0)


b32 player_is_grounded( player_t* player );
b32 player_is_moving( player_t* player );
const char* player_state_to_string( player_state_t state );
void player_init( player_t* player, asset_manager_t* am );
gs_vec2 player_get_bullet_velocity( player_t* player );
gs_vec2 player_get_bullet_offset( player_t* player );
void player_update_aabb( player_t* player );
void player_update( player_t* player, game_context_t* ctx );

#endif