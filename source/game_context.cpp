#include "game_context.h"

void game_context_init( game_context_t* ctx )
{
	gs_platform_i* platform = gs_engine_instance()->ctx.platform;
	gs_graphics_i* gfx = gs_engine_instance()->ctx.graphics;
	gs_audio_i* audio = gs_engine_instance()->ctx.audio;
	gs_vec2 fbs = platform->frame_buffer_size( platform->main_window() );

	// Initialize all game assets as well
	ctx->cb = gs_command_buffer_new();

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
	ctx->rt = gfx->construct_texture( t_desc );

	// Construct frame buffer
	ctx->fb = gfx->construct_frame_buffer( ctx->rt );
	// Construct quad batch api and link up function pointers
	ctx->player_batch = gs_quad_batch_new( NULL );
	ctx->background_batch = gs_quad_batch_new( NULL );

	game_context_initialize_assets( ctx );

	// Set material texture uniform
	gfx->set_material_uniform_sampler2d( ctx->player_batch.material, "u_tex", 
		asset_manager_get( ctx->am, gs_texture_t, "textures.contra_player_sprite" ), 0 );

	gfx->set_material_uniform_sampler2d( ctx->background_batch.material, "u_tex", 
		asset_manager_get( ctx->am, gs_texture_t, "textures.bg_elements" ), 0 );

	// Construct camera parameters
	ctx->camera.transform = gs_vqs_default();
	ctx->camera.transform.position = v3(0.f, 3.1f, -1.f);
	ctx->camera.fov = 60.f;
	ctx->camera.near_plane = 0.1f;
	ctx->camera.far_plane = 1000.f;
	ctx->camera.ortho_scale = 3.7f;
	ctx->camera.proj_type = gs_projection_type_orthographic;

	// Intialize player
	player_init( &ctx->player, &ctx->am);

	// Initialize bullet group
	entity_group_init( bullet_t, &ctx->entities.bullets );

	// Init aabb collision struct
	ctx->collision_objects = gs_dyn_array_new( aabb_t );
	gs_for_range_i( 100 )
	{
		aabb_t aabb = gs_default_val();
		aabb.min = v2((f32)i * 5.f, 0.f);
		aabb.max = gs_vec2_add(aabb.min, v2(1.f, 1.f));
		gs_dyn_array_push( ctx->collision_objects, aabb );
	}

	// Construct instance source and play on loop. Forever.
	// Fill out instance data to pass into audio subsystem
	gs_audio_instance_data_t inst = gs_audio_instance_data_new( asset_manager_get( ctx->am, gs_audio_source_t, "audio.level_1_bg" ) );
	inst.volume = 0.1f;						// Range from [0.f, 1.f]
	inst.loop = true;						// Tell whether or not audio should loop 
	inst.persistent = true;					// Whether or not instance should stick in memory after completing, if not then will be cleared from memory
	ctx->bg_music = audio->construct_instance( inst );
	audio->play( ctx->bg_music );
}

#define __asset_manager_load_sprite_anim( _name, speed, ... )\
do {\
	sprite_frame_t sprites[] =\
	{\
		__VA_ARGS__\
	};\
	u32 count = sizeof(sprites) / sizeof(sprite_frame_t);\
	asset_manager_load( ctx->am, sprite_frame_animation_asset_t, (_name), sprites, count, speed );\
} while (0)

void game_context_initialize_assets( game_context_t* ctx )
{
	// Construct asset manager
	ctx->am = asset_manager_new();

	gs_texture_parameter_desc desc = gs_texture_parameter_desc_default();
	desc.min_filter = gs_nearest;
	desc.mag_filter = gs_nearest;
	desc.generate_mips = false;

	// Load all textures into asset manager
	const char* texture_files[] = 
	{
		"textures/contra_player_sprite.png",
		"textures/bg_elements.png"
	};

	gs_for_range_i( sizeof(texture_files) / sizeof(const char*) )
	{
		char tmp[256];
		memset(tmp, 0, 256);
		gs_snprintf(tmp, 256, "./assets/%s", texture_files[i]);
		asset_manager_load( ctx->am, gs_texture_t, tmp, desc );
	}

	// Load atlas textures into asset manager
	asset_manager_load( ctx->am, gs_texture_t, "./assets/textures/bg_elements.png", desc );

	// All required audio files
	const char* audio_files[] = 
	{
		"audio/level_1_bg.mp3"
	};

	// Load audio sources into asset manager
	gs_for_range_i( sizeof(audio_files) / sizeof(const char*) )
	{
		char tmp[256];
		memset(tmp, 0, 256);
		gs_snprintf(tmp, 256, "./assets/%s", audio_files[i]);
		asset_manager_load( ctx->am, gs_audio_source_t, tmp );
	}

	/*==================
	// Sprite Animations
	==================*/

	const char* state = "";
	gs_texture_t tex = asset_manager_get( ctx->am, gs_texture_t, "textures.contra_player_sprite" );

	__asset_manager_load_sprite_anim(
		player_state_to_string(player_state(running, gun_forward, not_firing)),
		0.1f,
		sprite_frame_t_new( tex, v4(113.f, 81.f, 146.f, 120.f) ),
		sprite_frame_t_new( tex, v4(148.f, 81.f, 185.f, 120.f) ),
		sprite_frame_t_new( tex, v4(2.f, 81.f, 37.f, 120.f) ),
		sprite_frame_t_new( tex, v4(43.f, 82.f, 72.f, 120.f) ),
		sprite_frame_t_new( tex, v4(75.f, 81.f, 111.f, 120.f) ),
		sprite_frame_t_new( tex, v4(189.f, 82.f, 228.f, 120.f) )
	);

	/* Player State: Running, Gun Up, Not Firing */
	__asset_manager_load_sprite_anim(
		player_state_to_string(player_state(running, gun_up, not_firing)), 
		0.1f,
		sprite_frame_t_new( tex, v4(103.f, 172.f, 126.f, 222.f) ),
		sprite_frame_t_new( tex, v4(129.f, 172.f, 156.f, 222.f) ),
		sprite_frame_t_new( tex, v4(8.f, 172.f, 35.f, 222.f) ),
		sprite_frame_t_new( tex, v4(42.f, 171.f, 65.f, 222.f) ),
		sprite_frame_t_new( tex, v4(70.f, 171.f, 97.f, 222.f) ),
		sprite_frame_t_new( tex, v4(6.f, 226.f, 44.f, 277.f) )
	);

	/* Player State: Running, Gun Down, Not Firing */
	__asset_manager_load_sprite_anim(
		player_state_to_string(player_state(running, gun_down, not_firing)),
		0.1f,
		sprite_frame_t_new( tex, v4(120.f, 280.f, 146.f, 321.f) ),
		sprite_frame_t_new( tex, v4(155.f, 280.f, 182.f, 321.f) ),
		sprite_frame_t_new( tex, v4(46.f, 230.f, 74.f, 271.f) ),
		sprite_frame_t_new( tex, v4(82.f, 230.f, 107.f, 271.f) ),
		sprite_frame_t_new( tex, v4(113.f, 230.f, 140.f, 272.f) ),
		sprite_frame_t_new( tex, v4(184.f, 280.f, 220.f, 322.f) )
	);

	/* Player State: Idle, Gun Forward, Not Firing */
	__asset_manager_load_sprite_anim(
		player_state_to_string(player_state(idle, gun_forward, not_firing)),
		0.1f,
		sprite_frame_t_new( tex, v4(35.f, 2.f, 70.f, 43.f) )
	);

	/* Player State: Idle, Gun Up, Not Firing */
	__asset_manager_load_sprite_anim(
		player_state_to_string(player_state(idle, gun_up, not_firing)),
		0.1f,
		sprite_frame_t_new( tex, v4(167.f, 169.f, 198.f, 238.f) )
	);

	/* Player State: Idle Prone, Gun Forward, Not Firing */
	__asset_manager_load_sprite_anim(
		player_state_to_string(player_state(idle_prone, gun_forward, not_firing)),
		0.1f,
		sprite_frame_t_new( tex, v4(114.f, 26.f, 158.f, 41.f) )
	);

	/* Player State: Jumping, Null, Null */
	__asset_manager_load_sprite_anim(
		player_state_to_string(player_state(jumping, null, null)),
		0.8f,
		sprite_frame_t_new( tex, v4(0.f, 49.f, 25.f, 72.f) ),
		sprite_frame_t_new( tex, v4(30.f, 49.f, 49.f, 72.f) ),
		sprite_frame_t_new( tex, v4(59.f, 49.f, 82.f, 72.f) ),
		sprite_frame_t_new( tex, v4(93.f, 49.f, 112.f, 72.f) )
	);
}

void game_context_update( game_context_t* ctx )
{
	entity_group_update( bullet_t, &ctx->entities.bullets );	
}
