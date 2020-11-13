#ifndef CONTRA_AABB_H
#define CONTRA_AABB_H

#include <gs.h>

#include "defines.h"

// AABBs
typedef struct aabb_t
{
	gs_vec2 min;
	gs_vec2 max;
} aabb_t;

// Collision Resolution: Minimum Translation Vector 
_force_inline
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
_force_inline
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

_force_inline
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


#endif