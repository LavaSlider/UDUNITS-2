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
#include <stdio.h>	/* For printf */
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

typedef struct {
    void*	tree;
    int		namedSystemCount;	// The number of unique units system names
    int		namedSystemNamesCount;	// The total number of names including aliases
    int		(*compare)( const void*, const void* );
} NamedSystemToIndexMap;

typedef struct {
    void*		nextTree;
    const char*		name;
    int			index;
    unsigned short	flags;
} NamedSystemSearchEntry;

#define	NSSEFLAG_PRIMARY_NAME	0x01

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
	entry->nextTree = NULL;
	entry->index = index;
	entry->flags = 0x00;
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
	map->namedSystemCount = 0;	// The number of unique units system names
	map->namedSystemNamesCount = 0;	// The total number of names including aliases
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
    int	maxNamedSystemCount = INT_MAX; /* I made bitmaps extensible, not fixed */
    //printf( "Entering nstimSearch( map, \"%s\" )\n", id );

    if( index > map->namedSystemCount ) {
	//printf( "- Status set to US_BAD_ARG for non-existant index request\n" );
	ut_set_status( UT_BAD_ARG );
    }
    else if( index < 0 && map->namedSystemCount > maxNamedSystemCount ) {
	//printf( "- Status set to US_OS for too many named systems\n" );
	ut_set_status( UT_OS );
    }
    else if( map == NULL || id == NULL || *id == '\0' ) {
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
		    if( index < 0 ) {
			map->namedSystemCount++; // Bump the number of primary  names
			newEntry->flags |= NSSEFLAG_PRIMARY_NAME;
		    }
		    map->namedSystemNamesCount++;  // Bump the total number of names
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

    if( map == NULL || string == NULL || *string == '\0' ) {
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

static const NamedSystemSearchEntry*
findOrAddNamedSystem(
    const ut_system* const	system,
    SystemMap** const		systemMap,
    const char* const		name,
    int				index,
    int				(*compare)( const void*, const void* ))
{
    const NamedSystemSearchEntry* entry = NULL; /* Error, could not be added or not found */
    ut_status		status;
    // Make this function universion search/find (i.e., can add or not)
    int	canAdd = (compare != NULL);
    //printf( "Entering findOrAddNamedSystem( system, systemMap, \"%s\", compare )\n", name );
    //printf( "- Can add name is %s\n", canAdd ? "TRUE" : "FALSE" );

    if( system == NULL || systemMap == NULL ||
        name == NULL || *name == '\0' ||
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
        findOrAddNamedSystem( system, &systemToNameToIndex, system_name, -1, NULL );
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
    if( system == NULL || name == NULL || *name == '\0' ||
        system_name == NULL || *system_name == '\0' ) {
	status = UT_BAD_ARG;
    }
    else {
        int existingIndex = utFindNamedSystemIndex( system, system_name );
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

struct __ut_string_list {
    char**	_list;
    int		_len;
    int		_listSize;
};

ut_string_list*
ut_string_list_new() {
    ut_string_list* list = (ut_string_list*) calloc( 1, sizeof(ut_string_list) );
    return list;
}
ut_string_list*
ut_string_list_new_with_capacity( int n ) {
    ut_string_list *list = ut_string_list_new();
    if( list ) {
        list->_list = (char**) calloc( n + 1, sizeof(char *) );
	list->_listSize = n + 1;
    }
    return list;
}
void
ut_string_list_truncate_waste( ut_string_list* list ) {
    if( list && list->_list && list->_listSize > list->_len ) {
	list->_list = (char**) realloc( list->_list, list->_len * sizeof(char *) );
	list->_listSize = list->_len;
    }
}
void
ut_string_list_add_element( ut_string_list* list, const char *string ) {
    if( list && string ) {
	if( list->_list == NULL ) {
	    list->_list = (char**) malloc( sizeof(char *) );
	    list->_listSize = 1;
	    list->_len = 0;
	}
	if( list->_list != NULL ) {
	    if( list->_len >= list->_listSize ) {
		char **newList = (char**) realloc( list->_list, (list->_len + 1) * sizeof(char *) );
		if( newList == NULL ) {
		    ut_handle_error_message( strerror(errno) );
		    ut_handle_error_message(
			"Couldn't allocate %lu-bytes to extend ut_string_list",
			(list->_len + 1) * sizeof(char *) );
		}
		else {
		    list->_list = newList;
		    list->_listSize = list->_len + 1;
		}
	    }
	    if( list->_list && list->_len < list->_listSize ) {
	        list->_list[list->_len] = strdup( string );
		++list->_len;
	    }
	}
    }
}
void
ut_string_list_free( ut_string_list* list ) {
    if( list ) {
	for( int i = 0; i < list->_len; ++i )
	    if( list->_list[i] ) free( list->_list[i] );
	free( list->_list );
	free( list );
    }
}
int
ut_string_list_length( const ut_string_list* const list ) {
    return list ? list->_len : 0;
}
char *
ut_string_list_element( const ut_string_list* const list, int element ) {
    if( list && element >= 0 && element < list->_len )
        return list->_list[element];
    return NULL;
}

static ut_string_list*	__name_list =  NULL;
static int		__doAll     =  1;	// Set true/false to save/print all names
static int		__doUniq    =  0;	// Set true/false to save/print only primary names
static int		__aliasesOf = -1;	// Set to index of aliases to save/print
static void twalkGetNameListAction( const void *node, VISIT order, int level );

/*
 *  Get the list of named unit systems for the system.
 *
 *  If system_name is
 *    System name	Get aliases of named units system
 *    Empty string ("")	Get all primary unit system names
 *    NULL		Get all the defined named unit systems
 *
 *  Returns
 *    NULL		If system == NULL or has no named systems defined
 *    ut_string_list*	Containing the list of selected units
 *			system names per the system_name parameter.
 */
ut_string_list*
_ut_get_named_system_aliases( const ut_system* const  system, const char* const system_name ) {
    ut_string_list* list = NULL;
    if( system != NULL && systemToNameToIndex != NULL) {
	NamedSystemToIndexMap** const namedSystemToIndex =
		    (NamedSystemToIndexMap**) smFind( systemToNameToIndex, system );
	if( namedSystemToIndex && *namedSystemToIndex ) {
	    NamedSystemToIndexMap*	map = *namedSystemToIndex;
	    if( map && map->namedSystemNamesCount > 0 ) {
		if( system_name != NULL && *system_name != '\0' ) {
		    __aliasesOf = utFindNamedSystemIndex( system, system_name );
		    __doUniq = 0;	  // Not primary
		    __doAll = 0;	  // Not all
		}
		else if( system_name != NULL && *system_name == '\0' ) {
		    __aliasesOf = -1; // Not aliases of anything
		    __doUniq = 1;	  // Just primary
		    __doAll = 0;	  // Not all
		}
		else {
		    __aliasesOf = -1; // Not aliases of anything
		    __doUniq = 0;	  // Not primary
		    __doAll = 1;	  // Get them all
		}
		__name_list = ut_string_list_new_with_capacity( map->namedSystemNamesCount );
		twalk( map->tree, twalkGetNameListAction );
		ut_string_list_truncate_waste( __name_list );
		list = __name_list;
		__name_list = NULL;
	    }
	}
    }
    return list;
}

/*
 *  Get the list of named unit systems for the system.
 *
 *  If system_name is
 *    System name	Get aliases of named units system
 *    Empty string ("")	Get all primary unit system names
 *    NULL		Get all the defined named unit systems
 *
 *  Returns
 *    NULL		If system == NULL or has no named systems defined
 *    ut_string_list*	Containing the list of selected units
 *			system names per the system_name parameter.
 */
ut_string_list*
ut_get_named_system_aliases( const ut_system* const system, const char* const system_name ) {
    return _ut_get_named_system_aliases( system, system_name );
}
/*
 *  Get the list of the primary named unit systems for the system.
 *  That is, only unique named units systems will be returned.
 *
 *  Returns
 *    NULL		If system == NULL or has no named systems defined
 *    ut_string_list*	Containing the list of unique selected units
 *			system names.
 */
ut_string_list*
ut_get_named_systems( const ut_system* const system ) {
    return _ut_get_named_system_aliases( system, "" );
}

static void twalkGetNameListAction( const void *node, VISIT order, int level ) {
    NamedSystemSearchEntry**	nssep = (NamedSystemSearchEntry**) node;
    NamedSystemSearchEntry*	nsse = *nssep;

    switch( order ) {
    case preorder:
    case endorder:
	break;
    case postorder:
    case leaf:
	if( __doAll || (__doUniq && (nsse->flags & NSSEFLAG_PRIMARY_NAME)) ||
	    (__aliasesOf >= 0 && nsse->index == __aliasesOf) ) {
	    if( __name_list ) {
		ut_string_list_add_element( __name_list, nsse->name );
	    }
	    else {
		//printf( "-- level %2d: ", level );
		printf( "%2d %#04x \"%s\"", nsse->index, nsse->flags, nsse->name );
		printf( "\n" );
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

/******************************************************************************
 * Generic Bitmap Functions:
 ******************************************************************************/
/* Could this have been done with bit_strings (see man bit_test) */
typedef unsigned int chunkType;
typedef struct Bitmap {
    int		chunkCount;
    chunkType	*chunks;
} Bitmap;

/*
 *
 */
void
fprintfBitmap( FILE *fp, const char *fmt, Bitmap *bitmap ) {
    if( bitmap && bitmap->chunks ) {
	int	skip = 1;
	int	altForm = 0;
	int	leftAlign = 0;
	int	fieldWidth = 0;
	int	precision = 0;
	int	format = 'x';
	// Figure out the format options
	if( fmt ) {
	    if( *fmt == '%' ) {			++fmt; }
	    if( *fmt == '#' ) { altForm = 1;	++fmt; }
	    if( *fmt == '-' ) { leftAlign = 1;	++fmt; }
	    if( *fmt == '+' ) {			++fmt; }
	    if( *fmt == '0' ) { skip = 0;	++fmt; }
	    while( isdigit( *fmt ) ) {
		fieldWidth = 10 * fieldWidth + (*fmt - '0');
		++fmt;
	    }
	    if( *fmt == '.' ) {
		++fmt;
		while( isdigit( *fmt ) ) {
		    precision = 10 * precision + (*fmt - '0');
		    ++fmt;
		}
	    }
	    if( isalpha( *fmt ) ) { format = *fmt; ++fmt; }
	}
	if( altForm ) {
		if( format == 'X' )
			fprintf( fp, "0X" );
		else	fprintf( fp, "0x" );
	}
	for( int i = bitmap->chunkCount - 1; i >= 0; --i ) {
	    if( skip && bitmap->chunks[i] == 0 ) {
	    } else {
		if( skip ) {
	            fprintf( fp, "%x", bitmap->chunks[i] );
		    skip = 0;
		} else {
	            fprintf( fp, "%0*x", (int) (2 * sizeof(*bitmap->chunks)), bitmap->chunks[i] );
	        }
	    }
	}
    }
}
void
printfBitmap( const char *fmt, Bitmap *bitmap ) {
    fprintfBitmap( stdout, fmt, bitmap );
}

void
bitmapInit(
    Bitmap*	entry)
{
    if( entry != NULL ) {
	entry->chunkCount = -1;
	entry->chunks = NULL;
    }
}

void
bitmapReset(
    Bitmap*	entry)
{
    if( entry != NULL ) {
        if( entry->chunks ) free( (void *) entry->chunks );
	entry->chunks = NULL;
	entry->chunkCount = -1;
    }
}

Bitmap *
bitmapNew() {
    Bitmap*	entry = malloc( sizeof(Bitmap) );

    if( entry == NULL ) {
	ut_handle_error_message( strerror(errno) );
	ut_handle_error_message(
            "Couldn't allocate %lu-byte bitmap",
	    sizeof(Bitmap) );
    }
    else {
	bitmapInit( entry );
    }

    return entry;
}

int
bitmapCmp(
    const Bitmap*	b1,
    const Bitmap*	b2)
{
    int i, j;
    // Both NULL or point to the same place
    if( b1 == b2 )
	return 0;
    // Both empty
    if( b1 == NULL && b2 != NULL && b2->chunkCount <= 0 )
	return 0;
    if( b1 != NULL && b1->chunkCount <= 0 && b2 == NULL )
        return 0;
    if( b1 != NULL && b2 != NULL && b1->chunkCount <= 0 && b2->chunkCount <= 0 )
	return 0;
    // b1 NULL and b2 not (or they would be equal)
    if( b1 == NULL /*|| (b1 != NULL && b1->chunkCount <= 0)*/ ) {
	if( b2->chunkCount > 0 && b2->chunks != NULL ) {
	    for( i = 0; i < b2->chunkCount; ++i ) {
		if( b2->chunks[i] != 0 )
		    return -1;	// b1 empty, b2 not so b1 < b2
	    }
	}
	return 0;
    }
    // b1 not NULL and b2 NULL (or they would be equal)
    if( b2 == NULL /*|| b2->chunkCount <= 0*/ ) {
	if( b1->chunkCount > 0 && b1->chunks != NULL ) {
	    for( i = 0; i < b1->chunkCount; ++i ) {
		if( b1->chunks[i] != 0 )
		    return 1;	// b2 empty, b1 not so b1 > b2
	    }
	}
	return 0;
    }
    // Neither b1 nor b2 NULL
    if( b1->chunkCount > b2->chunkCount ) {
	i = b1->chunkCount - 1;
	while( i >= b2->chunkCount ) {
	    if( b1->chunks[i] != 0 )
		return 1;
	    --i;
	}
    }
    else if( b1->chunkCount < b2->chunkCount ) {
	i = b2->chunkCount - 1;
	while( i >= b1->chunkCount ) {
	    if( b2->chunks[i--] != 0 )
		return -1;
	}
    }
    else {
	i = b1->chunkCount - 1;
    }
    while( i >= 0 ) {
	if( b1->chunks[i] > b2->chunks[i] ) {
	    return 1;
	}
	else if( b1->chunks[i] < b2->chunks[i] ) {
	    return -1;
	}
	--i;
    }
    return 0;
}

Bitmap *
bitmapCopy(
    Bitmap*		dest,
    const Bitmap*	src)
{
    if( dest ) {
        bitmapReset( dest );
	if( src && src->chunkCount > 0 && src->chunks ) {
	    int n = src->chunkCount - 1;
	    while( n >= 0 ) {
		if( src->chunks[n] )
		    break;
		--n;
	    }
	    dest->chunkCount = n + 1;
	    dest->chunks = malloc( (n+1) * sizeof(*(dest->chunks)) );
	    for( int i = 0; i <= n; ++i )
		dest->chunks[i] = src->chunks[i];
	}
    }
    return dest;
}

Bitmap *
bitmapDup(
    const Bitmap*	src)
{
    Bitmap*		newBitmap = NULL;
    if( src ) {
	newBitmap = bitmapNew();
	bitmapCopy( newBitmap, src );
    }
    return newBitmap;
}

void
bitmapFree(
    Bitmap*	entry)
{
    bitmapReset( entry );
    free( entry );
}

int
bitIsSet(
    Bitmap*	bitmap,
    int		i)
{
    int	isSet = 0;
    if( i >= 0 && bitmap && bitmap->chunks ) {
	int chunkSize = sizeof(*(bitmap->chunks)) << 3;
	int chunk = i / chunkSize;
	if( chunk < bitmap->chunkCount ) {
	    int bit = i % chunkSize;
	    if( (bitmap->chunks[chunk] & (0x01 << bit)) != 0 )
		isSet = 1;
	}
    }
    return isSet;
}

int
setBit(
    Bitmap*	bitmap,
    int		n)
{
    int	wasSet = 0;
    if( n >= 0 && bitmap ) {
	size_t chunkSize = sizeof(*(bitmap->chunks)) << 3;
	int chunk = n / chunkSize;
	// Allocate space if needed
	if( chunk >= bitmap->chunkCount || bitmap->chunks == NULL ) {
	    bitmap->chunks = realloc( bitmap->chunks, (chunk+1) * sizeof(*(bitmap->chunks)) );
	    if( bitmap->chunks ) {
		int i = bitmap->chunkCount > 0 ? bitmap->chunkCount : 0;
		while( i <= chunk )
		    bitmap->chunks[i++] = 0;
	    }
	    bitmap->chunkCount = chunk + 1;
	}
	if( chunk < bitmap->chunkCount ) {
	    int bit = n % chunkSize;
	    if( (bitmap->chunks[chunk] & ((chunkType) (0x01 << bit))) != 0 )
		wasSet = 1;
	    //printf( "Before setting bit %d: ", n );
	    //printfBitmap( "%#x", bitmap );
	    //printf( "\n" );
	    bitmap->chunks[chunk] |= (chunkType) (0x01 << bit);
	    //printf( "After setting bit %d: ", n );
	    //printfBitmap( "%x", bitmap );
	    //printf( "\n" );
	}
    }
    else {
        // Error reporting?
    }
    return wasSet;
}

int
clearBit(
    Bitmap *bitmap,
    int			  i)
{
    int	wasSet = 0;
    if( i >= 0 && bitmap && bitmap->chunks ) {
	size_t chunkSize = sizeof(*(bitmap->chunks)) << 3;
	int chunk = i / chunkSize;
	if( chunk < bitmap->chunkCount ) {
	    int bit = i % chunkSize;
	    if( (bitmap->chunks[chunk] & ((chunkType) (0x01 << bit))) != 0 )
		wasSet = 1;
	    bitmap->chunks[chunk] &= ~((chunkType) (0x01 << bit));
	    /* Should we do any cleanup... if all the chunks are empty
	       we could free them. */
	}
    }
    return wasSet;
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
 *     registry = utAddNamedSystemToRegistry( system, registry, system_name );
 *     if( ut_get_status() != UT_SUCCESS ) {
 *	   fprintf( stderr, "Problem\n" );
 *     }
 */
NamedSystemRegistry*
utAddNamedSystemToRegistry(
    ut_system* const		system,
    NamedSystemRegistry*	bitmap,
    const char* const		system_name)
{
    ut_status	status;
    if( system == NULL ||
        system_name == NULL || *system_name == '\0' ) {
	status = UT_BAD_ARG;
    } else {
	if( bitmap == NULL ) {
	    bitmap = bitmapNew();
	    if( bitmap == NULL ) {
		status = UT_OS;
	    }
	}
	if( bitmap != NULL ) {
	    int	idx = utFindNamedSystemIndex( system, system_name );
	    if( idx < 0 ) {
		status = ut_add_named_system( system, system_name, UT_ASCII );
	    }
	    idx = utFindNamedSystemIndex( system, system_name );
	    if( idx < 0 ) {
		status = ut_get_status();
	    }
	    else {
		status = UT_SUCCESS;
		setBit( bitmap, idx );
	    }
	}
    }
    ut_set_status( status );
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
 *     status = utAddNamedSystemToRegistry( system, &registry, system_name );
 *     if( status != UT_SUCCESS ) {
 *	   fprintf( stderr, "Problem\n" );
 */
ut_status
utAddNamedSystemToRegistryLocation(
    ut_system* const		system,
    NamedSystemRegistry**	bitmapPointer,
    const char* const		system_name)
{

    ut_status status;
    if( bitmapPointer ) {
	*bitmapPointer = utAddNamedSystemToRegistry( system, *bitmapPointer, system_name );
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
 *    utRemoveNamedSystemFromRegistry( system, registry, system_name );
 */
ut_status
utRemoveNamedSystemFromRegistry(
    ut_system* const		system,
    NamedSystemRegistry*	bitmap,
    const char* const		system_name)
{
    int		idx = utFindNamedSystemIndex( system, system_name );
    clearBit( bitmap, idx );
    return ut_get_status();
}
/*
 *
 * UT_BAD_ARG	parameter null
 * UT_OS	failure to allocate
 *
 * usage:
 *    utRemoveNamedSystemFromRegistry( system, &registry, system_name );
 */
ut_status
utRemoveNamedSystemFromRegistryLocation(
    ut_system* const		system,
    NamedSystemRegistry**	bitmapPointer,
    const char* const		system_name)
{
    if( bitmapPointer )
	return utRemoveNamedSystemFromRegistry( system, *bitmapPointer, system_name );
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
    ut_set_status( UT_SUCCESS );
    return bitIsSet( bitmap,
		utFindNamedSystemIndex( system, system_name ) );
}

int
utNamedSystemIsInRegistryLocation(
    ut_system* const		system,
    NamedSystemRegistry**	bitmapPointer,
    const char* const		system_name)
{
    if( bitmapPointer == NULL ) {
        ut_set_status( UT_BAD_ARG );
	return 0;
    }
    ut_set_status( UT_SUCCESS );
    return utNamedSystemIsInRegistry( system, *bitmapPointer, system_name );
}

/*
 * Frees the bitmap and any allocated storage within it.
 */
void
utNamedSystemRegistryFree(
    NamedSystemRegistry*	bitmap)
{
    bitmapFree( bitmap );
}
void
utNamedSystemRegistryLocationFree(
    NamedSystemRegistry**	bitmapPointer)
{
    if( bitmapPointer != NULL ) {
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
    bitmapInit( bitmap );
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
    bitmapReset( bitmap );
}
