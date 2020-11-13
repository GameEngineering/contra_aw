#ifndef CONTRA_PLAYER_H
#define CONTRA_PLAYER_H

#include <gs.h>

#include "component.h"
#include "asset_manager.h"
#include "sprite.h"
#include "aabb.h"

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
	gs_vqs transform;
	gs_vec2 velocity;
	aabb_t aabb;
	sprite_frame_animation_t animations[ player_state_count ];	// Animations player states
	f32 heading;
	player_state_t state;
	f32 speed;
} player_t;

#define player_set_state( player, lower, upper, gun )\
	(player).state = player_state( lower, upper, gun )

b32 player_is_grounded( player_t* player );
b32 player_is_moving( player_t* player );
const char* player_state_to_string( player_t* player );
void player_init( player_t* player, asset_manager_t* am );
gs_vec2 player_get_bullet_velocity( player_t* player );
gs_vec2 player_get_bullet_offset( player_t* player );
void player_update_aabb( player_t* player );
void player_update( player_t* player, game_context_t* ctx );

#endif