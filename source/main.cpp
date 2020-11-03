#include <gs.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>
#include <GLFW/glfw3.h>

// Project Includes
#include "asset_manager.h"

// Useful Defines
#define use_animation_time 	true

gs_vec4 __gs_vec4_new( f32 x, f32 y, f32 z, f32 w )
{
	gs_vec4 v = gs_default_val();
	v.x = x;
	v.y = y;
	v.z = z;
	v.w = w;
	return v;
}

#define v4( x, y, z, w )\
	__gs_vec4_new( x, y, z, w )

gs_vec3 __gs_vec3_new( f32 x, f32 y, f32 z )
{
	gs_vec3 v = gs_default_val();
	v.x = x;
	v.y = y;
	v.z = z;
	return v;
}

#define v3( x, y, z )\
	__gs_vec3_new( x, y, z )

gs_vec2 __gs_vec2_new( f32 x, f32 y )
{
	gs_vec2 v = gs_default_val();
	v.x = x;
	v.y = y;
	return v;
}

#define v2( x, y )\
	__gs_vec2_new( x, y )

#define sprite_col_span 8.f
#define sprite_row_span 8.f
#define ground_level 0.f

typedef struct sprite_frame_t
{
	gs_vec4 uvs;
	gs_texture_t texture;	// Source texture atlas
} sprite_frame_t;

typedef struct sprite_frame_animation_t
{
	gs_dyn_array( sprite_frame_t ) frames;
	u32 current_frame;
	f32 speed;
} sprite_frame_animation_t;

// AABBs
typedef struct aabb_t
{
	gs_vec2 min;
	gs_vec2 max;
} aabb_t;

gs_vec2 aabb_aabb_mtv( aabb_t* a0, aabb_t* a1 )
{
	gs_vec2 diff = v2(a0->min.x - a1->min.x, a0->min.y - a1->min.y);	

	f32 l, r, b, t;
	gs_vec2 mtv = v2(0.f, 0.f);

	l = a1->min.x - a0->max.x;
	r = a1->max.x - a0->min.x;
	b = a1->min.y - a0->max.y;
	t = a1->max.y - a0->min.y;

	mtv.x = fabsf(l) > r ? r : l;
	mtv.y = fabsf(b) > t ? t : b;

	if ( fabsf(mtv.x) <= fabsf(mtv.y)) {
		mtv.y = 0.f;
	} else {
		mtv.x = 0.f;
	}
	
	return mtv;
}

// 2D AABB collision detection (rect. vs. rect.)
b32 aabb_vs_aabb( aabb_t* a, aabb_t* b )
{
	if ( a->max.x > b->min.x && 
		 a->max.y > b->min.y && 
		 a->min.x < b->max.x && 
		 a->min.y < b->max.y )
	{
		return true;
	}

	return false;
}

gs_vec4 aabb_window_coords( aabb_t* aabb, gs_camera_t* camera )
{
	gs_platform_i* platform = gs_engine_instance()->ctx.platform;
	gs_vec2 ws = platform->window_size( platform->main_window() );

	// AABB of the player
	gs_vec4 bounds = gs_default_val();
	gs_vec4 tl = v4(aabb->min.x, aabb->min.y, 0.f, 1.f);
	gs_vec4 br = v4(aabb->max.x, aabb->max.y, 0.f, 1.f);

	gs_mat4 view_mtx = gs_camera_get_view( camera );
	gs_mat4 proj_mtx = gs_camera_get_projection( camera, ws.x, ws.y );
	gs_mat4 vp = gs_mat4_mul( proj_mtx, view_mtx );

	// Transform verts
	tl = gs_mat4_mul_vec4( vp, tl );			
	br = gs_mat4_mul_vec4( vp, br );

	// Perspective divide	
	tl = gs_vec4_scale( tl, 1.f / tl.w );
	br = gs_vec4_scale( br, 1.f / br.w );

	// NDC [0.f, 1.f] and NDC
	tl.x = (tl.x * 0.5f + 0.5f);
	tl.y = (tl.y * 0.5f + 0.5f);
	br.x = (br.x * 0.5f + 0.5f);
	br.y = (br.y * 0.5f + 0.5f);

	// Window Space
	tl.x = tl.x * ws.x;
	tl.y = gs_map_range( 1.f, 0.f, 0.f, 1.f, tl.y ) * ws.y;
	br.x = br.x * ws.x;
	br.y = gs_map_range( 1.f, 0.f, 0.f, 1.f, br.y ) * ws.y;

	bounds = v4(tl.x, tl.y, br.x, br.y);

	return bounds;
}

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

sprite_frame_t sprite_frame_t_new( gs_texture_t tex, gs_vec4 uv )
{
	sprite_frame_t frame = gs_default_val();
	frame.texture = tex;
	frame.uvs = uv;
	return frame;
}

#define player_set_state( player, lower, upper, gun )\
	(player).state = player_state( lower, upper, gun )	

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

// Forward Decl.
void player_update( player_t* player );

// Forward Decls.
void camera_update();

// Global Decls.
_global gs_camera_t 			g_camera = gs_default_val();
_global gs_quad_batch_t 		g_player_batch = gs_default_val();
_global gs_quad_batch_t 		g_background_batch = gs_default_val();
_global gs_command_buffer_t 	g_cb = gs_default_val();
_global gs_frame_buffer_t 		g_fb = gs_default_val();
_global gs_texture_t 			g_rt = gs_default_val();
_global asset_manager_t 		g_am = gs_default_val();
_global player_t 				g_player = gs_default_val();
_global b8 						g_show_debug_window = false;

_global gs_dyn_array( aabb_t )  g_collision_objects = gs_default_val();

// Forward Decls.
gs_result app_init();
gs_result app_update();		// Use to update your application

void imgui_init();
void imgui_new_frame();
void imgui_render();
void do_ui();

int main( int argc, char** argv )
{
	// This is our app description. It gives internal hints to our engine for various things like 
	// window size, title, as well as update, init, and shutdown functions to be run. 
	gs_application_desc app = {0};
	app.window_title 		= "Contra 3";
	app.window_width 		= (s32)(1920.f * 0.7f);
	app.window_height 		= (s32)(1080.f * 0.7f);
	app.init 				= &app_init;
	app.update 				= &app_update;

	// Construct internal instance of our engine
	gs_engine* engine = gs_engine_construct( app );

	// Run the internal engine loop until completion
	gs_result res = engine->run();

	// Check result of engine after exiting loop
	if ( res != gs_result_success ) 
	{
		gs_println( "Error: Engine did not successfully finish running." );
		return -1;
	}

	gs_println( "Gunslinger exited successfully." );

	return 0;	
}

gs_result app_init()
{
	// Cache apis
	gs_platform_i* platform = gs_engine_instance()->ctx.platform;
	gs_graphics_i* gfx = gs_engine_instance()->ctx.graphics;
	gs_vec2 fbs = platform->frame_buffer_size( platform->main_window() );

	// Construct command buffer for grahics ops
	g_cb = gs_command_buffer_new();

	gs_texture_parameter_desc t_desc = gs_texture_parameter_desc_default();
	t_desc.texture_format = gs_texture_format_rgba8;
	t_desc.mag_filter = gs_nearest;
	t_desc.min_filter = gs_nearest;
	t_desc.generate_mips = false;
	t_desc.width = fbs.x;
	t_desc.height = fbs.y;
	t_desc.num_comps = 4;
	t_desc.data = NULL;

	// Construct render target
	g_rt = gfx->construct_texture( t_desc );

	// Construct frame buffer
	g_fb = gfx->construct_frame_buffer( g_rt );

	// Construct asset manager
	g_am = asset_manager_new();

	gs_texture_parameter_desc desc = gs_texture_parameter_desc_default();
	desc.min_filter = gs_nearest;
	desc.mag_filter = gs_nearest;
	desc.generate_mips = false;

	// Place them assets
	asset_manager_load( g_am, gs_texture_t, "./assets/contra_player_sprite.png", desc );
	asset_manager_load( g_am, gs_texture_t, "./assets/bg_elements.png", desc );

	// Construct quad batch api and link up function pointers
	g_player_batch = gs_quad_batch_new( NULL );
	g_background_batch = gs_quad_batch_new( NULL );

	// Set material texture uniform
	gfx->set_material_uniform_sampler2d( g_player_batch.material, "u_tex", 
		asset_manager_get( g_am, gs_texture_t, "contra_player_sprite" ), 0 );

	gfx->set_material_uniform_sampler2d( g_background_batch.material, "u_tex", 
		asset_manager_get( g_am, gs_texture_t, "bg_elements" ), 0 );

	// Construct camera parameters
	g_camera.transform = gs_vqs_default();
	g_camera.transform.position = v3(0.f, 3.1f, -1.f);
	g_camera.fov = 60.f;
	g_camera.near_plane = 0.1f;
	g_camera.far_plane = 1000.f;
	g_camera.ortho_scale = 3.7f;
	g_camera.proj_type = gs_projection_type_orthographic;

	// Intialize player
	player_init( &g_player, &g_am );

	imgui_init();

	// Init aabb collision struct
	g_collision_objects = gs_dyn_array_new( aabb_t );
	gs_for_range_i( 100 )
	{
		aabb_t aabb = gs_default_val();
		aabb.min = v2((f32)i * 2.f, 0.f);
		aabb.max = gs_vec2_add(aabb.min, v2(1.f, 1.f));
		gs_dyn_array_push( g_collision_objects, aabb );
	}

	return gs_result_success;
}

// Update your application here
gs_result app_update()
{
	// Grab global instance of engine, cache other pointers to interfaces
	gs_engine* engine = gs_engine_instance();
	gs_platform_i* platform = engine->ctx.platform;
	gs_graphics_i* gfx = engine->ctx.graphics;
	gs_command_buffer_t* cb = &g_cb;
	gs_quad_batch_t* qb = &g_player_batch;

	// If we press the escape key, exit the application
	if ( platform->key_pressed( gs_keycode_esc ) )
	{
		return gs_result_success;
	}

	if ( platform->key_pressed( gs_keycode_i ) )
	{
		g_show_debug_window = !g_show_debug_window;
	}

	const f32 dt = platform->time.delta;
	const f32 t = platform->elapsed_time();

	camera_update();
	player_update( &g_player );

	static s32 cur_frame = 0;
	#if use_animation_time
			static f32 _t = 0.f;
			_t += g_player.animations[g_player.state].speed;
			if ( _t >= 1.f )
			{
				_t = 0.f;
				cur_frame++;
			}
	#endif

	f32 scale_factor = gs_vec3_len( g_player.transform.scale );

	// Player
	gfx->quad_batch_begin( qb );
	{
		sprite_frame_animation_t* anim = &g_player.animations[g_player.state];
		u32 anim_frame_count = gs_dyn_array_size( anim->frames );

		// Render player animation
		anim->current_frame = cur_frame % anim_frame_count;

		sprite_frame_t* s = &anim->frames[anim->current_frame];
		f32 w = (f32)s->texture.width;
		f32 h = (f32)s->texture.height;
		gs_vec4 uvs = s->uvs;

		// Need UV information for tile in texture
		f32 l = g_player.heading == 1.f ? (uvs.x / w) : (uvs.z / w);
		f32 t = 1.f - (uvs.y / h);
		f32 r = g_player.heading == 1.f ? (uvs.z / w) : (uvs.x / w);
		f32 b = 1.f - (uvs.w / h);

		// Width and height of UVs to scale the quads
		f32 tw = fabsf(uvs.z - uvs.x);
		f32 th = fabsf(uvs.w - uvs.y);

		gs_default_quad_info_t quad_info = {0};
		quad_info.transform = g_player.transform;
		quad_info.transform.scale = gs_vec3_scale(v3(tw, th, 1.f), scale_factor);
		quad_info.uv = v4(l, b, r, t);
		quad_info.color = v4(1.f, 1.f, 1.f, 1.f);
		gfx->quad_batch_add( qb, &quad_info );
	}
	gfx->quad_batch_end( qb );

	// Background
	qb = &g_background_batch;
	gfx->quad_batch_begin( qb );
	{
		// Loop through tiled backgrounds
		gs_texture_t bg_tex = asset_manager_get( g_am, gs_texture_t, "bg_elements" );
		f32 w = (f32)bg_tex.width;
		f32 h = (f32)bg_tex.height;

		// Floor background layer
		gs_for_range_i( 1000 )
		{
			// Draw single quad for background batch for now	
			gs_vec4 bg_uv = v4(261.f, 22.f, 268.f, 109.f);

			// Need UV information for tile in texture
			f32 l = bg_uv.x / w;
			f32 t = 1.f - (bg_uv.y / h);
			f32 r = bg_uv.z / w;
			f32 b = 1.f - (bg_uv.w / h);

			// Width and height of UVs to scale the quads
			f32 tw = fabsf(bg_uv.z - bg_uv.x);
			f32 th = fabsf(bg_uv.w - bg_uv.y);

			gs_vec3 origin = v3(-5.f, 0.5f, 0.f);
			gs_vec3 cam_origin = g_camera.transform.position;

			gs_default_quad_info_t quad_info = {0};
			quad_info.transform = gs_vqs_default();
			quad_info.transform.scale = gs_vec3_scale(v3(tw, th, 1.f), scale_factor);
			quad_info.transform.position = v3(origin.x + tw * scale_factor * i, origin.y, 0.f);
			quad_info.uv = v4(l, b, r, t);
			quad_info.color = v4(1.f, 1.f, 1.f, 1.f);
			gfx->quad_batch_add( qb, &quad_info );
		}

		// Background layer sky
		gs_for_range_i( 1000 )
		{
			// Draw single quad for background batch for now	
			gs_vec4 bg_uv = v4(259.f, 114.f, 291.f, 255.f);

			// Need UV information for tile in texture
			f32 l = bg_uv.x / w;
			f32 t = 1.f - (bg_uv.y / h);
			f32 r = bg_uv.z / w;
			f32 b = 1.f - (bg_uv.w / h);

			// Width and height of UVs to scale the quads
			f32 tw = fabsf(bg_uv.z - bg_uv.x);
			f32 th = fabsf(bg_uv.w - bg_uv.y);

			gs_vec3 origin = v3(-5.f, 3.5f, 0.f);
			gs_vec3 cam_origin = g_camera.transform.position;

			gs_default_quad_info_t quad_info = {0};
			quad_info.transform = gs_vqs_default();
			quad_info.transform.scale = gs_vec3_scale(v3(tw, th, 1.f), scale_factor);
			quad_info.transform.position = v3(origin.x + tw * scale_factor * i + cam_origin.x * 0.95f, origin.y, 0.f);
			quad_info.uv = v4(l, b, r, t);
			quad_info.color = v4(1.f, 1.f, 1.f, 1.f);
			gfx->quad_batch_add( qb, &quad_info );
		}

		// Background layer 0 (buildings)
		gs_for_range_i( 100 )
		{
			// Draw single quad for background batch for now	
			gs_vec4 bg_uv = v4(0.f, 22.f, 254.f, 205.f);

			// Need UV information for tile in texture
			f32 l = bg_uv.x / w;
			f32 t = 1.f - (bg_uv.y / h);
			f32 r = bg_uv.z / w;
			f32 b = 1.f - (bg_uv.w / h);

			// Width and height of UVs to scale the quads
			f32 tw = fabsf(bg_uv.z - bg_uv.x);
			f32 th = fabsf(bg_uv.w - bg_uv.y);

			f32 x_offset = -5.f;
			gs_vec3 origin = v3(-5.f, 3.5f, 0.f);
			gs_vec3 cam_origin = g_camera.transform.position;

			gs_default_quad_info_t quad_info = {0};
			quad_info.transform = gs_vqs_default();
			quad_info.transform.scale = gs_vec3_scale(v3(tw, th, 1.f), scale_factor);
			quad_info.transform.position = v3(origin.x + tw * scale_factor * i + cam_origin.x * 0.9f, origin.y, 0.f);
			quad_info.uv = v4(l, b, r, t);
			quad_info.color = v4(1.f, 1.f, 1.f, 1.f);
			gfx->quad_batch_add( qb, &quad_info );
		}

		// Background layer 1
		// NOTE(john): just fence for now
		gs_for_range_i( 100 )
		{
			// Draw single quad for background batch for now	
			gs_vec4 bg_uv = v4(0.f, 0.f, 30.f, 19.f);

			// Need UV information for tile in texture
			f32 l = bg_uv.x / w;
			f32 t = 1.f - (bg_uv.y / h);
			f32 r = bg_uv.z / w;
			f32 b = 1.f - (bg_uv.w / h);

			// Width and height of UVs to scale the quads
			f32 tw = fabsf(bg_uv.z - bg_uv.x);
			f32 th = fabsf(bg_uv.w - bg_uv.y);

			gs_vec3 cam_origin = g_camera.transform.position;
			gs_vec3 origin = v3(-5.f, 1.4f, 0.f);

			gs_default_quad_info_t quad_info = {0};
			quad_info.transform = gs_vqs_default();
			quad_info.transform.scale = gs_vec3_scale(v3(tw, th, 1.f), scale_factor);
			quad_info.transform.position = v3(origin.x + tw * scale_factor * i - 3.f + cam_origin.x * 0.0f, origin.y, 0.f);
			quad_info.uv = v4(l, b, r, t);
			quad_info.color = v4(1.f, 1.f, 1.f, 1.f);
			gfx->quad_batch_add( qb, &quad_info );
		}

		// Background layer (barrels)
		gs_for_range_i( 100 )
		{
			// Draw single quad for background batch for now	
			gs_vec4 bg_uv = v4(35.f, 3.f, 67.f, 18.f);

			// Need UV information for tile in texture
			f32 l = bg_uv.x / w;
			f32 t = 1.f - (bg_uv.y / h);
			f32 r = bg_uv.z / w;
			f32 b = 1.f - (bg_uv.w / h);

			// Width and height of UVs to scale the quads
			f32 tw = fabsf(bg_uv.z - bg_uv.x);
			f32 th = fabsf(bg_uv.w - bg_uv.y);

			gs_vec3 cam_origin = g_camera.transform.position;
			gs_vec3 origin = v3(-5.f, 0.965f, 0.f);

			gs_default_quad_info_t quad_info = {0};
			quad_info.transform = gs_vqs_default();
			quad_info.transform.scale = gs_vec3_scale(v3(tw, th, 1.f), scale_factor);
			quad_info.transform.position = v3(origin.x + tw * scale_factor * i - 3.f + cam_origin.x * 0.f, origin.y, 0.f);
			quad_info.uv = v4(l, b, r, t);
			quad_info.color = v4(1.f, 1.f, 1.f, 1.f);
			gfx->quad_batch_add( qb, &quad_info );
		}

	} 
	gfx->quad_batch_end( qb );

	/*===============
	// Render scene
	================*/

	gfx->bind_frame_buffer( cb, g_fb );
	{
		// Set the render target for the frame buffer
		gfx->set_frame_buffer_attachment( cb, g_rt, 0 );

		// Main window size
		gs_vec2 ws = platform->window_size( platform->main_window() );
		gs_vec2 fbs = platform->frame_buffer_size( platform->main_window() );

		// Set clear color and clear screen
		f32 clear_color[4] = { 0.f, 0.f, 0.f, 1.f };
		gfx->set_view_clear( cb, clear_color );
		gfx->set_view_port( cb, fbs.x, fbs.y );
		gfx->set_depth_enabled( cb, false );
		gfx->set_blend_mode( cb, gs_blend_mode_src_alpha, gs_blend_mode_one_minus_src_alpha );

		// Create model/view/projection matrices from camera
		gs_mat4 view_mtx = gs_camera_get_view( &g_camera );
		gs_mat4 proj_mtx = gs_camera_get_projection( &g_camera, ws.x, ws.y );
		gs_mat4 model_mtx = gs_mat4_scale(v3(1.f, 1.f, 1.f));

		// Draw bg
		qb = &g_background_batch;
		gfx->set_material_uniform_mat4( qb->material, "u_model", model_mtx );
		gfx->set_material_uniform_mat4( qb->material, "u_view", view_mtx );
		gfx->set_material_uniform_mat4( qb->material, "u_proj", proj_mtx );

		// Need to submit quad batch
		gfx->quad_batch_submit( cb, qb );

		// Draw player
		qb = &g_player_batch;
		gfx->set_material_uniform_mat4( qb->material, "u_model", model_mtx );
		gfx->set_material_uniform_mat4( qb->material, "u_view", view_mtx );
		gfx->set_material_uniform_mat4( qb->material, "u_proj", proj_mtx );

		// Need to submit quad batch
		gfx->quad_batch_submit( cb, qb );
	}
	gfx->unbind_frame_buffer( cb );

	// Submit command buffer for rendering
	gfx->submit_command_buffer( cb );

	// Might render entire scene into imgui texture then display that as "backbuffer"

	// ImGui editor
	imgui_new_frame();
	{
	  	do_ui();
	}
    imgui_render();

	// Otherwise, continue
	return gs_result_in_progress;
}

void imgui_init()
{
	gs_platform_i* platform = gs_engine_instance()->ctx.platform;

	// Get main window from platform
	void* win = platform->raw_window_handle( platform->main_window() );

	 // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); 
    (void)io;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer bindings
    ImGui_ImplGlfw_InitForOpenGL(win, true);
    ImGui_ImplOpenGL3_Init();
}

void imgui_new_frame()
{
	// Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void imgui_render()
{
	gs_platform_i* platform = gs_engine_instance()->ctx.platform;

    // TODO(john): abstract this all away to be rendered via command buffer system
    ImGui::Render();
    gs_vec2 fbs = platform->frame_buffer_size( platform->main_window() );
    glViewport(0, 0, fbs.x, fbs.y);
    glClearColor(0.1f, 0.1f, 0.1f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void do_ui()
{
	gs_platform_i* platform = gs_engine_instance()->ctx.platform;
	gs_vec2 ws = platform->window_size( platform->main_window() );

	ImDrawList* dl = ImGui::GetBackgroundDrawList();
	dl->AddImage(
		(ImTextureID)gs_int_to_void_p(g_rt.id),
		ImVec2(0.f, 0.f),
		ImVec2(g_rt.width, g_rt.height),
		ImVec2(0.f, 1.f),
		ImVec2(1.f, 0.f)
	);

	// Grab player window bounds
	if ( g_show_debug_window )
	{
		gs_vec4 pb = aabb_window_coords( &g_player.aabb, &g_camera );
		gs_vec4 cb = gs_default_val();

		gs_for_range_i( gs_dyn_array_size( g_collision_objects ) )
		{
			// Get window coordinates of generic aabb_t
			cb = aabb_window_coords( &g_collision_objects[i], &g_camera );

			// Draw bounding rect around object
			dl->AddRect(
				ImVec2(cb.x, cb.w),
				ImVec2(cb.z, cb.y),
				ImColor(1.f, 1.f, 1.f, 1.f),
				1.f );
		}
		
		// Draw bounding rect around player
		dl->AddRect(
			ImVec2(pb.x, pb.w),
			ImVec2(pb.z, pb.y),
			ImColor(1.f, 1.f, 1.f, 1.f),
			1.f
		);

		// Draw aabb for ground plane
		aabb_t ground = gs_default_val();
		ground.min = v2(g_player.aabb.min.x - 100.f, 0.f);
		ground.max = v2(g_player.aabb.min.x + 100.f, -10.f);
		cb = aabb_window_coords( &ground, &g_camera );

		// Draw bounding rect for ground
		dl->AddRectFilled(
			ImVec2(cb.x, cb.w),
			ImVec2(cb.z, cb.y),
			ImColor(1.f, 0.f, 0.f, 0.5f)
		);

		ImGui::Begin( "Debug Info" );
		{
		    if (ImGui::CollapsingHeader("camera", NULL))
		    {
			    ImGui::SliderFloat("camera ortho scale", &g_camera.ortho_scale, 0.01f, 10.f, "%.2f");

			    if (ImGui::CollapsingHeader("transform##camera", NULL))
			    {
			    	ImGui::SliderFloat("x", &g_camera.transform.position.x, 0.f, 1000.f );
			    	ImGui::SliderFloat("y", &g_camera.transform.position.y, 0.f, 1000.f );
			    	ImGui::SliderFloat("z", &g_camera.transform.position.z, 0.f, 1000.f );
			    }
		    }

		    // Player transform information
		    if (ImGui::CollapsingHeader("player", NULL))
		    {
		    	ImGui::Text( "grounded: %s", player_is_grounded(&g_player) ? "true" : "false" );
		    	ImGui::Text( "moving: %s", player_is_moving(&g_player) ? "true" : "false" );
		    	ImGui::Text( "state: %s", player_state_to_string(&g_player) );
		    	ImGui::Text( "frame: %d", g_player.animations[g_player.state].current_frame);
			    if (ImGui::CollapsingHeader("transform", NULL) )
			    {
			    	ImGui::SliderFloat("x", &g_player.transform.position.x, 0.f, 1000.f );
			    	ImGui::SliderFloat("y", &g_player.transform.position.y, 0.f, 1000.f );
			    	ImGui::SliderFloat("z", &g_player.transform.position.z, 0.f, 1000.f );
			    }
		    }
		}
		ImGui::End();
	}
}

void camera_update()
{
	gs_platform_i* platform = gs_engine_instance()->ctx.platform;

	if ( platform->key_down( gs_keycode_q ) ) {
		g_camera.ortho_scale += 0.1f;
	}
	if ( platform->key_down( gs_keycode_e ) ) {
		g_camera.ortho_scale -= 0.1f;
	}
	if ( platform->key_down( gs_keycode_up ) ) {
		g_camera.transform.position.y += 0.1f;
	}
	if ( platform->key_down( gs_keycode_down ) ) {
		g_camera.transform.position.y -= 0.1f;
	}

	// Lerp towards player
	gs_vec3 offset = v3(0.f, 1.6f, 0.f);
	gs_vqs* xform = &g_camera.transform;
	xform->position.x = gs_interp_linear(xform->position.x, g_player.transform.position.x + offset.x, 0.05f);
	// xform->position.y = gs_interp_linear(xform->position.y, g_player.transform.position.y + offset.y, 0.05f);
}

void player_update( player_t* player )
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
	gs_for_range_i( gs_dyn_array_size( g_collision_objects ) )
	{
		if ( aabb_vs_aabb( &g_player.aabb, &g_collision_objects[i] ) )
		{
			// Get mvt then move player by mtv	
			gs_vec2 mtv = aabb_aabb_mtv( &g_player.aabb, &g_collision_objects[i] );
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
}



