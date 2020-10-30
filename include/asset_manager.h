#ifndef CONTRA_ASSET_MANAGER_H
#define CONTRA_ASSET_MANAGER_H

#include <gs.h>

// Declare hash table for texture type
gs_hash_table_decl( u64, gs_texture_t, gs_hash_u64, gs_hash_key_comp_std_type );
gs_slot_array_decl( gs_texture_t );
gs_slot_map_decl( u64, gs_texture_t );

typedef struct asset_manager_t
{
	gs_slot_map( u64, gs_texture_t ) textures;
} asset_manager_t;

_force_inline
asset_manager_t asset_manager_new()
{
	asset_manager_t am = gs_default_val();
	am.textures = gs_slot_map_new( u64, gs_texture_t );
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
gs_texture_t __asset_manager_get_gs_texture_t( asset_manager_t* am, const char* _id )
{
	u64 id = gs_hash_str_64( _id );
	return gs_slot_map_get( am->textures, id );
}

#define asset_manager_load( am, T, file_path, ... )\
	__asset_manager_load_##T( &(am), file_path, __VA_ARGS__ )

#define asset_manager_get( am, T, id )\
	__asset_manager_get_##T( &(am), id )

#endif
