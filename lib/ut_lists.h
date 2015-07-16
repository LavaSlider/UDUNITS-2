/**
 * @author David W. Stockton
 */

#ifndef UT_LISTS_H
#define UT_LISTS_H

#ifdef __cplusplus
extern "C" {
#endif


/*
 * Allocate and initialize an empty ut_string_list
 */
ut_string_list*
ut_string_list_new();

/*
 * Allocate and initialize an empty ut_string_list with an initial array
 * capacity specified. This only a suggestion to minimize realloc'ing of
 * the internal array.
 */
ut_string_list*
ut_string_list_new_with_capacity(
    int	n);

/*
 * Free the unused space in the internal array.
 */
void
ut_string_list_truncate_waste(
    ut_string_list*	list);

/*
 * Append a string to the end of the array.
 */
void
ut_string_list_add_element(
    ut_string_list*	list,
    const char* const	string);

#ifdef __cplusplus
}
#endif

#endif
