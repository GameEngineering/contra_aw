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

typedef struct sprite_frame_t
{
	gs_vec4 uvs;
	gs_texture_t texture;	// Source texture atlas
} sprite_frame_t;

typedef struct sprite_frame_animation_t
{
	gs_dyn_array( sprite_frame_t ) frames;
	u32 current_frame;
} sprite_frame_animation_t;

#define player_state( lower, upper, gun )\
	player_state_##lower##_##upper##_##gun

typedef enum player_state_t
{
	player_state(running, gun_forward, not_firing),
	player_state(idle, gun_forward, not_firing),
	player_state_count
} player_state_t;

typedef struct player_t
{
	sprite_frame_animation_t animations[ player_state_count ];	// Animations player states
	f32 scale_factor;
	f32 heading;
	player_state_t state;
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

gs_vec4 player_get_window_bounds( player_t* player, gs_camera_t* camera )
{
	gs_platform_i* platform = gs_engine_instance()->ctx.platform;
	gs_vec2 ws = platform->window_size( platform->main_window() );

	sprite_frame_animation_t* anim = &player->animations[player->state];
	u32 anim_frame_count = gs_dyn_array_size( anim->frames );
	sprite_frame_t* s = &anim->frames[anim->current_frame];
	gs_vec4 uvs = s->uvs;

	// Width and height of UVs to scale the quads
	f32 tw = fabsf(uvs.z - uvs.x);
	f32 th = fabsf(uvs.w - uvs.y);

	// Player's transform
	gs_vqs xform = gs_vqs_default();
	xform.position = v3(0.f, 0.f, 0.f);
	xform.scale = gs_vec3_scale(v3(tw, th, 1.f), player->scale_factor);
	xform.scale.z = 1.f;

	// AABB of the player
	gs_vec4 bounds = gs_default_val();

	// Define the object space quad for our player
	gs_vec4 tl = v4(-0.5f, -0.5f, 0.f, 1.f);
	gs_vec4 br = v4(0.5f, 0.5f, 0.f, 1.f);

	// Define matrices for transformations
	gs_mat4 model_mtx = gs_vqs_to_mat4( &xform );
	gs_mat4 view_mtx = gs_camera_get_view( camera );
	gs_mat4 proj_mtx = gs_camera_get_projection( camera, ws.x, ws.y );
	gs_mat4 mvp = gs_mat4_mul( proj_mtx, gs_mat4_mul( view_mtx, model_mtx ) );

	// Transform verts
	tl = gs_mat4_mul_vec4( mvp, tl );			
	br = gs_mat4_mul_vec4( mvp, br );

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

void player_init( player_t* player, asset_manager_t* am )
{
	player->scale_factor = 1.f / 40.f;
	player->heading = 1.f;
	player_set_state( *player, idle, gun_forward, not_firing );
	gs_texture_t tex = asset_manager_get( *am, gs_texture_t, "contra_player_sprite" );
	sprite_frame_animation_t* anim = NULL;

	/* Player State: Running, Gun Forward, Not Firing */
	anim = &player->animations[ player_state(running, gun_forward, not_firing) ];
	anim->frames = gs_dyn_array_new( sprite_frame_t );
	gs_dyn_array_push( anim->frames, sprite_frame_t_new( tex, v4(113.f, 81.f, 146.f, 120.f) ) );
	gs_dyn_array_push( anim->frames, sprite_frame_t_new( tex, v4(148.f, 81.f, 185.f, 120.f) ) );
	gs_dyn_array_push( anim->frames, sprite_frame_t_new( tex, v4(2.f, 81.f, 37.f, 120.f) ) );
	gs_dyn_array_push( anim->frames, sprite_frame_t_new( tex, v4(43.f, 82.f, 72.f, 120.f) ) );
	gs_dyn_array_push( anim->frames, sprite_frame_t_new( tex, v4(75.f, 81.f, 111.f, 120.f) ) );
	gs_dyn_array_push( anim->frames, sprite_frame_t_new( tex, v4(189.f, 82.f, 228.f, 120.f) ) );

	/* Player State: Idle, Gun Forward, Not Firing */
	anim = &player->animations[ player_state(idle, gun_forward, not_firing) ];
	anim->frames = gs_dyn_array_new( sprite_frame_t );
	gs_dyn_array_push( anim->frames, sprite_frame_t_new( tex, v4(35.f, 2.f, 70.f, 42.f) ) );
}

void player_update_input( player_t* player )
{
	gs_platform_i* platform = gs_engine_instance()->ctx.platform;

	// No input -> idle
	if ( platform->key_down( gs_keycode_left ) )
	{
		player_set_state( *player, running, gun_forward, not_firing );
		player->heading = -1.f;
	}
	else if ( platform->key_down( gs_keycode_right ) )
	{
		player_set_state( *player, running, gun_forward, not_firing );
		player->heading = 1.f;
	}
	else
	{
		player_set_state( *player, idle, gun_forward, not_firing );
	}
}

// Forward Decls.
void update_camera();

// Global Decls.
_global gs_camera_t 			g_camera = gs_default_val();
_global gs_quad_batch_t 		g_player_batch = gs_default_val();
_global gs_command_buffer_t 	g_cb = gs_default_val();
_global gs_frame_buffer_t 		g_fb = gs_default_val();
_global gs_texture_t 			g_rt = gs_default_val();
_global asset_manager_t 		g_am = gs_default_val();
_global player_t 				g_player = gs_default_val();
_global b8 						g_show_demo_window = true;

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
	app.window_width 		= 800;
	app.window_height 		= 600;
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

	// Construct quad batch api and link up function pointers
	g_player_batch = gs_quad_batch_new( NULL );

	// Set material texture uniform
	gfx->set_material_uniform_sampler2d( g_player_batch.material, "u_tex", 
		asset_manager_get( g_am, gs_texture_t, "contra_player_sprite" ), 0 );

	// Construct camera parameters
	g_camera.transform = gs_vqs_default();
	g_camera.transform.position = v3(0.f, 0.f, -1.f);
	g_camera.fov = 60.f;
	g_camera.near_plane = 0.1f;
	g_camera.far_plane = 1000.f;
	g_camera.ortho_scale = 2.f;
	g_camera.proj_type = gs_projection_type_orthographic;

	// Intialize player
	player_init( &g_player, &g_am );

	imgui_init();

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
	if ( engine->ctx.platform->key_pressed( gs_keycode_esc ) )
	{
		return gs_result_success;
	}

	const f32 dt = platform->time.delta;
	const f32 t = platform->elapsed_time();

	update_camera();
	player_update_input( &g_player );

	static s32 cur_frame = 0;
	#if use_animation_time
			static f32 _t = 0.f;
			_t += 0.1f;
			if ( _t >= 1.f )
			{
				_t = 0.f;
				cur_frame++;
			}
	#endif

	// Add some stuff to the quad batch
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
		quad_info.transform = gs_vqs_default();
		quad_info.transform.position = v3(0.f, 0.f, 0.f);
		quad_info.transform.scale = gs_vec3_scale(v3(tw, th, 1.f), g_player.scale_factor);
		quad_info.uv = v4(l, b, r, t);
		quad_info.color = v4(1.f, 1.f, 1.f, 1.f);
		gfx->quad_batch_add( qb, &quad_info );
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
		f32 clear_color[4] = { 0.2f, 0.2f, 0.2f, 1.f };
		gfx->set_view_clear( cb, clear_color );
		gfx->set_view_port( cb, fbs.x, fbs.y );
		gfx->set_depth_enabled( cb, false );
		gfx->set_blend_mode( cb, gs_blend_mode_src_alpha, gs_blend_mode_one_minus_src_alpha );

		// Create model/view/projection matrices from camera
		gs_mat4 view_mtx = gs_camera_get_view( &g_camera );
		gs_mat4 proj_mtx = gs_camera_get_projection( &g_camera, ws.x, ws.y );
		gs_mat4 model_mtx = gs_mat4_scale(v3(1.f, 1.f, 1.f));

		// Set necessary dynamic uniforms for quad batch material (defined in default shader in gs_quad_batch.h)
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

	ImDrawList* dl = ImGui::GetBackgroundDrawList();
	dl->AddImage(
		(ImTextureID)gs_int_to_void_p(g_rt.id),
		ImVec2(0.f, 0.f),
		ImVec2(g_rt.width, g_rt.height),
		ImVec2(0.f, 1.f),
		ImVec2(1.f, 0.f)
	);

	// Grab player window bounds
	gs_vec4 pb = player_get_window_bounds( &g_player, &g_camera );
	
	// Draw bounding rect around player
	dl->AddRect(
		ImVec2(pb.x, pb.w),
		ImVec2(pb.z, pb.y),
		ImColor(1.f, 1.f, 1.f, 1.f),
		1.f
	);

	ImGui::Begin( "Window" );
	{
		f32 ortho_scale = g_camera.ortho_scale;		
	    ImGui::SliderFloat("Camera Ortho Scale", &g_camera.ortho_scale, 0.01f, 10.f, "%.2f");
		ImGui::Text( "Test" );
	}
	ImGui::End();
}

void update_camera()
{
	gs_platform_i* platform = gs_engine_instance()->ctx.platform;

	if ( platform->key_down( gs_keycode_q ) ) {
		g_camera.ortho_scale += 0.1f;
	}
	if ( platform->key_down( gs_keycode_e ) ) {
		g_camera.ortho_scale -= 0.1f;
	}
	if (platform->key_down(gs_keycode_a)) {
		g_camera.transform.position.x -= 0.1f;
	}
	if (platform->key_down(gs_keycode_d)) {
		g_camera.transform.position.x += 0.1f;
	}
	if (platform->key_down(gs_keycode_w)) {
		g_camera.transform.position.y += 0.1f;
	}
	if (platform->key_down(gs_keycode_s)) {
		g_camera.transform.position.y -= 0.1f;
	}
}



