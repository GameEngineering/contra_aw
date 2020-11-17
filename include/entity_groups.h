#ifndef CONTRA_ENTITY_GROUPS_H
#define CONTRA_ENTITY_GROUPS_H

#include <gs.h>

#include "entity.h"
#include "component.h"

/*=====================
// Components
=====================*/

typedef struct transform_component_t
{
	_base( component_t );
	gs_vqs transform;	
} transform_component_t;

typedef struct rigid_body_component_t
{
	_base( component_t );
	gs_vec2 velocity;
	aabb_t aabb;				
} rigid_body_component_t;

typedef struct sprite_component_t
{
	_base( component_t );
	gs_texture_t atlas;
	gs_vec4 uv;
} sprite_component_t;

typedef struct sprite_animation_component_t
{
	_base( component_t );
	sprite_frame_animation_asset_t* animation;
	u32 current_frame;
	f32 current_time;
} sprite_animation_component_t;

gs_slot_array_decl( transform_component_t );
gs_slot_array_decl( sprite_component_t );
gs_slot_array_decl( rigid_body_component_t );
gs_slot_array_decl( sprite_animation_component_t );

#define component_update(T)\
	component_update_##T


#define system_update(T)\
	system_update_##T

_force_inline
void component_update(sprite_animation_component_t)(sprite_animation_component_t* comps, u32 n, b32 print)
{
	gs_for_range_i(n)
	{
		sprite_animation_component_t* ac = &comps[i];
		sprite_frame_animation_asset_t* anim = ac->animation;
		u32 anim_frame_count = gs_dyn_array_size(anim->frames);
		ac->current_time += ac->animation->speed;
		if (ac->current_time >= 1.f)
		{
			ac->current_frame++;
			ac->current_time = 0.f;
		}
		// Render player animation
		ac->current_frame = ac->current_frame % anim_frame_count;
	}
}

_force_inline
void system_update(sprite_aabb)(rigid_body_component_t* _rc, sprite_component_t* _sc, transform_component_t* _tc, u32 n)
{
	gs_for_range_i(n)
	{
		rigid_body_component_t* rc = &_rc[i];
		sprite_component_t* sc = &_sc[i]; 
		transform_component_t* tc = &_tc[i];

		// Update AABB (collision)
		gs_vec4 uvs = sc->uv;

		// Width and height of UVs to scale the quads
		f32 tw = fabsf(uvs.z - uvs.x);
		f32 th = fabsf(uvs.w - uvs.y);

		gs_vqs xform = gs_vqs_default();
		xform.position = tc->transform.position;
		xform.scale = tc->transform.scale;

		// Define the object space quad for our player
		gs_vec4 bl = v4(-0.5f, -0.5f, 0.f, 1.f);
		gs_vec4 tr = v4(0.5f, 0.5f, 0.f, 1.f);

		// Define matrices for transformations
		gs_mat4 model_mtx = gs_vqs_to_mat4( &xform );

		// Transform verts
		bl = gs_mat4_mul_vec4( model_mtx, bl );			
		tr = gs_mat4_mul_vec4( model_mtx, tr );

		rc->aabb.min = v2(bl.x, bl.y);
		rc->aabb.max = v2(tr.x, tr.y);
	}
}

_force_inline
void system_update(sprite_anim_aabb)(rigid_body_component_t* _rc, sprite_animation_component_t* _sc, transform_component_t* _tc, u32 n)
{
	gs_for_range_i(n)
	{
		rigid_body_component_t* rc = &_rc[i];
		sprite_animation_component_t* sc = &_sc[i]; 
		transform_component_t* tc = &_tc[i];

		// Update AABB (collision)
		sprite_frame_animation_asset_t* anim = sc->animation;
		f32 t = gs_engine_instance()->ctx.platform->elapsed_time();
		sprite_frame_t* s = &anim->frames[sc->current_frame];
		gs_vec4 uvs = s->uvs;

		// Width and height of UVs to scale the quads
		f32 tw = fabsf(uvs.z - uvs.x);
		f32 th = fabsf(uvs.w - uvs.y);

		gs_vqs xform = gs_vqs_default();
		xform.position = tc->transform.position;
		// xform.scale.x = 0.8f;
		// xform.scale.z = 1.f;

		// Define the object space quad for our player
		gs_vec4 bl = v4(-0.5f, -0.5f, 0.f, 1.f);
		gs_vec4 tr = v4(0.5f, 0.5f, 0.f, 1.f);

		// Define matrices for transformations
		gs_mat4 model_mtx = gs_vqs_to_mat4( &xform );

		// Transform verts
		bl = gs_mat4_mul_vec4( model_mtx, bl );			
		tr = gs_mat4_mul_vec4( model_mtx, tr );

		rc->aabb.min = v2(bl.x, bl.y);
		rc->aabb.max = v2(tr.x, tr.y);
	}
}

/*=====================
// Bullet Entity Group 
=====================*/

typedef struct bullet_data
{
	gs_vec3 position;
	gs_vec2 velocity;
} bullet_data;

typedef struct entity_group( bullet_t )
{
	_base( entity_group_t );
	gs_dyn_array( u32 ) entities;
	gs_slot_array( transform_component_t ) 	transforms;
	gs_slot_array( sprite_component_t ) 	sprites;
	gs_slot_array( rigid_body_component_t ) rigid_bodies;
} entity_group( bullet_t );

entity_group_init_forward_decl( bullet_t );
entity_group_update_forward_decl( bullet_t );
entity_group_add_forward_decl( bullet_t );
entity_group_remove_forward_decl( bullet_t );
entity_group_shutdown_forward_decl( bullet_t );

/*======================
// Red Guy Entity Group
======================*/

typedef struct red_guy_data
{
	gs_vec3 position;
} red_guy_data;

typedef struct entity_group( red_guy_t )
{
	_base( entity_group_t );
	gs_dyn_array(u32) entities;
	gs_slot_array(transform_component_t) 	transforms;
	gs_slot_array(sprite_animation_component_t) animations;
	gs_slot_array(rigid_body_component_t) rigid_bodies;
} entity_group(red_guy_t);

entity_group_init_forward_decl( red_guy_t );
entity_group_update_forward_decl( red_guy_t );
entity_group_add_forward_decl( red_guy_t );
entity_group_remove_forward_decl( red_guy_t );
entity_group_shutdown_forward_decl( red_guy_t );



#endif