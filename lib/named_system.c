/*
 * Copyright 2015 David W. Stockton
 *
 * Module for handling named units systems and their aliases.
 *
 * Named unit systems are user recognized unit systems like the SI
 * system, the metric system, etc., that include specific sets of
 * units.
 *
 * This file is part of the UDUNITS-2 package.  See the file COPYRIGHT
 * in the top-level source-directory of the package for copying and
 * redistribution conditions.
 */

/*LINTLIBRARY*/

#ifndef	_XOPEN_SOURCE
#   define _XOPEN_SOURCE 500
#endif

#include <ctype.h>
#include <errno.h>
#ifdef _MSC_VER
#include "tsearch.h"
#else
#include <search.h>
#endif
#include <stdlib.h>
#include <stdio.h>	/* For printf */
#if __DARWIN_C_LEVEL < 200112L && !defined(_C99_SOURCE) && !defined(__cplusplus)
__BEGIN_DECLS
int	 snprintf(char * __restrict, size_t, const char * __restrict, ...) __printflike(3, 4);
__END_DECLS
#endif /* __DARWIN_C_LEVEL < 200112L && !defined(_C99_SOURCE) && !defined(__cplusplus) */
#include <limits.h>	/* For INT_MAX */

#include <string.h>
// Add a declaration to suppress a compiler warning when
// compiling on Mountain Lion because the declaration is
// ifdef'd out in string.h
#if __DARWIN_C_LEVEL < 200112L
char *strdup(const char *);
#endif /* __DARWIN_C_LEVEL < 200112L */
#include <strings.h>	/* For strcasecmp() */

#include "named_system.h"
#include "udunits2.h"
#include "systemMap.h"
#include "bitmap.h"
#include "ut_lists.h"

typedef struct {
    void*	tree;
    int		namedSystemCount;	// The number of unique units system names
    int		namedSystemNamesCount;	// The total number of names including aliases
    int		(*compare)(const void*, const void*);
} NamedSystemToIndexMap;

typedef struct {
    const char*		name;		// The units system name (e.g., "SI")
    unsigned short	index;		// The bit index assigned to this system name
    unsigned short	flags;		// Added information about this name
} NamedSystemSearchEntry;
// Set to LONG_MAX, INT_MAX, SHRT_MAX, USHRT_MAX based on
// the type of the 'index' field in the NamedSystemSearchEntry
static unsigned long	maxNamedSystemCount = USHRT_MAX;
// Bits in the NamedSystemSearchEntry flags entry
#define	UT_NSSEFLAG_PRIMARY_NAME	0x01

/*
 * Since ut_units are always cloned when passed around
 * and the clones are only recognized by their equivalence
 * we need to keep the named system registry information
 * outside the ut_unit structure itself. Se we need another
 * storage tree to map between ut_units and named system
 * registry entries.
 *
 * With this it is possible to look up the registry for
 * a particular unit (no matter how it has been cloned)
 * and test its contents or modify it.
 *
 * The entries are unique based on ut_compare() of the
 * unit passed.
 */
typedef struct {
    void*		 tree;
    int			 unitsWithNamedSystemRegistryCount;
    int			 (*compare)(const void*, const void*);
} UnitToRegistryMap;
typedef struct {
    const ut_unit*	 unit;		// The key entry for the tree
    NamedSystemRegistry* registry;	// The bitmap of named unit systems
} UnitAndNamedSystemRegistry;

static SystemMap*	systemToNameToIndex = NULL;
static SystemMap*	systemToUnitToRegistry = NULL;


/******************************************************************************
 * Named System Search Entry:
 ******************************************************************************/

static NamedSystemSearchEntry*
nsseNew(
    const char*	name,
    int		index)
{
    NamedSystemSearchEntry*	entry = malloc(sizeof(NamedSystemSearchEntry));

    if (entry == NULL) {
	ut_handle_error_message(strerror(errno));
	ut_handle_error_message(
	    "Couldn't allocate %lu-byte named-system-search-entry",
	    sizeof(NamedSystemSearchEntry) );
    }
    else {
	entry->name = strdup(name);
	entry->index = index;
	entry->flags = 0x00;
    }

    return entry;
}

static void
nsseFree(
    NamedSystemSearchEntry* const	entry)
{
    //printf("--- Freeing named system search entry for \"%s\"\n", entry->name);
    if (entry) free((void *) entry->name);
    free(entry);
}

static int
nsseInsensitiveCompare(
    const void* const	entry1,
    const void* const	entry2)
{
    const char *name1 = ((const NamedSystemSearchEntry *) entry1)->name;
    const char *name2 = ((const NamedSystemSearchEntry *) entry2)->name;

    return strcasecmp(name1, name2);
}


/******************************************************************************
 * Units Search Entry:
 ******************************************************************************/

static UnitAndNamedSystemRegistry*
uansrNew(
    const ut_unit* const	unit)
{
    UnitAndNamedSystemRegistry*	entry = malloc(sizeof(UnitAndNamedSystemRegistry));

    if (entry == NULL) {
	ut_handle_error_message(strerror(errno));
	ut_handle_error_message(
	    "Couldn't allocate %lu-byte unit-and-name-system-registry",
	    sizeof(UnitAndNamedSystemRegistry) );
    }
    else {
	entry->unit = ut_clone(unit);
	entry->registry = NULL;
    }

    return entry;
}

static void
uansrFree(
    UnitAndNamedSystemRegistry* const	entry)
{
    if (entry) {
	ut_free((void *) entry->unit);
	utNamedSystemRegistryFree(entry->registry);
    }
    free(entry);
}

static int
uansrCompare(
    const void* const	entry1,
    const void* const	entry2)
{
    return ut_compare(
	((const UnitAndNamedSystemRegistry*)entry1)->unit,
	((const UnitAndNamedSystemRegistry*)entry2)->unit);
}


/******************************************************************************
 * Named-System-to-Index Map:
 *	Uses the named units system name as the key to get the
 *	proper bit index value.
 ******************************************************************************/

/*
 * Creates and initialized a new named-system-to-index map.
 *
 * Argument:
 *	compare		Pointer to the function used to compare entries
 *			in the named-system-to-index map.
 */
static NamedSystemToIndexMap*
nstimNew(
    int		(*compare)(const void*, const void*))
{
    NamedSystemToIndexMap*	map =
	(NamedSystemToIndexMap *) malloc(sizeof(NamedSystemToIndexMap));

    if (map == NULL) {
	ut_handle_error_message(strerror(errno));
	ut_handle_error_message(
	    "Couldn't allocate %lu-byte named-system-to-index-map",
	    sizeof(NamedSystemToIndexMap));
    }
    else {
	map->tree = NULL;
	map->namedSystemNamesCount = 0;	// The total number of names including aliases
	map->namedSystemCount = 0;	// The number of unique units system names
	map->compare = compare;
    }

    return map;
}

/*
 * Frees an named-system-to-index map.  All entries are freed.
 *
 * Argument:
 *	map		Pointer to the named-system-to-index map.
 */
static void
nstimFree(
    NamedSystemToIndexMap*	map)
{
    if (map != NULL) {
	while (map->tree != NULL) {
	    NamedSystemSearchEntry* entry = *(NamedSystemSearchEntry**) map->tree;

	    (void) tdelete(entry, &map->tree, map->compare);
	    nsseFree(entry);
	}
	free(map);
    }					/* valid argument */
}

/*
 * Returns the named system search-entry that matches an identifier.  Inserts a
 * new named system search-entry if no matching element is found.  Note that the
 * returned entry might have a different named system value if it was previously
 * inserted.
 *
 * Arguments:
 *	map		Pointer to the named-system-to-index map.
 *	id		The named system identifier.  May be freed upon return.
 *	index		The index to use if doing an alias, otherwise -1 for new.
 * Returns:
 *	NULL		"map" is NULL.
 *	NULL		"id" is NULL or the empty string.
 *	NULL		Insufficient storage space is available.
 *	else		Pointer to the named-system-search-entry that matches "id".
 */
static const NamedSystemSearchEntry*
nstimSearch(
    NamedSystemToIndexMap*	map,
    const char* const		name,
    int				index)
{
    const NamedSystemSearchEntry*	entry = NULL;	/* failure */
    //printf("Entering nstimSearch(map, \"%s\")\n", name);

    if (map == NULL || name == NULL || *name == '\0') {
	//printf("- Parameters are not good, status set to UT_BAD_ARG\n");
	ut_set_status (UT_BAD_ARG);
    }
    else if (index > map->namedSystemCount) {
	//printf("- Status set to US_BAD_ARG for non-existant index request\n");
	ut_set_status (UT_BAD_ARG);
    }
    else if (index < 0 && map->namedSystemCount >= maxNamedSystemCount) {
	//printf("- Status set to US_OS for too many named systems\n");
	// To be technically correct, I should check if the name is
	// already defined (using tfind). In which case then nothing
	// would be added and we would not be exceeeding any limits...
	ut_set_status (UT_OS);
	ut_handle_error_message(
	    "Seriously? You cannot define more than %lu unit system names",
	    (unsigned long) maxNamedSystemCount - 1);
    }
    else {
	NamedSystemSearchEntry* const*	treeEntryFound = NULL;
	void**				tree = &map->tree;
	//printf("- Parameters all seem good\n");
	NamedSystemSearchEntry* const	newEntry = nsseNew (name,
			index < 0 ? map->namedSystemCount : index);
	if (newEntry == NULL) {
	    //printf("- Status set to US_OS for NULL return from nsseNew()\n");
	    ut_set_status (UT_OS);
	}
	else {
	    treeEntryFound = tsearch (newEntry, tree, map->compare);
	    if (treeEntryFound == NULL) {
		//printf("- Status set to US_OS for NULL return from tsearch()\n");
		// Per the man page, the tsearch() function returns NULL if
		// rootp is NULL or allocation of a new node fails (usually due
		// to a lack of free memory).
		nsseFree (newEntry);
		ut_set_status (UT_OS);
	    }
	    else {
		//printf("- Got a non-null return from tsearch()\n");
		// If the tree entry returned is not the new entry created
		// then the name was found and the new one can be freed.
		if (*treeEntryFound != newEntry) {
		    //printf("- It is a non-new entry, a previously existing entry\n");
		    nsseFree (newEntry);
		    entry = *treeEntryFound;
		    // If an index was specified (i.e., we are trying to
		    // create an alias) but the name exists with a different
		    // index then this is an attempt to redefine the name.
		    if (index >= 0 && entry->index != index) {
			ut_set_status (UT_EXISTS);
			ut_handle_error_message(
			    "\"%s\" already maps to an existing but different named units system",
			    name);
		    }
		    else {
			ut_set_status (UT_SUCCESS);
		    }
		}
		// If the tree entry returned is the new one then we just
		// added a new named system and we need to bump the counts
		// unless we have forced an index.
		else {
		    //printf("- It is a new entry\n");
		    entry = newEntry;
		    map->namedSystemNamesCount++; // Bump the total number of names
		    // If the index was < 0 then this is not a primary name
		    if (index < 0) {
			map->namedSystemCount++;  // Bump the number of primary  names
			newEntry->flags |= UT_NSSEFLAG_PRIMARY_NAME;
		    }
		    ut_set_status (UT_SUCCESS);
		}
	    }
	}
    }
    return entry;
}


/*
 * Returns the named system search-entry that matches the string.
 *
 * Arguments:
 *	map		Pointer to the prefix-to-value map.
 *	string		Pointer to the string to be examined for a prefix.
 * Returns:
 *	NULL		"map" is NULL, "string" is NULL or the empty string.
 *	else		Pointer to the named-system-search-entry that matches
 *			the "string".
 *	ut_get_status() returns:
 *		UT_BAD_ARG
 *		UT_UNKNOWN
 *		UT_SUCCESS
 */
static const NamedSystemSearchEntry*
nstimFind(
    NamedSystemToIndexMap* const	map,
    const char* const			string)
{
    const NamedSystemSearchEntry*	entry = NULL;	/* failure */

    if (map == NULL || string == NULL || *string == '\0') {
	ut_set_status(UT_BAD_ARG);
    }
    else {
	void**			tree = &map->tree;
	NamedSystemSearchEntry		targetEntry;
	NamedSystemSearchEntry* const*	treeEntryPointer;

	targetEntry.name = string;
	treeEntryPointer = tfind(&targetEntry, tree, map->compare);

	if (treeEntryPointer == NULL || *treeEntryPointer == NULL) {
	    ut_set_status(UT_UNKNOWN);
	}
	else {
	    ut_set_status(UT_SUCCESS);
	    entry = *treeEntryPointer;
	}
    }
    return entry;
}


/******************************************************************************
 * Unit-to-Registry Map:
 *	Uses the ut_unit as the key to get the registry containing
 *	which named units systems the unit belongs to.
 ******************************************************************************/

static UnitToRegistryMap*
utrmNew(
    int		(*compare)(const void*, const void*))
{
    UnitToRegistryMap*	map =
	(UnitToRegistryMap *) malloc(sizeof(UnitToRegistryMap));

    if (map == NULL) {
	ut_handle_error_message(strerror(errno));
	ut_handle_error_message(
	    "Couldn't allocate %lu-byte unit-to-registry-map",
	    sizeof(UnitToRegistryMap) );
    }
    else {
	map->tree = NULL;
	map->unitsWithNamedSystemRegistryCount = 0;
	map->compare = compare;
    }

    return map;
}

/*
 * Frees a unit-to-registry map.  All entries are freed.
 *
 * Arguments:
 *	map		Pointer to the identifier-to-unit map.
 * Returns:
 */
static void
utrmFree(
    UnitToRegistryMap*	map)
{
    if (map != NULL) {
	while (map->tree != NULL) {
	    UnitAndNamedSystemRegistry* entry = *(UnitAndNamedSystemRegistry**) map->tree;

	    (void) tdelete(entry, &map->tree, map->compare);
	    uansrFree(entry);
	}
	free(map);
    }					/* valid arguments */
}

/*
 * Returns the unit-search-entry that matches a unit.  Inserts a
 * new unit-search-entry if no matching element is found.  Note that the
 * returned entry might have a different unit value if it was previously
 * inserted.
 *
 * Arguments:
 *	map		Pointer to the unit-to-registry map.
 *	unit		The unit.  May be freed upon return.
 * Returns:
 *	NULL		"map" is NULL.
 *	NULL		"unit" is NULL
 *	NULL		Insufficient storage space is available.
 *	else		Pointer to the named-system-search-entry that matches "id".
 */
static UnitAndNamedSystemRegistry*
utrmSearch(
    UnitToRegistryMap*		map,
    const ut_unit* const	unit)
{
    UnitAndNamedSystemRegistry*	entry = NULL;	/* failure */
    //printf("Entering utrmSearch(map, \"%s\")\n", id);

    if (map == NULL || unit == NULL) {
	//printf("- Parameters are not good, status set to UT_BAD_ARG\n");
	ut_set_status(UT_BAD_ARG);
    }
    else {
	UnitAndNamedSystemRegistry* const*	treeEntry = NULL;
	void**				tree = &map->tree;
	//printf("- Parameters all seem good\n");
	UnitAndNamedSystemRegistry* const	newEntry = uansrNew(unit);
	if (newEntry == NULL) {
	    //printf("- Status set to US_OS for NULL return from uansrNew()\n");
	    ut_set_status(UT_OS);
	}
	else {
	    // I could hard code the compare here too...
	    treeEntry = tsearch(newEntry, tree, map->compare);
	    if (treeEntry == NULL) {
		//printf("- Status set to US_OS for NULL return from tsearch()\n");
		uansrFree(newEntry);
		// The tsearch() function returns NULL if allocation of a new node fails
		// (usually due to a lack of free memory) and the tsearch() function
		// returns NULL if rootp is NULL
		ut_set_status(UT_OS);
	    }
	    else {
		//printf("- Got a non-null return from tsearch()\n");
		// If the tree entry returned is not the new entry created
		// then the name was found and the new one can be freed.
		if (*treeEntry != newEntry) {
		    //printf("- It is a non-new entry, a previously existing entry\n");
		    uansrFree(newEntry);
		    entry = *treeEntry;
		    ut_set_status(UT_EXISTS);
		}
		// If the tree entry returned is the new one then we just
		// added a new named system and we need to bump the count
		// unless we have forced an index.
		else {
		    //printf("- It is a new entry\n");
		    map->unitsWithNamedSystemRegistryCount++;
		    entry = newEntry;
		    ut_set_status(UT_SUCCESS);
		}
	    }
	}
    }
    return entry;
}


/*
 * Returns the unit search-entry that matches the unit
 *
 * Arguments:
 *	map		Pointer to the prefix-to-value map.
 *	string		Pointer to the string to be examined for a prefix.
 * Returns:
 *	NULL		"map" is NULL, "string" is NULL or the empty string.
 *	else		Pointer to the prefix-search-entry that matches the
 *			beginning of "string".
 *	ut_get_status() returns:
 *		UT_BAD_ARG
 *		UT_UNKNOWN
 *		UT_SUCCESS
 */
static UnitAndNamedSystemRegistry*
utrmFind(
    UnitToRegistryMap* const	map,
    const ut_unit* const	unit)
{
    UnitAndNamedSystemRegistry*	entry = NULL;	/* failure */

    if (map == NULL || unit == NULL) {
	ut_set_status(UT_BAD_ARG);
    }
    else {
	UnitAndNamedSystemRegistry*	lastEntry = NULL;
	void**			tree = &map->tree;

	UnitAndNamedSystemRegistry		targetEntry;
	UnitAndNamedSystemRegistry* const*	treeEntry;

	targetEntry.unit = unit;
	treeEntry = tfind(&targetEntry, tree, map->compare);

	if (treeEntry == NULL) {
	    ut_set_status(UT_UNKNOWN);
	}
	else {
	    lastEntry = *treeEntry;
	}

	if (lastEntry != NULL) {
	    ut_set_status(UT_SUCCESS);
	    entry = lastEntry;
	}
    }
    return entry;
}


/******************************************************************************
 * Public API:
 ******************************************************************************/

/*
 * Returns:
 *	NamedSystemSearchEntry	The record for the "name" provided.
 *	NULL			"name" not found or some error.
 *		UT_BAD_ARG	system, systemMap or name NULL, name an
 *				empty string or *systemMap NULL if not
 *				adding (this last situation maybe should
 *				not be a UT_BAD_ARG since this is really
 *				just a find failure so maybe should be
 *				UT_UNKNOWN or UT_SUCCESS)
 *		UT_OS		If memory allocation failure
 *		UT_EXISTS	If trying to add an alias but the name is
 *				already assigned to a different named system.
 */
static const NamedSystemSearchEntry*
findOrAddNamedSystem(
    const ut_system* const	system,
    SystemMap** const		systemMap,
    const char* const		name,
    int				index,
    int				(*compare)(const void*, const void*))
{
    const NamedSystemSearchEntry* entry = NULL; /* Error, could not be added or not found */
    ut_status		status = UT_UNKNOWN;
    // Make this function universion search/find (i.e., can add or not)
    int	canAdd = (compare != NULL);
    //printf("Entering findOrAddNamedSystem(system, systemMap, \"%s\", compare)\n", name);
    //printf("- Can add name is %s\n", canAdd ? "TRUE" : "FALSE");

    if (system == NULL || systemMap == NULL ||
	name == NULL || *name == '\0' ||
	(!canAdd && *systemMap == NULL)) { // <--- maybe this should be UT_UNKNOWN or UT_SUCCESS??
	//printf("- Status being set to UT_BAD_ARG for null pointer problem\n");
	status = UT_BAD_ARG;
    }
    else {
	if (*systemMap == NULL && (*systemMap = smNew()) == NULL) {
	    //printf("- Status being set to UT_OS for failure of smNew()\n");
	    status = UT_OS;
	}
	else {
	    NamedSystemToIndexMap** const namedSystemToIndex = (NamedSystemToIndexMap**) (canAdd
			? smSearch(*systemMap, system)
			: smFind(*systemMap, system));

	    if (namedSystemToIndex == NULL) {
		//printf("- Status being set to %s for namedSystemToIndex being null\n",
		//		canAdd ? "UT_OS" : "UT_UNKNOWN");
		status = canAdd ? UT_OS : UT_UNKNOWN;
	    }
	    else {
		if (canAdd && *namedSystemToIndex == NULL) {
		    *namedSystemToIndex = nstimNew(compare);

		    if (*namedSystemToIndex == NULL)
			//printf("- Status being set to UT_OS for nstimNew returning NULL\n");
			status = UT_OS;
		}

		if (*namedSystemToIndex != NULL) {
		    entry = canAdd ? nstimSearch(*namedSystemToIndex, name, index)
				   : nstimFind(*namedSystemToIndex, name);
		    if (entry == NULL) {
			//printf("- Status being set to UT_UNKNOWN for entry being NULL\n");
			//status = UT_UNKNOWN;
			status = ut_get_status();
		    }
		    else {
			//printf("- Status being set to UT_SUCCESS for non-NULL entry\n");
			status = UT_SUCCESS;
		    }			/* have named system entry */
		}			/* have named system to index entry */
	    }				/* have system-map entry */
	}				/* have system-map */
    }					/* valid arguments */
    ut_set_status(status);
    return entry;
}

/*
 * Adds a named system to a unit-system if it does not already exist.
 * If it exists then UT_EXISTS is returned.
 *
 * Arguments:
 *	system		Pointer to the unit-system.
 *	prefix		Pointer to the prefix (e.g., "mega", "M").  May be freed
 *			upon return.
 *	value		The value of the prefix (e.g., 1e6).
 *	systemMap	Pointer to system-map.
 *	compare		Prefix comparison function.
 * Returns:
 *	-1
 *	Index of name
 *
 *	UT_SUCCESS	Successfully added the new named system..
 *	UT_EXISTS	"name" already defined.
 *	UT_BAD_ARG	"system" is NULL, or "name" is NULL or empty.
 *	UT_OS		Operating-system failure.  See "errno".
 */
static const NamedSystemSearchEntry*
utAddNamedSystem(
    ut_system* const	system,
    SystemMap** const	systemMap,
    const char* const	name,
    int			index,
    int			(*compare)(const void*, const void*))
{
    return findOrAddNamedSystem(system, systemMap, name, index, compare);
}


/*
 * Looks up the named system and returns its index.
 *
 * Arguments:
 *	system		Pointer to the unit-system.
 *	system_name	Pointer to the system_name to search for. Case insensitive.
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
    const ut_system* const	system,
    const char* const		system_name)
{
    const NamedSystemSearchEntry* entry =
	findOrAddNamedSystem(system, &systemToNameToIndex, system_name, -1, NULL);
    return entry != NULL ? entry->index : -1;
}



/*
 * Frees resources associated with a unit-system.
 *
 * Arguments:
 *	system		Pointer to the unit-system to have its associated
 *			resources freed.
 */
void
namedSystemFreeSystem(
    ut_system*	system)
{
    if (system != NULL) {
	if (systemToNameToIndex != NULL) {

	    NamedSystemToIndexMap** const namedSystemToIndex =
		(NamedSystemToIndexMap**) smFind(systemToNameToIndex, system);
	    if (namedSystemToIndex != NULL)
		nstimFree(*namedSystemToIndex);
	    smRemove(systemToNameToIndex, system);
	}

	if (systemToUnitToRegistry != NULL) {
	    UnitToRegistryMap** const unitToRegistry =
		(UnitToRegistryMap **) smFind(systemToUnitToRegistry, system);
	    if (unitToRegistry != NULL)
		utrmFree(*unitToRegistry);
	    smRemove(systemToUnitToRegistry, system);
	}
    }	/* valid arguments and existing map */
}

/*
 * Adds a units system name to a unit-system.  A units system name is something
 * like "SI" or "metric".  Comparisons between units system names are
 * case-insensitive.
 *
 * Arguments:
 *	system		Pointer to the unit-system.
 *	name		Pointer to the units system name (e.g., "SI").
 *			May be freed upon return.
 *	encoding
 * Returns:
 *	UT_SUCCESS	Success.
 *	UT_BAD_ARG	"system" or "name" is NULL
 *	UT_OS		Operating-system failure.  See "errno".
 */
ut_status
ut_add_named_system(
    ut_system* const	system,
    const char* const	name,
    const ut_encoding	encoding)
{
    //printf("Entering ut_add_named_system(system, \"%s\", encoding)\n", name);
    utAddNamedSystem(system, &systemToNameToIndex, name, -1, nsseInsensitiveCompare);
    return ut_get_status();
}

/*
 * Adds an alias or alternate units system name to a unit-system.
 * A units system name is something like "SI" and an alternate name
 * might be "International System".
 *
 * Arguments:
 *	system		Pointer to the unit-system.
 *	name		Pointer to the new units system name (e.g., "SI").
 *			May be freed upon return.
 *	encoding
 *	system_name	Existing named system name
 * Returns:
 *	UT_SUCCESS	Success.
 *	UT_BAD_ARG	"system", "name" or "system_name" is NULL
 *	UT_OS		Operating-system failure.  See "errno".
 */
ut_status
ut_map_name_to_named_system(
    ut_system* const	system,
    const char* const	name,	/* New name */
    const ut_encoding	encoding,
    const char* const	system_name)
{
    ut_status	status;
    if (system == NULL || name == NULL || *name == '\0' ||
	system_name == NULL || *system_name == '\0') {
	status = UT_BAD_ARG;
    }
    else {
	int existingIndex = utFindNamedSystemIndex(system, system_name);
	if (existingIndex < 0) {
	    // Should I just create it or return an error here???
	    status = UT_UNKNOWN;
	}
	else {
	    int newIndex = utFindNamedSystemIndex(system, name);
	    if (newIndex >= 0) {
		if (newIndex != existingIndex) {
		    // This is a problem... we are trying to redefine the
		    // name. All the units would have to be updated to change
		    // the index of a named system.
		    status = UT_EXISTS;
		}
		else {
		    // The indexes match so this is really a noop...
		    status = UT_SUCCESS;
		}
	    }
	    else {
		const NamedSystemSearchEntry* entry = utAddNamedSystem(system,
			&systemToNameToIndex, name, existingIndex, nsseInsensitiveCompare);
		status = (entry != NULL) ? UT_SUCCESS : ut_get_status();
	    }
	}
    }
    ut_set_status(status);
    return status;
}


/******************************************************************************
 * Unit Membership To Named System:
 ******************************************************************************/

/*
 * Connecting to the ut_unit:
 *   This can be done completely blindly by sticking a pointer
 *   to an opaque structure into the ut_unit structure. This
 *   provides more separation of function but requires more
 *   fidling with the ut_unit structure.
 *
 *   Alternatively, instead of sticking a pointer into the
 *   ut_unit I could put the actual structure. This would now
 *   require that the ut_unit knew about the internals of
 *   the structure (because the compiler cannot instantiate
 *   it without know what it is) but then all the allocate
 *   and freeing, etc., could be automatic!
 */
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
 *     registry = utSetNamedSystemInoRegistry(system, registry, system_name);
 *     if (ut_get_status() != UT_SUCCESS) {
 *	   fprintf(stderr, "Problem\n");
 *     }
 */
NamedSystemRegistry*
utSetNamedSystemInRegistry(
    ut_system* const		system,
    NamedSystemRegistry*	bitmap,
    const char* const		system_name)
{
    ut_status	status;
    if (system == NULL ||
	system_name == NULL || *system_name == '\0') {
	status = UT_BAD_ARG;
    } else {
	if (bitmap == NULL) {
	    bitmap = bitmapNew();
	    if (bitmap == NULL) {
		status = UT_OS;
	    }
	}
	if (bitmap != NULL) {
	    int	idx = utFindNamedSystemIndex(system, system_name);
	    if (idx < 0) {
		status = ut_add_named_system(system, system_name, UT_ASCII);
	    }
	    idx = utFindNamedSystemIndex(system, system_name);
	    if (idx < 0) {
		status = ut_get_status();
	    }
	    else {
		status = UT_SUCCESS;
		setBit(bitmap, idx);
	    }
	}
    }
    ut_set_status(status);
    return bitmap;
}
/*
 * Adds the "system_name" to the registry, potentially creating
 * the registry and the "system_name"
 *
 * UT_BAD_ARG	parameter null
 * UT_OS	failure to allocate
 *
 * usage:
 *     status = utSetNamedSystemInRegistry(system, &registry, system_name);
 *     if (status != UT_SUCCESS) {
 *	   fprintf(stderr, "Problem\n");
 */
ut_status
utSetNamedSystemInRegistryLocation(
    ut_system* const		system,
    NamedSystemRegistry**	bitmapPointer,
    const char* const		system_name)
{

    ut_status status;
    if (bitmapPointer) {
	*bitmapPointer = utSetNamedSystemInRegistry(system, *bitmapPointer, system_name);
	status = ut_get_status();
    }
    else {
	status = UT_BAD_ARG;
    }
    return status;
}
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
 *    utRemoveNamedSystemFromRegistry(system, registry, system_name);
 */
ut_status
utRemoveNamedSystemFromRegistry(
    ut_system* const		system,
    NamedSystemRegistry*	bitmap,
    const char* const		system_name)
{
    int		idx = utFindNamedSystemIndex(system, system_name);
    clearBit(bitmap, idx);
    return ut_get_status();
}
/*
 *
 * UT_BAD_ARG	parameter null
 * UT_OS	failure to allocate
 *
 * usage:
 *    utRemoveNamedSystemFromRegistry(system, &registry, system_name);
 */
ut_status
utRemoveNamedSystemFromRegistryLocation(
    ut_system* const		system,
    NamedSystemRegistry**	bitmapPointer,
    const char* const		system_name)
{
    if (bitmapPointer)
	return utRemoveNamedSystemFromRegistry(system, *bitmapPointer, system_name);
    return UT_BAD_ARG;
}

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
    const char* const		system_name)
{
    ut_set_status(UT_SUCCESS);
    return bitIsSet(bitmap, utFindNamedSystemIndex(system, system_name));
}

int
utNamedSystemIsInRegistryLocation(
    ut_system* const		system,
    NamedSystemRegistry**	bitmapPointer,
    const char* const		system_name)
{
    if (bitmapPointer == NULL) {
	ut_set_status(UT_BAD_ARG);
	return 0;
    }
    ut_set_status(UT_SUCCESS);
    return utNamedSystemIsInRegistry(system, *bitmapPointer, system_name);
}

/*
 * Frees the bitmap and any allocated storage within it.
 */
void
utNamedSystemRegistryFree(
    NamedSystemRegistry*	bitmap)
{
    bitmapFree(bitmap);
}
void
utNamedSystemRegistryLocationFree(
    NamedSystemRegistry**	bitmapPointer)
{
    if (bitmapPointer != NULL) {
	utNamedSystemRegistryFree(*bitmapPointer);
	*bitmapPointer = NULL;
    }
}
/*
 * This is needed if the NameSystemRegistry is on the stack or
 * part of another structure (instead of a pointer to it).
 */
void
utNamedSystemRegistryInit(
    NamedSystemRegistry*	bitmap)
{
    bitmapInit(bitmap);
}
/*
 * This is needed to avoid a memory leak if the NameSystemRegistry
 * is on the stack or part of another structure (instead of a
 * pointer to it).
 */
void
utNamedSystemRegistryReset(
    NamedSystemRegistry*	bitmap)
{
    bitmapReset(bitmap);
}

/******************************************************************************
 * More Unit Membership To Named System:
 ******************************************************************************/

/*
 * Search the systemMap for UnitAndNamedSystemRegistry (unit,registry pair)
 * (If a compare function is given, then it can be created)
 *
 * If the unit or systemMap are NULL
 *	ut_set_status UT_BAD_ARG
 *	Return NULL
 * If we are just finding (!canAdd) and the *systemMap is NULL
 *	This means there are no units in the tree, so this
 *	should mean the same as if the registry was found.
 */
static UnitAndNamedSystemRegistry*
findOrAddUnitsSearchEntry(
    SystemMap** const		systemMap,
    const ut_unit* const	unit,
    int				(*compare)(const void*, const void*))
{
    UnitAndNamedSystemRegistry* entry = NULL; /* Error, could not be added or not found */
    ut_status		status = UT_UNKNOWN;
    // Make this function universion search/find (i.e., can add or not)
    int	canAdd = (compare != NULL);
    //printf("Entering findOrAddNamedSystem(system, systemMap, \"%s\", compare)\n", name);
    //printf("- Can add name is %s\n", canAdd ? "TRUE" : "FALSE");

    if (unit == NULL || systemMap == NULL) {
	//printf("------- Status being set to UT_BAD_ARG for null pointer problem\n");
	status = UT_BAD_ARG;
    }
    else if (!canAdd && *systemMap == NULL) {
	//printf("------- Status being set to UT_UNKNOWN since there is no systemMap\n");
	status = UT_UNKNOWN;
    }
    else {
	if (*systemMap == NULL) {
	    *systemMap = smNew();

	    if (*systemMap == NULL)
		//printf("- Status being set to UT_OS for failure of smNew()\n");
		status = UT_OS;
	}

	if (*systemMap != NULL) {
	    ut_system* system = ut_get_system(unit);
	    UnitToRegistryMap** const unitToRegistry = (UnitToRegistryMap **) (canAdd
			? smSearch(*systemMap, system)
			:   smFind(*systemMap, system));

	    if (unitToRegistry == NULL) {
		//printf("- Status being set to %s for namedSystemToIndex being null\n",
		//		canAdd ? "UT_OS" : "UT_UNKNOWN");
		status = canAdd ? UT_OS : UT_UNKNOWN;
	    }
	    else {
		if (canAdd && *unitToRegistry == NULL) {
		    *unitToRegistry = utrmNew(compare);

		    if (*unitToRegistry == NULL)
			//printf("- Status being set to UT_OS for nstimNew returning NULL\n");
			status = UT_OS;
		}

		if (*unitToRegistry != NULL) {
		    entry = canAdd ? utrmSearch(*unitToRegistry, unit)
				   :   utrmFind(*unitToRegistry, unit);
		    if (entry == NULL) {
			//printf("- Status being set to UT_UNKNOWN for entry being NULL\n");
			status = UT_UNKNOWN;
		    }
		    else {
			//printf("- Status being set to UT_SUCCESS for non-NULL entry\n");
			status = UT_SUCCESS;
		    }			/* have named system entry */
		} else {
		    printf("------ unitToRegistry is NULL\n");
		}			/* have named system to index entry */
	    }				/* have system-map entry */
	}				/* have system-map */
    }					/* valid arguments */
    //printf("----- Leaving findOrAddUnitsSearchEntry(), settgin status to %d\n", (int) status);
    ut_set_status(status);
    return entry;
}

/*
 * Adds a unit to a unit-system if it does not already exist.
 * If it exists then it just returns the search entry for it.
 *
 * Arguments:
 *	unit		The unit that the search entry is wanted for
 * Returns:
 *	NamedSystemSearchEntry	on success
 *	NULL			on failure
 *
 *	UT_SUCCESS	Successfully added the new named system..
 *	UT_EXISTS	"name" already defined.
 *	UT_BAD_ARG	"system" is NULL, or "name" is NULL or empty.
 *	UT_OS		Operating-system failure.  See "errno".
 */
static UnitAndNamedSystemRegistry*
utAddNamedSystemRegistryToUnit(
    const ut_unit* const	unit)
{
    return findOrAddUnitsSearchEntry(&systemToUnitToRegistry, unit, uansrCompare);
}
/*
 * Looks up the unit and returns its registry.
 *
 * Arguments:
 *	system		Pointer to the unit-system.
 *	unit		Pointer to the unit to search for. Case insensitive.
 * Returns:
 *	NamedSystemSearchEntry	on success
 *	NULL			on failure
 *		    ut_get_status() will return:
 *		    - UT_BAD_ARG	"string" or system was NULL.
 *		    - UT_UNKNOWN	A named units system was not discovered.
 *		    - UT_SUCCESS	Success.
 */
static const UnitAndNamedSystemRegistry*
utFindNamedSystemRegistryForUnit(
    const ut_unit* const	unit)
{
    return findOrAddUnitsSearchEntry(&systemToUnitToRegistry, unit, NULL);
}

/*
 * Tests whether the given unit is member of the named system.
 *
 * Potential Refinements:
 *	should the unit one be in all systems?
 *	should dimensionless units be in all systems?
 *
 * Arguments:
 *	unit		The unit to test
 *	system_name	The unit system to be checked (e.g., SI, metric, US, etc.)
 * Returns:
 *	NULL	No or Failure.  "ut_get_status()" will be
 *		    UT_BAD_ARG	"system_name" or "unit" is NULL.
 *		    UT_UNKNOWN	"system_name" is not defined for the system the unit belongs to.
 *		    UT_SUCCESS	the unit is not in the named units system
 *	else	non-zero indicating the unit was found in the named units system.
 */
int
ut_is_in_named_system(
    const ut_unit* const	unit,
    const char* const		system_name)
{
    int	found = 0;

    if (unit == NULL || system_name == NULL || *system_name == '\0') {
	ut_set_status(UT_BAD_ARG);
	ut_handle_error_message("ut_is_in_named_system(): NULL argument");
    }
    else {
	const UnitAndNamedSystemRegistry* entry = utFindNamedSystemRegistryForUnit(unit);
	if (entry) {
	    ut_system* system = ut_get_system(unit);
	    //printf("-- Checking for \"%s\" within the registry\n", system_name);
	    found = utNamedSystemIsInRegistry(system, entry->registry, system_name);
	} else {
	    // If the no registry could be foudn for the unit then it clearly
	    // is not in the named system... so is this success??
	    //printf("-- Did not find the unit entry when checking for \"%s\"\n", system_name);
	    //printf("   ut_get_status() returns %d\n", (int) ut_get_status());
	}
    }

    return found;
}

/*
 *  Specifies that the unit passed is part of the named unit system
 *  whose name is passed as the second argument. For example, specifying
 *  that centimeters is part of the metric system would look like:
 *
 *    ut_unit *cm = ut_get_unit_by_name(system, "centimeter");
 *    ud_add_unit_to_named_syste(cm, "metric system");
 *
 *  Note that if the named unit system does not exist it is silently
 *  created. This might be an issue since an encoding is not set.
 *
 *  Returns:
 *	UT_BAD_ARG	If unit or system name are NULL or an emptry
 *			string is passed for the system_name.
 *	UT_OS		For failures allocating space, etc.
 *	UT_SUCCESS	If successfully added.
 */
ut_status
ut_add_unit_to_named_system(
    const ut_unit* const	unit,
    const char* const		system_name)
{
    if (unit == NULL || system_name == NULL || *system_name == '\0' ) {
	ut_set_status(UT_BAD_ARG);
	ut_handle_error_message("ut_add_unit_to_named_system(): NULL argument");
    }
    else {
	UnitAndNamedSystemRegistry* entry = utAddNamedSystemRegistryToUnit(unit);
	if(entry) {
	    entry->registry = utSetNamedSystemInRegistry(ut_get_system(unit),
		entry->registry, system_name);
	}
    }
    return ut_get_status();
}

/*
 *  Specifies that the unit passed is not part of the named unit system
 *  whose name is passed as the second argument. For example, specifying
 *  that centimeters is not part of the US system would look like:
 *
 *    ut_unit *cm = ut_get_unit_by_name(system, "centimeter");
 *    ut_remove_unit_from_named_system(cm, "US system");
 *
 *  Note that if the named unit system does not exist or the unit
 *  is not part of the named unit system this function does nothing.
 *
 *  Returns:
 *	UT_BAD_ARG	If unit or system name are NULL or an emptry
 *			string is passed for the system_name.
 *	UT_OS		For failures allocating space, etc.
 *	UT_UNKNOWN	If the system_name does not exist.
 *	UT_SUCCESS	If successfully added.
 */
ut_status
ut_remove_unit_from_named_system(
    const ut_unit* const	unit,
    const char* const		system_name)
{
    if (unit == NULL || system_name == NULL || *system_name == '\0' ) {
	ut_set_status(UT_BAD_ARG);
	ut_handle_error_message("ut_remove_unit_from_named_system(): NULL argument");
    }
    else {
	const UnitAndNamedSystemRegistry* entry = utFindNamedSystemRegistryForUnit(unit);
	if(entry) {
	    utRemoveNamedSystemFromRegistry(ut_get_system(unit),
		entry->registry, system_name);
	    /* Note that if the registry entry is now empty (i.e., no bits set)
	     * it could be removed from the tree and deleted. */
	}
    }
    return ut_get_status();
}

/******************************************************************************
 * User interface for getting lists of named unit systems
 ******************************************************************************/
static ut_string_list*	    __name_list =  NULL;
static int		    __doAll     =  1;	// Set true/false to save/print all names
static int		    __doUniq    =  0;	// Set true/false to save/print only primary names
static int		    __aliasesOf = -1;	// Set to index of aliases to save/print
static NamedSystemRegistry* __registry	= NULL;	// Set to get only if set in registry
static void twalkGetNameListAction(const void *node, VISIT order, int level);

/*
 *  Get the list of named unit systems for the system.
 *
 *  If system_name is
 *    System name	Get aliases of named units system
 *    Empty string ("")	Get all primary unit system names
 *    NULL		Get all the defined named unit systems
 *  If unit is non-null only system names the units is assigned
 *  to are returned. That is, those selected by the "system_name"
 *  and include the unit.
 *
 *  Returns
 *    NULL		If system == NULL or has no named systems defined
 *    ut_string_list*	Containing the list of selected units
 *			system names per the system_name parameter.
 */
ut_string_list*
__ut_get_named_system_aliases(
    const ut_system* const	system,
    const char* const		system_name,
    const ut_unit* const	unit)
{
    ut_string_list* list = NULL;
    if (system != NULL /*&& systemToNameToIndex != NULL*/) {
	NamedSystemToIndexMap** const namedSystemToIndex =
	    (NamedSystemToIndexMap**) smFind(systemToNameToIndex, system);
	if (namedSystemToIndex && *namedSystemToIndex) {
	    NamedSystemToIndexMap*	map = *namedSystemToIndex;
	    if (map && map->namedSystemNamesCount > 0) {
		/* system_name == NULL ==> get all defined names */
		if (system_name == NULL) {
		    __aliasesOf = -1;	// Not aliases of anything
		    __doUniq = 0;	// Not primary
		    __doAll = 1;	// Get them all
		}
		/* system_name == "" ==> get unique/primary names */
		else if (*system_name == '\0') {
		    __aliasesOf = -1;	// Not aliases of anything
		    __doUniq = 1;	// Just primary
		    __doAll = 0;	// Not all
		}
		/* system_name given ==> get aliases for it */
		else {
		    __aliasesOf = utFindNamedSystemIndex(system, system_name);
		    __doUniq = 0;	// Not primary
		    __doAll = 0;	// Not all
		}
		/* by default do not restrict to a specific unit */
		__registry = NULL;
		if (unit) {
		    /* if a unit is specified and is assigned to systems */
		    /* restrict what is stipulated above to these systems */
		    const UnitAndNamedSystemRegistry* entry = utFindNamedSystemRegistryForUnit(unit);
		    if (entry) {
			__registry = entry->registry;
		    }
		    /* if a unit is specified and it is not assigned to */
		    /* any systems then we should return an empty list */
		    /* so turn off all the flags */
		    else {
			__aliasesOf = -1; // Not aliases of anything
			__doUniq = 0;	  // Not primary
			__doAll = 0;	  // Don't them all
		    }
		}
		/* allocate an empty list with adequate capacity */
		__name_list = ut_string_list_new_with_capacity(map->namedSystemNamesCount);
		twalk(map->tree, twalkGetNameListAction);
		ut_string_list_truncate_waste(__name_list);
		list = __name_list;
		__name_list = NULL;
		__registry = NULL;
	    }
	}
	else {
	    list = ut_string_list_new();
	}
    }
    return list;
}

/*
 * Get the list of named unit systems for the system that are
 * aliases for the "system_name" (including "system_name").
 *
 * The returned string list is sorted alphabetically.
 *
 * Arguments:
 *	system		The system to return the primary
 *			system names for.
 *	system_name	String that determines the unit
 *			system names to return.
 *	    If system_name is:
 *		System name	Gets all aliases of the
 *				named units system
 *		Empty string	Gets all primary unit
 *				system names
 *		NULL		Gets all the defined
 *				named unit systems
 * Returns:
 *	NULL		If system argument was NULL
 *			ut_get_status() returns UT_BAD_ARG.
 *	ut_string_list*	Containing the list of selected units
 *			system names per the system_name
 *			argument, which could be empty.
 *			ut_get_status() returns UT_SUCCESS.
 */
ut_string_list*
ut_get_named_system_aliases(
    const ut_system* const	system,
    const char* const		system_name)
{
    ut_string_list* list = NULL;
    if (system == NULL) {
	ut_set_status(UT_BAD_ARG);
	ut_handle_error_message("ut_get_named_system_aliases(): NULL argument");
    }
    else {
	ut_set_status(UT_SUCCESS);
	list = __ut_get_named_system_aliases(system, system_name, NULL);
    }
    return list;
}

/*
 * Get the list of the primary named unit systems for the system.
 * In other words, only unique named units systems will be returned.
 *
 * The returned string list is sorted alphabetically.
 *
 * Arguments:
 *	system		The system to return the primary
 *			system names for.
 * Returns:
 *	NULL		If system argument was NULL.
 *			ut_get_status() returns UT_BAD_ARG.
 *	ut_string_list*	Containing the list of unique units
 *			system names, which could be empty.
 *			ut_get_status() returns UT_SUCCESS.
 */
ut_string_list*
ut_get_named_systems(
    const ut_system* const	system)
{
    ut_string_list* list = NULL;
    if (system == NULL) {
	ut_set_status(UT_BAD_ARG);
	ut_handle_error_message("ut_get_named_systems(): NULL argument");
    }
    else {
	ut_set_status(UT_SUCCESS);
	list = __ut_get_named_system_aliases(system, "", NULL);
    }
    return list;
}

/*
 * Get the list of named unit systems that the given unit is a
 * member of. This gets just the primary names, that is one
 * string per named unit system..
 *
 * The returned string list is sorted alphabetically.
 *
 * Arguments:
 *	unit		The unit to get the system names for..
 * Returns:
 *	NULL		If unit argument was NULL
 *			ut_get_status() returns UT_BAD_ARG.
 *	ut_string_list*	Containing the list of names,
 *			which could be empty. ut_get_status()
 *			returns UT_SUCCESS.
 *
 * Enhancement consideration:
 *	For all the ut_get_name...() functions to have
 * an encoding argument.
 *
 * Enhancement consideration:
 *	Have another version with an options parameter
 * to control primary versus all system names in what is
 * returned.
 *
 * Enhancement consideration:
 *	Modify NamedSystemToIndexMap to have a char **
 * array to hold the primary system names. Then when the
 * name is wanted for index N it can be directly accessed
 * instead of walking through the tree looking. The
 * memory impact could be minimized by having the
 * NamedSystemSearchEntry name entry point to the same
 * string as the array and modify nsseFree() to not
 * free the name entry if it is a primary name entry.
 */
ut_string_list*
ut_get_named_systems_for_unit(
    const ut_unit* const	unit)
{
    ut_string_list* list = NULL;
    if (unit == NULL) {
	ut_set_status(UT_BAD_ARG);
	ut_handle_error_message("ut_get_named_systems_for_unit(): NULL argument");
    }
    else {
	ut_set_status(UT_SUCCESS);
	list = __ut_get_named_system_aliases(ut_get_system(unit), "", unit);
    }
    return list;
}

static void twalkGetNameListAction(
    const void*	node,
    VISIT	order,
    int		level)
{
    NamedSystemSearchEntry**	nssep = (NamedSystemSearchEntry**) node;
    NamedSystemSearchEntry*	nsse = *nssep;

    switch (order) {
    case preorder:
    case endorder:
	break;
    case postorder:
    case leaf:
	if ((__doAll || (__doUniq && (nsse->flags & UT_NSSEFLAG_PRIMARY_NAME)) ||
	    (__aliasesOf >= 0 && nsse->index == __aliasesOf)) &&
	    (!__registry || bitIsSet(__registry, nsse->index))) {
	    if (__name_list) {
		ut_string_list_add_element(__name_list, nsse->name);
	    }
	    else {
		//printf("-- level %2d: ", level);
		printf("%2d %#04x \"%s\"", nsse->index, nsse->flags, nsse->name);
		printf("\n");
	    }
	}
	break;
    default:
	ut_handle_error_message(
	    "twalk unknown VISIT order: %d: named units system \"%s\" entry",
	    order, nsse->name );
	break;
    }
}
