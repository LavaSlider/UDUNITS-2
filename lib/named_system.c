/*
 * Copyright 2015 David W. Stockton
 *
 * Named unit systems are user recognized unit systems like the SI
 * system, the metric system, etc., that include specific sets of
 * units.
 *
 * This file is part of the UDUNITS-2 package.  See the file COPYRIGHT
 * in the top-level source-directory of the package for copying and
 * redistribution conditions.
 */
/*
 * Module for handling named units systems and their aliases.
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
//#include <stdio.h>	/* For printf */

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

typedef struct {
    void*	tree;
    int		namedSystemCount;
    int		(*compare)( const void*, const void* );
} NamedSystemToIndexMap;

typedef struct {
    void*		nextTree;
    int			index;
    const char*		name;
} NamedSystemSearchEntry;

static SystemMap*	systemToNameToIndex = NULL;


/******************************************************************************
 * Named System Search Entry:
 ******************************************************************************/

static NamedSystemSearchEntry*
nsseNew(
    const char*	name,
    int		index)
{
    NamedSystemSearchEntry*	entry = malloc( sizeof(NamedSystemSearchEntry) );

    if( entry == NULL ) {
	ut_handle_error_message( strerror(errno) );
	ut_handle_error_message(
            "Couldn't allocate %lu-byte named-system-search-entry",
	    sizeof(NamedSystemSearchEntry) );
    }
    else {
	entry->name = strdup( name );
	entry->index = index;
	entry->nextTree = NULL;
    }

    return entry;
}

static void
nsseFree(
    NamedSystemSearchEntry* const	entry)
{
    //printf( "--- Freeing named system search entry for \"%s\"\n", entry->name );
    if( entry ) free( (void *) entry->name );
    free( entry );
}

static int
nsseInsensitiveCompare(
    const void* const	entry1,
    const void* const	entry2)
{
    const char *name1 = ((const NamedSystemSearchEntry *) entry1)->name;
    const char *name2 = ((const NamedSystemSearchEntry *) entry2)->name;

    return strcasecmp( name1, name2 );
}


/******************************************************************************
 * Named-System-to-Index Map:
 ******************************************************************************/

static NamedSystemToIndexMap*
nstimNew(
    int		(*compare)(const void*, const void*))
{
    NamedSystemToIndexMap*	map =
	(NamedSystemToIndexMap *) malloc( sizeof(NamedSystemToIndexMap) );

    if( map == NULL ) {
	ut_handle_error_message( strerror(errno) );
	ut_handle_error_message(
            "Couldn't allocate %lu-byte named-system-to-index-map",
	    sizeof(NamedSystemToIndexMap) );
    }
    else {
	map->tree = NULL;
	map->namedSystemCount = 0;
	map->compare = compare;
    }

    return map;
}

/*
 * Frees an identifier-to-unit map.  All entries are freed.
 *
 * Arguments:
 *	map		Pointer to the identifier-to-unit map.
 * Returns:
 */
static void
nstimFree(
    NamedSystemToIndexMap*	map)
{
    if( map != NULL ) {
	while( map->tree != NULL ) {
	    NamedSystemSearchEntry* entry = *(NamedSystemSearchEntry**) map->tree;

	    (void) tdelete( entry, &map->tree, map->compare );
	    nsseFree( entry );
	}
	free( map );
    }					/* valid arguments */
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
 *	NULL		"value" is 0.
 *	NULL		Insufficient storage space is available.
 *	else		Pointer to the named-system-search-entry that matches "id".
 */
static const NamedSystemSearchEntry*
nstimSearch(
    NamedSystemToIndexMap*	map,
    const char* const		id,
    int				index)
{
    const NamedSystemSearchEntry*	entry = NULL;	/* failure */
    int	maxNamedSystemCount = 32; /* This is tied to the ut_unit implementation */
    //printf( "Entering nstimSearch( map, \"%s\" )\n", id );

    if( index > map->namedSystemCount ) {
	//printf( "- Status set to US_BAD_ARG for non-existant index request\n" );
	ut_set_status( UT_BAD_ARG );
    }
    else if( index < 0 && map->namedSystemCount > maxNamedSystemCount ) {
	//printf( "- Status set to US_OS for too many named systems\n" );
	ut_set_status( UT_OS );
    }
    else if( id == NULL || map == NULL || strlen(id) <= 0 ) {
	//printf( "- Parameters are not good, status set to UT_BAD_ARG\n" );
	ut_set_status( UT_BAD_ARG );
    }
    else {
	NamedSystemSearchEntry* const*	treeEntry = NULL;
	void**				tree = &map->tree;
	//printf( "- Parameters all seem good\n" );
	NamedSystemSearchEntry* const	newEntry = nsseNew( id, index < 0 ? map->namedSystemCount : index );
	if( newEntry == NULL ) {
	    //printf( "- Status set to US_OS for NULL return from nsseNew()\n" );
	    ut_set_status( UT_OS );
	}
	else {
	    treeEntry = tsearch( newEntry, tree, map->compare );
	    if( treeEntry == NULL ) {
	        //printf( "- Status set to US_OS for NULL return from tsearch()\n" );
		nsseFree( newEntry );
		// The tsearch() function returns NULL if allocation of a new node fails
		// (usually due to a lack of free memory) and the tsearch() function
		// returns NULL if rootp is NULL
		ut_set_status( UT_OS );
	    }
	    else {
	        //printf( "- Got a non-null return from tsearch()\n" );
		tree = &(*treeEntry)->nextTree;	/* next binary-search tree */
		// If the tree entry returned is not the new entry created
		// then the name was found and the new one can be freed.
		if( newEntry != *treeEntry ) {
	            //printf( "- It is a non-new entry, a previously existing entry\n" );
		    nsseFree( newEntry );
		    entry = *treeEntry;
		    ut_set_status( (index >= 0 && entry->index != index) ?
		    			UT_EXISTS : UT_SUCCESS );
		}
		// If the tree entry returned is the new one then we just
		// added a new named system and we need to bump the count
		// unless we have forced an index.
		else {
	            //printf( "- It is a new entry\n" );
		    if( index < 0 ) map->namedSystemCount++;
		    entry = newEntry;
		    ut_set_status( UT_SUCCESS );
		}
	    }
	}
    }
    return entry;
}


/*
 * Returns the named system search-entry that matches the beginning of a string.
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
static const NamedSystemSearchEntry*
nstimFind(
    NamedSystemToIndexMap* const	map,
    const char* const			string)
{
    const NamedSystemSearchEntry*	entry = NULL;	/* failure */

    if( string == NULL || map == NULL || strlen(string) <= 0 ) {
	ut_set_status( UT_BAD_ARG );
    }
    else {
	NamedSystemSearchEntry*	lastEntry = NULL;
	void**			tree = &map->tree;

	NamedSystemSearchEntry		targetEntry;
	NamedSystemSearchEntry* const*	treeEntry;
		
	targetEntry.name = string;
	treeEntry = tfind( &targetEntry, tree, map->compare );

	if( treeEntry == NULL ) {
	    ut_set_status( UT_UNKNOWN );
	}
	else {
	    lastEntry = *treeEntry;
	}

	if( lastEntry != NULL && lastEntry->index >= 0 ) {
	    ut_set_status( UT_SUCCESS );
	    entry = lastEntry;
    	}
    }
    return entry;
}


/******************************************************************************
 * Public API:
 ******************************************************************************/

const NamedSystemSearchEntry*
findOrAddNamedSystem(
    ut_system* const	system,
    SystemMap** const	systemMap,
    const char* const	name,
    int			index,
    int			(*compare)( const void*, const void* ))
{
    const NamedSystemSearchEntry* entry = NULL; /* Error, could not be added or not found */
    ut_status		status;
    // Make this function universion search/find (i.e., can add or not)
    int	canAdd = (compare != NULL);
    //printf( "Entering findOrAddNamedSystem( system, systemMap, \"%s\", compare )\n", name );
    //printf( "- Can add name is %s\n", canAdd ? "TRUE" : "FALSE" );

    if( system == NULL || systemMap == NULL ||
        name == NULL || strlen(name) == 0 ||
	(!canAdd && *systemMap == NULL) ) {
	//printf( "- Status being set to UT_BAD_ARG for null pointer problem\n" );
	status = UT_BAD_ARG;
    }
    else {
	if( *systemMap == NULL ) {
	    *systemMap = smNew();

	    if( *systemMap == NULL )
		//printf( "- Status being set to UT_OS for failure of smNew()\n" );
		status = UT_OS;
	}

	if( *systemMap != NULL ) {
	    NamedSystemToIndexMap** const namedSystemToIndex = (NamedSystemToIndexMap**) (canAdd
			? smSearch( *systemMap, system )
			: smFind( *systemMap, system ));

	    if( namedSystemToIndex == NULL ) {
		//printf( "- Status being set to %s for namedSystemToIndex being null\n",
		//		canAdd ? "UT_OS" : "UT_UNKNOWN" );
		status = canAdd ? UT_OS : UT_UNKNOWN;
	    }
	    else {
		if( canAdd && *namedSystemToIndex == NULL ) {
		    *namedSystemToIndex = nstimNew( compare );

		    if( *namedSystemToIndex == NULL )
			//printf( "- Status being set to UT_OS for nstimNew returning NULL\n" );
			status = UT_OS;
		}

		if( *namedSystemToIndex != NULL ) {
		    entry = canAdd ? nstimSearch( *namedSystemToIndex, name, index )
				   : nstimFind( *namedSystemToIndex, name );
		    if( entry == NULL ) {
			//printf( "- Status being set to UT_UNKNOWN for entry being NULL\n" );
			status = UT_UNKNOWN;
		    }
		    else {
			//printf( "- Status being set to UT_SUCCESS for non-NULL entry\n" );
			status = UT_SUCCESS;
		    }			/* have named system entry */
		}			/* have named system to index entry */
	    }				/* have system-map entry */
	}				/* have system-map */
    }					/* valid arguments */
    ut_set_status( status );
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
addNamedSystem(
    ut_system* const	system,
    SystemMap** const	systemMap,
    const char* const	name,
    int			index,
    int			(*compare)( const void*, const void* ))
{
    return findOrAddNamedSystem( system, systemMap, name, index, compare );
}


/*
 * Looks up the named system and returns its index.
 *
 * Arguments:
 *	system	Pointer to the unit-system.
 *	string	Pointer to the string to be search for.
 * Returns:
 *	-1		String not found or error
 *	index		The index value (zero to max) assigned to the named system
 *
 *	ut_get_status() will return:
 *		UT_BAD_ARG	"string" or system is NULL.
 *		UT_UNKNOWN	A name-prefix was not discovered.
 *		UT_SUCCESS	Success.  "*value" and "*len" will be set if non-NULL.
 */
int
utFindNamedSystemIndex(
    ut_system* const	system,
    const char* const	string)
{
    const NamedSystemSearchEntry* entry = findOrAddNamedSystem( system, &systemToNameToIndex, string, -1, NULL );
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
    if( system != NULL && systemToNameToIndex != NULL) {
	NamedSystemToIndexMap** const namedSystemToIndex =
		    (NamedSystemToIndexMap**) smFind( systemToNameToIndex, system );

	if( namedSystemToIndex != NULL )
	    nstimFree( *namedSystemToIndex );

	smRemove( systemToNameToIndex, system );
	systemToNameToIndex = 0;
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
    //printf( "Entering ut_add_named_system( system, \"%s\", encoding )\n", name );
    addNamedSystem( system, &systemToNameToIndex, name, -1, nsseInsensitiveCompare );
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
 *	named_system	Existing named system name
 * Returns:
 *	UT_SUCCESS	Success.
 *	UT_BAD_ARG	"system", "name" or "named_system" is NULL
 *	UT_OS		Operating-system failure.  See "errno".
 */
ut_status
ut_map_name_to_named_system(
    ut_system* const	system,
    const char* const	name,	/* New name */
    const ut_encoding	encoding,
    const char* const	named_system)
{
    ut_status	status;
    if( system == NULL || name == NULL || strlen(name) <= 0 ||
        named_system == NULL || strlen(named_system) <= 0 ) {
	status = UT_BAD_ARG;
    }
    else {
        int existingIndex = utFindNamedSystemIndex( system, named_system );
        if( existingIndex < 0 ) {
	    // Should I just create it or return an error here???
	    status = UT_UNKNOWN;
        }
        else {
	    int newIndex = utFindNamedSystemIndex( system, name );
	    if( newIndex >= 0 ) {
	        if( newIndex != existingIndex ) {
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
		const NamedSystemSearchEntry* entry = addNamedSystem( system, &systemToNameToIndex,
			name, existingIndex, nsseInsensitiveCompare );
		status = (entry != NULL) ? UT_SUCCESS : ut_get_status();
	    }
        }
    }
    return status;
}
