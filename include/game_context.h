#ifndef CONTRA_GAME_CONTEXT_H
#define CONTRA_GAME_CONTEXT_H

#include <gs.h>

#include "entity.h"
#include "player.h"
#include "asset_manager.h"
#include "entity_groups.h"

typedef struct entity_set_t
{
	entity_group(bullet_t) 	bullets;
	entity_group(red_guy_t) red_guys;
} entity_set_t;

// typedef struct entity_t {} entity_t;

// typedef struct entity_manager_t
// {
// 	gs_slot_map(u32, transform_component_t) 		transform_components;
// 	gs_slot_map(u32, rigid_body_component_t) 		rigid_body_components;
// 	gs_slot_map(u32, sprite_animation_component_t) 	sprite_animation_components;
// } entity_manager_t;

/*
	void update_bullet_system()
	{
		// Look at specific components? Would need to update particular entities that fit this "group"
	}
*/

typedef struct game_context_t
{
	player_t 				player;
	entity_set_t 			entities;
	gs_camera_t 			camera;
	gs_quad_batch_t 		foreground_batch;
	gs_quad_batch_t 		background_batch;
	gs_quad_batch_t 		enemy_batch;
	gs_command_buffer_t 	cb;
	gs_frame_buffer_t 		fb;
	gs_texture_t 			rt;
	asset_manager_t 		am;
	b8 						show_debug_window;
	gs_dyn_array( aabb_t )  collision_objects;
	gs_handle_audio_instance bg_music;
} game_context_t;

void game_context_init( game_context_t* ctx );
void game_context_initialize_assets( game_context_t* ctx );
void game_context_update( game_context_t* ctx );

#endif