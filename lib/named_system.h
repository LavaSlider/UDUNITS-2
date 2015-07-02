/**
 * @author David W. Stockton
 *
 * OK, the contents of this file are not quite right.
 * The functions defined here should be in "udunits2.h"
 * and what should be defined in here are the internal
 * functions used within the library
 * like void namedSystemFreeSystem( ut_system* system )
 * to be called in ut_system_free()
 * and int utFindNamedSystemIndex( ut_system* const system, const char* const string )
 * to be called when dealing with units
 */

#ifndef UT_NAMED_SYSTEM_H
#define UT_NAMED_SYSTEM_H

#include "udunits2.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Looks up the named system and returns its index.
 *
 * Arguments:
 *	system	Pointer to the unit-system.
 *	string	Pointer to the string to be search for.
 * Returns:
 *	index	Success. The index value (zero to max) assigned to the named system
 *	-1	Failure. String not found or error
 *		    ut_get_status() will return:
 *		    - UT_BAD_ARG	"string" or system was NULL.
 *		    - UT_UNKNOWN	A named units system was not discovered.
 *		    - UT_SUCCESS	Success.
 */
int
utFindNamedSystemIndex(
    ut_system* const	system,
    const char* const	string);

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

#ifdef __cplusplus
}
#endif

#endif
