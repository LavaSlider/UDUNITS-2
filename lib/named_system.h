/**
 * @author David W. Stockton
 *
 * void namedSystemFreeSystem( ut_system* system )
 *
 */

#ifndef UT_NAMED_SYSTEM_H
#define UT_NAMED_SYSTEM_H

#include "udunits2.h"

#ifdef __cplusplus
extern "C" {
#endif

#define AUTO_CREATE_NAMED_SYSTEMS
#undef AUTO_CREATE_NAMED_SYSTEMS

/*
 * Frees resources associated with a unit-system.
 *
 * Arguments:
 *	system		Pointer to the unit-system to have its associated
 *			resources freed.
 */
void
namedSystemFreeSystem(
    ut_system*	system);


/* Definition of where named system information is stored for a unit */
typedef struct Bitmap NamedSystemRegistry;

/*
 * Adds the "system_name" from the system to the bitmap.
 *
 * Returns:
 *    bitmap	The bitmap passed, or a newly allocated one if NULL
 *		was passed, updated to reference the "system_name".
 *		The "system_name" is automatically added to the
 *		system if needed.
 *    ut_get_status() will return:
 *	UT_BAD_ARG	If system or system_name is NULL or system_name
 *			is empty.
 *	UT_OS		If unable to allocate needed space.
 *	UT_SUCCESS	The registry was properly updated.
 *
 * usage:
 *     NamedSystemRegistry*	registry = NULL;
 *     registry = utSetNamedSystemInRegistry( system, registry, system_name );
 *     if( ut_get_status() != UT_SUCCESS ) {
 *	   fprintf( stderr, "Problem\n" );
 *     }
 */
NamedSystemRegistry*
utSetNamedSystemInRegistry(
    ut_system* const		system,
    NamedSystemRegistry*	bitmap,
    const char* const		system_name);

/*
 * Clears the reference to the system_name in the system from the
 * bitmap passed.
 *
 * Returns:
 *    UT_SUCCESS	If successfully cleared. This includes if
 *			it was not set in the first place.
 *    UT_BAD_ARG	If system is NULL or system_named is NULL
 *			or empty. This is not returned if bitmap
 *			is NULL since that is equivalent to the
 *			bit not being set.
 *    UT_UNKOWN		The system_name is not defined in the
 *			system. Depending on the circumstances this
 *			might not be considered an error.
 *
 * usage:
 *    utRemoveNamedSystemFromRegistry( system, registry, system_name );
 */
ut_status
utRemoveNamedSystemFromRegistry(
    ut_system* const		system,
    NamedSystemRegistry*	bitmap,
    const char* const		system_name);
/*
 * Tests whether the "system_name" from the system is set in the bitmap.
 *
 * The bitmap parameter can be NULL, which is equivalent to a bitmap with
 * no bits set.
 *
 * Returns:
 *    non-zero	If the "system_name" is set in the bitmap provided.
 *    0		If the "system_name" is not set in the bitmap provided
 *		or if there is some error.
 *
 * ut_get_status() returns:
 *	UT_SUCCESS	If all went well
 *	UT_UNKNOWN	If "system_name" is not defined for the system
 */
int
utNamedSystemIsInRegistry(
    ut_system* const		system,
    NamedSystemRegistry*	bitmap,
    const char* const		system_name);
/*
 * Frees the bitmap and any allocated storage within it.
 */
void
utNamedSystemRegistryFree(
    NamedSystemRegistry*	bitmap);

/*============================================================================*/
/* These should probably go away... */
ut_status
utSetNamedSystemInRegistryLocation(
    ut_system* const		system,
    NamedSystemRegistry**	bitmapPointer,
    const char* const		system_name);
ut_status
utRemoveNamedSystemFromRegistryLocation(
    ut_system* const		system,
    NamedSystemRegistry**	bitmapPointer,
    const char* const		system_name);
int
utNamedSystemIsInRegistryLocation(
    ut_system* const		system,
    NamedSystemRegistry**	bitmapPointer,
    const char* const		system_name);
void
utNamedSystemRegistryLocationFree(
    NamedSystemRegistry**	bitmapPointer);

#ifdef __cplusplus
}
#endif

#endif
