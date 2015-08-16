/*
 * Copyright 2013 University Corporation for Atmospheric Research
 *
 * This file is part of the UDUNITS-2 package.  See the file COPYRIGHT
 * in the top-level source-directory of the package for copying and
 * redistribution conditions.
 */
#ifndef UT_UNITS2_H_INCLUDED
#define UT_UNITS2_H_INCLUDED

#include <stdarg.h>
#include <stddef.h>

#ifdef _MSC_VER
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <io.h>

#define _USE_MATH_DEFINES

/* Define a bunch of variables to use the ISO C++ conformant name instead
   of the POSIX name. This quiets a lot of the warnings thrown by MSVC. */
#define read _read
#define open _open
#define close _close
#define strdup _strdup
#define strcasecmp stricmp
#define stricmp _stricmp
#define isatty _isatty

//We must accomodate the lack of snprintf in MSVC.
//c99_snprintf is defined in c99_snprintf.c, in lib/.
#define snprintf c99_snprintf

int c99_snprintf(
   char* str,
     size_t size,
     const char* format,
     ...);

int c99_vsnprintf(
char* str,
  size_t size,
  const char* format,
  va_list ap);
  
#endif

/* If we are working in Visual Studio and have a
   shared library, we will need to do some slight-of-hand
   in order to make it generate a proper export library.
*/

#if defined(DLL_UDUNITS2) /* define when library is a DLL */
#   if defined(DLL_EXPORT) /* defined when building the library */
#     define MSC_EXTRA __declspec(dllexport)
#   else
#     define MSC_EXTRA __declspec(dllimport)
#   endif
#else
# define MSC_EXTRA
#endif /* defined(DLL_UDUNITS2) */

#define EXTERNL MSC_EXTRA extern


#include "converter.h"

typedef struct ut_system	ut_system;
typedef union ut_unit		ut_unit;

enum utStatus {
    UT_SUCCESS = 0,	/* Success */
    UT_BAD_ARG,	        /* An argument violates the function's contract */
    UT_EXISTS,		/* Unit, prefix, or identifier already exists */
    UT_NO_UNIT,		/* No such unit exists */
    UT_OS,		/* Operating-system error.  See "errno". */
    UT_NOT_SAME_SYSTEM,	/* The units belong to different unit-systems */
    UT_MEANINGLESS,	/* The operation on the unit(s) is meaningless */
    UT_NO_SECOND,	/* The unit-system doesn't have a unit named "second" */
    UT_VISIT_ERROR,	/* An error occurred while visiting a unit */
    UT_CANT_FORMAT,	/* A unit can't be formatted in the desired manner */
    UT_SYNTAX,		/* string unit representation contains syntax error */
    UT_UNKNOWN,		/* string unit representation contains unknown word */
    UT_OPEN_ARG,	/* Can't open argument-specified unit database */
    UT_OPEN_ENV,	/* Can't open environment-specified unit database */
    UT_OPEN_DEFAULT,	/* Can't open installed, default, unit database */
    UT_PARSE		/* Error parsing unit specification */
};
typedef enum utStatus          ut_status;

enum utEncoding {
    UT_ASCII = 0,
    UT_ISO_8859_1 = 1,
    UT_LATIN1 = UT_ISO_8859_1,
    UT_UTF8 = 2
};
typedef enum utEncoding        ut_encoding;

#define UT_NAMES	4
#define UT_DEFINITION	8


/*
 * Data-structure for a visitor to a unit:
 */
typedef struct ut_visitor {
    /*
     * Visits a basic-unit.  A basic-unit is a base unit like "meter" or a non-
     * dimensional but named unit like "radian".
     *
     * Arguments:
     *	unit		Pointer to the basic-unit.
     *	arg		Client pointer passed to ut_accept_visitor().
     * Returns:
     *	UT_SUCCESS	Success.
     *	else		Failure.
     */
    ut_status	(*visit_basic)(const ut_unit* unit, void* arg); 

    /*
     * Visits a product-unit.  A product-unit is a product of zero or more
     * basic-units, each raised to a non-zero power.
     *
     * Arguments:
     *	unit		Pointer to the product-unit.
     *	count		The number of basic-units in the product.  May be zero.
     *	basicUnits	Pointer to an array of basic-units in the product.
     *	powers		Pointer to an array of powers to which the respective
     *			basic-units are raised.
     *	arg		Client pointer passed to ut_accept_visitor().
     * Returns:
     *	UT_SUCCESS	Success.
     *	else		Failure.
     */
    ut_status	(*visit_product)(const ut_unit* unit, int count,
	const ut_unit* const* basicUnits, const int* powers, void* arg); 

    /*
     * Visits a Galilean-unit.  A Galilean-unit has an underlying unit and a
     * non-unity scale factor or a non-zero offset.
     *
     * Arguments:
     *	unit		Pointer to the Galilean-unit.
     *	scale		The scale factor (e.g., 1000 for a kilometer when the
     *			underlying unit is a meter).
     *	underlyingUnit	Pointer to the underlying unit.
     *	offset		Pointer to the underlying unit.
     *	arg		Client pointer passed to ut_accept_visitor().
     * Returns:
     *	UT_SUCCESS	Success.
     *	else		Failure.
     */
    ut_status	(*visit_galilean)(const ut_unit* unit, double scale,
	const ut_unit* underlyingUnit, double offset, void* arg); 

    /*
     * Visits a timestamp-unit.  A timestamp-unit has an underlying unit of time
     * and an encoded time-origin.
     *
     * Arguments:
     *	unit		Pointer to the timestamp-unit.
     *	timeUnit	Pointer to the underlying unit of time.
     *  origin          Encoded origin of the timestamp-unit.
     *	arg		Client pointer passed to ut_accept_visitor().
     * Returns:
     *	UT_SUCCESS	Success.
     *	else		Failure.
     */
    ut_status	(*visit_timestamp)(const ut_unit* unit,
	const ut_unit* timeUnit, double origin, void* arg); 

    /*
     * Visits a logarithmic-unit.  A logarithmic-unit has a logarithmic base and
     * a unit that specifies the reference level.
     *
     * Arguments:
     *	unit		Pointer to the logarithmic-unit.
     *  base            The logarithmic base (e.g., 2, M_E, 10).
     *	reference	Pointer to the unit that specifies the reference level.
     *	arg		Client pointer passed to ut_accept_visitor().
     * Returns:
     *	UT_SUCCESS	Success.
     *	else		Failure.
     */
    ut_status	(*visit_logarithmic)(const ut_unit* unit, double base,
	const ut_unit* reference, void* arg); 
} ut_visitor;


typedef int (*ut_error_message_handler)(const char* fmt, va_list args);


#ifdef __cplusplus
EXTERNL "C" {
#endif


/******************************************************************************
 * Unit System:
 ******************************************************************************/


/**
 * Returns the pathname of the XML database.
 *
 * @param path      The pathname of the XML file or NULL.
 * @param status    Status. One of UT_OPEN_ARG, UT_OPEN_ENV, or UT_OPEN_DEFAULT.
 * @return          If "path" is not NULL, then it is returned; otherwise, the
 *                  pathname specified by the environment variable
 *                  UDUNITS2_XML_PATH is returned if set; otherwise, the
 *                  compile-time pathname of the installed, default, unit
 *                  database is returned.
 */
EXTERNL const char*
ut_get_path_xml(
	const char*	path,
	ut_status*  status);

/*
 * Returns the unit-system corresponding to an XML file.  This is the usual way
 * that a client will obtain a unit-system.
 *
 * Arguments:
 *	path	The pathname of the XML file or NULL.  If NULL, then the
 *		pathname specified by the environment variable UDUNITS2_XML_PATH
 *		is used if set; otherwise, the compile-time pathname of the
 *		installed, default, unit database is used.
 * Returns:
 *	NULL	Failure.  "ut_get_status()" will be
 *		    UT_OPEN_ARG		"path" is non-NULL but file couldn't be
 *					opened.  See "errno" for reason.
 *		    UT_OPEN_ENV		"path" is NULL and environment variable
 *					UDUNITS2_XML_PATH is set but file
 *					couldn't be opened.  See "errno" for
 *					reason.
 *		    UT_OPEN_DEFAULT	"path" is NULL, environment variable
 *					UDUNITS2_XML_PATH is unset, and the
 *					installed, default, unit database
 *					couldn't be opened.  See "errno" for
 *					reason.
 *		    UT_PARSE		Couldn't parse unit database.
 *		    UT_OS		Operating-system error.  See "errno".
 *	else	Pointer to the unit-system defined by "path".
 */
EXTERNL ut_system*
ut_read_xml(
    const char*	path);


/*
 * Returns a new unit-system.  On success, the unit-system will only contain
 * the dimensionless unit one.  See "ut_get_dimensionless_unit_one()".
 *
 * Returns:
 *	NULL	Failure.  "ut_get_status()" will be:
 *		    UT_OS	Operating-system error.  See "errno".
 *	else	Pointer to a new unit system.
 */
EXTERNL ut_system*
ut_new_system(void);


/*
 * Frees a unit-system.  All unit-to-identifier and identifier-to-unit mappings
 * will be removed.
 *
 * Arguments:
 *	system		Pointer to the unit-system to be freed.  Use of "system"
 *			upon return results in undefined behavior.
 */
EXTERNL void
ut_free_system(
    ut_system*	system);


/*
 * Returns the unit-system to which a unit belongs.
 *
 * Arguments:
 *	unit	Pointer to the unit in question.
 * Returns:
 *	NULL	Failure.  "ut_get_status()" will be
 *		    UT_BAD_ARG	"unit" is NULL.
 *	else	Pointer to the unit-system to which "unit" belongs.
 */
EXTERNL ut_system*
ut_get_system(
    const ut_unit* const	unit);


/*
 * Returns the dimensionless-unit one of a unit-system.
 *
 * Arguments:
 *	system	Pointer to the unit-system for which the dimensionless-unit one
 *		will be returned.
 * Returns:
 *	NULL	Failure.  "ut_get_status()" will be:
 *		    UT_BAD_ARG	"system" is NULL.
 *	else	Pointer to the dimensionless-unit one associated with "system".
 *		While not necessary, the pointer may be passed to ut_free()
 *		when the unit is no longer needed by the client.
 */
EXTERNL ut_unit*
ut_get_dimensionless_unit_one(
    const ut_system* const	system);


/*
 * Returns the unit with a given name from a unit-system.  Name comparisons
 * are case-insensitive.
 *
 * Arguments:
 *	system	Pointer to the unit-system.
 *	name	Pointer to the name of the unit to be returned.
 * Returns:
 *	NULL	Failure.  "ut_get_status()" will be
 *		    UT_SUCCESS		"name" doesn't map to a unit of
 *					"system".
 *		    UT_BAD_ARG		"system" or "name" is NULL.
 *	else	Pointer to the unit of the unit-system with the given name.
 *		The pointer should be passed to ut_free() when the unit is
 *		no longer needed.
 */
EXTERNL ut_unit*
ut_get_unit_by_name(
    const ut_system* const	system,
    const char* const		name);


/*
 * Returns the unit with a given symbol from a unit-system.  Symbol 
 * comparisons are case-sensitive.
 *
 * Arguments:
 *	system		Pointer to the unit-system.
 *	symbol		Pointer to the symbol associated with the unit to be
 *			returned.
 * Returns:
 *	NULL	Failure.  "ut_get_status()" will be
 *		    UT_SUCCESS		"symbol" doesn't map to a unit of
 *					"system".
 *		    UT_BAD_ARG		"system" or "symbol" is NULL.
 *	else	Pointer to the unit in the unit-system with the given symbol.
 *		The pointer should be passed to ut_free() when the unit is no
 *		longer needed.
 */
EXTERNL ut_unit*
ut_get_unit_by_symbol(
    const ut_system* const	system,
    const char* const		symbol);


/*
 * Sets the "second" unit of a unit-system.  This function must be called before
 * the first call to "ut_offset_by_time()". ut_read_xml() calls this function if the
 * resulting unit-system contains a unit named "second".
 *
 * Arguments:
 *	second		Pointer to the "second" unit.
 * Returns:
 *	UT_BAD_ARG	"second" is NULL.
 *	UT_EXISTS	The second unit of the unit-system to which "second"
 *			belongs is set to a different unit.
 *	UT_SUCCESS	Success.
 */
EXTERNL ut_status
ut_set_second(
    const ut_unit* const	second);


/******************************************************************************
 * Defining Unit Prefixes:
 ******************************************************************************/


/*
 * Adds a name-prefix to a unit-system.  A name-prefix is something like "mega"
 * or "milli".  Comparisons between name-prefixes are case-insensitive.
 *
 * Arguments:
 *	system		Pointer to the unit-system.
 *	name		Pointer to the name-prefix (e.g., "mega").  May be freed
 *			upon return.
 *	value		The value of the prefix (e.g., 1e6).
 * Returns:
 *	UT_SUCCESS	Success.
 *	UT_BAD_ARG	"system" or "name" is NULL, or "value" is 0.
 *	UT_EXISTS	"name" already maps to a different value.
 *	UT_OS		Operating-system failure.  See "errno".
 */
EXTERNL ut_status
ut_add_name_prefix(
    ut_system* const	system,
    const char* const	name,
    const double	value);


/*
 * Adds a symbol-prefix to a unit-system.  A symbol-prefix is something like
 * "M" or "y".  Comparisons between symbol-prefixes are case-sensitive.
 *
 * Arguments:
 *	system		Pointer to the unit-system.
 *	symbol		Pointer to the symbol-prefix (e.g., "M").  May be freed
 *			upon return.
 *	value		The value of the prefix (e.g., 1e6).
 * Returns:
 *	UT_SUCCESS	Success.
 *	UT_BADSYSTEM	"system" or "symbol" is NULL.
 *	UT_BAD_ARG	"value" is 0.
 *	UT_EXISTS	"symbol" already maps to a different value.
 *	UT_OS		Operating-system failure.  See "errno".
 */
EXTERNL ut_status
ut_add_symbol_prefix(
    ut_system* const	system,
    const char* const	symbol,
    const double	value);


/******************************************************************************
 * Defining and Deleting Units:
 ******************************************************************************/


/*
 * Adds a base-unit to a unit-system.  Clients that use ut_read_xml() should not
 * normally need to call this function.
 *
 * Arguments:
 *	system	Pointer to the unit-system to which to add the new base-unit.
 * Returns:
 *	NULL	Failure.  "ut_get_status()" will be
 *		    UT_BAD_ARG		"system" or "name" is NULL.
 *		    UT_OS		Operating-system error.  See "errno".
 *	else	Pointer to the new base-unit.  The pointer should be passed to
 *		ut_free() when the unit is no longer needed by the client (the
 *		unit will remain in the unit-system).
 */
EXTERNL ut_unit*
ut_new_base_unit(
    ut_system* const	system);


/*
 * Adds a dimensionless-unit to a unit-system.  In the SI system of units, the
 * derived-unit radian is a dimensionless-unit.  Clients that use ut_read_xml()
 * should not normally need to call this function.
 *
 * Arguments:
 *	system	Pointer to the unit-system to which to add the new
 *		dimensionless-unit.
 * Returns:
 *	NULL	Failure.  "ut_get_status()" will be
 *		    UT_BAD_ARG		"system" is NULL.
 *		    UT_OS		Operating-system error.  See "errno".
 *	else	Pointer to the new dimensionless-unit.  The pointer should be
 *		passed to ut_free() when the unit is no longer needed by the
 *		client (the unit will remain in the unit-system).
 */
EXTERNL ut_unit*
ut_new_dimensionless_unit(
    ut_system* const	system);


/*
 * Returns a clone of a unit.
 *
 * Arguments:
 *	unit	Pointer to the unit to be cloned.
 * Returns:
 *	NULL	Failure.  ut_get_status() will be
 *		    UT_OS	Operating-system failure.  See "errno".
 *		    UT_BAD_ARG	"unit" is NULL.
 *	else	Pointer to the clone of "unit".  The pointer should be
 *		passed to ut_free() when the unit is no longer needed by the
 *		client.
 */
EXTERNL ut_unit*
ut_clone(
    const ut_unit*	unit);


/*
 * Frees resources associated with a unit.  This function should be invoked on
 * all units that are no longer needed by the client.  Use of the unit upon
 * return from this function will result in undefined behavior.
 *
 * Arguments:
 *	unit	Pointer to the unit to have its resources freed or NULL.
 */
EXTERNL void
ut_free(
    ut_unit* const	unit);


/******************************************************************************
 * Mapping between Units and Names:
 ******************************************************************************/


/*
 * Returns the name in a given encoding to which a unit maps.
 *
 * Arguments:
 *	unit		Pointer to the unit whose name should be returned.
 *	encoding	The desired encoding of the name.
 * Returns:
 *	NULL		Failure.  "ut_get_status()" will be
 *			    UT_BAD_ARG		"unit" is NULL.
 *			    UT_SUCCESS		"unit" doesn't map to a name in
 *						in the given encoding.
 *	else		Pointer to the name in the given encoding to which
 *			"unit" maps.
 */
EXTERNL const char*
ut_get_name(
    const ut_unit* const	unit,
    const ut_encoding		encoding);


/*
 * Adds a mapping from a name to a unit.
 *
 * Arguments:
 *	name		Pointer to the name to be mapped to "unit".  May be
 *			freed upon return.
 *      encoding        The character encoding of "name".
 *	unit		Pointer to the unit to be mapped-to by "name".  May be
 *			freed upon return.
 * Returns:
 *	UT_BAD_ARG	"name" or "unit" is NULL.
 *	UT_OS		Operating-system error.  See "errno".
 *	UT_EXISTS	"name" already maps to a different unit.
 *	UT_SUCCESS	Success.
 */
EXTERNL ut_status
ut_map_name_to_unit(
    const char* const		name,
    const ut_encoding		encoding,
    const ut_unit* const	unit);


/*
 * Removes a mapping from a name to a unit.  After this function,
 * ut_get_unit_by_name(system,name) will no longer return a unit.
 *
 * Arguments:
 *	system		The unit-system to which the unit belongs.
 *	name		The name of the unit.
 *      encoding        The character encoding of "name".
 * Returns:
 *	UT_SUCCESS	Success.
 *	UT_BAD_ARG	"system" or "name" is NULL.
 */
EXTERNL ut_status
ut_unmap_name_to_unit(
    ut_system*		system,
    const char* const	name,
    const ut_encoding   encoding);


/*
 * Adds a mapping from a unit to a name.
 *
 * Arguments:
 *	unit		Pointer to the unit to be mapped to "name".  May be
 *			freed upon return.
 *	name		Pointer to the name to be mapped-to by "unit".  May be
 *			freed upon return.
 *	encoding	The encoding of "name".
 * Returns:
 *	UT_SUCCESS	Success.
 *	UT_BAD_ARG	"unit" or "name" is NULL, or "name" is not in the
 *                      specified encoding.
 *	UT_OS		Operating-system error.  See "errno".
 *	UT_EXISTS	"unit" already maps to a name.
 */
EXTERNL ut_status
ut_map_unit_to_name(
    const ut_unit* const	unit,
    const char* const		name,
    ut_encoding			encoding);


/*
 * Removes a mapping from a unit to a name.
 *
 * Arguments:
 *	unit		Pointer to the unit.  May be freed upon return.
 *	encoding	The encoding to be removed.  No other encodings will be
 *			removed.
 * Returns:
 *	UT_BAD_ARG	"unit" is NULL.
 *	UT_SUCCESS	Success.
 */
EXTERNL ut_status
ut_unmap_unit_to_name(
    const ut_unit* const	unit,
    ut_encoding			encoding);


/******************************************************************************
 * Mapping between Units and Symbols:
 ******************************************************************************/


/*
 * Returns the symbol in a given encoding to which a unit maps.
 *
 * Arguments:
 *	unit		Pointer to the unit whose symbol should be returned.
 *	encoding	The desired encoding of the symbol.
 * Returns:
 *	NULL		Failure.  "ut_get_status()" will be
 *			    UT_BAD_ARG		"unit" is NULL.
 *			    UT_SUCCESS		"unit" doesn't map to a symbol
 *						in the given encoding.
 *	else		Pointer to the symbol in the given encoding to which
 *			"unit" maps.
 */
EXTERNL const char*
ut_get_symbol(
    const ut_unit* const	unit,
    const ut_encoding	encoding);


/*
 * Adds a mapping from a symbol to a unit.
 *
 * Arguments:
 *	symbol		Pointer to the symbol to be mapped to "unit".  May be
 *			freed upon return.
 *      ut_encoding     The character encoding of "symbol".
 *	unit		Pointer to the unit to be mapped-to by "symbol".  May
 *			be freed upon return.
 * Returns:
 *	UT_BAD_ARG	"symbol" or "unit" is NULL.
 *	UT_OS		Operating-system error.  See "errno".
 *	UT_EXISTS	"symbol" already maps to a different unit.
 *	UT_SUCCESS	Success.
 */
EXTERNL ut_status
ut_map_symbol_to_unit(
    const char* const		symbol,
    const ut_encoding		encoding,
    const ut_unit* const	unit);


/*
 * Removes a mapping from a symbol to a unit.  After this function,
 * ut_get_unit_by_symbol(system,symbol) will no longer return a unit.
 *
 * Arguments:
 *	system		The unit-system to which the unit belongs.
 *	symbol		The symbol of the unit.
 *      encoding        The character encoding of "symbol".
 * Returns:
 *	UT_SUCCESS	Success.
 *	UT_BAD_ARG	"system" or "symbol" is NULL.
 */
EXTERNL ut_status
ut_unmap_symbol_to_unit(
    ut_system*		system,
    const char* const	symbol,
    const ut_encoding   encoding);


/*
 * Adds a mapping from a unit to a symbol.
 *
 * Arguments:
 *	unit		Pointer to the unit to be mapped to "symbol".  May be
 *			freed upon return.
 *	symbol		Pointer to the symbol to be mapped-to by "unit".  May
 *			be freed upon return.
 *	encoding	The encoding of "symbol".
 * Returns:
 *	UT_SUCCESS	Success.
 *	UT_BAD_ARG	"unit" or "symbol" is NULL.
 *	UT_OS		Operating-system error.  See "errno".
 *	UT_EXISTS	"unit" already maps to a symbol.
 */
EXTERNL ut_status
ut_map_unit_to_symbol(
    const ut_unit*		unit,
    const char* const		symbol,
    ut_encoding			encoding);


/*
 * Removes a mapping from a unit to a symbol.
 *
 * Arguments:
 *	unit		Pointer to the unit to be unmapped to a symbol.  May be
 *			freed upon return.
 *	encoding	The encoding to be removed.  The mappings for "unit" in
 *			other encodings will not be removed.
 * Returns:
 *	UT_SUCCESS	Success.
 *	UT_BAD_ARG	"unit" is NULL.
 */
EXTERNL ut_status
ut_unmap_unit_to_symbol(
    const ut_unit* const	unit,
    ut_encoding			encoding);


/******************************************************************************
 * Getting Information about a Unit:
 ******************************************************************************/


/*
 * Indicates if a given unit is dimensionless or not.  Note that logarithmic
 * units are dimensionless by definition.
 *
 * Arguments:
 *	unit	Pointer to the unit in question.
 * Returns:
 *	0	"unit" is dimensionfull or an error occurred.  "ut_get_status()"
 *		 will be
 *		    UT_BAD_ARG		"unit" is NULL.
 *		    UT_SUCCESS		"unit" is dimensionfull.
 *	else	"unit" is dimensionless.
 */
EXTERNL int
ut_is_dimensionless(
    const ut_unit* const	unit);


/*
 * Indicates if two units belong to the same unit-system.
 *
 * Arguments:
 *	unit1		Pointer to a unit.
 *	unit2		Pointer to another unit.
 * Returns:
 *	0		Failure or the units belong to different unit-systems.
 *			"ut_get_status()" will be
 *	    		    UT_BAD_ARG		"unit1" or "unit2" is NULL.
 *	    		    UT_SUCCESS		The units belong to different
 *						unit-systems.
 *	else		The units belong to the same unit-system.
 */
EXTERNL int
ut_same_system(
    const ut_unit* const	unit1,
    const ut_unit* const	unit2);


/*
 * Compares two units.  Returns a value less than, equal to, or greater than
 * zero as the first unit is considered less than, equal to, or greater than
 * the second unit, respectively.  Units from different unit-systems never
 * compare equal.
 *
 * Arguments:
 *	unit1		Pointer to a unit or NULL.
 *	unit2		Pointer to another unit or NULL.
 * Returns:
 *	<0	The first unit is less than the second unit.
 *	 0	The first and second units are equal or both units are NULL.
 *	>0	The first unit is greater than the second unit.
 */
EXTERNL int
ut_compare(
    const ut_unit* const	unit1,
    const ut_unit* const	unit2);


/*
 * Indicates if numeric values in one unit are convertible to numeric values in
 * another unit via "ut_get_converter()".  In making this determination, 
 * dimensionless units are ignored.
 *
 * Arguments:
 *	unit1		Pointer to a unit.
 *	unit2		Pointer to another unit.
 * Returns:
 *	0		Failure.  "ut_get_status()" will be
 *	    		    UT_BAD_ARG		"unit1" or "unit2" is NULL.
 *			    UT_NOT_SAME_SYSTEM	"unit1" and "unit2" belong to
 *						different unit-sytems.
 *			    UT_SUCCESS		Conversion between the units is
 *						not possible (e.g., "unit1" is
 *						"meter" and "unit2" is
 *						"kilogram").
 *	else	Numeric values can be converted between the units.
 */
EXTERNL int
ut_are_convertible(
    const ut_unit* const	unit1,
    const ut_unit* const	unit2);


/*
 * Returns a converter of numeric values in one unit to numeric values in
 * another unit.  The returned converter should be passed to cv_free() when it is
 * no longer needed by the client.
 *
 * NOTE:  Leap seconds are not taken into account when converting between
 * timestamp units.
 *
 * Arguments:
 *	from		Pointer to the unit from which to convert values.
 *	to		Pointer to the unit to which to convert values.
 * Returns:
 *	NULL		Failure.  "ut_get_status()" will be:
 *			    UT_BAD_ARG		"from" or "to" is NULL.
 *			    UT_NOT_SAME_SYSTEM	"from" and "to" belong to
 *						different unit-systems.
 *			    UT_MEANINGLESS	Conversion between the units is
 *						not possible.  See
 *						"ut_are_convertible()".
 *	else		Pointer to the appropriate converter.  The pointer
 *			should be passed to cv_free() when no longer needed by
 *			the client.
 */
EXTERNL cv_converter*
ut_get_converter(
    ut_unit* const	from,
    ut_unit* const	to);


/******************************************************************************
 * Arithmetic Unit Manipulation:
 ******************************************************************************/


/*
 * Returns a unit equivalent to another unit scaled by a numeric factor,
 * e.g.,
 *	const ut_unit*	meter = ...
 *	const ut_unit*	kilometer = ut_scale(1000, meter);
 *
 * Arguments:
 *	factor		The numeric scale factor.
 *	unit		Pointer to the unit to be scaled.
 * Returns:
 *	NULL		Failure.  "ut_get_status()" will be
 *			    UT_BAD_ARG  	"factor" is 0 or "unit" is NULL.
 *			    UT_OS		Operating-system error.  See
 *						"errno".
 *	else		Pointer to the resulting unit.  The pointer should be
 *			passed to ut_free() when the unit is no longer needed by
 *			the client.
 */
EXTERNL ut_unit*
ut_scale(
    const double		factor,
    const ut_unit* const	unit);


/*
 * Returns a unit equivalent to another unit offset by a numeric amount,
 * e.g.,
 *	const ut_unit*	kelvin = ...
 *	const ut_unit*	celsius = ut_offset(kelvin, 273.15);
 *
 * Arguments:
 *	unit		Pointer to the unit to be offset.
 *	offset		The numeric offset.
 * Returns:
 *	NULL		Failure.  "ut_get_status()" will be
 *			    UT_BAD_ARG		"unit" is NULL.
 *			    UT_OS		Operating-system error.  See
 *						"errno".
 *	else		Pointer to the resulting unit.  The pointer should be
 *			passed to ut_free() when the unit is no longer needed by
 *			the client.
 */
EXTERNL ut_unit*
ut_offset(
    const ut_unit* const	unit,
    const double	offset);


/*
 * Returns a unit equivalent to another unit relative to a particular time.
 * e.g.,
 *	const ut_unit*	second = ...
 *	const ut_unit*	secondsSinceTheEpoch =
 *              ut_offset_by_time(second, ut_encode_time(1970, 1, 1, 0, 0, 0.0));
 *
 * Arguments:
 *	unit	Pointer to the time-unit to be made relative to a time-origin.
 *	origin	The origin as returned by ut_encode_time().
 * Returns:
 *	NULL	Failure.  "ut_get_status()" will be
 *		    UT_BAD_ARG		"unit" is NULL.
 *		    UT_OS		Operating-system error.  See "errno".
 *		    UT_MEANINGLESS	Creation of a timestamp unit based on
 *					"unit" is not meaningful.
 *		    UT_NO_SECOND	The associated unit-system doesn't
 *					contain a "second" unit.  See
 *					ut_set_second().
 *	else	Pointer to the resulting unit.  The pointer should be passed
 *		to ut_free() when the unit is no longer needed by the client.
 */
EXTERNL ut_unit*
ut_offset_by_time(
    const ut_unit* const	unit,
    const double	origin);


/*
 * Returns the result of multiplying one unit by another unit.
 *
 * Arguments:
 *	unit1	Pointer to a unit.
 *	unit2	Pointer to another unit.
 * Returns:
 *	NULL	Failure.  "ut_get_status()" will be:
 *		    UT_BAD_ARG		"unit1" or "unit2" is NULL.
 *		    UT_NOT_SAME_SYSTEM	"unit1" and "unit2" belong to
 *					different unit-systems.
 *		    UT_OS		Operating-system error. See "errno".
 *	else	Pointer to the resulting unit.  The pointer should be passed
 *		to ut_free() when the unit is no longer needed by the client.
 */
EXTERNL ut_unit*
ut_multiply(
    const ut_unit* const	unit1,
    const ut_unit* const	unit2);


/*
 * Returns the inverse (i.e., reciprocal) of a unit.  This convenience function
 * is equal to "ut_raise(unit, -1)".
 *
 * Arguments:
 *	unit	Pointer to the unit.
 * Returns:
 *	NULL	Failure.  "ut_get_status()" will be:
 *		    UT_BAD_ARG		"unit" is NULL.
 *		    UT_OS		Operating-system error. See "errno".
 *	else	Pointer to the resulting unit.  The pointer should be passed to
 *		ut_free() when the unit is no longer needed by the client.
 */
EXTERNL ut_unit*
ut_invert(
    const ut_unit* const	unit);


/*
 * Returns the result of dividing one unit by another unit.  This convenience
 * function is equivalent to the following sequence:
 *     {
 *         ut_unit* inverse = ut_invert(denom);
 *         ut_multiply(numer, inverse);
 *         ut_free(inverse);
 *     }
 *
 * Arguments:
 *	numer	Pointer to the numerator (top, dividend) unit.
 *	denom	Pointer to the denominator (bottom, divisor) unit.
 * Returns:
 *	NULL	Failure.  "ut_get_status()" will be:
 *		    UT_BAD_ARG		"numer" or "denom" is NULL.
 *		    UT_NOT_SAME_SYSTEM	"unit1" and "unit2" belong to
 *					different unit-systems.
 *		    UT_OS		Operating-system error. See "errno".
 *	else	Pointer to the resulting unit.  The pointer should be passed to
 *		ut_free() when the unit is no longer needed by the client.
 */
EXTERNL ut_unit*
ut_divide(
    const ut_unit* const	numer,
    const ut_unit* const	denom);


/*
 * Returns the result of raising a unit to a power.
 *
 * Arguments:
 *	unit	Pointer to the unit.
 *	power	The power by which to raise "unit".  Must be greater than or 
 *		equal to -255 and less than or equal to 255.
 * Returns:
 *	NULL	Failure.  "ut_get_status()" will be:
 *		    UT_BAD_ARG		"unit" is NULL or "power" is invalid.
 *		    UT_OS		Operating-system error. See "errno".
 *	else	Pointer to the resulting unit.  The pointer should be passed to
 *		ut_free() when the unit is no longer needed by the client.
 */
EXTERNL ut_unit*
ut_raise(
    const ut_unit* const	unit,
    const int			power);


/*
 * Returns the result of taking the root of a unit.
 *
 * Arguments:
 *	unit	Pointer to the unit.
 *	root	The root to take of "unit".  Must be greater than or 
 *		equal to 1 and less than or equal to 255.
 * Returns:
 *	NULL	Failure.  "ut_get_status()" will be:
 *		    UT_BAD_ARG		"unit" is NULL, or "root" is invalid.
 *		                        In particular, all powers of base units
 *		                        in "unit" must be integral multiples of
 *		                        "root".
 *		    UT_OS		Operating-system error. See "errno".
 *	else	Pointer to the resulting unit.  The pointer should be passed to
 *		ut_free() when the unit is no longer needed by the client.
 */
EXTERNL ut_unit*
ut_root(
    const ut_unit* const	unit,
    const int			root);


/*
 * Returns the logarithmic unit corresponding to a logarithmic base and a
 * reference level.  For example, the following creates a decibel unit with a
 * one milliwatt reference level:
 *
 *     const ut_unit* watt = ...;
 *     const ut_unit* milliWatt = ut_scale(0.001, watt);
 *
 *     if (milliWatt != NULL) {
 *         const ut_unit* bel_1_mW = ut_log(10.0, milliWatt);
 *
 *         if (bel_1_mW != NULL) {
 *             const ut_unit* decibel_1_mW = ut_scale(0.1, bel_1_mW);
 *
 *             if (decibel_1_mW != NULL) {
 *                 ...
 *                 ut_free(decibel_1_mW);
 *             }			// "decibel_1_mW" allocated
 *
 *             ut_free(bel_1_mW);
 *         }				// "bel_1_mW" allocated
 *
 *         ut_free(milliWatt);
 *     }				// "milliWatt" allocated
 *
 * Arguments:
 *	base		The logarithmic base (e.g., 2, M_E, 10).  Must be
 *                      greater than one.  "M_E" is defined in <math.h>.
 *	reference	Pointer to the reference value as a unit.
 * Returns:
 *	NULL		Failure.  "ut_get_status()" will be:
 *			    UT_BAD_ARG	        "base" is invalid or "reference"
 *                                              is NULL.
 *			    UT_OS		Operating-system error. See
 *						"errno".
 *	else		Pointer to the resulting unit.  The pointer should be
 *			passed to ut_free() when the unit is no longer needed by
 *			the client.
 */
EXTERNL ut_unit*
ut_log(
    const double		base,
    const ut_unit* const	reference);


/******************************************************************************
 * Parsing and Formatting Units:
 ******************************************************************************/


/*
 * Returns the binary representation of a unit corresponding to a string
 * representation.
 *
 * Arguments:
 *	system		Pointer to the unit-system in which the parsing will
 *			occur.
 *	string		The string to be parsed (e.g., "millimeters").  There
 *			should be no leading or trailing whitespace in the
 *			string.  See ut_trim().
 *	encoding	The encoding of "string".
 * Returns:
 *	NULL		Failure.  "ut_get_status()" will be one of
 *			    UT_BAD_ARG		"system" or "string" is NULL.
 *			    UT_SYNTAX		"string" contained a syntax
 *						error.
 *			    UT_UNKNOWN		"string" contained an unknown
 *						identifier.
 *			    UT_OS		Operating-system failure.  See
 *						"errno".
 *	else		Pointer to the unit corresponding to "string".
 */
EXTERNL ut_unit*
ut_parse(
    const ut_system* const	system,
    const char* const		string,
    const ut_encoding		encoding);


/*
 * Removes leading and trailing whitespace from a string.
 *
 * Arguments:
 *	string		NUL-terminated string.  Will be modified if it contains
 *                      whitespace..
 *	encoding	The character-encoding of "string".
 * Returns:
 *      "string", with all leading and trailing whitespace removed.
 */
EXTERNL char*
ut_trim(
    char* const	        string,
    const ut_encoding	encoding);


/*
 * Formats a unit.
 *
 * Arguments:
 *	unit		Pointer to the unit to be formatted.
 *	buf		Pointer to the buffer into which to format "unit".
 *	size		Size of the buffer in bytes.
 *	opts		Formatting options: bitwise-OR of zero or more of the
 *			following:
 *			    UT_NAMES		Use unit names instead of
 *						symbols
 *                          UT_DEFINITION       The formatted string should be
 *                                              the definition of "unit" in
 *                                              terms of basic-units instead of
 *						stopping any expansion at the
 *						highest level possible.
 *			    UT_ASCII		The string should be formatted
 *						using the ASCII character set
 *						(default).
 *			    UT_LATIN1		The string should be formatted
 *						using the ISO Latin-1 (alias
 *						ISO-8859-1) character set.
 *			    UT_UTF8		The string should be formatted
 *						using the UTF-8 character set.
 *			UT_LATIN1 and UT_UTF8 are mutually exclusive: they may
 *			not both be specified.
 * Returns:
 *	-1		Failure:  "ut_get_status()" will be
 *			    UT_BAD_ARG		"unit" or "buf" is NULL, or both
 *                                              UT_LATIN1 and UT_UTF8 specified.
 *			    UT_CANT_FORMAT	"unit" can't be formatted in
 *						the desired manner.
 *      else		Success.  Number of characters printed in "buf".  If
 *			the number is equal to the size of the buffer, then the
 *			buffer is too small to have a terminating NUL character.
 */
EXTERNL int
ut_format(
    const ut_unit* const	unit,
    char*		buf,
    size_t		size,
    unsigned		opts);


/*
 * Accepts a visitor to a unit.
 *
 * Arguments:
 *	unit		Pointer to the unit to accept the visitor.
 *	visitor		Pointer to the visitor of "unit".
 *	arg		An arbitrary pointer that will be passed to "visitor".
 * Returns:
 *	UT_BAD_ARG	"unit" or "visitor" is NULL.
 *	UT_VISIT_ERROR	A error occurred in "visitor" while visiting "unit".
 *	UT_SUCCESS	Success.
 */
EXTERNL ut_status
ut_accept_visitor(
    const ut_unit* const		unit,
    const ut_visitor* const	visitor,
    void* const			arg);


/******************************************************************************
 * String List Public API Functions::
 ******************************************************************************/
typedef struct __ut_string_list ut_string_list;
/*
 * Free all the allocated memory for the string list. Must be called on
 * all ut_string_lists return by library functions.
 */
EXTERNL void
ut_string_list_free(
    ut_string_list* list);
/*
 * Return the number of elements in the list
 */
EXTERNL int
ut_string_list_length(
    const ut_string_list* const	list);
/*
 * Return the specified string from the list (zeor based)
 */
EXTERNL char*
ut_string_list_element(
    const ut_string_list* const	list,
    int				element);
/*
 * Returns a malloc'd string containing all the list entries
 * in the same order, with the separator string between each
 * entry except finalSeparator between the last two.
 *
 * If the list argument is NULL then NULL is returned.
 *
 * For the ut_string_list:
 *	(red, green, yellow, blue)
 * After:
 *   char *foo = ut_string_list_implode(list,", "," and ");
 * foo contains: "red, green, yellow and blue"
 *
 * After:
 *   char *foo = ut_string_list_implode(list,", ",NULL);
 * foo contains: "red, green, yellow, blue"
 *
 */
EXTERNL char*
ut_string_list_implode(
    const ut_string_list* const	list,
    const char* const		separator,
    const char* const		finalSeparator);

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
EXTERNL ut_string_list*
ut_string_explode(
    const char* const		string,
    const char* const		separator,
    const char* const		finalSeparator);


/******************************************************************************
 * Named Units System Handling:
 ******************************************************************************/

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
EXTERNL ut_status
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
EXTERNL ut_status
ut_map_name_to_named_system(
    ut_system* const	system,
    const char* const	new_name,	/* New name */
    const ut_encoding	encoding,
    const char* const	named_system);	/* Existing name */

/*
 * Tests if the system_name is defined for the unit-system.
 *
 * Arguments:
 *	system		Pointer to the unit-system to be checked
 *	system_name	Pointer to the string to check.
 *
 * This function is NULL and empty string tolerant.
 */
int
ut_named_system_exists_in_system(
    const ut_system* const	system,
    const char* const		system_name);

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
EXTERNL ut_string_list*
ut_get_named_systems(
    const ut_system* const system);

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
EXTERNL ut_string_list*
ut_get_named_system_aliases(
    const ut_system* const	system,
    const char* const		system_name);

/*
 *  Specifies that the unit passed is part of the named unit system
 *  whose name is passed as the second argument. For example, specifying
 *  that centimeters is part of the metric system would look like:
 *
 *    ut_unit *cm = ut_get_unit_by_name( system, "centimeter" );
 *    ud_add_unit_to_named_syste( cm, "metric system" );
 *
 *  Note that if the named unit system does not exist it is silently
 *  created. This might be an issue since an encoding is not set.
 *
 *  Returns:
 *	UT_BAD_ARG	If unit or system name are NULL or an emptry
 *			string is passed for the system_name.
 *	UT_UNKNOWN	If ut_add_unit_can_create_new_named_system()
 *			returns returns zero and the system_name
 *			has not be created with ut_add_named_system().
 *	UT_OS		For failures allocating space, etc.
 *	UT_SUCCESS	If successfully added.
 */
EXTERNL ut_status
ut_add_unit_to_named_system(
    const ut_unit* const	unit,
    const char* const		system_name);

/*
 *  Returns whether ut_add_unit_to_named_system() will automatically
 *  and silently create non-existent named unit systems or return
 *  UT_UNKNOWN.
 */
EXTERNL int
ut_add_unit_can_create_new_named_system();

/*
 *  Changes the status of whether ut_add_unit_to_named_system() will
 *  automatically and silently create non-existent named unit systems
 *  or return UT_UNKOWN. This function returns the previous state so
 *	int oldState = ut_set_add_unit_can_create_new_named_system(1);
 *	ut_set_add_unit_can_create_new_named_system(oldState);
 *  will make sure ut_add_unit_to_named_system() will create new
 *  named unit systems then return it to whatever its previous status
 *  was.
 */
EXTERNL int
ut_set_add_unit_can_create_new_named_system(
    int yesOrNo );

/*
 *  Specifies that the unit passed is not part of the named unit system
 *  whose name is passed as the second argument. For example, specifying
 *  that centimeters is not part of the US system would look like:
 *
 *    ut_unit *cm = ut_get_unit_by_name( system, "centimeter" );
 *    ut_remove_unit_from_named_system( cm, "US system" );
 *
 *  Note that if the named unit system does not exist or the unit
 *  is not part of the named unit system this function does nothing.
 *
 *  Returns:
 *	UT_BAD_ARG	If unit or system name are NULL or an emptry
 *			string is passed for the system_name.
 *	UT_OS		For failures allocating space, etc.
 *	UT_UNKNOWN	If the system_name does not exist.
 *	UT_SUCCESS	If successfully removed.
 */
EXTERNL ut_status
ut_remove_unit_from_named_system(
    const ut_unit* const	unit,
    const char* const		system_name);

/*
 * Tests whether the given unit is member of the named system.
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
EXTERNL int
ut_is_in_named_system(
    const ut_unit* const	unit,
    const char* const		system_name);

/*
 * Get the list of named unit systems that the given unit is a
 * member of. This gets just the primary names, that is one
 * string per named unit system..
 *
 * The returned string list is sorted alphabetically.
 *
 * Arguments:
 *    unit		The unit to get the system names for..
 * Returns:
 *    NULL		If unit argument was NULL
 *			ut_get_status() returns UT_BAD_ARG.
 *    ut_string_list*	Containing the list of names,
 *			which could be empty. ut_get_status()
 *			returns UT_SUCCESS.
 */
EXTERNL ut_string_list*
ut_get_named_systems_for_unit(
    const ut_unit* const	unit);

/*
 * Get the unit from the named unit system that the provided
 * unit can be converted to that is closest in magnitude to it.
 * For example, converting kilometers to the US system should
 * give miles not inches. Note that this compares the relative
 * scales of the units and does not know about values.
 *
 * Arguments:
 *	system_name	The desired or target named unit system
 *	unit		The unit to get the compatible unit for
 * Returns:
 *	NULL		If unit or system_name argument was NULL
 *			or if the system_name is an empty string
 *			(ut_get_status() returns UT_BAD_ARG).
 *			If the system_name passed does not exist
 *			for the system that the unit belongs to
 *			(ut_get_status() returns UT_UNKNOWN).
 *			If there is no unit in the target named
 *			unit system that the unit can be converted
 *			to (ut_get_status() returns UT_SUCCESS).
 *	ut_unit*	The compatible unit. This should be passed
 *			to ut_free() when no longer needed.
 *			ut_get_status() returns UT_SUCCESS.
 */
EXTERNL ut_unit*
ut_unit_from_named_system_convertible_with_unit(
    const char* const		system_name,
    const ut_unit* const	unit);


/******************************************************************************
 * Time Handling:
 ******************************************************************************/


/*
 * Encodes a date as a double-precision value.
 *
 * Arguments:
 *	year		The year.
 *	month		The month.
 *	day		The day (1 = the first of the month).
 * Returns:
 *	The date encoded as a scalar value.
 */
EXTERNL double
ut_encode_date(
    int		year,
    int		month,
    int		day);


/*
 * Encodes a time as a double-precision value.
 *
 * Arguments:
 *	hours		The number of hours (0 = midnight).
 *	minutes		The number of minutes.
 *	seconds		The number of seconds.
 * Returns:
 *	The clock-time encoded as a scalar value.
 */
EXTERNL double
ut_encode_clock(
    int		hours,
    int		minutes,
    double	seconds);


/*
 * Encodes a time as a double-precision value.  The convenience function is
 * equivalent to "ut_encode_date(year,month,day) + 
 * ut_encode_clock(hour,minute,second)"
 *
 * Arguments:
 *	year	The year.
 *	month	The month.
 *	day	The day.
 *	hour	The hour.
 *	minute	The minute.
 *	second	The second.
 * Returns:
 *	The time encoded as a scalar value.
 */
EXTERNL double
ut_encode_time(
    const int		year,
    const int		month,
    const int		day,
    const int		hour,
    const int		minute,
    const double	second);


/*
 * Decodes a time from a double-precision value.
 *
 * Arguments:
 *      value           The value to be decoded.
 *      year            Pointer to the variable to be set to the year.
 *      month           Pointer to the variable to be set to the month.
 *      day             Pointer to the variable to be set to the day.
 *      hour            Pointer to the variable to be set to the hour.
 *      minute          Pointer to the variable to be set to the minute.
 *      second          Pointer to the variable to be set to the second.
 *      resolution      Pointer to the variable to be set to the resolution
 *                      of the decoded time in seconds.
 */
EXTERNL void
ut_decode_time(
    double	value,
    int		*year,
    int		*month,
    int		*day,
    int		*hour,
    int		*minute,
    double	*second,
    double	*resolution);


/******************************************************************************
 * Error Handling:
 ******************************************************************************/


/*
 * Returns the status of the last operation by the units module.  This function
 * will not change the status.
 */
EXTERNL ut_status
ut_get_status(void);


/*
 * Sets the status of the units module.  This function would not normally be
 * called by the user unless they were doing their own parsing or formatting.
 *
 * Arguments:
 *	status	The status of the units module.
 */
EXTERNL void
ut_set_status(
    ut_status	status);


/*
 * Handles an error-message.
 *
 * Arguments:
 *	fmt	The format for the error-message.
 *	...	The arguments for "fmt".
 * Returns:
 *	<0	An output error was encountered.
 *	else	The number of bytes of "fmt" and "arg" written excluding any
 *		terminating NUL.
 */
EXTERNL int
ut_handle_error_message(
    const char* const	fmt,
    ...);


/*
 * Returns the previously-installed error-message handler and optionally
 * installs a new handler.  The initial handler is "ut_write_to_stderr()".
 *
 * Arguments:
 *      handler		NULL or pointer to the error-message handler.  If NULL,
 *			then the handler is not changed.  The 
 *			currently-installed handler can be obtained this way.
 * Returns:
 *	Pointer to the previously-installed error-message handler.
 */
EXTERNL ut_error_message_handler
ut_set_error_message_handler(
    ut_error_message_handler	handler);


/*
 * Writes an error-message to the standard-error stream when received and
 * appends a newline.  This is the initial error-message handler.
 *
 * Arguments:
 *	fmt	The format for the error-message.
 *	args	The arguments of "fmt".
 * Returns:
 *	<0	A output error was encountered.  See "errno".
 *	else	The number of bytes of "fmt" and "arg" written excluding any
 *		terminating NUL.
 */
EXTERNL int
ut_write_to_stderr(
    const char* const	fmt,
    va_list		args);


/*
 * Does nothing with an error-message.
 *
 * Arguments:
 *	fmt	The format for the error-message.
 *	args	The arguments of "fmt".
 * Returns:
 *	0	Always.
 */
EXTERNL int
ut_ignore(
    const char* const	fmt,
    va_list		args);


#ifdef __cplusplus
}
#endif

#endif
