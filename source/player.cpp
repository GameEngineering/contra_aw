#include "player.h"
#include "asset_manager.h"
#include "game_context.h"

b32 player_is_grounded( player_t* player )
{
	return (player->velocity.y == 0.f);
}

b32 player_is_moving( player_t* player )
{
	return (player->velocity.x != 0.f );
}

const char* player_state_to_string( player_state_t state )
{
	switch ( state )
	{
		case player_state(running, gun_forward, not_firing): 	return "player_state_running_gun_forward_not_firing"; break;
		case player_state(running, gun_down, not_firing): 		return "player_state_running_gun_down_not_firing"; break;
		case player_state(running, gun_up, not_firing): 		return "player_state_running_gun_up_not_firing"; break;
		case player_state(jumping, null, null): 				return "player_state_jumping_null_null"; break;
		case player_state(idle, gun_forward, not_firing): 		return "player_state_idle_gun_forward_not_firing"; break;
		case player_state(idle, gun_up, not_firing): 			return "player_state_idle_gun_up_not_firing"; break;
		case player_state(idle_prone, gun_forward, not_firing): return "player_state_idle_prone_gun_forward_not_firing"; break;
		default: 												return "unknown";	
	}

	return "unknown";
}

void player_init( player_t* player, asset_manager_t* am )
{
	player->speed = 0.05f;
	player->transform = gs_vqs_default();
	player->transform.scale = gs_vec3_scale(v3(1.f, 1.f, 1.f), 1.f / 68.f );
	player->transform.position = v3(0.f, 0.5f, 0.f);
	player->velocity = v2(0.f, 0.f);

	player->heading = 1.f;
	player_set_state( *player, idle, gun_forward, not_firing );
	gs_texture_t tex = asset_manager_get( *am, gs_texture_t, "contra_player_sprite" );
	sprite_frame_animation_t* anim = NULL;

	// Animation Component
	player->animation_comp.animation = asset_manager_get( *am, sprite_frame_animation_asset_t, player_state_to_string(player->state) ); 
}

gs_vec2 player_get_bullet_velocity( player_t* player )
{
	switch ( player->state )
	{
		case player_state(running, gun_forward, not_firing): 	return v2(1.f, 0.f); break;
		case player_state(running, gun_down, not_firing): 		return v2(1.8f, -0.8f); break;
		case player_state(running, gun_up, not_firing): 		return v2(1.5f, 0.8f); break;
		case player_state(jumping, null, null): 				return v2(1.f, 0.f); break;
		case player_state(idle, gun_forward, not_firing): 		return v2(1.f, 0.f); break;
		case player_state(idle, gun_up, not_firing): 			return v2(0.f, 1.f); break;
		case player_state(idle_prone, gun_forward, not_firing): return v2(1.f, 0.f); break;
		default: 												return v2(1.f, 0.f); break;	
	}
}

gs_vec2 player_get_bullet_offset( player_t* player )
{
	gs_vec3 offset = v3( 0.5f * player->heading, 0.2f, 0.f );

	switch ( player->state )
	{
		case player_state(running, gun_forward, not_firing): 	return v2(0.55f, 0.2f); break;
		case player_state(running, gun_down, not_firing): 		return v2(0.4f, -0.3f); break;
		case player_state(running, gun_up, not_firing): 		return v2(0.4f, 0.7f); break;
		case player_state(jumping, null, null): 				return v2(0.5f, 0.2f); break;
		case player_state(idle, gun_forward, not_firing): 		return v2(0.5f, 0.2f); break;
		case player_state(idle, gun_up, not_firing): 			return v2(0.1f, 1.0f); break;
		case player_state(idle_prone, gun_forward, not_firing): return v2(0.6f, -0.05f); break;
		default: 												return v2(0.5f, 0.2f); break;	
	}
}

f32 player_get_animation_height_offset( player_t* player )
{
	switch ( player->state )
	{
		case player_state(running, gun_up, not_firing): 		return -5.f; break;
		case player_state(idle, gun_up, not_firing): 			return -28.f; break;
		default: 												return 0.f; break;	
	}
}

f32 player_get_animation_aabb_scale( player_t* player )
{
	switch ( player->state )
	{
		case player_state(idle_prone, gun_forward, not_firing): return 18.f; break;
		case player_state(jumping, null, null): 				return 15.f; break;
		default: 												return 45.f; break;	
	}
}

void player_update_aabb( player_t* player )
{
	f32 t = gs_engine_instance()->ctx.platform->elapsed_time();
	sprite_animation_component_t* ac = &player->animation_comp;
	sprite_frame_animation_asset_t* anim = ac->animation;
	u32 anim_frame_count = gs_dyn_array_size( anim->frames );
	sprite_frame_t* s = &anim->frames[ac->current_frame];
	gs_vec4 uvs = s->uvs;

	// Depending on the state, need to get certain "offsets" for the player's aabb

	// Width and height of UVs to scale the quads
	f32 tw = fabsf(uvs.z - uvs.x);
	f32 th = fabsf(uvs.w - uvs.y + player_get_animation_height_offset(player));

	// Player's transform
	gs_vqs xform = gs_vqs_default();
	xform.position = player->transform.position;
	xform.scale = gs_vec3_scale(v3(tw, player_get_animation_aabb_scale(player), 1.f), gs_vec3_len(player->transform.scale));
	xform.scale.z = 1.f;

	// Define the object space quad for our player
	gs_vec4 bl = v4(-0.5f, -0.5f, 0.f, 1.f);
	gs_vec4 tr = v4(0.5f, 0.5f, 0.f, 1.f);

	// Define matrices for transformations
	gs_mat4 model_mtx = gs_vqs_to_mat4( &xform );

	// Transform verts
	bl = gs_mat4_mul_vec4( model_mtx, bl );			
	tr = gs_mat4_mul_vec4( model_mtx, tr );

	player->aabb.min = v2(bl.x, bl.y);
	player->aabb.max = v2(tr.x, tr.y);
}

void player_update( player_t* player, game_context_t* ctx )
{
	gs_platform_i* platform = gs_engine_instance()->ctx.platform;

	/*============
	// Movement
	============*/

	gs_vec2 dir = v2(0.f, 0.f);

	// No input -> idle
	if ( platform->key_down( gs_keycode_a ) )
	{
		dir.x -= 1.f;
		if ( player_is_grounded(player) ) {
			player_set_state( *player, running, gun_forward, not_firing );
		}
		player->heading = -1.f;
	}
	if ( platform->key_down( gs_keycode_d ) )
	{
		dir.x += 1.f;
		if ( player_is_grounded(player) ) {
			player_set_state( *player, running, gun_forward, not_firing );
		}
		player->heading = 1.f;
	}

		// Normalize player velocity
	dir = gs_vec2_norm( dir );
	dir.x *= player->speed;
	player->velocity.x = dir.x;

	// Need to set to jump state
	if ( platform->key_pressed( gs_keycode_space ) && player_is_grounded(player) ) 
	{
		player->velocity.y = 0.25f;
		player_set_state( *player, jumping, null, null );
	}

	// if (player->state != player_state(jumping, null, null) && gs_vec2_len(player->velocity) == 0.f)
	// {
	// 	player_set_state( *player, idle, gun_forward, not_firing );
	// }

	// Add gravity to player's velocity
	player->velocity.y -= 0.015f;

	// Move player based on normalized direction
	player->transform.position = gs_vec3_add(player->transform.position, v3(player->velocity.x, player->velocity.y, 0.f));

	/*=============
	// AABB Update
	=============*/

	player_update_aabb( player );

	/*=============
	// Collisions
	=============*/

	// Check with floor
	aabb_t ground = gs_default_val();
	ground.min = v2(player->aabb.min.x - 100.f, -10.f);
	ground.max = v2(player->aabb.min.x + 100.f, 0.f);
	if ( aabb_vs_aabb( &player->aabb, &ground ) )
	{
		// Get mvt then move player by mtv	
		gs_vec2 mtv = aabb_aabb_mtv( &player->aabb, &ground );
		gs_println( "mtv: %.2f, %.2f", mtv.x, mtv.y);
		player->transform.position = gs_vec3_add( player->transform.position, v3(mtv.x, mtv.y, 0.f) );

		if ( mtv.y != 0.f ) {
			player->velocity.y = 0.f;
		}

		// Reupdate aabb
		player_update_aabb( player );
	}

	// Check against world
	gs_for_range_i( gs_dyn_array_size( ctx->collision_objects ) )
	{
		if ( aabb_vs_aabb( &ctx->player.aabb, &ctx->collision_objects[i] ) )
		{
			// Get mvt then move player by mtv	
			gs_vec2 mtv = aabb_aabb_mtv( &ctx->player.aabb, &ctx->collision_objects[i] );
			player->transform.position = gs_vec3_add( player->transform.position, v3(mtv.x, mtv.y, 0.f) );

			if ( mtv.y != 0.f ) {
				player->velocity.y = 0.f;
			}

			// Reupdate aabb
			player_update_aabb( player );
		}
	}

	// If player is grounded
	if ( player_is_grounded( player ) ) 
	{ 
		if ( !player_is_moving( player ) )
		{
			if ( platform->key_down( gs_keycode_s ) )
			{
				player_set_state( *player, idle_prone, gun_forward, not_firing );
			}
			else if ( platform->key_down( gs_keycode_w ) )
			{
				player_set_state( *player, idle, gun_up, not_firing );
			}
			
			else 
			{
				player_set_state( *player, idle, gun_forward, not_firing );
			}
		}
		else 
		{
			if ( platform->key_down( gs_keycode_w ) )
			{
				player_set_state( *player, running, gun_up, not_firing );
			}
			if ( platform->key_down( gs_keycode_s ) )
			{
				player_set_state( *player, running, gun_down, not_firing );
			}
		}
	}

	player_update_aabb( player );

	// Shooty Shoots
	static b32 firing = false;
	if ( platform->mouse_down( gs_mouse_lbutton ) )
	{
		static f32 _t = 0.f;	
		_t += 0.1f;
		b32 fire = false;
		f32 rate_of_fire = 0.25f;

		if ( _t > rate_of_fire || !firing )
		{
			_t = 0.f;	
			fire = true;
			firing = true;
		}

		// Add a bullet
		if ( fire )
		{
			gs_vec2 bv = player_get_bullet_velocity( player );
			gs_vec2 bo = player_get_bullet_offset( player );

			bullet_data bd = gs_default_val();
			bd.position = gs_vec3_add( v3(bo.x * player->heading, bo.y, 0.f), player->transform.position );
			bd.velocity = bv;
			bd.velocity.x *= ctx->player.heading;
			entity_group_add( bullet_t, &ctx->entities.bullets, &bd );
		}
	}

	if ( platform->mouse_released( gs_mouse_lbutton ) )
	{
		firing = false;
	}

	// Set animation based on player state
	player->animation_comp.animation = asset_manager_get( ctx->am, sprite_frame_animation_asset_t, player_state_to_string(player->state) );

	// Tick animation based on state
	static s32 cur_frame = 0;
	static f32 _t = 0.f;
	sprite_animation_component_t* ac = &player->animation_comp;
	sprite_frame_animation_asset_t* anim = ac->animation;
	u32 anim_frame_count = gs_dyn_array_size( anim->frames );
	_t += ac->animation->speed;
	if ( _t >= 1.f )
	{
		_t = 0.f;
		cur_frame++;
	}
	
	// Render player animation
	ac->current_frame = cur_frame % anim_frame_count;
}
