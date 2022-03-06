/*********************************************************************************
 *
 *  FILENAME:
 *      hmap_intf.h
 *  
 *  DESCRIPTION:
 *      The hash map interface.
 *
 ********************************************************************************/


#ifndef HMAP_INTF_H_GUARD
#define HMAP_INTF_H_GUARD


/*--------------------------------------------------------------------------------
                                     INCLUDES
--------------------------------------------------------------------------------*/


/*--------------------------------------------------------------------------------
                             PREPROCESSOR DEFINITIONS
--------------------------------------------------------------------------------*/


/*--------------------------------------------------------------------------------
                                      TYPES
--------------------------------------------------------------------------------*/

/*-------------------------------------------------------------
Return status of HMAP interface functions.
-------------------------------------------------------------*/
typedef unsigned char HMAP_bool_t8;
enum
    {
    HMAP_BOOL_FALSE,
    HMAP_BOOL_TRUE
    };

/*-------------------------------------------------------------
Return status of HMAP interface functions.
-------------------------------------------------------------*/
typedef unsigned char HMAP_status_t8;
enum
    {
    HMAP_STATUS_SUCCESS,
    HMAP_STATUS_INVALID_ARG,
    HMAP_STATUS_INVALID_DEF,
    HMAP_STATUS_KEY_NOT_IN_MAP,
    HMAP_STATUS_MAP_UNINITIALIZED,

    HMAP_STATUS_COUNT
    };

/*-------------------------------------------------------------
Hash values are unsigned integers.
-------------------------------------------------------------*/
typedef unsigned int HMAP_hash_val_type;

/*-------------------------------------------------------------
Hash function used in hash map.
-------------------------------------------------------------*/
typedef unsigned char HMAP_hash_func_t8;
enum
    {
    HMAP_HASH_FUNC_SDBM,
    HMAP_HASH_FUNC_CUSTOM,

    HMAP_HASH_FUNC_COUNT
    };

/*-------------------------------------------------------------
Anonymous data type.
-------------------------------------------------------------*/
typedef struct
    {
    void              * ptr;
    unsigned int        size;
    } HMAP_anon_type;

/*-------------------------------------------------------------
Hash functions shall take a pointer to the key to be hashed, as
well as the size of the key in bytes, and return the resulting
hash as an unsigned integer.
-------------------------------------------------------------*/
typedef HMAP_hash_val_type HMAP_hash_func_type
    (
    const HMAP_anon_type  * key     /* key to hash           */
    );

typedef HMAP_hash_func_type * HMAP_hash_fptr_type;

/*-------------------------------------------------------------
Function for allocating memory.
-------------------------------------------------------------*/
typedef void * HMAP_malloc_func
    (
    unsigned long long  size        /* num bytes to allocate */
    );

typedef HMAP_malloc_func * HMAP_malloc_fptr;

/*-------------------------------------------------------------
Function for freeing allocated memory.
-------------------------------------------------------------*/
typedef void HMAP_free_func
    (
    void              * memory      /* memory block to free  */
    );

typedef HMAP_free_func * HMAP_free_fptr;

/*-------------------------------------------------------------
Hash map definition.
-------------------------------------------------------------*/
typedef struct
    {
    unsigned int        map_size;   /* number of buckets/map */
    HMAP_hash_func_t8   hash_type;  /* hash algorithm to use */
    HMAP_hash_fptr_type hash;       /* custom hash function  */
    HMAP_malloc_fptr    malloc;     /* memory allocator      */
    HMAP_free_fptr      free;       /* memory deallocator    */
    } HMAP_def_type;

/*-------------------------------------------------------------
The public hash map object.
-------------------------------------------------------------*/
typedef struct
    {
    void              * data;
    } HMAP_obj_type;


/*--------------------------------------------------------------------------------
                                 MEMORY CONSTANTS
--------------------------------------------------------------------------------*/


/*--------------------------------------------------------------------------------
                                 STATIC VARIABLES
--------------------------------------------------------------------------------*/


/*--------------------------------------------------------------------------------
                                    PROCEDURES
--------------------------------------------------------------------------------*/

HMAP_status_t8 HMAP_create
    (
    HMAP_def_type     * hmap_def,   /* hash map definition              */
    HMAP_obj_type     * out_obj     /* out: hash map object             */
    );

    
HMAP_status_t8 HMAP_destroy
    (
    HMAP_obj_type     * obj         /* hash map object                  */
    );

HMAP_status_t8 HMAP_get_data
    (
    HMAP_obj_type     * obj,        /* hash map object                  */
    const HMAP_anon_type
                      * key,        /* hash map entry key               */
    HMAP_anon_type    * data        /* out: entry data                  */
    );

HMAP_status_t8 HMAP_get_entry_count
    (
    HMAP_obj_type     * obj,        /* hash map object                  */
    unsigned int      * entry_count /* out: number of entries in map    */
    );

HMAP_status_t8 HMAP_get_hash
    (
    HMAP_obj_type     * obj,        /* hash map object                  */
    HMAP_anon_type
                const * data,       /* data to hash                     */
    HMAP_hash_val_type* hash        /* out: hash value of data          */
    );

HMAP_status_t8 HMAP_get_size
    (
    HMAP_obj_type     * obj,        /* hash map object                  */
    unsigned int      * size        /* out: total size of map (bytes)   */
    );

HMAP_bool_t8 HMAP_key_in_map
    (
    HMAP_obj_type     * obj,        /* hash map object                  */
    const HMAP_anon_type    
                      * key         /* hash map entry key               */
    );

HMAP_status_t8 HMAP_remove_entry
    (
    HMAP_obj_type     * obj,        /* hash map object                  */
    const HMAP_anon_type    
                      * key         /* hashmap entry key                */
    );

HMAP_status_t8 HMAP_set_data
    (
    HMAP_obj_type     * obj,        /* hash map object                  */
    const HMAP_anon_type    
                      * key,        /* hashmap entry key                */
    const HMAP_anon_type    
                      * data        /* entry data                       */
    );


#endif /* HMAP_INTF_H_GUARD */
