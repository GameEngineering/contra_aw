#ifndef CONTRA_ASSET_MANAGER_H
#define CONTRA_ASSET_MANAGER_H

#include <gs.h>

#include "sprite.h"

// Declare hash table for texture type
gs_hash_table_decl( u64, gs_texture_t, gs_hash_u64, gs_hash_key_comp_std_type );
gs_slot_array_decl( gs_texture_t );
gs_slot_map_decl( u64, gs_texture_t );

typedef gs_audio_source_t* gs_audio_source_ptr;
gs_hash_table_decl( u64, gs_audio_source_ptr, gs_hash_u64, gs_hash_key_comp_std_type );
gs_slot_array_decl( gs_audio_source_ptr );
gs_slot_map_decl( u64, gs_audio_source_ptr );

typedef sprite_frame_animation_asset_t* sprite_frame_animation_asset_ptr;
gs_hash_table_decl( u64, sprite_frame_animation_asset_ptr, gs_hash_u64, gs_hash_key_comp_std_type );
gs_slot_array_decl( sprite_frame_animation_asset_ptr );
gs_slot_map_decl( u64, sprite_frame_animation_asset_ptr );

typedef struct asset_manager_t
{
	gs_slot_map( u64, gs_texture_t ) textures;
	gs_slot_map( u64, gs_audio_source_ptr ) audio;
	gs_slot_map( u64, sprite_frame_animation_asset_ptr ) sprite_animations;
} asset_manager_t;

_force_inline
asset_manager_t asset_manager_new()
{
	asset_manager_t am = gs_default_val();
	am.textures = gs_slot_map_new( u64, gs_texture_t );
	am.audio = gs_slot_map_new( u64, gs_audio_source_ptr );
	am.sprite_animations = gs_slot_map_new( u64, sprite_frame_animation_asset_ptr );
	return am;
}

_force_inline
void get_qualified_asset_name( char* buffer, usize sz, const char* file_path )
{
	char tmp[1024] = {0};

	// Lower case
	gs_util_str_to_lower(file_path, buffer, sz);

	// Replace '/' with '.'
	gs_util_string_replace(buffer, tmp, sz, '/', '.' );
	memset( buffer, 0, sz );

	// Remove first 9 characters
	gs_util_string_substring(tmp, buffer, sz, 9, gs_string_length(tmp) );
	memset(tmp, 0, 1024);

	// Strip out file extension (assuming .png for now), so remove last 4 characters
	gs_util_string_substring(buffer, tmp, sz, 0, gs_string_length(buffer) - 4);
	memset(buffer, 0, sz);
	memcpy(buffer, tmp, gs_string_length(tmp));
}

_force_inline
void __asset_manager_load_gs_texture_t( asset_manager_t* am, const char* file_path, gs_texture_parameter_desc desc )
{
	char buffer[256] = {0};

	// Need to qualify the asset name
	get_qualified_asset_name( buffer, 256, file_path );

	gs_graphics_i* gfx = gs_engine_instance()->ctx.graphics;

	// Load texture
	gs_texture_t tex = gfx->construct_texture_from_file( file_path, &desc );

	// Place texture into asset manager
	gs_slot_map_insert( am->textures, gs_hash_str_64(buffer), tex );
}

_force_inline
void __asset_manager_load_gs_audio_source_t( asset_manager_t* am, const char* file_path )
{
	char buffer[256] = {0};

	// Need to qualify the asset name
	get_qualified_asset_name( buffer, 256, file_path );

	gs_audio_i* audio = gs_engine_instance()->ctx.audio;

	// Constuct audio resource to play
	gs_audio_source_ptr src = audio->load_audio_source_from_file( file_path );

	// Place texture into asset manager
	gs_slot_map_insert( am->audio, gs_hash_str_64(buffer), src );
}

_force_inline
void __asset_manager_load_sprite_frame_animation_asset_t( asset_manager_t* am, const char* name, sprite_frame_t* frames, u32 count, f32 speed )
{
	gs_assert( count != 0 && frames != NULL );

	sprite_frame_animation_asset_t* anim = (sprite_frame_animation_asset_t*)gs_malloc( sizeof(sprite_frame_animation_asset_t) );
	anim->frames = gs_dyn_array_new( sprite_frame_t );
	anim->speed = speed;

	// Create sprite frame animation
	gs_for_range_i( count )
	{
		gs_dyn_array_push( anim->frames, frames[i] );
	}	

	// Place texture into asset manager
	gs_slot_map_insert( am->sprite_animations, gs_hash_str_64(name), anim );
}

_force_inline
gs_texture_t __asset_manager_get_gs_texture_t( asset_manager_t* am, const char* _id )
{
	u64 id = gs_hash_str_64( _id );
	return gs_slot_map_get( am->textures, id );
}

_force_inline
gs_audio_source_ptr __asset_manager_get_gs_audio_source_t( asset_manager_t* am, const char* _id )
{
	u64 id = gs_hash_str_64( _id );
	return gs_slot_map_get( am->audio, id );
}

_force_inline
sprite_frame_animation_asset_t* __asset_manager_get_sprite_frame_animation_asset_t( asset_manager_t* am, const char* _id )
{
	u64 id = gs_hash_str_64( _id );
	return gs_slot_map_get( am->sprite_animations, id );
}

#define asset_manager_load( am, T, file_path, ... )\
	__asset_manager_load_##T( &(am), file_path, __VA_ARGS__ )

#define asset_manager_get( am, T, id )\
	__asset_manager_get_##T( &(am), id )

#endif
