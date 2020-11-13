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

gs_slot_array_decl( transform_component_t );
gs_slot_array_decl( sprite_component_t );
gs_slot_array_decl( rigid_body_component_t );

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


#endif