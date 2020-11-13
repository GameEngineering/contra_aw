#ifndef CONTRA_GAME_CONTEXT_H
#define CONTRA_GAME_CONTEXT_H

#include <gs.h>

#include "entity.h"
#include "player.h"
#include "asset_manager.h"
#include "entity_groups.h"

typedef struct entity_set_t
{
	entity_group( bullet_t ) bullets;
} entity_set_t;

typedef struct game_context_t
{
	player_t 				player;
	entity_set_t 			entities;
	gs_camera_t 			camera;
	gs_quad_batch_t 		player_batch;
	gs_quad_batch_t 		background_batch;
	gs_command_buffer_t 	cb;
	gs_frame_buffer_t 		fb;
	gs_texture_t 			rt;
	asset_manager_t 		am;
	b8 						show_debug_window;
	gs_dyn_array( aabb_t )  collision_objects;
} game_context_t;

_force_inline
void game_context_init( game_context_t* ctx )
{
	// Initialize bullet group
	entity_group_init( bullet_t, &ctx->entities.bullets );
}

_force_inline
void game_context_update( game_context_t* ctx )
{
	entity_group_update( bullet_t, &ctx->entities.bullets );	
}

#endif