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

const char* player_state_to_string( player_t* player )
{
	switch ( player->state )
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

	/* Player State: Running, Gun Forward, Not Firing */
	anim = &player->animations[ player_state(running, gun_forward, not_firing) ];
	anim->frames = gs_dyn_array_new( sprite_frame_t );
	anim->speed = 0.1f;
	gs_dyn_array_push( anim->frames, sprite_frame_t_new( tex, v4(113.f, 81.f, 146.f, 120.f) ) );
	gs_dyn_array_push( anim->frames, sprite_frame_t_new( tex, v4(148.f, 81.f, 185.f, 120.f) ) );
	gs_dyn_array_push( anim->frames, sprite_frame_t_new( tex, v4(2.f, 81.f, 37.f, 120.f) ) );
	gs_dyn_array_push( anim->frames, sprite_frame_t_new( tex, v4(43.f, 82.f, 72.f, 120.f) ) );
	gs_dyn_array_push( anim->frames, sprite_frame_t_new( tex, v4(75.f, 81.f, 111.f, 120.f) ) );
	gs_dyn_array_push( anim->frames, sprite_frame_t_new( tex, v4(189.f, 82.f, 228.f, 120.f) ) );

	/* Player State: Running, Gun Up, Not Firing */
	anim = &player->animations[ player_state(running, gun_up, not_firing) ];
	anim->frames = gs_dyn_array_new( sprite_frame_t );
	anim->speed = 0.1f;
	gs_dyn_array_push( anim->frames, sprite_frame_t_new( tex, v4(103.f, 172.f, 126.f, 222.f) ) );
	gs_dyn_array_push( anim->frames, sprite_frame_t_new( tex, v4(129.f, 172.f, 156.f, 222.f) ) );
	gs_dyn_array_push( anim->frames, sprite_frame_t_new( tex, v4(8.f, 172.f, 35.f, 222.f) ) );
	gs_dyn_array_push( anim->frames, sprite_frame_t_new( tex, v4(42.f, 171.f, 65.f, 222.f) ) );
	gs_dyn_array_push( anim->frames, sprite_frame_t_new( tex, v4(70.f, 171.f, 97.f, 222.f) ) );
	gs_dyn_array_push( anim->frames, sprite_frame_t_new( tex, v4(6.f, 226.f, 44.f, 277.f) ) );

	/* Player State: Running, Gun Down, Not Firing */
	anim = &player->animations[ player_state(running, gun_down, not_firing) ];
	anim->frames = gs_dyn_array_new( sprite_frame_t );
	anim->speed = 0.1f;
	gs_dyn_array_push( anim->frames, sprite_frame_t_new( tex, v4(120.f, 280.f, 146.f, 321.f) ) );
	gs_dyn_array_push( anim->frames, sprite_frame_t_new( tex, v4(155.f, 280.f, 182.f, 321.f) ) );
	gs_dyn_array_push( anim->frames, sprite_frame_t_new( tex, v4(46.f, 230.f, 74.f, 271.f) ) );
	gs_dyn_array_push( anim->frames, sprite_frame_t_new( tex, v4(82.f, 230.f, 107.f, 271.f) ) );
	gs_dyn_array_push( anim->frames, sprite_frame_t_new( tex, v4(113.f, 230.f, 140.f, 272.f) ) );
	gs_dyn_array_push( anim->frames, sprite_frame_t_new( tex, v4(184.f, 280.f, 220.f, 322.f) ) );

	/* Player State: Idle, Gun Forward, Not Firing */
	anim = &player->animations[ player_state(idle, gun_forward, not_firing) ];
	anim->frames = gs_dyn_array_new( sprite_frame_t );
	anim->speed = 0.1f;
	gs_dyn_array_push( anim->frames, sprite_frame_t_new( tex, v4(35.f, 2.f, 70.f, 43.f) ) );

	/* Player State: Idle, Gun Up, Not Firing */
	anim = &player->animations[ player_state(idle, gun_up, not_firing) ];
	anim->frames = gs_dyn_array_new( sprite_frame_t );
	anim->speed = 0.1f;
	gs_dyn_array_push( anim->frames, sprite_frame_t_new( tex, v4(167.f, 169.f, 198.f, 238.f) ) );

	/* Player State: Idle Prone, Gun Forward, Not Firing */
	anim = &player->animations[ player_state(idle_prone, gun_forward, not_firing) ];
	anim->frames = gs_dyn_array_new( sprite_frame_t );
	anim->speed = 0.1f;
	gs_dyn_array_push( anim->frames, sprite_frame_t_new( tex, v4(114.f, 26.f, 158.f, 41.f) ) );

	/* Player State: Jumping, Null, Null */
	anim = &player->animations[ player_state(jumping, null, null) ];
	anim->frames = gs_dyn_array_new( sprite_frame_t );
	anim->speed = 0.8f;
	gs_dyn_array_push( anim->frames, sprite_frame_t_new( tex, v4(0.f, 49.f, 25.f, 72.f) ) );
	gs_dyn_array_push( anim->frames, sprite_frame_t_new( tex, v4(30.f, 49.f, 49.f, 72.f) ) );
	gs_dyn_array_push( anim->frames, sprite_frame_t_new( tex, v4(59.f, 49.f, 82.f, 72.f) ) );
	gs_dyn_array_push( anim->frames, sprite_frame_t_new( tex, v4(93.f, 49.f, 112.f, 72.f) ) );
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

void player_update_aabb( player_t* player )
{
	f32 t = gs_engine_instance()->ctx.platform->elapsed_time();
	sprite_frame_animation_t* anim = &player->animations[player->state];
	u32 anim_frame_count = gs_dyn_array_size( anim->frames );
	sprite_frame_t* s = &anim->frames[anim->current_frame];
	gs_vec4 uvs = s->uvs;

	// Width and height of UVs to scale the quads
	f32 tw = fabsf(uvs.z - uvs.x);
	f32 th = fabsf(uvs.w - uvs.y);

	// Player's transform
	gs_vqs xform = gs_vqs_default();
	xform.position = player->transform.position;
	xform.scale = gs_vec3_scale(v3(tw, th, 1.f), gs_vec3_len(player->transform.scale));
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

	if (player->state != player_state(jumping, null, null) && gs_vec2_len(player->velocity) == 0.f)
	{
		player_set_state( *player, idle, gun_forward, not_firing );
	}

	// Add gravity to player's velocity
	player->velocity.y -= 0.015f;

	// Move player based on normalized direction
	player->transform.position = gs_vec3_add(player->transform.position, v3(player->velocity.x, player->velocity.y, 0.f));

	if ( player->transform.position.y <= 0.5f )
	{
		player->transform.position.y = 0.5f;
		player->velocity.y = 0.f;
	}

	/*=============
	// AABB Update
	=============*/

	player_update_aabb( player );

	/*=============
	// Collisions
	=============*/

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
				player->transform.position.y -= 0.3f;
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
}
