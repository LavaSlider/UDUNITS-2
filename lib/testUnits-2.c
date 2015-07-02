/*
 * Copyright 2015 David W. Stockton
 *
 * This file was added for testing the named units systems.
 *
 * This file is part of the UDUNITS-2 package.  See the file COPYRIGHT
 * in the top-level source-directory of the package for copying and
 * redistribution conditions.
 */
#ifndef	_XOPEN_SOURCE
#   define _XOPEN_SOURCE 500
#endif


#include <float.h>
#include <glob.h>
#include <math.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>

#include "udunits2.h"
#include "named_system.h"
int utFindNamedSystemIndex( ut_system* const system, const char* const string );
void namedSystemFreeSystem( ut_system*	system );

static const char*  xmlPath;
static ut_system*	unitSystem;
static ut_unit*		kilogram;
static ut_unit*		meter;
static ut_unit*		radian;
static ut_unit*		kelvin;
static ut_unit*		second;
static ut_unit*		minute;
static ut_unit*		kilometer;
static ut_unit*		micron;
static ut_unit*		rankine;
static ut_unit*		celsius;
static ut_unit*		fahrenheit;
static ut_unit*		meterPerSecondSquared;
static ut_unit*		meterSquaredPerSecondSquared;
static ut_unit*		joulePerKilogram;
static ut_unit*		watt;
static ut_unit*		wattSquared;
static ut_unit*		cubicMeter;
static ut_unit*		cubicMicron;
static ut_unit*		BZ;
static ut_unit*		dBZ;
static ut_unit*		secondsSinceTheEpoch;
static ut_unit*		minutesSinceTheMillenium;
static ut_unit*		hertz;
static ut_unit*		megahertz;

static unsigned		asciiName = UT_ASCII | UT_NAMES;
static unsigned		asciiNameDef = UT_ASCII | UT_NAMES | UT_DEFINITION;
static unsigned		asciiSymbolDef = UT_ASCII | UT_DEFINITION;
static unsigned		latin1SymbolDef = UT_LATIN1 | UT_DEFINITION;
static unsigned		utf8SymbolDef = UT_UTF8 | UT_DEFINITION;


/*
 * Only called once.
 */
static int
setup(
    void)
{
	return ((unitSystem = ut_new_system()) == NULL)
        ? -1
        : 0;
}


/*
 * Only called once.
 */
static int
teardown(
    void)
{
    ut_free_system(unitSystem);

    return 0;
}


static void
test_named_system(void)
{
    ut_system*	system = ut_new_system();

    CU_ASSERT_PTR_NOT_NULL(system);
    CU_ASSERT_EQUAL(ut_add_named_system(system, "SI", UT_ASCII), UT_SUCCESS);
    CU_ASSERT_EQUAL(ut_add_named_system(system, "SI", UT_ASCII), UT_SUCCESS);
    CU_ASSERT_EQUAL(ut_add_named_system(system, "si", UT_ASCII), UT_SUCCESS);
    CU_ASSERT_EQUAL(ut_add_named_system(system, "Metric", UT_ASCII), UT_SUCCESS);
    CU_ASSERT_EQUAL(ut_add_named_system(NULL, "Metric", UT_ASCII), UT_BAD_ARG);
    CU_ASSERT_EQUAL(ut_add_named_system(system, NULL, UT_ASCII), UT_BAD_ARG);

    CU_ASSERT_EQUAL(utFindNamedSystemIndex(system, "Metric"), 1);
    CU_ASSERT_EQUAL(ut_get_status(), UT_SUCCESS);
    CU_ASSERT_EQUAL(utFindNamedSystemIndex(system, "SI"), 0);
    CU_ASSERT_EQUAL(ut_get_status(), UT_SUCCESS);
    CU_ASSERT_EQUAL(utFindNamedSystemIndex(system, "Metric"), 1);
    CU_ASSERT_EQUAL(ut_get_status(), UT_SUCCESS);
    CU_ASSERT_EQUAL(utFindNamedSystemIndex(NULL, "Metric"), -1);
    CU_ASSERT_EQUAL(ut_get_status(), UT_BAD_ARG);
    CU_ASSERT_EQUAL(utFindNamedSystemIndex(system, NULL), -1);
    CU_ASSERT_EQUAL(ut_get_status(), UT_BAD_ARG);
    CU_ASSERT_EQUAL(utFindNamedSystemIndex(system, "NoSuchSystem"), -1);
    CU_ASSERT_EQUAL(ut_get_status(), UT_UNKNOWN);

    CU_ASSERT_EQUAL(ut_map_name_to_named_system(system, "International System", UT_ASCII, "SI"), UT_SUCCESS);
    CU_ASSERT_EQUAL(utFindNamedSystemIndex(system, "SI"), 0);
    CU_ASSERT_EQUAL(ut_get_status(), UT_SUCCESS);
    CU_ASSERT_EQUAL(utFindNamedSystemIndex(system, "International System"), 0);
    CU_ASSERT_EQUAL(ut_get_status(), UT_SUCCESS);
    CU_ASSERT_EQUAL(ut_map_name_to_named_system(system, "International System", UT_ASCII, "SI"), UT_SUCCESS);
    CU_ASSERT_EQUAL(utFindNamedSystemIndex(system, "SI"), 0);
    CU_ASSERT_EQUAL(ut_get_status(), UT_SUCCESS);
    CU_ASSERT_EQUAL(utFindNamedSystemIndex(system, "International System"), 0);
    CU_ASSERT_EQUAL(ut_get_status(), UT_SUCCESS);
    CU_ASSERT_EQUAL(ut_map_name_to_named_system(system, "SI", UT_ASCII, "SI"), UT_SUCCESS);
    CU_ASSERT_EQUAL(utFindNamedSystemIndex(system, "SI"), 0);
    CU_ASSERT_EQUAL(ut_get_status(), UT_SUCCESS);
    CU_ASSERT_EQUAL(utFindNamedSystemIndex(system, "International System"), 0);
    CU_ASSERT_EQUAL(ut_get_status(), UT_SUCCESS);
    CU_ASSERT_EQUAL(utFindNamedSystemIndex(system, "Metric"), 1);
    CU_ASSERT_EQUAL(ut_get_status(), UT_SUCCESS);
    CU_ASSERT_EQUAL(ut_add_named_system(system, "US", UT_ASCII), UT_SUCCESS);
    CU_ASSERT_EQUAL(utFindNamedSystemIndex(system, "US"), 2);
    CU_ASSERT_EQUAL(ut_get_status(), UT_SUCCESS);
    CU_ASSERT_EQUAL(ut_map_name_to_named_system(system, "US Common System", UT_ASCII, "US"), UT_SUCCESS);
    CU_ASSERT_EQUAL(utFindNamedSystemIndex(system, "SI"), 0);
    CU_ASSERT_EQUAL(ut_get_status(), UT_SUCCESS);
    CU_ASSERT_EQUAL(utFindNamedSystemIndex(system, "us common system"), 2);
    CU_ASSERT_EQUAL(ut_get_status(), UT_SUCCESS);
    CU_ASSERT_EQUAL(ut_map_name_to_named_system(NULL, "International System", UT_ASCII, "SI"), UT_BAD_ARG);
    CU_ASSERT_EQUAL(ut_map_name_to_named_system(system, NULL, UT_ASCII, "SI"), UT_BAD_ARG);
    CU_ASSERT_EQUAL(ut_map_name_to_named_system(system, "International System", UT_ASCII, NULL), UT_BAD_ARG);
    CU_ASSERT_EQUAL(ut_map_name_to_named_system(system, "US", UT_ASCII, "SI"), UT_EXISTS);
    CU_ASSERT_EQUAL(ut_map_name_to_named_system(system, "SI", UT_ASCII, "US"), UT_EXISTS);
    CU_ASSERT_EQUAL(ut_map_name_to_named_system(system, "SI", UT_ASCII, "NoSuchSystem"), UT_UNKNOWN);
    CU_ASSERT_EQUAL(ut_map_name_to_named_system(system, "Jiberish", UT_ASCII, "NoSuchSystem"), UT_UNKNOWN);

    // Using the unit system setup by setup() make sure there is independence
    CU_ASSERT_PTR_NOT_NULL(unitSystem);
    CU_ASSERT_EQUAL(ut_add_named_system(unitSystem, "British", UT_ASCII), UT_SUCCESS);
    CU_ASSERT_EQUAL(utFindNamedSystemIndex(unitSystem, "British"), 0);
    CU_ASSERT_EQUAL(ut_get_status(), UT_SUCCESS);
    CU_ASSERT_EQUAL(ut_map_name_to_named_system(NULL, "International System", UT_ASCII, "SI"), UT_BAD_ARG);
    CU_ASSERT_EQUAL(ut_map_name_to_named_system(unitSystem, "International System", UT_ASCII, "SI"), UT_UNKNOWN);
    CU_ASSERT_EQUAL(ut_map_name_to_named_system(unitSystem, "Limey System", UT_ASCII, "British"), UT_SUCCESS);
    CU_ASSERT_EQUAL(utFindNamedSystemIndex(unitSystem, "British"), 0);
    CU_ASSERT_EQUAL(utFindNamedSystemIndex(unitSystem, "limey system"), 0);
    CU_ASSERT_EQUAL(ut_get_status(), UT_SUCCESS);

    namedSystemFreeSystem(system);	// This needs to be added to ut_free_system()
    ut_free_system(system);
}

int
main(
    const int		    argc,
    const char* const*	argv)
{
    int	exitCode = EXIT_FAILURE;

    xmlPath = argv[1]
            ? argv[1]
            : getenv("UDUNITS2_XML_PATH");

    if (CU_initialize_registry() == CUE_SUCCESS) {
	CU_Suite*	testSuite = CU_add_suite(__FILE__, setup, teardown);

	if (testSuite != NULL) {
	    CU_ADD_TEST(testSuite, test_named_system);
	    /*
	    */

	    ut_set_error_message_handler(ut_ignore);

	    if (CU_basic_run_tests() == CUE_SUCCESS) {
		if (CU_get_number_of_tests_failed() == 0)
		    exitCode = EXIT_SUCCESS;
	    }
	}

	CU_cleanup_registry();
    }

    return exitCode;
}
