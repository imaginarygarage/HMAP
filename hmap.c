/*********************************************************************************
 *
 *  FILENAME:
 *      hmap.c
 *  
 *  DESCRIPTION:
 *      Public hash mapping functions.
 *
 ********************************************************************************/


/*--------------------------------------------------------------------------------
                                     INCLUDES
--------------------------------------------------------------------------------*/

#include "hmap_intf.h"


/*--------------------------------------------------------------------------------
                             PREPROCESSOR DEFINITIONS
--------------------------------------------------------------------------------*/

#define HMAP_INVALID_POINTER    ( (void *)0 )


/*--------------------------------------------------------------------------------
                                      TYPES
--------------------------------------------------------------------------------*/

/*-------------------------------------------------------------
Map entry type. Typedef prior to struct definition in order to
reference own type (next and previous)
-------------------------------------------------------------*/
typedef struct hmap_entry_struct hmap_entry_type;
struct hmap_entry_struct
    {
    HMAP_anon_type      data;       /* pointer to entry data */
    HMAP_anon_type      key;        /* pointer to key data   */
    HMAP_hash_val_type  key_hash;   /* key's hashed value    */
    hmap_entry_type   * next;       /* next entry in bucket  */
    hmap_entry_type   * previous;   /* prev entry in bucket  */
    unsigned int        size;       /* size of entry in bytes*/
    };

/*-------------------------------------------------------------
The hash map's private data.
-------------------------------------------------------------*/
typedef struct hmap_map_struct
    {
    hmap_entry_type  ** buckets;    /* array of map buckets  */
    unsigned int        buckets_len;/* num buckets in map    */
    unsigned int        data_size;  /* total size of all data*/
    unsigned int        entry_count;/* num entries in map    */
    unsigned int        key_size;   /* total size of all keys*/
    unsigned int        size;       /* total size of map     */
    HMAP_hash_fptr_type hash;       /* hashing function      */
    HMAP_free_fptr      free;       /* deallocate memory     */
    HMAP_malloc_fptr    malloc;     /* allocae memory        */
    } hmap_map_type;


/*--------------------------------------------------------------------------------
                                 MEMORY CONSTANTS
--------------------------------------------------------------------------------*/


/*--------------------------------------------------------------------------------
                                 STATIC VARIABLES
--------------------------------------------------------------------------------*/


/*--------------------------------------------------------------------------------
                                    PROCEDURES
--------------------------------------------------------------------------------*/

static HMAP_bool_t8 anon_data_match
    (
    HMAP_anon_type    
                const * data_1,     /* anonymous data to be compared    */
    HMAP_anon_type    
                const * data_2      /* anonymous data to be compared    */
    );

static void copy_anon_data
    (
    HMAP_anon_type    * destination,/* copy source data here            */
    HMAP_anon_type    
                const * source      /* source of data to be copied      */
    );

static hmap_entry_type * create_entry
    (
    hmap_map_type     * map,        /* hash map private data            */
    HMAP_anon_type    
                const * key,        /* entry key                        */
    HMAP_anon_type    
                const * data        /* entry data                       */
    );

static void destroy_entry
    (
    hmap_map_type     * map,        /* hash map private data            */
    hmap_entry_type   * entry       /* entry to destroy                 */
    );

static hmap_entry_type ** get_bucket_by_hash
    (
    hmap_map_type     * map,        /* hash map private data            */
    HMAP_hash_val_type  key_hash    /* hash value of key                */
    );

static unsigned int get_entry_by_hash
    (
    hmap_map_type     * map,        /* hash map private data            */
    HMAP_hash_val_type  key_hash,   /* hash map entry key               */
    hmap_entry_type  ** entries,    /* out: array of entry ptrs for hash*/
    unsigned int        entries_len /* capacity of entries array        */
    );

static hmap_entry_type * get_entry_by_key
    (
    hmap_map_type     * map,        /* hash map private data            */
    HMAP_anon_type    
                const * key         /* hash map entry key               */
    );

static HMAP_hash_val_type hash_sdbm
    (
    HMAP_anon_type    
                const * key         /* hash map entry key               */
    );


/*************************************************************************
 *
 *  Procedure:
 *      HMAP_create
 *
 *  Description:
 *      Create a new hash map.
 *
 ************************************************************************/
HMAP_status_t8 HMAP_create
    (
    HMAP_def_type     * hmap_def,   /* hash map definition              */
    HMAP_obj_type     * out_obj     /* out: hash map object             */
    )
{
/*-------------------------------------------------------------
Local variables
-------------------------------------------------------------*/
unsigned int            i;
hmap_map_type         * map;

/*-------------------------------------------------------------
Verify inputs are not HMAP_INVALID_POINTER
-------------------------------------------------------------*/
if( hmap_def == HMAP_INVALID_POINTER 
 || out_obj  == HMAP_INVALID_POINTER )
    {
    return( HMAP_STATUS_INVALID_ARG );
    } 

/*-------------------------------------------------------------
Verify memory allocator and deallocator functions are defined.
-------------------------------------------------------------*/
if( hmap_def->malloc == HMAP_INVALID_POINTER 
 || hmap_def->free   == HMAP_INVALID_POINTER )
    {
    return( HMAP_STATUS_INVALID_DEF );
    } 

/*-------------------------------------------------------------
Initialize variables
-------------------------------------------------------------*/
map = HMAP_INVALID_POINTER;
out_obj->data = HMAP_INVALID_POINTER;

/*-------------------------------------------------------------
Allocate and initialize the private map data.
-------------------------------------------------------------*/
map = hmap_def->malloc( sizeof(*map) );
map->malloc = hmap_def->malloc;
map->free = hmap_def->free;
map->entry_count = 0;
map->data_size = 0;
map->key_size = 0;
map->buckets_len = hmap_def->map_size;
map->buckets = map->malloc( map->buckets_len * sizeof(*map->buckets) );
for( i = 0; i < map->buckets_len; i++ )
    {
    map->buckets[ i ] = HMAP_INVALID_POINTER;
    }
map->size = sizeof(*map) + sizeof(*map->buckets) * map->buckets_len;

/*-------------------------------------------------------------
Set the appropriate hashing function per map definition.
-------------------------------------------------------------*/
switch( hmap_def->hash_type )
    {
    case HMAP_HASH_FUNC_CUSTOM:
        /*-----------------------------------------------------
        A custom hash function must be provided if the defined
        hash type is HMAP_HASH_CUSTOM.
        -----------------------------------------------------*/
        if( hmap_def->hash == HMAP_INVALID_POINTER )
            {
            return( HMAP_STATUS_INVALID_DEF );
            }
        map->hash = hmap_def->hash;
        break;
    
    case HMAP_HASH_FUNC_SDBM:
    default:
        map->hash = hash_sdbm;
        break;
    }

/*-------------------------------------------------------------
Construct the public map object.
-------------------------------------------------------------*/
out_obj->data = map;

/*-------------------------------------------------------------
Object has been created successfully.
-------------------------------------------------------------*/
return( HMAP_STATUS_SUCCESS );

}   /* HMAP_create() */


/*************************************************************************
 *
 *  Procedure:
 *      HMAP_destroy
 *
 *  Description:
 *      Destroy the hash map object.
 *
 ************************************************************************/
HMAP_status_t8 HMAP_destroy
    (
    HMAP_obj_type     * obj       /* hash map object                    */
    )
{
/*-------------------------------------------------------------
Local variables
-------------------------------------------------------------*/
unsigned int            i;
hmap_entry_type       * entry;
hmap_map_type         * map;

/*-------------------------------------------------------------
Verify inputs are not HMAP_INVALID_POINTER
-------------------------------------------------------------*/
if( obj == HMAP_INVALID_POINTER )
    {
    return( HMAP_STATUS_INVALID_ARG );
    } 

/*-------------------------------------------------------------
Verify interface object has been successfully initialized.
-------------------------------------------------------------*/
if( obj->data == HMAP_INVALID_POINTER )
    {
    return( HMAP_STATUS_MAP_UNINITIALIZED );
    }

/*-------------------------------------------------------------
Initialize variables
-------------------------------------------------------------*/
map = (hmap_map_type *)obj->data;

/*-------------------------------------------------------------
Free the map entries.
-------------------------------------------------------------*/
for( i = 0; i < map->buckets_len; i++ )
    {
    while( map->buckets[ i ] != HMAP_INVALID_POINTER )
        {
        /*-----------------------------------------------------
        Pop the top entry from the bucket and then destroy it.
        -----------------------------------------------------*/
        entry = map->buckets[ i ];
        map->buckets[ i ] = entry->next;
        destroy_entry( map, entry );
        }
    }

/*-------------------------------------------------------------
Free the map buckets.
-------------------------------------------------------------*/
map->free( map->buckets );

/*-------------------------------------------------------------
Free the hash map.
-------------------------------------------------------------*/
map->free( map );
obj->data = HMAP_INVALID_POINTER;

/*-------------------------------------------------------------
The hash map object has been destroyed successfully.
-------------------------------------------------------------*/
return( HMAP_STATUS_SUCCESS );

}   /* HMAP_destroy() */


/*************************************************************************
 *
 *  Procedure:
 *      HMAP_get_data
 *
 *  Description:
 *      Get the data associated with the key.
 *
 ************************************************************************/
HMAP_status_t8 HMAP_get_data
    (
    HMAP_obj_type     * obj,        /* hash map object                  */
    const HMAP_anon_type
                      * key,        /* hash map entry key               */
    HMAP_anon_type    * data        /* out: entry data                  */
    )
{
/*-------------------------------------------------------------
Local variables
-------------------------------------------------------------*/
hmap_entry_type       * entry;
unsigned int            i;
hmap_map_type         * map;

/*-------------------------------------------------------------
Verify inputs are not HMAP_INVALID_POINTER.
-------------------------------------------------------------*/
if( obj  == HMAP_INVALID_POINTER 
 || key  == HMAP_INVALID_POINTER
 || data == HMAP_INVALID_POINTER )
    {
    return( HMAP_STATUS_INVALID_ARG );
    } 

/*-------------------------------------------------------------
Verify interface object has been successfully initialized.
-------------------------------------------------------------*/
if( obj->data == HMAP_INVALID_POINTER )
    {
    return( HMAP_STATUS_MAP_UNINITIALIZED );
    }

/*-------------------------------------------------------------
Get the private hash map data.
-------------------------------------------------------------*/
map = (hmap_map_type *)obj->data;

/*-------------------------------------------------------------
Get the entry associated with the key.
-------------------------------------------------------------*/
entry = get_entry_by_key( map, key );

/*-------------------------------------------------------------
Verify a valid entry was found.
-------------------------------------------------------------*/
if( entry == HMAP_INVALID_POINTER )
    {
    return( HMAP_STATUS_KEY_NOT_IN_MAP );
    }

/*-------------------------------------------------------------
Get the entry data.
-------------------------------------------------------------*/
copy_anon_data( data, &entry->data );

return( HMAP_STATUS_SUCCESS );

}   /* HMAP_get_data() */


/*************************************************************************
 *
 *  Procedure:
 *      HMAP_get_entry_count
 *
 *  Description:
 *      Get the number of entries in the map.
 *
 ************************************************************************/
HMAP_status_t8 HMAP_get_entry_count
    (
    HMAP_obj_type     * obj,        /* hash map object                  */
    unsigned int      * entry_count /* out: number of entries in map    */
    )
{
/*-------------------------------------------------------------
Local variables
-------------------------------------------------------------*/
hmap_entry_type       * entry;
hmap_map_type         * map;

/*-------------------------------------------------------------
Verify inputs are not HMAP_INVALID_POINTER.
-------------------------------------------------------------*/
if( obj  == HMAP_INVALID_POINTER )
    {
    return( HMAP_STATUS_INVALID_ARG );
    } 

/*-------------------------------------------------------------
Verify interface object has been successfully initialized.
-------------------------------------------------------------*/
if( obj->data == HMAP_INVALID_POINTER )
    {
    return( HMAP_STATUS_MAP_UNINITIALIZED );
    }

/*-------------------------------------------------------------
Get the private hash map data.
-------------------------------------------------------------*/
map = (hmap_map_type *)obj->data;

/*-------------------------------------------------------------
Get the entry associated with the key.
-------------------------------------------------------------*/
*entry_count = map->entry_count;

return( HMAP_STATUS_SUCCESS );

}   /* HMAP_get_entry_count() */


/*************************************************************************
 *
 *  Procedure:
 *      HMAP_get_hash
 *
 *  Description:
 *      Get the hash value of provided data.
 *
 ************************************************************************/
HMAP_status_t8 HMAP_get_hash
    (
    HMAP_obj_type     * obj,        /* hash map object                  */
    HMAP_anon_type
                const * data,       /* data to hash                     */
    HMAP_hash_val_type* hash        /* out: hash value of data          */
    )
{
/*-------------------------------------------------------------
Local variables
-------------------------------------------------------------*/
hmap_map_type         * map;

/*-------------------------------------------------------------
Verify inputs are not HMAP_INVALID_POINTER.
-------------------------------------------------------------*/
if( obj  == HMAP_INVALID_POINTER
 || data == HMAP_INVALID_POINTER
 || hash == HMAP_INVALID_POINTER )
    {
    return( HMAP_STATUS_INVALID_ARG );
    } 

/*-------------------------------------------------------------
Verify interface object has been successfully initialized.
-------------------------------------------------------------*/
if( obj->data == HMAP_INVALID_POINTER )
    {
    return( HMAP_STATUS_MAP_UNINITIALIZED );
    }

/*-------------------------------------------------------------
Get the private hash map data.
-------------------------------------------------------------*/
map = (hmap_map_type *)obj->data;

/*-------------------------------------------------------------
Hash the given data.
-------------------------------------------------------------*/
*hash = map->hash( data );

return( HMAP_STATUS_SUCCESS );

}   /* HMAP_get_hash() */


/*************************************************************************
 *
 *  Procedure:
 *      HMAP_get_size
 *
 *  Description:
 *      Get the total memory allocated to the hash map.
 *
 ************************************************************************/
HMAP_status_t8 HMAP_get_size
    (
    HMAP_obj_type     * obj,        /* hash map object                  */
    unsigned int      * size        /* out: total size of map (bytes)   */
    )
{
/*-------------------------------------------------------------
Local variables
-------------------------------------------------------------*/
hmap_entry_type       * entry;
hmap_map_type         * map;

/*-------------------------------------------------------------
Verify inputs are not HMAP_INVALID_POINTER.
-------------------------------------------------------------*/
if( obj  == HMAP_INVALID_POINTER )
    {
    return( HMAP_STATUS_INVALID_ARG );
    } 

/*-------------------------------------------------------------
Verify interface object has been successfully initialized.
-------------------------------------------------------------*/
if( obj->data == HMAP_INVALID_POINTER )
    {
    return( HMAP_STATUS_MAP_UNINITIALIZED );
    }

/*-------------------------------------------------------------
Get the private hash map data.
-------------------------------------------------------------*/
map = (hmap_map_type *)obj->data;

/*-------------------------------------------------------------
Get the size of the hash map.
-------------------------------------------------------------*/
*size = map->size;

return( HMAP_STATUS_SUCCESS );

}   /* HMAP_get_size() */


/*************************************************************************
 *
 *  Procedure:
 *      HMAP_key_in_map
 *
 *  Description:
 *      Returns true if the key exists in the given map.
 *
 ************************************************************************/
HMAP_bool_t8 HMAP_key_in_map
    (
    HMAP_obj_type     * obj,        /* hash map object                  */
    const HMAP_anon_type    
                      * key         /* hash map entry key               */
    )
{
/*-------------------------------------------------------------
Local variables
-------------------------------------------------------------*/
hmap_entry_type       * entry;
hmap_map_type         * map;

/*-------------------------------------------------------------
Verify inputs are not HMAP_INVALID_POINTER.
-------------------------------------------------------------*/
if( obj  == HMAP_INVALID_POINTER 
 || key  == HMAP_INVALID_POINTER )
    {
    return( HMAP_BOOL_FALSE );
    } 

/*-------------------------------------------------------------
Verify interface object has been successfully initialized.
-------------------------------------------------------------*/
if( obj->data == HMAP_INVALID_POINTER )
    {
    return( HMAP_BOOL_FALSE );
    }

/*-------------------------------------------------------------
Get the private hash map data.
-------------------------------------------------------------*/
map = (hmap_map_type *)obj->data;

/*-------------------------------------------------------------
Get the entry associated with the key.
-------------------------------------------------------------*/
entry = get_entry_by_key( map, key );

/*-------------------------------------------------------------
Verify a valid entry was found.
-------------------------------------------------------------*/
return( entry != HMAP_INVALID_POINTER );

}   /* HMAP_key_in_map() */


/*************************************************************************
 *
 *  Procedure:
 *      HMAP_remove_entry
 *
 *  Description:
 *      Remove the entry for the given key from the map.
 *
 ************************************************************************/
HMAP_status_t8 HMAP_remove_entry
    (
    HMAP_obj_type     * obj,        /* hash map object                  */
    const HMAP_anon_type    
                      * key         /* hashmap entry key                */
    )
{
/*-------------------------------------------------------------
Local variables
-------------------------------------------------------------*/
hmap_entry_type      ** bucket;
hmap_entry_type       * entry;
HMAP_hash_val_type      hash;
unsigned int            i;
unsigned int            index;
hmap_map_type         * map;

/*-------------------------------------------------------------
Verify inputs are not HMAP_INVALID_POINTER.
-------------------------------------------------------------*/
if( obj  == HMAP_INVALID_POINTER 
 || key  == HMAP_INVALID_POINTER )
    {
    return( HMAP_STATUS_INVALID_ARG );
    } 

/*-------------------------------------------------------------
Verify interface object has been successfully initialized.
-------------------------------------------------------------*/
if( obj->data == HMAP_INVALID_POINTER )
    {
    return( HMAP_STATUS_MAP_UNINITIALIZED );
    }

/*-------------------------------------------------------------
Resolve the map structure.
-------------------------------------------------------------*/
map = (hmap_map_type *)obj->data;

/*-------------------------------------------------------------
Find the matching entry in the map.
-------------------------------------------------------------*/
entry = get_entry_by_key( map, key );

/*-------------------------------------------------------------
Remove the entry if it was found to exist.
-------------------------------------------------------------*/
if( entry != HMAP_INVALID_POINTER )
    {
    /*---------------------------------------------------------
    Remove the entry from the bucket's linked list and then 
    destroy it.
    ---------------------------------------------------------*/
    if( entry->previous != HMAP_INVALID_POINTER )
        {
        entry->previous->next = entry->next;
        if( entry->next != HMAP_INVALID_POINTER )
            {
            entry->next->previous = entry->previous;
            }
        }
    else
        {
        bucket = get_bucket_by_hash( map, entry->key_hash );
        *bucket = entry->next;
        }

    destroy_entry( map, entry );
    }

return( HMAP_STATUS_SUCCESS );

}   /* HMAP_remove_entry() */


/*************************************************************************
 *
 *  Procedure:
 *      HMAP_set_data
 *
 *  Description:
 *      Set the data associated with the key. If an entry for the key does
 *      not already exist, an entry will be created.
 *
 ************************************************************************/
HMAP_status_t8 HMAP_set_data
    (
    HMAP_obj_type     * obj,        /* hash map object                  */
    const HMAP_anon_type    
                      * key,        /* hashmap entry key                */
    const HMAP_anon_type    
                      * data        /* entry data                       */
    )
{
/*-------------------------------------------------------------
Local variables
-------------------------------------------------------------*/
hmap_entry_type      ** bucket;
hmap_entry_type       * entry;
HMAP_hash_val_type      hash;
unsigned int            index;
hmap_map_type         * map;

/*-------------------------------------------------------------
Verify inputs are not HMAP_INVALID_POINTER.
-------------------------------------------------------------*/
if( obj  == HMAP_INVALID_POINTER 
 || key  == HMAP_INVALID_POINTER
 || data == HMAP_INVALID_POINTER )
    {
    return( HMAP_STATUS_INVALID_ARG );
    } 

/*-------------------------------------------------------------
Verify interface object has been successfully initialized.
-------------------------------------------------------------*/
if( obj->data == HMAP_INVALID_POINTER )
    {
    return( HMAP_STATUS_MAP_UNINITIALIZED );
    }

/*-------------------------------------------------------------
Initialize variables
-------------------------------------------------------------*/
map = (hmap_map_type *)obj->data;

/*-------------------------------------------------------------
Find the matching entry.
-------------------------------------------------------------*/
entry = get_entry_by_key( map, key );

/*-------------------------------------------------------------
Create a new entry if no matching entry was found.
-------------------------------------------------------------*/
if( entry == HMAP_INVALID_POINTER )
    {
    /*---------------------------------------------------------
    Create the new entry.
    ---------------------------------------------------------*/
    entry = create_entry( map, key, data );

    /*---------------------------------------------------------
    Push the new entry to the top of its bucket.
    ---------------------------------------------------------*/
    bucket = get_bucket_by_hash( map, entry->key_hash );
    entry->next = *bucket;
    if( entry->next )
        {
        entry->next->previous = entry;
        }
    *bucket = entry;
    }

/*-------------------------------------------------------------
Update the size of the data, if necessary.
-------------------------------------------------------------*/
if( entry->data.size != data->size )
    {
    map->data_size += data->size - entry->data.size;
    map->size += data->size - entry->data.size;

    map->free( entry->data.ptr );
    entry->data.ptr = map->malloc( data->size );
    entry->data.size = data->size;
    }

/*-------------------------------------------------------------
Set the entry data.
-------------------------------------------------------------*/
copy_anon_data( &entry->data, data );

return( HMAP_STATUS_SUCCESS );

}   /* HMAP_set_data() */


/*************************************************************************
 *
 *  Procedure:
 *      anon_data_match
 *
 *  Description:
 *      Returns true if the specified memory blocks match.
 *
 ************************************************************************/
static HMAP_bool_t8 anon_data_match
    (
    HMAP_anon_type    
                const * data_1,     /* anonymous data to be compared    */
    HMAP_anon_type    
                const * data_2      /* anonymous data to be compared    */
    )
{
/*-------------------------------------------------------------
Local variables
-------------------------------------------------------------*/
unsigned int            i;
HMAP_bool_t8            match;

/*-------------------------------------------------------------
First, check if the sizes match.
-------------------------------------------------------------*/
if( data_1->size != data_2->size )
    {
    return( HMAP_BOOL_FALSE );
    }

/*-------------------------------------------------------------
Check if the data is a byte-for-byte match.
-------------------------------------------------------------*/
for( i = 0; i < data_1->size; i++ )
    {
    if( ( (char *)( data_1->ptr ) )[ i ] != ( (char *)( data_2->ptr ) )[ i ] )
        {
        return( HMAP_BOOL_FALSE );
        }
    }

return( HMAP_BOOL_TRUE );

}   /* anon_data_match() */


/*************************************************************************
 *
 *  Procedure:
 *      copy_anon_data
 *
 *  Description:
 *      Copy anonymous data from a source to tha destination.
 *
 ************************************************************************/
static void copy_anon_data
    (
    HMAP_anon_type    * destination,/* copy source data here            */
    HMAP_anon_type    
                const * source      /* source of data to be copied      */
    )
{
/*-------------------------------------------------------------
Local variables
-------------------------------------------------------------*/
unsigned int            i;

/*-------------------------------------------------------------
Copy the source data to the destination.
-------------------------------------------------------------*/
destination->size = source->size;
for( i = 0; i < source->size; i++ )
    {
    ( (char *)( destination->ptr ) )[ i ] = ( (char *)( source->ptr ) )[ i ];
    }

}   /* copy_anon_data() */

/*************************************************************************
 *
 *  Procedure:
 *      create_entry
 *
 *  Description:
 *      Allocate and define a new map entry.
 *
 ************************************************************************/
static hmap_entry_type * create_entry
    (
    hmap_map_type     * map,        /* hash map private data            */
    HMAP_anon_type    
                const * key,        /* entry key                        */
    HMAP_anon_type    
                const * data        /* entry data                       */
    )
{
/*-------------------------------------------------------------
Local variables
-------------------------------------------------------------*/
hmap_entry_type       * entry;

/*-------------------------------------------------------------
Allocate and define the new map entry.
-------------------------------------------------------------*/
entry = map->malloc( sizeof( *entry ) );
entry->data.size = data->size;
entry->data.ptr = map->malloc( entry->data.size );
entry->key.size = key->size;
entry->key_hash = map->hash( key );
entry->next = HMAP_INVALID_POINTER;
entry->previous = HMAP_INVALID_POINTER;
entry->key.ptr = map->malloc( entry->key.size );
copy_anon_data( &entry->key, key );
entry->size = entry->data.size + entry->key.size + sizeof( *entry );

/*-------------------------------------------------------------
Update map entry count and size data.
-------------------------------------------------------------*/
map->entry_count++;
map->data_size += data->size;
map->key_size += key->size;
map->size += entry->size;

return( entry );

}   /* create_entry() */


/*************************************************************************
 *
 *  Procedure:
 *      destroy_entry
 *
 *  Description:
 *      Deallocate entry memory.
 *
 ************************************************************************/
static void destroy_entry
    (
    hmap_map_type     * map,        /* hash map private data            */
    hmap_entry_type   * entry       /* entry to destroy                 */
    )
{
/*-------------------------------------------------------------
Update map entry count and size data.
-------------------------------------------------------------*/
map->entry_count--;
map->data_size -= entry->data.size;
map->key_size -= entry->key.size;
map->size -= entry->size;

/*-------------------------------------------------------------
Free the entry memory.
-------------------------------------------------------------*/
map->free( entry->data.ptr );
map->free( entry->key.ptr );
map->free( entry );

}   /* destroy_entry() */


/*************************************************************************
 *
 *  Procedure:
 *      get_bucket_by_hash
 *
 *  Description:
 *      Get a pointer to the map bucket associated with this hash value.
 *
 ************************************************************************/
static hmap_entry_type ** get_bucket_by_hash
    (
    hmap_map_type     * map,        /* hash map private data            */
    HMAP_hash_val_type  key_hash    /* hash value of key                */
    )
{
/*-------------------------------------------------------------
Local variables
-------------------------------------------------------------*/
unsigned int            index;

/*-------------------------------------------------------------
Convert the key's hash to its bucket index.
-------------------------------------------------------------*/
index = key_hash % map->buckets_len;

/*-------------------------------------------------------------
Return the bucket.
-------------------------------------------------------------*/
return( &map->buckets[ index ] );

}   /* get_bucket_by_hash() */


/*************************************************************************
 *
 *  Procedure:
 *      get_entry_by_hash
 *
 *  Description:
 *      Get pointers to the entries associated with this hash. Returns
 *      the number of entries found to match the given hash and writes
 *      them to the entries array.
 *
 ************************************************************************/
static unsigned int get_entry_by_hash
    (
    hmap_map_type     * map,        /* hash map private data            */
    HMAP_hash_val_type  key_hash,   /* hash map entry key               */
    hmap_entry_type  ** entries,    /* out: array of entry ptrs for hash*/
    unsigned int        entries_len /* capacity of entries array        */
    )
{
/*-------------------------------------------------------------
Local variables
-------------------------------------------------------------*/
hmap_entry_type      ** bucket;
hmap_entry_type       * entry;

/*-------------------------------------------------------------
Get the bucket containing the key's hash.
-------------------------------------------------------------*/
bucket = get_bucket_by_hash( map, key_hash );

/*-------------------------------------------------------------
Find all entries matching the hash.
-------------------------------------------------------------*/
entries_len = 0;
entry = *bucket;
while( entry != HMAP_INVALID_POINTER )
    {
    if( entry->key_hash == key_hash )
        {
        entries[ entries_len++ ] = entry;
        }
    entry = entry->next;
    }

/*-------------------------------------------------------------
Return the number of matching entries found.
-------------------------------------------------------------*/
return( entries_len );

}   /* get_entry_by_hash() */


/*************************************************************************
 *
 *  Procedure:
 *      get_entry_by_key
 *
 *  Description:
 *      Get a pointer to the entry associated with this key. Returns
 *      HMAP_INVALID_POINTER if the key does not exist in the hash map.
 *
 ************************************************************************/
static hmap_entry_type * get_entry_by_key
    (
    hmap_map_type     * map,        /* hash map private data            */
    HMAP_anon_type    
                const * key         /* hash map entry key               */
    )
{
/*-------------------------------------------------------------
Local preprocessor definitions
-------------------------------------------------------------*/
#define MAX_COLLISIONS  ( 5 )

/*-------------------------------------------------------------
Local variables
-------------------------------------------------------------*/
hmap_entry_type       * entries[ MAX_COLLISIONS ];
unsigned int            entries_count;
hmap_entry_type       * entry;
HMAP_anon_type        * entry_key;
unsigned int            i;
unsigned int            key_hash;

/*-------------------------------------------------------------
Calculate the key's hash and get the matching entries.
-------------------------------------------------------------*/
key_hash = map->hash( key );
entries_count = get_entry_by_hash( map, key_hash, entries, MAX_COLLISIONS );

/*-------------------------------------------------------------
Find the matching entry.
-------------------------------------------------------------*/
entry = HMAP_INVALID_POINTER;
for( i = 0; i < entries_count; i++ )
    {
    entry_key = &entries[ i ]->key;
    if( anon_data_match( entry_key, key ) )
        {
        entry = entries[ i ];
        break;
        }
    }
    
/*-------------------------------------------------------------
Returns an invalid pointer if no entry was found.
-------------------------------------------------------------*/
return( entry );

/*-------------------------------------------------------------
Clean up local preprocessor definitions.
-------------------------------------------------------------*/
#undef MAX_COLLISIONS

}   /* get_entry_by_key() */


/*************************************************************************
 *
 *  Procedure:
 *      hash_sdbm
 *
 *  Description:
 *      The default hash function implements the SDBM algorithm
 *
 ************************************************************************/
static HMAP_hash_val_type hash_sdbm
    (
    const HMAP_anon_type
                      * key         /* hash map entry key               */
    )
{
/*-------------------------------------------------------------
Local variables
-------------------------------------------------------------*/
HMAP_hash_val_type      hash;
unsigned int            i;
unsigned char         * key_bytes;

/*-------------------------------------------------------------
Initalize variables
-------------------------------------------------------------*/
hash = 0;
key_bytes = (unsigned char *)( key->ptr );

/*-------------------------------------------------------------
Hash the key
-------------------------------------------------------------*/
for( i = 0; i < key->size; i++ )
    {
    /*---------------------------------------------------------
    hash = ( hash * 65599 ) + byte
    ---------------------------------------------------------*/
    hash = (hash << 16) + (hash << 6) - hash + key_bytes[ i ];
    }

return( hash );

}   /* hash_sdbm() */
