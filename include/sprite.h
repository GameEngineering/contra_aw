#ifndef CONTRA_SPRITE_H
#define CONTRA_SPRITE_H

#include <gs.h>

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

_force_inline
sprite_frame_t sprite_frame_t_new( gs_texture_t tex, gs_vec4 uv )
{
	sprite_frame_t frame = gs_default_val();
	frame.texture = tex;
	frame.uvs = uv;
	return frame;
}

#endif