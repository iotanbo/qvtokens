
#ifndef M01_ENUMS_AND_DEFS_HPP
#define M01_ENUMS_AND_DEFS_HPP


// Frontend connection initial buffer size.
#define M01_FRONTEND_CONN_INITIAL_BUF_SIZE 128


// Custom [M01 app] object type enumeration.
// Every user object assigned to uv's [handle->data] 
// must have fist field [M01ObjType user_obj_type].
typedef enum {
    M01OBJ_FRONTEND_CONN_CONTEXT = 1,


} M01ObjType;


#endif  // M01_ENUMS_AND_DEFS_HPP