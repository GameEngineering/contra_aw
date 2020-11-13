#ifndef CONTRA_ENTITY_H
#define CONTRA_ENTITY_H

#include <gs.h>

#define entity_group( T )\
	entity_group_##T

typedef struct entity_group_t
{
	u32 count;
	u32 max_count;
	u32 free_id;
} entity_group_t;

#define entity_group_id_invalid u32_max

_force_inline
entity_group_t entity_group_default()
{
	entity_group_t eg = gs_default_val();
	eg.count = 0;
	eg.max_count = 0;
	eg.free_id = entity_group_id_invalid;
	return eg;
}

#define entity_group_init_decl( T, ... )\
	void __entity_group_init_##T( entity_group_##T* _group )\
		__VA_ARGS__

#define entity_group_update_decl( T, ... )\
	void __entity_group_update_##T( entity_group_##T* _group )\
		__VA_ARGS__

#define entity_group_shutdown_decl( T, ... )\
	void __entity_group_shutdown_##T( entity_group_##T* _group )\
		__VA_ARGS__

#define entity_group_add_decl( T, ... )\
	u32 __entity_group_add_##T( entity_group_##T* _group, void* _entity_data )\
		__VA_ARGS__

#define entity_group_remove_decl( T, ... )\
	void __entity_group_remove_##T( entity_group_##T* _group, u32 _id )\
		__VA_ARGS__

#define entity_group_init_forward_decl( T, ... )\
	void __entity_group_init_##T( entity_group_##T* _group )\
		__VA_ARGS__

#define entity_group_update_forward_decl( T, ... )\
	void __entity_group_update_##T( entity_group_##T* _group );

#define entity_group_shutdown_forward_decl( T, ... )\
	void __entity_group_shutdown_##T( entity_group_##T* _group );

#define entity_group_add_forward_decl( T, ... )\
	u32 __entity_group_add_##T( entity_group_##T* _group, void* _entity_data );

#define entity_group_remove_forward_decl( T, ... )\
	void __entity_group_remove_##T( entity_group_##T* _group, u32 _id );

#define entity_group_init( T, _group )\
	__entity_group_init_##T( _group )

#define entity_group_update( T, _group )\
	__entity_group_update_##T( _group )

#define entity_group_shutdown( T, _group )\
	__entity_group_shutdown_##T( _group )

#define entity_group_add( T, _group, _data )\
	__entity_group_add_##T( _group, _data )

#define entity_group_remove( T, _group, _id )\
	__entity_group_remove_##T( _group, _id )


#endif