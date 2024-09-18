/* Copyright (C) 2001-2023 Artifex Software, Inc.
   All Rights Reserved.

   This software is provided AS-IS with no warranty, either express or
   implied.

   This software is distributed under license and may not be copied,
   modified or distributed except as expressly authorized under the terms
   of the license contained in the file LICENSE in this distribution.

   Refer to licensing information at http://www.artifex.com or contact
   Artifex Software, Inc.,  39 Mesa Street, Suite 108A, San Francisco,
   CA 94129, USA, for further information.
*/


/* Definition of structure type descriptors and extern_st */

#ifndef gsstype_INCLUDED
#  define gsstype_INCLUDED

#include "gsmemory.h"

/* Define an opaque type for the garbage collector state. */
typedef struct gc_state_s gc_state_t;

/*
 * Define the structure used to return an enumerated pointer.  Ordinary
 * object pointers use only the ptr element; strings also use size.
 */
typedef struct enum_ptr_s {
    const void *ptr;
    uint size;
} enum_ptr_t;

/*
 * The first argument of enum_ptrs procedures formerly was not const *, and
 * EV_CONST was defined as empty.  Unfortunately, changing EV_CONST to const
 * produced many compiler warnings from places that cast this argument to a
 * non-const non-void pointer type.
 */
#define EV_CONST const

/* Define the procedures for structure types. */

                /* Clear the marks of a structure. */

#define struct_proc_clear_marks(proc)\
  void proc(const gs_memory_t *cmem, void /*obj_header_t*/ *pre, uint size,\
    const gs_memory_struct_type_t *pstype)

                /* Enumerate the pointers in a structure. */

#define struct_proc_enum_ptrs(proc)\
  gs_ptr_type_t proc(const gs_memory_t *mem, EV_CONST void /*obj_header_t*/ *ptr, uint size,\
    int index, enum_ptr_t *pep, const gs_memory_struct_type_t *pstype,\
    gc_state_t *gcst)

                /* Relocate all the pointers in this structure. */

#define struct_proc_reloc_ptrs(proc)\
  void proc(void /*obj_header_t*/ *ptr, uint size,\
    const gs_memory_struct_type_t *pstype, gc_state_t *gcst)

                /*
                 * Finalize this structure just before freeing it.
                 * Finalization procedures must not allocate or resize
                 * any objects in any space managed by the allocator,
                 * and must not assume that any objects in such spaces
                 * referenced by this structure still exist.  However,
                 * finalization procedures may free such objects, and
                 * may allocate, free, and reference objects allocated
                 * in other ways, such as objects allocated with malloc
                 * by libraries.
                 */

#define struct_proc_finalize(proc)\
  void proc(const gs_memory_t *cmem, void /*obj_header_t*/ *ptr)

/*
 * A descriptor for an object (structure) type.
 */
typedef struct struct_shared_procs_s struct_shared_procs_t;

/*
 * Objects within the 'obj' allocator are currently limited to be uint's,
 * even though the interface allows for size_t's. This means that gc'd
 * allocations are never bigger than 4Gig. To change this, change the
 * following type, at the cost of more overhead for each object.
 */
typedef uint obj_size_t;

struct gs_memory_struct_type_s {
        obj_size_t ssize;
        struct_name_t sname;

        /* ------ Procedures shared among many structure types. ------ */
        /* Note that this pointer is usually 0. */

        const struct_shared_procs_t *shared;

        /* ------ Procedures specific to this structure type. ------ */

        struct_proc_clear_marks((*clear_marks));
        struct_proc_enum_ptrs((*enum_ptrs));
        struct_proc_reloc_ptrs((*reloc_ptrs));
        struct_proc_finalize((*finalize));

        /* A pointer to additional data for the above procedures. */

        const void *proc_data;
};

/*
 * Because of bugs in some compilers' bookkeeping for undefined structure
 * types, any file that uses extern_st must include this file.
 * (If it weren't for these bugs, the definition of extern_st could
 * go in gsmemory.h.)
 */
#define extern_st(st) extern const gs_memory_struct_type_t st

#endif /* gsstype_INCLUDED */
