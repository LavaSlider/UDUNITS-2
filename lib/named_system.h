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
 * Adds a units system name to a unit-system.  A units system name is something
 * like "SI" or "metric".  Comparisons between units system names are
 * case-insensitive.
 *
 * Arguments:
 *	system		Pointer to the unit-system.
 *	name		Pointer to the units system name (e.g., "SI").
 *			May be freed upon return.
 *	encoding	(Pretty much ignored at present)
 * Returns:
 *	UT_SUCCESS	Success.
 *	UT_BAD_ARG	"system" or "name" is NULL
 *	UT_OS		Operating-system failure.  See "errno".
 */
ut_status
ut_add_named_system(
    ut_system* const	system,
    const char* const	name,
    const ut_encoding	encoding);

/*
 * Adds an alias or alternate units system name to a unit-system.
 * A units system name is something like "SI" and an alternate name
 * might be "International System".
 *
 * Arguments:
 *	system		Pointer to the unit-system.
 *	name		Pointer to the new units system name (e.g., "SI").
 *			May be freed upon return.
 *	encoding	(Pretty much ignored at present)
 *	named_system	Existing named system name
 * Returns:
 *	UT_SUCCESS	Success.
 *	UT_BAD_ARG	"system", "name" or "named_system" is NULL
 *	UT_UNKNOWN	"named_system" does not exist
 *	UT_EXISTS	If the new name already exists and is not a synonym
 *			for "named_system"
 *	UT_OS		Operating-system failure.  See "errno".
 */
ut_status
ut_map_name_to_named_system(
    ut_system* const	system,
    const char* const	name,		/* New name */
    const ut_encoding	encoding,
    const char* const	named_system);	/* Existing name */

#ifdef __cplusplus
}
#endif

#endif
