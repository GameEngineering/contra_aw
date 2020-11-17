#include <gs.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>
#include <GLFW/glfw3.h>

// Project Includes
#include "asset_manager.h"
#include "entity.h"
#include "component.h"
#include "aabb.h"
#include "defines.h"
#include "player.h"
#include "game_context.h"

// Forward Decls.
void camera_update();

// Global Decls.
_global game_context_t 			g_ctx = gs_default_val();

// Forward Decls.
gs_result app_init();
gs_result app_update();		// Use to update your application

void imgui_init();
void imgui_new_frame();
void imgui_render();
void debug_ui();

int main( int argc, char** argv )
{
	// This is our app description. It gives internal hints to our engine for various things like 
	// window size, title, as well as update, init, and shutdown functions to be run. 
	gs_application_desc app = {0};
	app.window_title 		= "Contra 3";
	app.window_width 		= (s32)(1920.f * 0.7f);
	app.window_height 		= (s32)(1080.f * 0.7f);
	app.frame_rate 			= 60;
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
	// Initialize the game context
	game_context_init( &g_ctx );

	// Initialize debug ui
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
	gs_command_buffer_t* cb = &g_ctx.cb;
	gs_quad_batch_t* qb = &g_ctx.foreground_batch;

	// If we press the escape key, exit the application
	if ( platform->key_pressed( gs_keycode_esc ) )
	{
		return gs_result_success;
	}

	if ( platform->key_pressed( gs_keycode_i ) )
	{
		g_ctx.show_debug_window = !g_ctx.show_debug_window;
	}

	const f32 dt = platform->time.delta;
	const f32 t = platform->elapsed_time();

	camera_update();
	player_update( &g_ctx.player, &g_ctx );

	// Update game context
	game_context_update( &g_ctx );

	f32 scale_factor = gs_vec3_len( g_ctx.player.transform.scale );

	// Player
	gfx->quad_batch_begin( qb );
	{
		sprite_animation_component_t* ac = &g_ctx.player.animation_comp; 
		sprite_frame_animation_asset_t* anim = ac->animation;
		u32 anim_frame_count = gs_dyn_array_size( anim->frames );

		sprite_frame_t* s = &anim->frames[ac->current_frame];
		f32 w = (f32)s->texture.width;
		f32 h = (f32)s->texture.height;
		gs_vec4 uvs = s->uvs;

		// Need UV information for tile in texture
		f32 l = g_ctx.player.heading == 1.f ? (uvs.x / w) : (uvs.z / w);
		f32 t = 1.f - (uvs.y / h);
		f32 r = g_ctx.player.heading == 1.f ? (uvs.z / w) : (uvs.x / w);
		f32 b = 1.f - (uvs.w / h);

		// Width and height of UVs to scale the quads
		f32 tw = fabsf(uvs.z - uvs.x);
		f32 th = fabsf(uvs.w - uvs.y);

		gs_default_quad_info_t quad_info = {0};
		quad_info.transform = g_ctx.player.transform;
		quad_info.transform.scale = gs_vec3_scale(v3(tw, th, 1.f), scale_factor);
		quad_info.uv = v4(l, b, r, t);
		quad_info.color = v4(1.f, 1.f, 1.f, 1.f);
		gfx->quad_batch_add( qb, &quad_info );
	}
	gfx->quad_batch_end( qb );

	// Background
	qb = &g_ctx.background_batch;
	gfx->quad_batch_begin( qb );
	{
		// Loop through tiled backgrounds
		gs_texture_t bg_tex = asset_manager_get( g_ctx.am, gs_texture_t, "textures.bg_elements" );
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
			gs_vec3 cam_origin = g_ctx.camera.transform.position;

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
			gs_vec3 cam_origin = g_ctx.camera.transform.position;

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
			gs_vec3 cam_origin = g_ctx.camera.transform.position;

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

			gs_vec3 cam_origin = g_ctx.camera.transform.position;
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

			gs_vec3 cam_origin = g_ctx.camera.transform.position;
			gs_vec3 origin = v3(-5.f, 0.965f, 0.f);

			gs_default_quad_info_t quad_info = {0};
			quad_info.transform = gs_vqs_default();
			quad_info.transform.scale = gs_vec3_scale(v3(tw, th, 1.f), scale_factor);
			quad_info.transform.position = v3(origin.x + tw * scale_factor * i - 3.f + cam_origin.x * 0.f, origin.y, 0.f);
			quad_info.uv = v4(l, b, r, t);
			quad_info.color = v4(1.f, 1.f, 1.f, 1.f);
			gfx->quad_batch_add( qb, &quad_info );
		}

		// Draw dem bullets
		entity_group(bullet_t)* bullets = &g_ctx.entities.bullets;
		gs_for_range_i( gs_dyn_array_size( bullets->entities ) )
		{
			u32 id = bullets->entities[i];
			transform_component_t* xform = &gs_slot_array_get( bullets->transforms, id );					
			sprite_component_t* sprite = &gs_slot_array_get( bullets->sprites, id );

			// Draw single quad for background batch for now	
			gs_vec4 uv = sprite->uv;

			// Need UV information for tile in texture
			f32 l = uv.x / w;
			f32 t = 1.f - (uv.y / h);
			f32 r = uv.z / w;
			f32 b = 1.f - (uv.w / h);

			// Width and height of UVs to scale the quads
			f32 tw = fabsf(uv.z - uv.x);
			f32 th = fabsf(uv.w - uv.y);

			gs_vec3 cam_origin = g_ctx.camera.transform.position;
			gs_vec3 origin = v3(-5.f, 0.965f, 0.f);

			gs_default_quad_info_t quad_info = {0};
			quad_info.transform = gs_vqs_default();
			quad_info.transform.scale = gs_vec3_scale(v3(tw, th, 1.f), scale_factor);
			quad_info.transform.position = xform->transform.position;
			quad_info.uv = v4(l, b, r, t);
			quad_info.color = v4(1.f, 1.f, 1.f, 1.f);
			gfx->quad_batch_add( qb, &quad_info );
		}
	} 
	gfx->quad_batch_end(qb);

	qb = &g_ctx.enemy_batch;
	gfx->quad_batch_begin(qb);
	{
		// Draw enemies
		entity_group(red_guy_t*) red_guys = &g_ctx.entities.red_guys;
		gs_for_range_i(gs_dyn_array_size(red_guys->entities))
		{
			u32 id = red_guys->entities[i];
			transform_component_t* xform = &gs_slot_array_get(red_guys->transforms, id);					
			sprite_animation_component_t* ac = &gs_slot_array_get(red_guys->animations, id);
			sprite_frame_animation_asset_t* anim = ac->animation;
			sprite_frame_t* sprite = &anim->frames[ac->current_frame];

			// Draw single quad for background batch for now	
			gs_vec4 uv = sprite->uvs;
			f32 w = sprite->texture.width;
			f32 h = sprite->texture.height;

			// Need UV information for tile in texture
			f32 l = uv.x / w;
			f32 t = 1.f - (uv.y / h);
			f32 r = uv.z / w;
			f32 b = 1.f - (uv.w / h);

			// Width and height of UVs to scale the quads
			f32 tw = fabsf(uv.z - uv.x);
			f32 th = fabsf(uv.w - uv.y);

			gs_vec3 cam_origin = g_ctx.camera.transform.position;

			gs_default_quad_info_t quad_info = {0};
			quad_info.transform = gs_vqs_default();
			quad_info.transform.scale = gs_vec3_scale(v3(tw, th, 1.f), scale_factor);
			quad_info.transform.position = xform->transform.position;
			quad_info.uv = v4(l, b, r, t);
			quad_info.color = v4(1.f, 1.f, 1.f, 1.f);
			gfx->quad_batch_add(qb, &quad_info);
		}
	}
	gfx->quad_batch_end(qb);

	/*===============
	// Render scene
	================*/

	gfx->bind_frame_buffer( cb, g_ctx.fb );
	{
		// Set the render target for the frame buffer
		gfx->set_frame_buffer_attachment( cb, g_ctx.rt, 0 );

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
		gs_mat4 view_mtx = gs_camera_get_view( &g_ctx.camera );
		gs_mat4 proj_mtx = gs_camera_get_projection( &g_ctx.camera, ws.x, ws.y );
		gs_mat4 model_mtx = gs_mat4_scale(v3(1.f, 1.f, 1.f));

		// Draw bg
		qb = &g_ctx.background_batch;
		gfx->set_material_uniform_mat4( qb->material, "u_model", model_mtx );
		gfx->set_material_uniform_mat4( qb->material, "u_view", view_mtx );
		gfx->set_material_uniform_mat4( qb->material, "u_proj", proj_mtx );

		// Need to submit quad batch
		gfx->quad_batch_submit( cb, qb );

		// Draw player
		qb = &g_ctx.foreground_batch;
		gfx->set_material_uniform_mat4( qb->material, "u_model", model_mtx );
		gfx->set_material_uniform_mat4( qb->material, "u_view", view_mtx );
		gfx->set_material_uniform_mat4( qb->material, "u_proj", proj_mtx );

		// Need to submit quad batch
		gfx->quad_batch_submit( cb, qb );

		// Draw enemies
		qb = &g_ctx.enemy_batch;
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
	  	debug_ui();
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

void debug_ui()
{
	gs_platform_i* platform = gs_engine_instance()->ctx.platform;
	gs_audio_i* audio = gs_engine_instance()->ctx.audio;

	gs_vec2 ws = platform->window_size( platform->main_window() );

	ImDrawList* dl = ImGui::GetBackgroundDrawList();
	dl->AddImage(
		(ImTextureID)gs_int_to_void_p(g_ctx.rt.id),
		ImVec2(0.f, 0.f),
		ImVec2(g_ctx.rt.width, g_ctx.rt.height),
		ImVec2(0.f, 1.f),
		ImVec2(1.f, 0.f)
	);

	// Grab player window bounds
	if ( g_ctx.show_debug_window )
	{
		gs_vec4 pb = aabb_window_coords( &g_ctx.player.aabb, &g_ctx.camera );
		gs_vec4 cb = gs_default_val();

		gs_for_range_i( gs_dyn_array_size( g_ctx.collision_objects ) )
		{
			// Get window coordinates of generic aabb_t
			cb = aabb_window_coords( &g_ctx.collision_objects[i], &g_ctx.camera );

			// Draw bounding rect around object
			dl->AddRect(
				ImVec2(cb.x, cb.w),
				ImVec2(cb.z, cb.y),
				ImColor(1.f, 1.f, 1.f, 1.f),
				1.f );
		}

		entity_group(bullet_t)* bullets = &g_ctx.entities.bullets;
		gs_for_range_i( gs_dyn_array_size( bullets->entities ) )
		{
			u32 id = bullets->entities[i];
			rigid_body_component_t* rbc = gs_slot_array_get_ptr( bullets->rigid_bodies, id );

			// Get window coordinates of generic aabb_t
			cb = aabb_window_coords( &rbc->aabb, &g_ctx.camera );

			// Draw bounding rect around object
			dl->AddRect(
				ImVec2(cb.x, cb.w),
				ImVec2(cb.z, cb.y),
				ImColor(1.f, 1.f, 1.f, 1.f),
				1.f );
		}

		entity_group(red_guy_t)* enemies = &g_ctx.entities.red_guys;
		gs_for_range_i( gs_dyn_array_size( enemies->entities ) )
		{
			u32 id = enemies->entities[i];
			rigid_body_component_t* rbc = gs_slot_array_get_ptr( enemies->rigid_bodies, id );

			// Get window coordinates of generic aabb_t
			cb = aabb_window_coords( &rbc->aabb, &g_ctx.camera );

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
		ground.min = v2(g_ctx.player.aabb.min.x - 100.f, 0.f);
		ground.max = v2(g_ctx.player.aabb.min.x + 100.f, -10.f);
		cb = aabb_window_coords( &ground, &g_ctx.camera );

		// Draw bounding rect for ground
		dl->AddRectFilled(
			ImVec2(cb.x, cb.w),
			ImVec2(cb.z, cb.y),
			ImColor(1.f, 0.f, 0.f, 0.5f)
		);

		ImGui::Begin( "Debug Info" );
		{
			ImGui::Text("frame_rate: %.2f ms", platform->time.frame);

		    if (ImGui::CollapsingHeader("camera", NULL))
		    {
			    ImGui::SliderFloat("camera ortho scale", &g_ctx.camera.ortho_scale, 0.01f, 10.f, "%.2f");

			    if (ImGui::CollapsingHeader("transform##camera", NULL))
			    {
			    	ImGui::SliderFloat("x", &g_ctx.camera.transform.position.x, 0.f, 1000.f );
			    	ImGui::SliderFloat("y", &g_ctx.camera.transform.position.y, 0.f, 1000.f );
			    	ImGui::SliderFloat("z", &g_ctx.camera.transform.position.z, 0.f, 1000.f );
			    }
		    }

		    // Player transform information
		    if (ImGui::CollapsingHeader("player", NULL))
		    {
		    	ImGui::Text( "grounded: %s", player_is_grounded(&g_ctx.player) ? "true" : "false" );
		    	ImGui::Text( "moving: %s", player_is_moving(&g_ctx.player) ? "true" : "false" );
		    	ImGui::Text( "state: %s", player_state_to_string(g_ctx.player.state) );
		    	ImGui::Text( "frame: %d", g_ctx.player.animation_comp.current_frame);
			    if (ImGui::CollapsingHeader("transform", NULL) )
			    {
			    	ImGui::SliderFloat("x", &g_ctx.player.transform.position.x, 0.f, 1000.f );
			    	ImGui::SliderFloat("y", &g_ctx.player.transform.position.y, 0.f, 1000.f );
			    	ImGui::SliderFloat("z", &g_ctx.player.transform.position.z, 0.f, 1000.f );
			    }
		    }

		    // DEBUG Bullets
		    if (ImGui::CollapsingHeader("bullets", NULL))
		    {
		    	entity_group(bullet_t)* bullets = &g_ctx.entities.bullets;

		   		ImGui::Text( "amount: %zu", gs_dyn_array_size(bullets->entities)); 

		    	// Transform components
		    	for 
		    	( 
		    		gs_slot_array_iter(transform_component_t) it = gs_slot_array_iter_new(bullets->transforms);
			 		gs_slot_array_iter_valid(bullets->transforms, it );
			 		gs_slot_array_iter_advance(bullets->transforms, it)
		    	)	
		    	{
		    		// Grab iterator data and print to screen
		    		transform_component_t* xform = it.data;		
				    if (ImGui::CollapsingHeader("transform", NULL) )
				    {
				    	ImGui::SliderFloat("x", &xform->transform.position.x, 0.f, 1000.f );
				    	ImGui::SliderFloat("y", &xform->transform.position.y, 0.f, 1000.f );
				    	ImGui::SliderFloat("z", &xform->transform.position.z, 0.f, 1000.f );
				    }
		    	}
		    }

		    // Enemies
		    if ( ImGui::CollapsingHeader("enemies", NULL))
		    {
		    	entity_group(red_guy_t)* enemies = &g_ctx.entities.red_guys;
		    	ImGui::Text("amount: %zu", gs_dyn_array_size(enemies->entities));
		    }

		    // Game Context
		    if ( ImGui::CollapsingHeader("game_context", NULL))
		    {
		    	// Grab instance data, maniuplate, reset instance data	
				f32 vol = audio->get_volume( g_ctx.bg_music );
				if ( ImGui::SliderFloat("volume", &vol, 0.f, 1.f ) )
				{
					audio->set_volume( g_ctx.bg_music, vol );
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
		g_ctx.camera.ortho_scale += 0.1f;
	}
	if ( platform->key_down( gs_keycode_e ) ) {
		g_ctx.camera.ortho_scale -= 0.1f;
	}
	if ( platform->key_down( gs_keycode_up ) ) {
		g_ctx.camera.transform.position.y += 0.1f;
	}
	if ( platform->key_down( gs_keycode_down ) ) {
		g_ctx.camera.transform.position.y -= 0.1f;
	}

	// Lerp towards player
	gs_vec3 offset = v3(0.f, 1.6f, 0.f);
	gs_vqs* xform = &g_ctx.camera.transform;
	xform->position.x = gs_interp_linear(xform->position.x, g_ctx.player.transform.position.x + offset.x, 0.05f);
	// xform->position.y = gs_interp_linear(xform->position.y, g_ctx.player.transform.position.y + offset.y, 0.05f);
}

entity_group_update_decl(bullet_t,
{
	gs_platform_i* platform = gs_engine_instance()->ctx.platform;

	entity_group(bullet_t)* group = _group;

	sprite_component_t* sc 		= group->sprites.data;
	rigid_body_component_t* rc 	= group->rigid_bodies.data;
	transform_component_t* tc 	= group->transforms.data;

	u32 handle_idx = 0;
	u32 handles_to_destroy[1024];
	memset(handles_to_destroy, 0, 1024 * sizeof(u32));

	system_update(sprite_aabb)(rc, sc, tc, gs_slot_array_size(group->sprites));

	// Move group and then collide
	gs_for_range_i( gs_dyn_array_size( group->entities ) )
	{
		b32 collided = false;
		transform_component_t* xform = gs_slot_array_get_ptr( group->transforms, group->entities[i] );
		rigid_body_component_t* rbody= gs_slot_array_get_ptr( group->rigid_bodies, group->entities[i] );
		sprite_component_t* sprite = gs_slot_array_get_ptr( group->sprites, group->entities[i]);

		// Move xform by velocity of rbody
		gs_vec3 vel = gs_vec3_scale(gs_vec3_norm(v3(rbody->velocity.x, rbody->velocity.y, 0.f)), 0.1f);
		xform->transform.position = gs_vec3_add( xform->transform.position, vel );
		xform->transform.rotation = gs_quat_default();

		// Update aabb
		gs_vec4 uvs = sprite->uv;

		// Width and height of UVs to scale the quads
		f32 tw = fabsf(uvs.z - uvs.x);
		f32 th = fabsf(uvs.w - uvs.y);

		xform->transform.scale = gs_vec3_scale(v3(tw, th, 1.f), 1.f / 40.f);
		xform->transform.scale.z = 1.f;

		// Define the object space quad for our player
		gs_vec4 bl = v4(-0.5f, -0.5f, 0.f, 1.f);
		gs_vec4 tr = v4(0.5f, 0.5f, 0.f, 1.f);

		// Define matrices for transformations
		gs_mat4 model_mtx = gs_vqs_to_mat4( &xform->transform );

		// Transform verts
		bl = gs_mat4_mul_vec4( model_mtx, bl );			
		tr = gs_mat4_mul_vec4( model_mtx, tr );

		rbody->aabb.min = v2(bl.x, bl.y);
		rbody->aabb.max = v2(tr.x, tr.y);

		// Do collisions against world
		gs_for_range_j( gs_dyn_array_size( g_ctx.collision_objects ) )
		{
			// If collision occurs
			if ( aabb_vs_aabb( &rbody->aabb, &g_ctx.collision_objects[j] ) )
			{
				// Set handle to destroy
				collided = true;
			}
		}

		// Do collisions against enemies
		gs_for_range_j(gs_dyn_array_size(g_ctx.entities.red_guys.entities))
		{
			u32 eid = g_ctx.entities.red_guys.entities[j];
			rigid_body_component_t* rbc = gs_slot_array_get_ptr(g_ctx.entities.red_guys.rigid_bodies, eid);
			if (aabb_vs_aabb(&rbc->aabb, &rbody->aabb))
			{
				collided = true;	

				// Delete this entity
				entity_group_remove(red_guy_t, &g_ctx.entities.red_guys, eid);

				break;
			}
		}

		// If collision occurs
		if ( rbody->aabb.min.y < ground_level || rbody->aabb.max.y < ground_level )
		{
			collided = true;
		}

		// Check for out of frame
		gs_vec4 bc = aabb_window_coords( &rbody->aabb, &g_ctx.camera );
		gs_vec2 wc = platform->window_size( platform->main_window() );	

		aabb_t aabb_bc = gs_default_val();
		aabb_bc.min = v2(bc.x, bc.y);
		aabb_bc.max = v2(bc.z, bc.w);

		// Check bc against the wc 
		// If object is completely out of frame, then delete
		aabb_t aabb_window = gs_default_val();
		aabb_window.min = v2(0.f, 0.f);	
		aabb_window.max = wc;

		if ( !aabb_vs_aabb( &aabb_bc, &aabb_window ) )
		{
			collided = true;
		}

		// Set handle to destroy
		if ( collided )
		{
			handles_to_destroy[handle_idx++] = group->entities[i];
		}
	}

	// Iterate through handles to destroy
	gs_for_range_i( handle_idx )
	{
		u32 id = handles_to_destroy[i];		
		entity_group_remove( bullet_t, group, id );
	}
});

entity_group_init_decl( bullet_t,
{
	// Cache pointer to entity group
	entity_group(bullet_t)* group = _group;
	group->_base = entity_group_default();
	group->entities = gs_dyn_array_new( u32 );
	group->transforms = gs_slot_array_new( transform_component_t );
	group->sprites = gs_slot_array_new( sprite_component_t );
	group->rigid_bodies = gs_slot_array_new( rigid_body_component_t );
});

entity_group_add_decl(bullet_t,
{
	entity_group(bullet_t)*group = _group;
	bullet_data* data = (bullet_data*)_entity_data;

	u32 handle = gs_slot_array_invalid_handle;

	// Transform component
	transform_component_t xform = gs_default_val();	
	xform.transform.position = data->position;

	// Sprite component  
	sprite_component_t sprite = gs_default_val();
	sprite.uv = v4(31.f, 31.f, 36.f, 36.f);

	// Rigid body component
	rigid_body_component_t rigid_body = gs_default_val();
	rigid_body.velocity = data->velocity;

	// Insert component information (create entity)
	gs_slot_array_insert(group->transforms, xform);
	gs_slot_array_insert(group->sprites, sprite);
	handle = gs_slot_array_insert(group->rigid_bodies, rigid_body);
	gs_dyn_array_push(group->entities, handle);

	return handle;
});

entity_group_remove_decl( bullet_t,
{
	entity_group( bullet_t )* bullets = _group;

	// Remove id
	gs_slot_array_erase( bullets->transforms, _id );
	gs_slot_array_erase( bullets->sprites, _id );
	gs_slot_array_erase( bullets->rigid_bodies, _id );

	// Need to remove the id from the array of handles
	// Iterate through handles, find idx of id, swap and pop with back
	u32 idx = u32_max;
	gs_for_range_i( gs_dyn_array_size( bullets->entities ) )
	{
		if ( bullets->entities[i] == _id )
		{
			idx = i;
			break;
		}	
	}

	if ( idx != u32_max )
	{
		// Swap and pop
		bullets->entities[idx] = gs_dyn_array_back(bullets->entities);
		gs_dyn_array_pop(bullets->entities);
	}
});

entity_group_shutdown_decl( bullet_t,
{
	// Cache pointer to entity group
	entity_group( bullet_t )* bullets = _group;

	// Shutdown bullet stuff here...
	gs_slot_array_free( bullets->transforms );
	gs_slot_array_free( bullets->sprites );
	gs_slot_array_free( bullets->rigid_bodies );
});

entity_group_init_decl( red_guy_t,
{
	// Cache pointer to entity group
	entity_group( red_guy_t )* group = _group;
	group->_base = entity_group_default();
	group->entities = gs_dyn_array_new( u32 );
	group->transforms = gs_slot_array_new( transform_component_t );
	group->animations = gs_slot_array_new( sprite_animation_component_t );
	group->rigid_bodies = gs_slot_array_new( rigid_body_component_t );
});

entity_group_update_decl(red_guy_t, 
{
	entity_group( red_guy_t )* group = _group;

	sprite_animation_component_t* sc = group->animations.data;
	rigid_body_component_t* rc = group->rigid_bodies.data;
	transform_component_t* tc = group->transforms.data;

	// Update all animations
	component_update(sprite_animation_component_t)(sc, gs_slot_array_size(group->animations), false);
	system_update(sprite_anim_aabb)(rc, sc, tc, gs_slot_array_size(group->animations));

	gs_for_range_i( gs_dyn_array_size( group->entities ) )
	{
		// Entity id
		u32 id = group->entities[i];
		tc = gs_slot_array_get_ptr(group->transforms, id);
		rc = gs_slot_array_get_ptr(group->rigid_bodies, id);
		sc = gs_slot_array_get_ptr(group->animations, id);

		/*=============
		// Collisions
		=============*/

		// Default collision response against other AABBs

		// Check with floor
		aabb_t ground = gs_default_val();
		ground.min = v2(rc->aabb.min.x - 100.f, -10.f);
		ground.max = v2(rc->aabb.min.x + 100.f, 0.f);
		if (aabb_vs_aabb(&rc->aabb, &ground))
		{
			// Get mvt then move player by mtv	
			gs_vec2 mtv = aabb_aabb_mtv(&rc->aabb, &ground);
			tc->transform.position = gs_vec3_add(tc->transform.position, v3(mtv.x, mtv.y, 0.f));
		}

		// Check against world
		gs_for_range_i(gs_dyn_array_size(g_ctx.collision_objects ))
		{
			if (aabb_vs_aabb(&rc->aabb, &g_ctx.collision_objects[i]))
			{
				// Get mvt then move player by mtv	
				gs_vec2 mtv = aabb_aabb_mtv(&rc->aabb, &g_ctx.collision_objects[i]);
				tc->transform.position = gs_vec3_add(tc->transform.position, v3(mtv.x, mtv.y, 0.f));
			}
		}
	}
});

entity_group_add_decl( red_guy_t,
{
	entity_group( red_guy_t )* group = _group;
	red_guy_data* data = (red_guy_data*)_entity_data;

	u32 handle = gs_slot_array_invalid_handle;

	// Transform component
	transform_component_t xform = gs_default_val();	
	xform.transform.position = data->position;

	// Sprite component  
	sprite_animation_component_t anim_comp = gs_default_val();

	// Animation Component
	anim_comp.animation = asset_manager_get( g_ctx.am, sprite_frame_animation_asset_t, "red_guy_running" ); 

	// Rigid body component
	rigid_body_component_t rigid_body = gs_default_val();

	// Insert component information (create entity)
	gs_slot_array_insert( group->transforms, xform );
	gs_slot_array_insert( group->animations, anim_comp );
	handle = gs_slot_array_insert( group->rigid_bodies, rigid_body );
	gs_dyn_array_push( group->entities, handle );

	return handle;
});

entity_group_remove_decl( red_guy_t,
{
	entity_group(red_guy_t)* group = _group;

	// Remove id
	gs_slot_array_erase(group->transforms, _id);
	gs_slot_array_erase(group->animations, _id);
	gs_slot_array_erase(group->rigid_bodies, _id);

	// Need to remove the id from the array of handles
	// Iterate through handles, find idx of id, swap and pop with back
	u32 idx = u32_max;
	gs_for_range_i(gs_dyn_array_size(group->entities))
	{
		if (group->entities[i] == _id)
		{
			idx = i;
			break;
		}	
	}

	if (idx != u32_max)
	{
		// Swap and pop
		group->entities[idx] = gs_dyn_array_back(group->entities);
		gs_dyn_array_pop(group->entities);
	}
});

entity_group_shutdown_decl( red_guy_t,
{
});





