#ifndef CONTRA_DEFINES_H
#define CONTRA_DEFINES_H

#include <gs.h>

// Useful Defines
#define use_animation_time 	true
#define player_scale_factor (1.f / 68.f)

_force_inline
void debug_print_transform( gs_vqs* xform )
{
	gs_println( "Transform: " );
	gs_println( "\tT: %.2f %.2f %.2f", xform->position.x, xform->position.y, xform->position.z );
	gs_println( "\tR: %.2f %.2f %.2f %.2f", xform->rotation.x, xform->rotation.y, xform->rotation.z, xform->rotation.w );
	gs_println( "\tS: %.2f %.2f %.2f", xform->scale.x, xform->scale.y, xform->scale.z );
}

_force_inline
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

_force_inline
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

_force_inline
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


#endif