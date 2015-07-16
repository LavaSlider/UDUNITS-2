/*
 * Copyright 2015 David W. Stockton
 *
 * Utility extension for providing and managing lists for the user.
 *
 * This file is part of the UDUNITS-2 package.  See the file COPYRIGHT
 * in the top-level source-directory of the package for copying and
 * redistribution conditions.
 *
 * This was writtn quick and dirty... a generic lists structure
 * should be created that has an array of void pointers then variants
 * like ut_string_list, ut_unit_list, etc. would be easier. For now
 * we only need string lists so that is what we have.
 */

/*LINTLIBRARY*/

#ifndef	_XOPEN_SOURCE
#   define _XOPEN_SOURCE 500
#endif

#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>	/* For printf */
#if __DARWIN_C_LEVEL < 200112L && !defined(_C99_SOURCE) && !defined(__cplusplus)
__BEGIN_DECLS
int	 snprintf(char * __restrict, size_t, const char * __restrict, ...) __printflike(3, 4);
__END_DECLS
#endif /* __DARWIN_C_LEVEL < 200112L && !defined(_C99_SOURCE) && !defined(__cplusplus) */

#include <string.h>
// Add a declaration to suppress a compiler warning when
// compiling on Mountain Lion because the declaration is
// ifdef'd out in string.h
#if __DARWIN_C_LEVEL < 200112L
char *strdup(const char *);
#endif /* __DARWIN_C_LEVEL < 200112L */
#include "udunits2.h"
#include "ut_lists.h"

/******************************************************************************
 * String Lists
 * Below is the private API for ut_string_lists (declared in ut_lists.h)
 ******************************************************************************/
/* typedef of this to ut_string_list is in udunits2.h */
struct __ut_string_list {
    char**	_list;		// The malloc'd array for the strings
    int		_len;		// The number of strings in the array
    int		_listSize;	// The size of the array
};


/*
 * Allocate and initialize an empty ut_string_list
 */
ut_string_list*
ut_string_list_new()
{
    ut_string_list* list = (ut_string_list*) calloc(1, sizeof(ut_string_list));
    if (list == NULL) {
	ut_handle_error_message(strerror(errno));
	ut_handle_error_message(
		"Couldn't allocate %lu-bytes to create new ut_string_list",
		sizeof(ut_string_list) );
    }
    return list;
}


/*
 * Allocate and initialize an empty ut_string_list with an initial array
 * capacity specified. This only a suggestion to minimize realloc'ing of
 * the internal array.
 */
ut_string_list*
ut_string_list_new_with_capacity(
    int	n)
{
    ut_string_list *list = ut_string_list_new();
    if (list && n > 0) {
	list->_listSize = n;
	list->_list = (char**) calloc(n, sizeof(char *));
	if (list->_list == NULL) {
	    ut_handle_error_message(strerror(errno));
	    ut_handle_error_message(
		"Couldn't allocate %lu-bytes to initialize ut_string_list",
		n * sizeof(char *) );
	    list->_listSize = 0;
	}
    }
    return list;
}


/*
 * Free the unused space in the internal array.
 */
void
ut_string_list_truncate_waste(
    ut_string_list*	list)
{
    if (list && list->_list && list->_listSize > list->_len) {
	/* Potential realloc memory leak */
	/* but we are reducing the size so low risk of realloc failure */
	list->_list = (char**) realloc(list->_list, list->_len * sizeof(char *));
	list->_listSize = list->_len;
    }
}


/*
 * Append a string to the end of the array.
 */
void
ut_string_list_add_element(
    ut_string_list*	list,
    const char* const	string)
{
    if (list) {
	if (list->_list == NULL) {
	    list->_list = (char**) malloc(sizeof(char *));
	    if (list->_list == NULL ) {
		ut_handle_error_message(strerror(errno));
		ut_handle_error_message(
		    "Couldn't allocate %lu-bytes to initialize ut_string_list",
		    sizeof(char *) );
		list->_listSize = 0;
	    }
	    else {
		list->_listSize = 1;
	    }
	    list->_len = 0;
	}
	if (list->_list != NULL) {
	    if (list->_len >= list->_listSize) {
		char **newList = (char**) realloc(list->_list, (list->_len + 1) * sizeof(char *));
		if (newList == NULL) {
		    ut_handle_error_message(strerror(errno));
		    ut_handle_error_message(
			"Couldn't allocate %lu-bytes to extend ut_string_list",
			(list->_len + 1) * sizeof(char *) );
		}
		else {
		    list->_list = newList;
		    list->_listSize = list->_len + 1;
		}
	    }
	    if (list->_list && list->_len < list->_listSize) {
	        list->_list[list->_len] = string ? strdup(string) : NULL;
		++list->_len;
	    }
	}
    }
}


/*-----------------------------------------------------------------------------
 * Below is the public API for ut_string_lists (declared in udunits2.h)
 *----------------------------------------------------------------------------*/
/*
 * Free all the allocated memory for the string list. Must be called on
 * all ut_string_lists return by library functions.
 */
void
ut_string_list_free(
    ut_string_list* list)
{
    if (list) {
	for (int i = 0; i < list->_len; ++i)
	    if (list->_list[i]) free(list->_list[i]);
	free(list->_list);
	list->_list = NULL;
	list->_listSize = 0;
	list->_len = 0;
	free(list);
    }
}


/*
 * Return the number of elements in the list
 */
int
ut_string_list_length(
    const ut_string_list* const	list)
{
    return list ? list->_len : 0;
}


/*
 * Return the specified string from the list (zero based)
 */
char*
ut_string_list_element(
    const ut_string_list* const	list,
    int				element)
{
    if (list && element >= 0 && element < list->_len)
	return list->_list[element];
    return NULL;
}


/*
 * Returns a malloc'd string containing all the list entries
 * in the same order, with the separator string between each
 * entry except finalSeparator between the last two.
 *
 * For the ut_string_list:
 *	("red", "green", "yellow", "blue")
 * After:
 *   char *foo = ut_string_list_implode(list,", "," and ");
 * foo contains: "red, green, yellow and blue"
 *
 * After:
 *   char *foo = ut_string_list_implode(list,", ",NULL);
 * foo contains: "red, green, yellow, blue"
 *
 * There is no escaping of embedded separators within the
 * output string. So for the ut_string_list:
 *	("one", "two", "two-and-a-half", "three")
 *
 * After:
 *   char *foo = ut_string_list_implode(list,"-",NULL);
 * foo contains: "one-two-two-and-a-half-three"
 * not: "one-two-two\-and\-a\-half-three"
 * 
 */
char*
ut_string_list_implode(
    const ut_string_list* const	list,
    const char* const		separator,
    const char* const		finalSeparator)
{
    char* string = NULL;
    if (list) {
	int		sep1Size = 0;
	const char*	sep1 = "";
	int		sep2Size = 0;
	const char*	sep2 = "";
	if (separator) {
	    sep1Size = sep2Size = strlen(separator);
	    if (sep1Size > 0) {
	        sep1 = separator;
		sep2 = separator;
	    }
        }
	if (finalSeparator) {
	    sep2Size = strlen(finalSeparator);
	    if (sep2Size > 0) {
		sep2 = finalSeparator;
	    }
        }
	int	totalSize = 1; // Space for the null terminator
	if (list->_len > 1)
	    totalSize += sep2Size;
	if (list->_len > 2)
	    totalSize += (list->_len - 2) * sep1Size;
	for (int i = 0; i < list->_len; ++i) {
	    totalSize += list->_list[i] ? strlen(list->_list[i]) : 0;
	}
	string = malloc( totalSize );
	char *p = string;
	if( p ) *p = '\0';
	for (int i = 0; p && i < list->_len; ++i) {
	    if (i > 0 && i == list->_len - 1) {
	        strcpy(p, sep2);
		p += sep2Size;
	    }
	    else if (i > 0) {
	        strcpy(p, sep1);
		p += sep1Size;
	    }
	    if (list->_list[i]) {
	        strcpy(p, list->_list[i]);
	        p += strlen(list->_list[i]);
	    }
	}
    }
    return string;
}


/*
 * Returns a pointer ut_string_list created by separating
 * the input string into pieces in the inverse of what
 * implode does.
 *
 * After:
 *   list = ut_string_list_explode("red, green, yellow and blue", "," and ");
 * list contains: (red, green, yellow, blue)
 *
 * After:
 *   list = ut_string_list_implode("red, green, yellow, blue",", ",NULL);
 * list contains: (red, green, yellow, blue)
 *
 * There is currently no mechanism to embed separators within a string
 * (i.e., there is no way to escape separators)
 * 
 */
ut_string_list*
ut_string_explode(
    const char* const	_string,
    const char* const	_separator,
    const char* const	_finalSeparator)
{
   ut_string_list*	list = NULL;

   if (_string) {
	char*	string = strdup(_string);
	int		sep1Size = 0, sep2Char = 0;
	int		sep1Char = 0, sep2Size = 0;
	const char*	sep1 = "";
	const char*	sep2 = "";
	if (_separator) {
	    sep1Size = strlen(_separator);
	    if (sep1Size > 0) {
	        sep1 = _separator;
		sep1Char = *sep1;
	    }
        }
	if (_finalSeparator) {
	    sep2Size = strlen(_finalSeparator);
	    if (sep2Size > 0) {
		sep2 = _finalSeparator;
		sep2Char = *sep2;
	    }
        }
	char* lastSep2 = NULL;
	if( sep2Size > 0 ) {
	    char* p = string;
	    while ((p=strstr(p,sep2)) != NULL) {
	    	lastSep2 = p;
		++p;
	    }
	}
	if (sep1Size > 0) {
	    int	n = 1;
	    char* p = string;
	    while ((p=strstr(p,sep1)) != NULL) {
		if (lastSep2 && p > lastSep2) {
		    break;
		}
	    	p += sep1Size;
		++n;
	    }
	    if (lastSep2) {
	        ++n;
	    }
	    list = ut_string_list_new_with_capacity(n);
	    char* s = p = string;
	    while( (p=strstr(p,sep1)) != NULL ) {
		if (lastSep2 && p > lastSep2) {
		    break;
		}
		*p = '\0';
		ut_string_list_add_element(list,s);
	    	p += sep1Size;
		s = p;
	    }
	    if (lastSep2) {
		*lastSep2 = '\0';
		ut_string_list_add_element(list,s);
		s = lastSep2 + sep2Size;
		ut_string_list_add_element(list,s);
	    }
	    else {
		ut_string_list_add_element(list,s);
	    }
	}
	else {
	    list = ut_string_list_new_with_capacity(1);
	    ut_string_list_add_element(list,string);
	}
	free (string);
   }

   return list;
}
