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
#include "ut_lists.h"
#include "bitmap.h"

static const char*  xmlPath;
static ut_system*	unitSystem;

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
test_string_list(void)
{
    ut_string_list* list1 = NULL;
    char* str1 = "hello";
    char* str2 = "world";
    char* str3 = "goodbye";
    char* tmp;

    CU_ASSERT_PTR_NULL(ut_string_list_implode(NULL,"-","="));

    list1 = ut_string_list_new();
    CU_ASSERT_PTR_NOT_NULL(list1);
    CU_ASSERT_PTR_NOT_NULL((tmp = ut_string_list_implode(list1,"-","=")));
    free(tmp);
    ut_string_list_free(list1);

    ut_string_list_free(NULL);

    list1 = ut_string_list_new_with_capacity( -3 );
    CU_ASSERT_PTR_NOT_NULL(list1);
    ut_string_list_free(list1);
    list1 = ut_string_list_new_with_capacity( 0 );
    CU_ASSERT_PTR_NOT_NULL(list1);
    ut_string_list_free(list1);
    list1 = ut_string_list_new_with_capacity( 10 );
    CU_ASSERT_PTR_NOT_NULL(list1);
    ut_string_list_free(list1);

    list1 = ut_string_list_new();
    CU_ASSERT_PTR_NOT_NULL(list1);
    CU_ASSERT_EQUAL(ut_string_list_length(NULL), 0);
    CU_ASSERT_EQUAL(ut_string_list_length(list1), 0);
    CU_ASSERT_PTR_NULL(ut_string_list_element(NULL,0));
    CU_ASSERT_PTR_NULL(ut_string_list_element(list1,-20));
    CU_ASSERT_PTR_NULL(ut_string_list_element(list1,0));
    CU_ASSERT_PTR_NULL(ut_string_list_element(list1,7));
    ut_string_list_add_element(NULL, NULL);
    ut_string_list_add_element(NULL, str1);
    ut_string_list_add_element(list1, str1);
    CU_ASSERT_EQUAL(ut_string_list_length(list1), 1);
    CU_ASSERT_NOT_EQUAL(ut_string_list_element(list1,0), str1);
    if (ut_string_list_element(list1,0))
	CU_ASSERT_STRING_EQUAL(ut_string_list_element(list1,0), str1);
    tmp = ut_string_list_implode(list1,NULL,NULL);
    if (tmp) {
	CU_ASSERT_STRING_EQUAL(tmp,str1);
	free(tmp);
    }
    tmp = ut_string_list_implode(list1,NULL,"=");
    if (tmp) {
	CU_ASSERT_STRING_EQUAL(tmp,str1);
	free(tmp);
    }
    tmp = ut_string_list_implode(list1,"-",NULL);
    if (tmp) {
	CU_ASSERT_STRING_EQUAL(tmp,str1);
	free(tmp);
    }
    tmp = ut_string_list_implode(list1,"-","=");
    if (tmp) {
	CU_ASSERT_STRING_EQUAL(tmp,str1);
	free(tmp);
    }
    ut_string_list_add_element(list1, NULL);
    CU_ASSERT_EQUAL(ut_string_list_length(list1), 2);
    CU_ASSERT_PTR_NULL(ut_string_list_element(list1,-20));
    CU_ASSERT_NOT_EQUAL(ut_string_list_element(list1,0), str1);
    if (ut_string_list_element(list1,0))
	CU_ASSERT_STRING_EQUAL(ut_string_list_element(list1,0), str1);
    CU_ASSERT_PTR_NULL(ut_string_list_element(list1,1));
    CU_ASSERT_PTR_NULL(ut_string_list_element(list1,2));
    CU_ASSERT_STRING_EQUAL((tmp = ut_string_list_implode(list1,NULL,NULL)),str1);
    free(tmp);
    CU_ASSERT_STRING_EQUAL((tmp = ut_string_list_implode(list1,NULL,"=")),"hello=");
    free(tmp);
    CU_ASSERT_STRING_EQUAL((tmp = ut_string_list_implode(list1,"-",NULL)),"hello-");
    free(tmp);
    CU_ASSERT_STRING_EQUAL((tmp = ut_string_list_implode(list1,"-","=")),"hello=");
    free(tmp);
    ut_string_list_add_element(list1, str2);
    CU_ASSERT_EQUAL(ut_string_list_length(list1), 3);
    CU_ASSERT_NOT_EQUAL(ut_string_list_element(list1,0), str1);
    if (ut_string_list_element(list1,0))
	CU_ASSERT_STRING_EQUAL(ut_string_list_element(list1,0), str1);
    CU_ASSERT_PTR_NULL(ut_string_list_element(list1,1));
    CU_ASSERT_NOT_EQUAL(ut_string_list_element(list1,2), str2);
    if (ut_string_list_element(list1,2))
	CU_ASSERT_STRING_EQUAL(ut_string_list_element(list1,2), str2);
    CU_ASSERT_PTR_NULL(ut_string_list_element(list1,3));
    CU_ASSERT_STRING_EQUAL((tmp = ut_string_list_implode(list1,NULL,NULL)),"helloworld");
    free(tmp);
    CU_ASSERT_STRING_EQUAL((tmp = ut_string_list_implode(list1,NULL,"=")),"hello=world");
    free(tmp);
    CU_ASSERT_STRING_EQUAL((tmp = ut_string_list_implode(list1,"-",NULL)),"hello--world");
    free(tmp);
    CU_ASSERT_STRING_EQUAL((tmp = ut_string_list_implode(list1,"-","=")),"hello-=world");
    free(tmp);
    ut_string_list_truncate_waste(list1);
    CU_ASSERT_EQUAL(ut_string_list_length(list1), 3);
    CU_ASSERT_NOT_EQUAL(ut_string_list_element(list1,0), str1);
    if (ut_string_list_element(list1,0))
	CU_ASSERT_STRING_EQUAL(ut_string_list_element(list1,0), str1);
    CU_ASSERT_PTR_NULL(ut_string_list_element(list1,1));
    CU_ASSERT_NOT_EQUAL(ut_string_list_element(list1,2), str2);
    if (ut_string_list_element(list1,2))
	CU_ASSERT_STRING_EQUAL(ut_string_list_element(list1,2), str2);
    CU_ASSERT_PTR_NULL(ut_string_list_element(list1,3));
    ut_string_list_add_element(list1, str3);
    CU_ASSERT_EQUAL(ut_string_list_length(list1), 4);
    CU_ASSERT_NOT_EQUAL(ut_string_list_element(list1,0), str1);
    if (ut_string_list_element(list1,0))
	CU_ASSERT_STRING_EQUAL(ut_string_list_element(list1,0), str1);
    CU_ASSERT_PTR_NULL(ut_string_list_element(list1,1));
    CU_ASSERT_NOT_EQUAL(ut_string_list_element(list1,2), str2);
    if (ut_string_list_element(list1,2))
	CU_ASSERT_STRING_EQUAL(ut_string_list_element(list1,2), str2);
    CU_ASSERT_NOT_EQUAL(ut_string_list_element(list1,3), str3);
    if (ut_string_list_element(list1,3))
	CU_ASSERT_STRING_EQUAL(ut_string_list_element(list1,3), str3);
    CU_ASSERT_PTR_NULL(ut_string_list_element(list1,4));
    CU_ASSERT_STRING_EQUAL((tmp = ut_string_list_implode(list1,NULL,NULL)),"helloworldgoodbye");
    free(tmp);
    CU_ASSERT_STRING_EQUAL((tmp = ut_string_list_implode(list1,NULL,"=")),"helloworld=goodbye");
    free(tmp);
    CU_ASSERT_STRING_EQUAL((tmp = ut_string_list_implode(list1,"-",NULL)),"hello--world-goodbye");
    free(tmp);
    CU_ASSERT_STRING_EQUAL((tmp = ut_string_list_implode(list1,"-","=")),"hello--world=goodbye");
    free(tmp);
    ut_string_list_free(list1);

    list1 = ut_string_list_new_with_capacity( 10 );
    CU_ASSERT_PTR_NOT_NULL(list1);
    CU_ASSERT_EQUAL(ut_string_list_length(NULL), 0);
    CU_ASSERT_EQUAL(ut_string_list_length(list1), 0);
    CU_ASSERT_PTR_NULL(ut_string_list_element(NULL,0));
    CU_ASSERT_PTR_NULL(ut_string_list_element(list1,-20));
    CU_ASSERT_PTR_NULL(ut_string_list_element(list1,0));
    CU_ASSERT_PTR_NULL(ut_string_list_element(list1,7));
    ut_string_list_add_element(NULL, NULL);
    ut_string_list_add_element(NULL, str1);
    ut_string_list_add_element(list1, str1);
    CU_ASSERT_EQUAL(ut_string_list_length(list1), 1);
    CU_ASSERT_NOT_EQUAL(ut_string_list_element(list1,0), str1);
    if (ut_string_list_element(list1,0))
	CU_ASSERT_STRING_EQUAL(ut_string_list_element(list1,0), str1);
    ut_string_list_add_element(list1, NULL);
    CU_ASSERT_EQUAL(ut_string_list_length(list1), 2);
    CU_ASSERT_PTR_NULL(ut_string_list_element(list1,-20));
    CU_ASSERT_NOT_EQUAL(ut_string_list_element(list1,0), str1);
    if (ut_string_list_element(list1,0))
	CU_ASSERT_STRING_EQUAL(ut_string_list_element(list1,0), str1);
    CU_ASSERT_PTR_NULL(ut_string_list_element(list1,1));
    CU_ASSERT_PTR_NULL(ut_string_list_element(list1,2));
    ut_string_list_add_element(list1, str2);
    CU_ASSERT_EQUAL(ut_string_list_length(list1), 3);
    CU_ASSERT_NOT_EQUAL(ut_string_list_element(list1,0), str1);
    if (ut_string_list_element(list1,0))
	CU_ASSERT_STRING_EQUAL(ut_string_list_element(list1,0), str1);
    CU_ASSERT_PTR_NULL(ut_string_list_element(list1,1));
    CU_ASSERT_NOT_EQUAL(ut_string_list_element(list1,2), str2);
    if (ut_string_list_element(list1,2))
	CU_ASSERT_STRING_EQUAL(ut_string_list_element(list1,2), str2);
    CU_ASSERT_PTR_NULL(ut_string_list_element(list1,3));
    ut_string_list_truncate_waste(list1);
    CU_ASSERT_EQUAL(ut_string_list_length(list1), 3);
    CU_ASSERT_NOT_EQUAL(ut_string_list_element(list1,0), str1);
    if (ut_string_list_element(list1,0))
	CU_ASSERT_STRING_EQUAL(ut_string_list_element(list1,0), str1);
    CU_ASSERT_PTR_NULL(ut_string_list_element(list1,1));
    CU_ASSERT_NOT_EQUAL(ut_string_list_element(list1,2), str2);
    if (ut_string_list_element(list1,2))
	CU_ASSERT_STRING_EQUAL(ut_string_list_element(list1,2), str2);
    CU_ASSERT_PTR_NULL(ut_string_list_element(list1,3));
    ut_string_list_add_element(list1, str3);
    CU_ASSERT_EQUAL(ut_string_list_length(list1), 4);
    CU_ASSERT_NOT_EQUAL(ut_string_list_element(list1,0), str1);
    if (ut_string_list_element(list1,0))
	CU_ASSERT_STRING_EQUAL(ut_string_list_element(list1,0), str1);
    CU_ASSERT_PTR_NULL(ut_string_list_element(list1,1));
    CU_ASSERT_NOT_EQUAL(ut_string_list_element(list1,2), str2);
    if (ut_string_list_element(list1,2))
	CU_ASSERT_STRING_EQUAL(ut_string_list_element(list1,2), str2);
    CU_ASSERT_NOT_EQUAL(ut_string_list_element(list1,3), str3);
    if (ut_string_list_element(list1,3))
	CU_ASSERT_STRING_EQUAL(ut_string_list_element(list1,3), str3);
    CU_ASSERT_PTR_NULL(ut_string_list_element(list1,4));
    ut_string_list_free(list1);

    list1 = ut_string_explode( NULL,",",NULL );
    CU_ASSERT_PTR_NULL(list1);
    list1 = ut_string_explode( "zero,one,two,three",NULL,NULL );
    CU_ASSERT_PTR_NOT_NULL(list1);
    CU_ASSERT_EQUAL(ut_string_list_length(list1), 1);
    if (ut_string_list_element(list1,0))
	CU_ASSERT_STRING_EQUAL(ut_string_list_element(list1,0), "zero,one,two,three");
    ut_string_list_free(list1);
    list1 = ut_string_explode( "zero,one,two,three",",",NULL );
    CU_ASSERT_PTR_NOT_NULL(list1);
    CU_ASSERT_EQUAL(ut_string_list_length(list1), 4);
    if (ut_string_list_element(list1,0))
	CU_ASSERT_STRING_EQUAL(ut_string_list_element(list1,0), "zero");
    if (ut_string_list_element(list1,1))
	CU_ASSERT_STRING_EQUAL(ut_string_list_element(list1,1), "one");
    if (ut_string_list_element(list1,2))
	CU_ASSERT_STRING_EQUAL(ut_string_list_element(list1,2), "two");
    if (ut_string_list_element(list1,3))
	CU_ASSERT_STRING_EQUAL(ut_string_list_element(list1,3), "three");
    list1 = ut_string_explode( "zero,,two,three",",",NULL );
    CU_ASSERT_PTR_NOT_NULL(list1);
    CU_ASSERT_EQUAL(ut_string_list_length(list1), 4);
    if (ut_string_list_element(list1,0))
	CU_ASSERT_STRING_EQUAL(ut_string_list_element(list1,0), "zero");
    if (ut_string_list_element(list1,1))
	CU_ASSERT_STRING_EQUAL(ut_string_list_element(list1,1), "");
    if (ut_string_list_element(list1,2))
	CU_ASSERT_STRING_EQUAL(ut_string_list_element(list1,2), "two");
    if (ut_string_list_element(list1,3))
	CU_ASSERT_STRING_EQUAL(ut_string_list_element(list1,3), "three");
    ut_string_list_free(list1);
    list1 = ut_string_explode( "zero, one, two, three",", ",NULL );
    CU_ASSERT_PTR_NOT_NULL(list1);
    CU_ASSERT_EQUAL(ut_string_list_length(list1), 4);
    if (ut_string_list_element(list1,0))
	CU_ASSERT_STRING_EQUAL(ut_string_list_element(list1,0), "zero");
    if (ut_string_list_element(list1,1))
	CU_ASSERT_STRING_EQUAL(ut_string_list_element(list1,1), "one");
    if (ut_string_list_element(list1,2))
	CU_ASSERT_STRING_EQUAL(ut_string_list_element(list1,2), "two");
    if (ut_string_list_element(list1,3))
	CU_ASSERT_STRING_EQUAL(ut_string_list_element(list1,3), "three");
    CU_ASSERT_STRING_EQUAL((tmp = ut_string_list_implode(list1,", "," and ")),"zero, one, two and three");
    ut_string_list_free(list1);
    list1 = ut_string_explode( tmp,", "," and " );
    free(tmp);
    CU_ASSERT_PTR_NOT_NULL(list1);
    CU_ASSERT_EQUAL(ut_string_list_length(list1), 4);
    if (ut_string_list_element(list1,0))
	CU_ASSERT_STRING_EQUAL(ut_string_list_element(list1,0), "zero");
    if (ut_string_list_element(list1,1))
	CU_ASSERT_STRING_EQUAL(ut_string_list_element(list1,1), "one");
    if (ut_string_list_element(list1,2))
	CU_ASSERT_STRING_EQUAL(ut_string_list_element(list1,2), "two");
    if (ut_string_list_element(list1,3))
	CU_ASSERT_STRING_EQUAL(ut_string_list_element(list1,3), "three");
    ut_string_list_free(list1);
}

int utFindNamedSystemIndex( ut_system* const system, const char* const system_name );

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
    CU_ASSERT_EQUAL(ut_add_named_system(system, "", UT_ASCII), UT_BAD_ARG);

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

    ut_free_system(system);
}

static void
test_named_system_name_getting(void)
{
    ut_system*	system = ut_new_system();
    ut_string_list* namedSystems;

    CU_ASSERT_PTR_NULL(ut_get_named_systems( NULL ));
    CU_ASSERT_EQUAL(ut_get_status(), UT_BAD_ARG);
    CU_ASSERT_PTR_NOT_NULL(system);
    CU_ASSERT_PTR_NULL(ut_get_named_system_aliases( NULL, NULL ));
    CU_ASSERT_EQUAL(ut_get_status(), UT_BAD_ARG);

    namedSystems = ut_get_named_systems( system );
    CU_ASSERT_EQUAL(ut_get_status(), UT_SUCCESS);
    CU_ASSERT_PTR_NOT_NULL(namedSystems);
    CU_ASSERT_EQUAL(ut_string_list_length(namedSystems),0);
    ut_string_list_free( namedSystems );

    namedSystems = ut_get_named_system_aliases( system, NULL );
    CU_ASSERT_EQUAL(ut_get_status(), UT_SUCCESS);
    CU_ASSERT_PTR_NOT_NULL(namedSystems);
    CU_ASSERT_EQUAL(ut_string_list_length(namedSystems),0);
    ut_string_list_free( namedSystems );
    namedSystems = ut_get_named_system_aliases( system, "" );
    CU_ASSERT_EQUAL(ut_get_status(), UT_SUCCESS);
    CU_ASSERT_PTR_NOT_NULL(namedSystems);
    CU_ASSERT_EQUAL(ut_string_list_length(namedSystems),0);
    ut_string_list_free( namedSystems );
    namedSystems = ut_get_named_system_aliases( system, "US" );
    CU_ASSERT_EQUAL(ut_get_status(), UT_SUCCESS);
    CU_ASSERT_PTR_NOT_NULL(namedSystems);
    CU_ASSERT_EQUAL(ut_string_list_length(namedSystems),0);
    ut_string_list_free( namedSystems );

    CU_ASSERT_EQUAL(ut_add_named_system(system, "SI", UT_ASCII), UT_SUCCESS);
    CU_ASSERT_EQUAL(ut_add_named_system(system, "Metric", UT_ASCII), UT_SUCCESS);
    CU_ASSERT_EQUAL(ut_add_named_system(system, "US", UT_ASCII), UT_SUCCESS);
    namedSystems = ut_get_named_systems( system );
    CU_ASSERT_PTR_NOT_NULL(namedSystems);
    CU_ASSERT_EQUAL(ut_string_list_length(namedSystems),3);
    if (ut_string_list_element(namedSystems,0))
	CU_ASSERT_STRING_EQUAL(ut_string_list_element(namedSystems,0), "Metric");
    if (ut_string_list_element(namedSystems,1))
	CU_ASSERT_STRING_EQUAL(ut_string_list_element(namedSystems,1), "SI");
    if (ut_string_list_element(namedSystems,2))
	CU_ASSERT_STRING_EQUAL(ut_string_list_element(namedSystems,2), "US");
    ut_string_list_free( namedSystems );

    CU_ASSERT_EQUAL(ut_map_name_to_named_system(system, "International System", UT_ASCII, "SI"), UT_SUCCESS);
    CU_ASSERT_EQUAL(ut_map_name_to_named_system(system, "US Conventional System", UT_ASCII, "US"), UT_SUCCESS);
    CU_ASSERT_EQUAL(ut_map_name_to_named_system(system, "US Common", UT_ASCII, "US"), UT_SUCCESS);
    CU_ASSERT_EQUAL(ut_map_name_to_named_system(system, "US Common System", UT_ASCII, "US"), UT_SUCCESS);
    CU_ASSERT_EQUAL(ut_map_name_to_named_system(system, "US Common System", UT_ASCII, "SI"), UT_EXISTS);
    CU_ASSERT_EQUAL(ut_map_name_to_named_system(system, "US Common System", UT_ASCII, "US Common"), UT_SUCCESS);
    namedSystems = ut_get_named_systems( system );
    CU_ASSERT_PTR_NOT_NULL(namedSystems);
    CU_ASSERT_EQUAL(ut_string_list_length(namedSystems),3);
    if (ut_string_list_element(namedSystems,0))
	CU_ASSERT_STRING_EQUAL(ut_string_list_element(namedSystems,0), "Metric");
    if (ut_string_list_element(namedSystems,1))
	CU_ASSERT_STRING_EQUAL(ut_string_list_element(namedSystems,1), "SI");
    if (ut_string_list_element(namedSystems,2))
	CU_ASSERT_STRING_EQUAL(ut_string_list_element(namedSystems,2), "US");
    ut_string_list_free( namedSystems );
    namedSystems = ut_get_named_system_aliases( system, "US" );
    CU_ASSERT_PTR_NOT_NULL(namedSystems);
    CU_ASSERT_EQUAL(ut_string_list_length(namedSystems),4);
    if (ut_string_list_element(namedSystems,0))
	CU_ASSERT_STRING_EQUAL(ut_string_list_element(namedSystems,0), "US");
    if (ut_string_list_element(namedSystems,1))
	CU_ASSERT_STRING_EQUAL(ut_string_list_element(namedSystems,1), "US Common");
    if (ut_string_list_element(namedSystems,2))
	CU_ASSERT_STRING_EQUAL(ut_string_list_element(namedSystems,2), "US Common System");
    if (ut_string_list_element(namedSystems,3))
	CU_ASSERT_STRING_EQUAL(ut_string_list_element(namedSystems,3), "US Conventional System");
    ut_string_list_free( namedSystems );
    namedSystems = ut_get_named_system_aliases( system, NULL );
    CU_ASSERT_EQUAL(ut_get_status(), UT_SUCCESS);
    CU_ASSERT_PTR_NOT_NULL(namedSystems);
    CU_ASSERT_EQUAL(ut_string_list_length(namedSystems),7);
    if (ut_string_list_element(namedSystems,0))
	CU_ASSERT_STRING_EQUAL(ut_string_list_element(namedSystems,0), "International System");
    if (ut_string_list_element(namedSystems,1))
	CU_ASSERT_STRING_EQUAL(ut_string_list_element(namedSystems,1), "Metric");
    if (ut_string_list_element(namedSystems,2))
	CU_ASSERT_STRING_EQUAL(ut_string_list_element(namedSystems,2), "SI");
    if (ut_string_list_element(namedSystems,3))
	CU_ASSERT_STRING_EQUAL(ut_string_list_element(namedSystems,3), "US");
    if (ut_string_list_element(namedSystems,4))
	CU_ASSERT_STRING_EQUAL(ut_string_list_element(namedSystems,4), "US Common");
    if (ut_string_list_element(namedSystems,5))
	CU_ASSERT_STRING_EQUAL(ut_string_list_element(namedSystems,5), "US Common System");
    if (ut_string_list_element(namedSystems,6))
	CU_ASSERT_STRING_EQUAL(ut_string_list_element(namedSystems,6), "US Conventional System");
    ut_string_list_free( namedSystems );

    namedSystems = ut_get_named_system_aliases( system, "" );
    CU_ASSERT_PTR_NOT_NULL(namedSystems);
    CU_ASSERT_EQUAL(ut_get_status(), UT_SUCCESS);
    CU_ASSERT_EQUAL(ut_string_list_length(namedSystems),3);
    if (ut_string_list_element(namedSystems,0))
	CU_ASSERT_STRING_EQUAL(ut_string_list_element(namedSystems,0), "Metric");
    if (ut_string_list_element(namedSystems,1))
	CU_ASSERT_STRING_EQUAL(ut_string_list_element(namedSystems,1), "SI");
    if (ut_string_list_element(namedSystems,2))
	CU_ASSERT_STRING_EQUAL(ut_string_list_element(namedSystems,2), "US");
    ut_string_list_free( namedSystems );

    namedSystems = ut_get_named_system_aliases( system, "NoSuchName" );
    CU_ASSERT_NOT_EQUAL(ut_get_status(), UT_SUCCESS);
    CU_ASSERT_EQUAL(ut_get_status(), UT_UNKNOWN);
    CU_ASSERT_PTR_NOT_NULL(namedSystems);
    CU_ASSERT_EQUAL(ut_string_list_length(namedSystems),0);
    ut_string_list_free( namedSystems );

    //printf( "Getting the list of named systems:\n" );
    //namedSystems = ut_get_named_systems( system );
    // Print all the names
    //for( int i = 0; i < ut_string_list_length(namedSystems); ++i ) {
        //printf( "%d: %s\n", i, ut_string_list_element(namedSystems,i) );
    //}
    //ut_string_list_free( namedSystems );
    //printf( "Getting the list of named system aliases of \"US\":\n" );
    //namedSystems = ut_get_named_system_aliases( system, "US" );
    // Print all the names
    //for( int i = 0; i < ut_string_list_length(namedSystems); ++i ) {
        //printf( "%d: %s\n", i, ut_string_list_element(namedSystems,i) );
    //}
    //ut_string_list_free( namedSystems );

    ut_free_system(system);
}


static void
test_bitmap(void)
{
    Bitmap *membership;
    Bitmap *membership2;
    char	buf[1024];
    int		size = 1024;

    membership = bitmapNew();
    CU_ASSERT_PTR_NOT_NULL(membership);
    CU_ASSERT_FALSE(bitIsSet(NULL, 0));
    CU_ASSERT_FALSE(bitIsSet(membership, -1));
    CU_ASSERT_FALSE(bitIsSet(membership, 0));
    CU_ASSERT_FALSE(bitIsSet(membership, 1));
    CU_ASSERT_FALSE(bitIsSet(membership, 3));
    CU_ASSERT_FALSE(setBit(NULL, 0));
    CU_ASSERT_FALSE(setBit(membership, -1));
    CU_ASSERT_FALSE(setBit(membership, 2));
    CU_ASSERT_EQUAL(snprintfBitmap( buf, size, "%x", membership),1);
    CU_ASSERT_STRING_EQUAL(buf, "4");
    CU_ASSERT_EQUAL(snprintfBitmap( buf, size, "%#x", membership),3);
    CU_ASSERT_STRING_EQUAL(buf, "0x4");
    CU_ASSERT_EQUAL(snprintfBitmap( buf, size, "%#0x", membership),3);
    CU_ASSERT_STRING_EQUAL(buf, "0x4");
    CU_ASSERT_EQUAL(snprintfBitmap( buf, size, "%#05x", membership),5);
    CU_ASSERT_STRING_EQUAL(buf, "0x004");
    CU_ASSERT_EQUAL(snprintfBitmap( buf, size, "%#-05x", membership),5);
    CU_ASSERT_STRING_EQUAL(buf, "0x004");
    CU_ASSERT_EQUAL(snprintfBitmap( buf, size, "%#5x", membership),5);
    CU_ASSERT_STRING_EQUAL(buf, "  0x4");
    CU_ASSERT_EQUAL(snprintfBitmap( buf, size, "%#-5x", membership),5);
    CU_ASSERT_STRING_EQUAL(buf, "0x4  ");
    CU_ASSERT_EQUAL(snprintfBitmap( buf, size, "%#X", membership),3);
    CU_ASSERT_STRING_EQUAL(buf, "0X4");
    CU_ASSERT_EQUAL(snprintfBitmap( buf, size, "%b", membership),3);
    CU_ASSERT_STRING_EQUAL(buf, "100");
    CU_ASSERT_EQUAL(snprintfBitmap( buf, size, "%#b", membership),5);
    CU_ASSERT_STRING_EQUAL(buf, "0b100");
    CU_ASSERT_EQUAL(snprintfBitmap( buf, size, "%#9b", membership),9);
    CU_ASSERT_STRING_EQUAL(buf, "    0b100");
    CU_ASSERT_EQUAL(snprintfBitmap( buf, size, "%#-9b", membership),9);
    CU_ASSERT_STRING_EQUAL(buf, "0b100    ");
    CU_ASSERT_EQUAL(snprintfBitmap( buf, size, "%#09b", membership),9);
    CU_ASSERT_STRING_EQUAL(buf, "0b0000100");
    CU_ASSERT_EQUAL(snprintfBitmap( buf, size, "%#-09b", membership),9);
    CU_ASSERT_STRING_EQUAL(buf, "0b0000100");
    CU_ASSERT_EQUAL(snprintfBitmap( buf, size, "%o", membership),1);
    CU_ASSERT_STRING_EQUAL(buf, "4");
    CU_ASSERT_EQUAL(snprintfBitmap( buf, size, "%#o", membership),2);
    CU_ASSERT_STRING_EQUAL(buf, "04");
    CU_ASSERT_EQUAL(snprintfBitmap( buf, size, "%#9o", membership),9);
    CU_ASSERT_STRING_EQUAL(buf, "       04");
    CU_ASSERT_EQUAL(snprintfBitmap( buf, size, "%#-9o", membership),9);
    CU_ASSERT_STRING_EQUAL(buf, "04       ");
    CU_ASSERT_EQUAL(snprintfBitmap( buf, size, "%#09o", membership),9);
    CU_ASSERT_STRING_EQUAL(buf, "000000004");
    CU_ASSERT_EQUAL(snprintfBitmap( buf, size, "%#-09o", membership),9);
    CU_ASSERT_STRING_EQUAL(buf, "000000004");
    CU_ASSERT_FALSE(bitIsSet(membership, 0));
    CU_ASSERT_FALSE(bitIsSet(membership, 1));
    CU_ASSERT_TRUE(bitIsSet(membership, 2));
    CU_ASSERT_FALSE(bitIsSet(membership, 3));
    CU_ASSERT_FALSE(bitIsSet(membership, 71));
    CU_ASSERT_FALSE(setBit(membership, 71));
    CU_ASSERT_EQUAL(snprintfBitmap( buf, size, "%x", membership),18);
    CU_ASSERT_STRING_EQUAL(buf, "800000000000000004");
    CU_ASSERT_EQUAL(snprintfBitmap( buf, size, "%o", membership),24);
    CU_ASSERT_STRING_EQUAL(buf, "400000000000000000000004");
    CU_ASSERT_EQUAL(snprintfBitmap( buf, 7, "%x", membership),18);
    CU_ASSERT_STRING_EQUAL(buf, "800000");
    CU_ASSERT_EQUAL(snprintfBitmap( buf, size, "%b", membership),72);
    CU_ASSERT_STRING_EQUAL(buf, "100000000000000000000000000000000000000000000000000000000000000000000100");
    CU_ASSERT_TRUE(bitIsSet(membership, 71));
    CU_ASSERT_TRUE(setBit(membership, 2));
    CU_ASSERT_FALSE(bitIsSet(membership, 0));
    CU_ASSERT_FALSE(bitIsSet(membership, 1));
    CU_ASSERT_TRUE(bitIsSet(membership, 2));
    CU_ASSERT_FALSE(setBit(membership, -1));

    CU_ASSERT_FALSE(clearBit(NULL, 2));
    CU_ASSERT_FALSE(clearBit(membership, -1));
    CU_ASSERT_TRUE(bitIsSet(membership, 2));
    CU_ASSERT_TRUE(clearBit(membership, 2));
    CU_ASSERT_FALSE(bitIsSet(membership, 2));
    CU_ASSERT_FALSE(clearBit(membership, 2));
    CU_ASSERT_FALSE(clearBit(membership, 2400));
    CU_ASSERT_EQUAL(snprintfBitmap( buf, size, "%x", membership),18);
    CU_ASSERT_STRING_EQUAL(buf, "800000000000000000");
    CU_ASSERT_EQUAL(snprintfBitmap( buf, size, "%o", membership),24);
    CU_ASSERT_STRING_EQUAL(buf, "400000000000000000000000");
    CU_ASSERT_EQUAL(snprintfBitmap( buf, size, "%b", membership),72);
    CU_ASSERT_STRING_EQUAL(buf, "100000000000000000000000000000000000000000000000000000000000000000000000");

    // Bitmap comparison testing
    CU_ASSERT_TRUE(bitmapCmp(NULL, NULL) == 0);
    CU_ASSERT_TRUE(bitmapCmp(membership, membership) == 0);
    CU_ASSERT_TRUE(bitmapCmp(NULL, membership) < 0);
    CU_ASSERT_TRUE(bitmapCmp(membership, NULL) > 0);

    membership2 = bitmapNew();
    CU_ASSERT_PTR_NOT_NULL(membership2);
    CU_ASSERT_TRUE(bitmapCmp(membership2, membership2) == 0);
    CU_ASSERT_TRUE(bitmapCmp(NULL, membership2) == 0);
    CU_ASSERT_TRUE(bitmapCmp(membership2, NULL) == 0);

    CU_ASSERT_TRUE(bitmapCmp(membership, membership2) > 0);
    CU_ASSERT_TRUE(bitmapCmp(membership2, membership) < 0);
    bitmapReset( membership );
    CU_ASSERT_TRUE(bitmapCmp(membership, membership2) == 0);
    CU_ASSERT_TRUE(bitmapCmp(membership2, membership) == 0);

    CU_ASSERT_FALSE(setBit(membership, 2));
    CU_ASSERT_FALSE(setBit(membership2, 2));
    CU_ASSERT_TRUE(bitmapCmp(membership, membership2) == 0);
    CU_ASSERT_TRUE(bitmapCmp(membership2, membership) == 0);
    CU_ASSERT_FALSE(setBit(membership2, 962));
    CU_ASSERT_TRUE(bitmapCmp(membership, membership2) < 0);
    CU_ASSERT_TRUE(bitmapCmp(membership2, membership) > 0);
    CU_ASSERT_TRUE(clearBit(membership2, 962));
    CU_ASSERT_TRUE(bitIsSet(membership2, 2));
    CU_ASSERT_FALSE(bitIsSet(membership2, 962));
    CU_ASSERT_TRUE(bitmapCmp(membership, membership2) == 0);
    CU_ASSERT_TRUE(bitmapCmp(membership2, membership) == 0);

    bitmapReset( membership );
    CU_ASSERT_FALSE(setBit(membership, 70));
    CU_ASSERT_EQUAL(snprintfBitmap( buf, size, "%x", membership),18);
    CU_ASSERT_STRING_EQUAL(buf, "400000000000000000");
    CU_ASSERT_EQUAL(snprintfBitmap( buf, size, "%o", membership),24);
    CU_ASSERT_STRING_EQUAL(buf, "200000000000000000000000");
    CU_ASSERT_EQUAL(snprintfBitmap( buf, size, "%b", membership),71);
    CU_ASSERT_STRING_EQUAL(buf, "10000000000000000000000000000000000000000000000000000000000000000000000");

    bitmapReset( membership );
    CU_ASSERT_FALSE(setBit(membership, 69));
    CU_ASSERT_EQUAL(snprintfBitmap( buf, size, "%x", membership),18);
    CU_ASSERT_STRING_EQUAL(buf, "200000000000000000");
    CU_ASSERT_EQUAL(snprintfBitmap( buf, size, "%o", membership),24);
    CU_ASSERT_STRING_EQUAL(buf, "100000000000000000000000");
    CU_ASSERT_EQUAL(snprintfBitmap( buf, size, "%b", membership),70);
    CU_ASSERT_STRING_EQUAL(buf, "1000000000000000000000000000000000000000000000000000000000000000000000");

    bitmapReset( membership );
    CU_ASSERT_FALSE(setBit(membership, 68));
    CU_ASSERT_EQUAL(snprintfBitmap( buf, size, "%x", membership),18);
    CU_ASSERT_STRING_EQUAL(buf, "100000000000000000");
    CU_ASSERT_EQUAL(snprintfBitmap( buf, size, "%o", membership),23);
    CU_ASSERT_STRING_EQUAL(buf, "40000000000000000000000");
    CU_ASSERT_EQUAL(snprintfBitmap( buf, size, "%b", membership),69);
    CU_ASSERT_STRING_EQUAL(buf, "100000000000000000000000000000000000000000000000000000000000000000000");
    // Extend it
    CU_ASSERT_FALSE(setBit(membership, 65));
    CU_ASSERT_EQUAL(snprintfBitmap( buf, size, "%x", membership),18);
    CU_ASSERT_STRING_EQUAL(buf, "120000000000000000");
    CU_ASSERT_EQUAL(snprintfBitmap( buf, size, "%o", membership),23);
    CU_ASSERT_STRING_EQUAL(buf, "44000000000000000000000");
    CU_ASSERT_EQUAL(snprintfBitmap( buf, size, "%b", membership),69);
    CU_ASSERT_STRING_EQUAL(buf, "100100000000000000000000000000000000000000000000000000000000000000000");
    // Extend some more
    CU_ASSERT_FALSE(setBit(membership, 32));
    CU_ASSERT_EQUAL(snprintfBitmap( buf, size, "%x", membership),18);
    CU_ASSERT_STRING_EQUAL(buf, "120000000100000000");
    CU_ASSERT_EQUAL(snprintfBitmap( buf, size, "%o", membership),23);
    CU_ASSERT_STRING_EQUAL(buf, "44000000000040000000000");
    CU_ASSERT_EQUAL(snprintfBitmap( buf, size, "%b", membership),69);
    CU_ASSERT_STRING_EQUAL(buf, "100100000000000000000000000000000000100000000000000000000000000000000");
    // Extend some more
    CU_ASSERT_FALSE(setBit(membership, 31));
    CU_ASSERT_EQUAL(snprintfBitmap( buf, size, "%x", membership),18);
    CU_ASSERT_STRING_EQUAL(buf, "120000000180000000");
    CU_ASSERT_EQUAL(snprintfBitmap( buf, size, "%o", membership),23);
    CU_ASSERT_STRING_EQUAL(buf, "44000000000060000000000");
    CU_ASSERT_EQUAL(snprintfBitmap( buf, size, "%b", membership),69);
    CU_ASSERT_STRING_EQUAL(buf, "100100000000000000000000000000000000110000000000000000000000000000000");
    // Extend some more
    CU_ASSERT_FALSE(setBit(membership, 30));
    CU_ASSERT_EQUAL(snprintfBitmap( buf, size, "%x", membership),18);
    CU_ASSERT_STRING_EQUAL(buf, "1200000001c0000000");
    CU_ASSERT_EQUAL(snprintfBitmap( buf, size, "%o", membership),23);
    CU_ASSERT_STRING_EQUAL(buf, "44000000000070000000000");
    CU_ASSERT_EQUAL(snprintfBitmap( buf, size, "%b", membership),69);
    CU_ASSERT_STRING_EQUAL(buf, "100100000000000000000000000000000000111000000000000000000000000000000");
    // Extend some more
    CU_ASSERT_FALSE(setBit(membership, 29));
    CU_ASSERT_EQUAL(snprintfBitmap( buf, size, "%x", membership),18);
    CU_ASSERT_STRING_EQUAL(buf, "1200000001e0000000");
    CU_ASSERT_EQUAL(snprintfBitmap( buf, size, "%o", membership),23);
    CU_ASSERT_STRING_EQUAL(buf, "44000000000074000000000");
    CU_ASSERT_EQUAL(snprintfBitmap( buf, size, "%b", membership),69);
    CU_ASSERT_STRING_EQUAL(buf, "100100000000000000000000000000000000111100000000000000000000000000000");
    // Extend some more
    CU_ASSERT_FALSE(setBit(membership, 33));
    CU_ASSERT_EQUAL(snprintfBitmap( buf, size, "%x", membership),18);
    CU_ASSERT_STRING_EQUAL(buf, "1200000003e0000000");
    CU_ASSERT_EQUAL(snprintfBitmap( buf, size, "%o", membership),23);
    CU_ASSERT_STRING_EQUAL(buf, "44000000000174000000000");
    CU_ASSERT_EQUAL(snprintfBitmap( buf, size, "%b", membership),69);
    CU_ASSERT_STRING_EQUAL(buf, "100100000000000000000000000000000001111100000000000000000000000000000");
    // Extend some more
    CU_ASSERT_TRUE(clearBit(membership, 32));
    CU_ASSERT_EQUAL(snprintfBitmap( buf, size, "%x", membership),18);
    CU_ASSERT_STRING_EQUAL(buf, "1200000002e0000000");
    CU_ASSERT_EQUAL(snprintfBitmap( buf, size, "%o", membership),23);
    CU_ASSERT_STRING_EQUAL(buf, "44000000000134000000000");
    CU_ASSERT_EQUAL(snprintfBitmap( buf, size, "%b", membership),69);
    CU_ASSERT_STRING_EQUAL(buf, "100100000000000000000000000000000001011100000000000000000000000000000");

    bitmapReset( membership );
    CU_ASSERT_PTR_NOT_NULL(membership);
    CU_ASSERT_FALSE(setBit(membership, 962));
    CU_ASSERT_TRUE(bitIsSet(membership, 962));
    CU_ASSERT_EQUAL(snprintfBitmap( buf, 15, "%b", membership), 963);
    CU_ASSERT_STRING_EQUAL(buf, "10000000000000");
    CU_ASSERT_EQUAL(snprintfBitmap( buf, 15, "%#b", membership), 965);
    CU_ASSERT_STRING_EQUAL(buf, "0b100000000000");

    bitmapReset( membership );
    CU_ASSERT_PTR_NOT_NULL(membership);
    bitmapReset( membership2 );
    CU_ASSERT_PTR_NOT_NULL(membership2);
    CU_ASSERT_FALSE(setBit(membership, 62));
    CU_ASSERT_FALSE(setBit(membership2, 66));
    CU_ASSERT_TRUE(bitmapCmp(membership, membership2) < 0);
    CU_ASSERT_TRUE(bitmapCmp(membership2, membership) > 0);

    CU_ASSERT_EQUAL(snprintfBitmap( buf, size, "%x", NULL),6);
    CU_ASSERT_STRING_EQUAL(buf, "(null)");
    CU_ASSERT_EQUAL(snprintfBitmap( buf, 5, "%x", NULL),6);
    CU_ASSERT_STRING_EQUAL(buf, "(nul");
    CU_ASSERT_EQUAL(snprintfBitmap( buf, size, "hello %x goodbye", NULL),20);
    CU_ASSERT_STRING_EQUAL(buf, "hello (null) goodbye");
    CU_ASSERT_EQUAL(snprintfBitmap( buf, size, "hello %#05x goodbye", NULL),20);
    CU_ASSERT_STRING_EQUAL(buf, "hello (null) goodbye");
    CU_ASSERT_EQUAL(snprintfBitmap( buf, 8, "hello %#05x goodbye", NULL),20);
    CU_ASSERT_STRING_EQUAL(buf, "hello (");
    CU_ASSERT_EQUAL(snprintfBitmap( buf, 4, "hello %#05x goodbye", NULL),20);
    CU_ASSERT_STRING_EQUAL(buf, "hel");

    bitmapReset( membership );
    CU_ASSERT_PTR_NOT_NULL(membership);
    CU_ASSERT_EQUAL(snprintfBitmap( buf, size, "%x", membership),1);
    CU_ASSERT_STRING_EQUAL(buf, "0");
    CU_ASSERT_EQUAL(snprintfBitmap( buf, size, "%#x", membership),1);
    CU_ASSERT_STRING_EQUAL(buf, "0");
    CU_ASSERT_EQUAL(snprintfBitmap( buf, size, "%05x", membership),5);
    CU_ASSERT_STRING_EQUAL(buf, "00000");
    CU_ASSERT_EQUAL(snprintfBitmap( buf, size, "%-05x", membership),5);
    CU_ASSERT_STRING_EQUAL(buf, "00000");
    CU_ASSERT_EQUAL(snprintfBitmap( buf, size, "%#05x", membership),5);
    CU_ASSERT_STRING_EQUAL(buf, "00000");
    CU_ASSERT_EQUAL(snprintfBitmap( buf, size, "%#-05x", membership),5);
    CU_ASSERT_STRING_EQUAL(buf, "00000");
    CU_ASSERT_EQUAL(snprintfBitmap( buf, size, "%#5x", membership),5);
    CU_ASSERT_STRING_EQUAL(buf, "    0");
    CU_ASSERT_EQUAL(snprintfBitmap( buf, size, "%#-5x", membership),5);
    CU_ASSERT_STRING_EQUAL(buf, "0    ");
    CU_ASSERT_EQUAL(snprintfBitmap( buf, size, "%5x", membership),5);
    CU_ASSERT_STRING_EQUAL(buf, "    0");
    CU_ASSERT_EQUAL(snprintfBitmap( buf, size, "%-5x", membership),5);
    CU_ASSERT_STRING_EQUAL(buf, "0    ");
    CU_ASSERT_EQUAL(snprintfBitmap( buf, size, "%o", membership),1);
    CU_ASSERT_STRING_EQUAL(buf, "0");
    CU_ASSERT_EQUAL(snprintfBitmap( buf, size, "%#o", membership),1);
    CU_ASSERT_STRING_EQUAL(buf, "0");
    CU_ASSERT_EQUAL(snprintfBitmap( buf, size, "%05o", membership),5);
    CU_ASSERT_STRING_EQUAL(buf, "00000");
    CU_ASSERT_EQUAL(snprintfBitmap( buf, size, "%-05o", membership),5);
    CU_ASSERT_STRING_EQUAL(buf, "00000");
    CU_ASSERT_EQUAL(snprintfBitmap( buf, size, "%#05o", membership),5);
    CU_ASSERT_STRING_EQUAL(buf, "00000");
    CU_ASSERT_EQUAL(snprintfBitmap( buf, size, "%#-05o", membership),5);
    CU_ASSERT_STRING_EQUAL(buf, "00000");
    CU_ASSERT_EQUAL(snprintfBitmap( buf, size, "%#5o", membership),5);
    CU_ASSERT_STRING_EQUAL(buf, "    0");
    CU_ASSERT_EQUAL(snprintfBitmap( buf, size, "%#-5o", membership),5);
    CU_ASSERT_STRING_EQUAL(buf, "0    ");
    CU_ASSERT_EQUAL(snprintfBitmap( buf, size, "%5o", membership),5);
    CU_ASSERT_STRING_EQUAL(buf, "    0");
    CU_ASSERT_EQUAL(snprintfBitmap( buf, size, "%-5o", membership),5);
    CU_ASSERT_STRING_EQUAL(buf, "0    ");
    CU_ASSERT_FALSE(setBit(membership, 232));
    CU_ASSERT_TRUE(bitIsSet(membership, 232));
    CU_ASSERT_TRUE(clearBit(membership, 232));
    CU_ASSERT_FALSE(bitIsSet(membership, 232));
    CU_ASSERT_EQUAL(snprintfBitmap( buf, size, "%x", membership),1);
    CU_ASSERT_STRING_EQUAL(buf, "0");
    CU_ASSERT_EQUAL(snprintfBitmap( buf, size, "%#x", membership),1);
    CU_ASSERT_STRING_EQUAL(buf, "0");
    CU_ASSERT_EQUAL(snprintfBitmap( buf, size, "%05x", membership),5);
    CU_ASSERT_STRING_EQUAL(buf, "00000");
    CU_ASSERT_EQUAL(snprintfBitmap( buf, size, "%-05x", membership),5);
    CU_ASSERT_STRING_EQUAL(buf, "00000");
    CU_ASSERT_EQUAL(snprintfBitmap( buf, size, "%#05x", membership),5);
    CU_ASSERT_STRING_EQUAL(buf, "00000");
    CU_ASSERT_EQUAL(snprintfBitmap( buf, size, "%#-05x", membership),5);
    CU_ASSERT_STRING_EQUAL(buf, "00000");
    CU_ASSERT_EQUAL(snprintfBitmap( buf, size, "%#5x", membership),5);
    CU_ASSERT_STRING_EQUAL(buf, "    0");
    CU_ASSERT_EQUAL(snprintfBitmap( buf, size, "%#-5x", membership),5);
    CU_ASSERT_STRING_EQUAL(buf, "0    ");
    CU_ASSERT_EQUAL(snprintfBitmap( buf, size, "%5x", membership),5);
    CU_ASSERT_STRING_EQUAL(buf, "    0");
    CU_ASSERT_EQUAL(snprintfBitmap( buf, size, "%-5x", membership),5);
    CU_ASSERT_STRING_EQUAL(buf, "0    ");
    bitmapReset( membership );
    CU_ASSERT_PTR_NOT_NULL(membership);
    bitmapReset( membership2 );
    CU_ASSERT_PTR_NOT_NULL(membership2);
    CU_ASSERT_TRUE(bitmapCmp(membership, membership2) == 0);
    CU_ASSERT_TRUE(bitmapCmp(membership2, membership) == 0);
    CU_ASSERT_FALSE(setBit(membership, 2));
    CU_ASSERT_FALSE(setBit(membership2, 3));
    CU_ASSERT_EQUAL(snprintfBitmap( buf, size, "%x", membership2),1);
    CU_ASSERT_STRING_EQUAL(buf, "8");
    CU_ASSERT_EQUAL(snprintfBitmap( buf, size, "some text before with a %sign in it %x", membership2),36);
    CU_ASSERT_STRING_EQUAL(buf, "some text before with a 8ign in it x");
    CU_ASSERT_EQUAL(snprintfBitmap( buf, size, "some text before with a %%sign in it %x", membership2),37);
    CU_ASSERT_STRING_EQUAL(buf, "some text before with a %sign in it 8");
    CU_ASSERT_EQUAL(snprintfBitmap( buf, size, "%#x some text after with a %sign in it", membership2),37);
    CU_ASSERT_STRING_EQUAL(buf, "0x8 some text after with a sign in it");
    CU_ASSERT_EQUAL(snprintfBitmap( buf, size, "%#x some text after with a %%sign in it", membership2),38);
    CU_ASSERT_STRING_EQUAL(buf, "0x8 some text after with a %sign in it");
    CU_ASSERT_TRUE(bitmapCmp(membership, membership2) < 0);
    CU_ASSERT_TRUE(bitmapCmp(membership2, membership) > 0);
    CU_ASSERT_TRUE(clearBit(membership2, 3));
    CU_ASSERT_TRUE(bitmapCmp(membership, membership2) > 0);
    CU_ASSERT_TRUE(bitmapCmp(membership2, membership) < 0);
    CU_ASSERT_FALSE(setBit(membership2, 38));
    CU_ASSERT_TRUE(bitmapCmp(membership, membership2) < 0);
    CU_ASSERT_TRUE(bitmapCmp(membership2, membership) > 0);
    CU_ASSERT_TRUE(clearBit(membership2, 38));
    CU_ASSERT_TRUE(bitmapCmp(membership, membership2) > 0);
    CU_ASSERT_TRUE(bitmapCmp(membership2, membership) < 0);

    // Test copy
    CU_ASSERT_PTR_NULL(bitmapCopy(NULL,membership));
    CU_ASSERT_PTR_NOT_NULL(bitmapCopy(membership2,NULL));
    CU_ASSERT_PTR_NOT_NULL(bitmapCopy(membership2,membership));
    CU_ASSERT_TRUE(bitmapCmp(membership, membership2) == 0);
    CU_ASSERT_TRUE(bitmapCmp(membership2, membership) == 0);

    // Test dup
    bitmapFree( membership2 );
    membership2 = bitmapDup( NULL );
    CU_ASSERT_PTR_NULL(membership2);
    membership2 = bitmapDup( membership );
    CU_ASSERT_PTR_NOT_NULL(membership2);
    CU_ASSERT_TRUE(bitmapCmp(membership, membership2) == 0);
    CU_ASSERT_TRUE(bitmapCmp(membership2, membership) == 0);
    CU_ASSERT_FALSE(setBit(membership2, 8));
    CU_ASSERT_FALSE(bitmapCmp(membership, membership2) == 0);
    CU_ASSERT_FALSE(bitmapCmp(membership2, membership) == 0);

    bitmapFree( membership );
    bitmapFree( membership2 );
}

static void
test_named_system_registry(void)
{
    NamedSystemRegistry* registry = NULL;
    ut_system*	system = ut_new_system();

    CU_ASSERT_PTR_NOT_NULL(system);
    CU_ASSERT_PTR_NULL(registry);
    CU_ASSERT_FALSE(utNamedSystemIsInRegistry(system, registry, "SI"));
    CU_ASSERT_EQUAL(ut_get_status(), UT_UNKNOWN);

    registry = utSetNamedSystemInRegistry(NULL, registry, "SI");
    CU_ASSERT_PTR_NULL(registry);
    CU_ASSERT_EQUAL(ut_get_status(), UT_BAD_ARG);
    registry = utSetNamedSystemInRegistry(system, registry, NULL);
    CU_ASSERT_PTR_NULL(registry);
    CU_ASSERT_EQUAL(ut_get_status(), UT_BAD_ARG);
    registry = utSetNamedSystemInRegistry(system, registry, "");
    CU_ASSERT_PTR_NULL(registry);
    CU_ASSERT_EQUAL(ut_get_status(), UT_BAD_ARG);
#ifndef AUTO_CREATE_NAMED_SYSTEMS
    registry = utSetNamedSystemInRegistry(system, registry, "SI");
    CU_ASSERT_PTR_NULL(registry);
    CU_ASSERT_NOT_EQUAL(ut_get_status(), UT_SUCCESS);
    CU_ASSERT_EQUAL(ut_get_status(), UT_UNKNOWN);
    CU_ASSERT_FALSE(utNamedSystemIsInRegistry(system, registry, "SI"));
    CU_ASSERT_EQUAL(ut_get_status(), UT_UNKNOWN);
    CU_ASSERT_EQUAL(ut_add_named_system(system, "SI", UT_ASCII), UT_SUCCESS);
#endif
    registry = utSetNamedSystemInRegistry(system, registry, "SI");
    CU_ASSERT_PTR_NOT_NULL(registry);
    CU_ASSERT_EQUAL(ut_get_status(), UT_SUCCESS);
    CU_ASSERT_TRUE(utNamedSystemIsInRegistry(system, registry, "SI"));

    registry = utSetNamedSystemInRegistry(NULL, registry, "SI");
    CU_ASSERT_PTR_NOT_NULL(registry);
    CU_ASSERT_EQUAL(ut_get_status(), UT_BAD_ARG);
    registry = utSetNamedSystemInRegistry(system, registry, NULL);
    CU_ASSERT_PTR_NOT_NULL(registry);
    CU_ASSERT_EQUAL(ut_get_status(), UT_BAD_ARG);
    registry = utSetNamedSystemInRegistry(system, registry, "");
    CU_ASSERT_PTR_NOT_NULL(registry);
    CU_ASSERT_EQUAL(ut_get_status(), UT_BAD_ARG);

    CU_ASSERT_FALSE(utNamedSystemIsInRegistry(system, registry, "US"));
#ifndef AUTO_CREATE_NAMED_SYSTEMS
    CU_ASSERT_EQUAL(ut_get_status(), UT_UNKNOWN);
    registry = utSetNamedSystemInRegistry(system, registry, "US");
    CU_ASSERT_PTR_NOT_NULL(registry);
    CU_ASSERT_EQUAL(ut_get_status(), UT_UNKNOWN);
    CU_ASSERT_EQUAL(ut_add_named_system(system, "US", UT_ASCII), UT_SUCCESS);
#endif
    registry = utSetNamedSystemInRegistry(system, registry, "US");
    CU_ASSERT_PTR_NOT_NULL(registry);
    CU_ASSERT_EQUAL(ut_get_status(), UT_SUCCESS);
    CU_ASSERT_TRUE(utNamedSystemIsInRegistry(system, registry, "SI"));
    CU_ASSERT_TRUE(utNamedSystemIsInRegistry(system, registry, "US"));

    CU_ASSERT_EQUAL(utRemoveNamedSystemFromRegistry(NULL,registry,"SI"), UT_BAD_ARG);
    CU_ASSERT_EQUAL(utRemoveNamedSystemFromRegistry(system,registry,NULL), UT_BAD_ARG);
    CU_ASSERT_EQUAL(utRemoveNamedSystemFromRegistry(system,registry,""), UT_BAD_ARG);

    CU_ASSERT_EQUAL(utRemoveNamedSystemFromRegistry(system,registry,"NoSuch System"), UT_UNKNOWN);

    CU_ASSERT_EQUAL(utRemoveNamedSystemFromRegistry(system,NULL,"SI"), UT_SUCCESS);
    CU_ASSERT_TRUE(utNamedSystemIsInRegistry(system, registry, "SI"));
    CU_ASSERT_EQUAL(utRemoveNamedSystemFromRegistry(system,registry,"SI"), UT_SUCCESS);
    CU_ASSERT_FALSE(utNamedSystemIsInRegistry(system, registry, "SI"));
    CU_ASSERT_EQUAL(utRemoveNamedSystemFromRegistry(system,registry,"SI"), UT_SUCCESS);
    CU_ASSERT_FALSE(utNamedSystemIsInRegistry(system, registry, "SI"));
    CU_ASSERT_TRUE(utNamedSystemIsInRegistry(system, registry, "US"));

#if 0
    // These are not possible with the current implementation since
    // since 'stackRegistry' has an incomplete type 'NamedSystemRegistry'
    // (aka 'struct Bitmap'), i.e., the struct's content is opaque
    // or hidden.
    NamedSystemRegistry stackRegistry;
    utNamedSystemRegistryInit(&stackRegistry);
    CU_ASSERT_PTR_NULL(&stackRegistry);
    CU_ASSERT_FALSE(utNamedSystemIsInRegistry(system, &stackRegistry, "SI"));
    CU_ASSERT_EQUAL(ut_get_status(), UT_UNKNOWN);
#endif

    utNamedSystemRegistryFree(registry);
    ut_free_system(system);
}

// These are for alternate implementation style
static void
test_named_system_registry_location(void)
{
    // I need to add some tests for both NULL and registry=NULL
    NamedSystemRegistry* registry = NULL;
    ut_system*	system = ut_new_system();

    CU_ASSERT_PTR_NOT_NULL(system);
    CU_ASSERT_PTR_NULL(registry);
    CU_ASSERT_FALSE(utNamedSystemIsInRegistryLocation(system, &registry, "SI"));
    CU_ASSERT_EQUAL(ut_get_status(), UT_UNKNOWN);

    utSetNamedSystemInRegistryLocation(NULL, &registry, "SI");
    CU_ASSERT_PTR_NULL(registry);
    CU_ASSERT_EQUAL(ut_get_status(), UT_BAD_ARG);
    utSetNamedSystemInRegistryLocation(system, NULL, "SI");
    CU_ASSERT_PTR_NULL(registry);
    CU_ASSERT_EQUAL(ut_get_status(), UT_BAD_ARG);
    utSetNamedSystemInRegistryLocation(system, &registry, NULL);
    CU_ASSERT_PTR_NULL(registry);
    CU_ASSERT_EQUAL(ut_get_status(), UT_BAD_ARG);
    utSetNamedSystemInRegistryLocation(system, &registry, "");
    CU_ASSERT_PTR_NULL(registry);
    CU_ASSERT_EQUAL(ut_get_status(), UT_BAD_ARG);
#ifndef AUTO_CREATE_NAMED_SYSTEMS
    utSetNamedSystemInRegistryLocation(system, &registry, "SI");
    CU_ASSERT_PTR_NULL(registry);
    CU_ASSERT_NOT_EQUAL(ut_get_status(), UT_SUCCESS);
    CU_ASSERT_EQUAL(ut_get_status(), UT_UNKNOWN);
    CU_ASSERT_FALSE(utNamedSystemIsInRegistryLocation(system, &registry, "SI"));
    CU_ASSERT_EQUAL(ut_add_named_system(system, "SI", UT_ASCII), UT_SUCCESS);
#endif
    utSetNamedSystemInRegistryLocation(system, &registry, "SI");
    CU_ASSERT_PTR_NOT_NULL(registry);
    CU_ASSERT_EQUAL(ut_get_status(), UT_SUCCESS);
    CU_ASSERT_NOT_EQUAL(ut_get_status(), UT_UNKNOWN);
    CU_ASSERT_TRUE(utNamedSystemIsInRegistryLocation(system, &registry, "SI"));

    utSetNamedSystemInRegistryLocation(NULL, &registry, "SI");
    CU_ASSERT_PTR_NOT_NULL(registry);
    CU_ASSERT_EQUAL(ut_get_status(), UT_BAD_ARG);
    utSetNamedSystemInRegistryLocation(system, &registry, NULL);
    CU_ASSERT_PTR_NOT_NULL(registry);
    CU_ASSERT_EQUAL(ut_get_status(), UT_BAD_ARG);
    utSetNamedSystemInRegistryLocation(system, &registry, "");
    CU_ASSERT_PTR_NOT_NULL(registry);
    CU_ASSERT_EQUAL(ut_get_status(), UT_BAD_ARG);

    CU_ASSERT_FALSE(utNamedSystemIsInRegistryLocation(system, &registry, "US"));
#ifndef AUTO_CREATE_NAMED_SYSTEMS
    CU_ASSERT_EQUAL(ut_get_status(), UT_UNKNOWN);
    utSetNamedSystemInRegistryLocation(system, &registry, "US");
    CU_ASSERT_PTR_NOT_NULL(registry);
    CU_ASSERT_EQUAL(ut_get_status(), UT_UNKNOWN);
    CU_ASSERT_EQUAL(ut_add_named_system(system, "US", UT_ASCII), UT_SUCCESS);
#endif
    utSetNamedSystemInRegistryLocation(system, &registry, "US");
    CU_ASSERT_PTR_NOT_NULL(registry);
    CU_ASSERT_EQUAL(ut_get_status(), UT_SUCCESS);
    CU_ASSERT_TRUE(utNamedSystemIsInRegistryLocation(system, &registry, "SI"));
    CU_ASSERT_TRUE(utNamedSystemIsInRegistryLocation(system, &registry, "US"));

    CU_ASSERT_EQUAL(utRemoveNamedSystemFromRegistryLocation(NULL,&registry,"SI"), UT_BAD_ARG);
    CU_ASSERT_EQUAL(utRemoveNamedSystemFromRegistryLocation(system,&registry,NULL), UT_BAD_ARG);
    CU_ASSERT_EQUAL(utRemoveNamedSystemFromRegistryLocation(system,&registry,""), UT_BAD_ARG);

    CU_ASSERT_EQUAL(utRemoveNamedSystemFromRegistryLocation(system,&registry,"NoSuch System"), UT_UNKNOWN);

    CU_ASSERT_EQUAL(utRemoveNamedSystemFromRegistryLocation(system,NULL,"SI"), UT_BAD_ARG);
    CU_ASSERT_TRUE(utNamedSystemIsInRegistryLocation(system, &registry, "SI"));
    CU_ASSERT_EQUAL(utRemoveNamedSystemFromRegistryLocation(system,&registry,"SI"), UT_SUCCESS);
    CU_ASSERT_FALSE(utNamedSystemIsInRegistryLocation(system, &registry, "SI"));
    CU_ASSERT_EQUAL(utRemoveNamedSystemFromRegistryLocation(system,&registry,"SI"), UT_SUCCESS);
    CU_ASSERT_FALSE(utNamedSystemIsInRegistryLocation(system, &registry, "SI"));
    CU_ASSERT_TRUE(utNamedSystemIsInRegistryLocation(system, &registry, "US"));

#if 0
    // These are not possible with the current implementation since
    // since 'stackRegistry' has an incomplete type 'NamedSystemRegistry'
    // (aka 'struct Bitmap'), i.e., the struct's content is opaque
    // or hidden.
    NamedSystemRegistry stackRegistry;
    utNamedSystemRegistryInit(&stackRegistry);
    CU_ASSERT_PTR_NULL(&stackRegistry);
    CU_ASSERT_FALSE(utNamedSystemIsInRegistry(system, &stackRegistry, "SI"));
    CU_ASSERT_EQUAL(ut_get_status(), UT_UNKNOWN);
#endif

    utNamedSystemRegistryLocationFree(&registry);
    CU_ASSERT_PTR_NULL(registry);
    ut_free_system(system);
}

static void
test_named_system_public_interface(void)
{
    ut_system*	system	= NULL;
    ut_unit*	unit1	= NULL;
    ut_unit*	unit2	= NULL;
    ut_unit*	unit3	= NULL;
    ut_unit*	unit4	= NULL;
    int	len;

    system = ut_new_system();
    CU_ASSERT_PTR_NOT_NULL(system);

    unit1 = ut_new_base_unit( system );
    CU_ASSERT_PTR_NOT_NULL(unit1);
    unit2 = ut_new_dimensionless_unit( system );
    CU_ASSERT_PTR_NOT_NULL(unit2);
    unit3 = ut_clone( unit1 );
    CU_ASSERT_PTR_NOT_NULL(unit3);
    unit4 = ut_new_base_unit( system );
    CU_ASSERT_PTR_NOT_NULL(unit4);

    CU_ASSERT_FALSE(ut_is_in_named_system(NULL,NULL));
    CU_ASSERT_EQUAL(ut_get_status(), UT_BAD_ARG);
    CU_ASSERT_FALSE(ut_is_in_named_system(NULL,""));
    CU_ASSERT_EQUAL(ut_get_status(), UT_BAD_ARG);
    CU_ASSERT_FALSE(ut_is_in_named_system(NULL,"SI"));
    CU_ASSERT_EQUAL(ut_get_status(), UT_BAD_ARG);
    CU_ASSERT_FALSE(ut_is_in_named_system(unit1,NULL));
    CU_ASSERT_EQUAL(ut_get_status(), UT_BAD_ARG);
    CU_ASSERT_FALSE(ut_is_in_named_system(unit1,""));
    CU_ASSERT_EQUAL(ut_get_status(), UT_BAD_ARG);

    CU_ASSERT_FALSE(ut_is_in_named_system(unit1,"NoSuchSystem"));
    CU_ASSERT_EQUAL(ut_get_status(), UT_UNKNOWN);

    CU_ASSERT_EQUAL(ut_add_unit_to_named_system(NULL,NULL), UT_BAD_ARG);
    CU_ASSERT_EQUAL(ut_add_unit_to_named_system(NULL,"SI"), UT_BAD_ARG);
    CU_ASSERT_EQUAL(ut_add_unit_to_named_system(unit1,NULL), UT_BAD_ARG);
    CU_ASSERT_EQUAL(ut_add_unit_to_named_system(unit1,""), UT_BAD_ARG);

    CU_ASSERT_FALSE(ut_is_in_named_system(unit1,"SI"));
    CU_ASSERT_EQUAL(ut_get_status(), UT_UNKNOWN); // UT_UNKNOWN since "SI" has not been added
    CU_ASSERT_FALSE(ut_is_in_named_system(unit2,"SI"));
    CU_ASSERT_EQUAL(ut_get_status(), UT_UNKNOWN); // UT_UNKNOWN since "SI" has not been added
    CU_ASSERT_FALSE(ut_is_in_named_system(unit3,"SI"));
    CU_ASSERT_EQUAL(ut_get_status(), UT_UNKNOWN); // UT_UNKNOWN since "SI" has not been added
#ifndef AUTO_CREATE_NAMED_SYSTEMS
    CU_ASSERT_EQUAL(ut_add_unit_to_named_system(unit1,"SI"), UT_UNKNOWN);
    CU_ASSERT_EQUAL(ut_add_named_system(system, "SI", UT_ASCII), UT_SUCCESS);
#endif
    CU_ASSERT_EQUAL(ut_add_unit_to_named_system(unit1,"SI"), UT_SUCCESS);
    CU_ASSERT_TRUE(ut_is_in_named_system(unit1,"SI"));
    CU_ASSERT_EQUAL(ut_get_status(), UT_SUCCESS);
    CU_ASSERT_FALSE(ut_is_in_named_system(unit2,"SI"));
    CU_ASSERT_EQUAL(ut_get_status(), UT_SUCCESS);
    CU_ASSERT_TRUE(ut_is_in_named_system(unit3,"SI"));
    CU_ASSERT_EQUAL(ut_get_status(), UT_SUCCESS);
    CU_ASSERT_EQUAL(ut_add_unit_to_named_system(unit1,"SI"), UT_SUCCESS);
    CU_ASSERT_TRUE(ut_is_in_named_system(unit1,"SI"));
    CU_ASSERT_EQUAL(ut_get_status(), UT_SUCCESS);
    CU_ASSERT_FALSE(ut_is_in_named_system(unit2,"SI"));
    CU_ASSERT_EQUAL(ut_get_status(), UT_SUCCESS);
    CU_ASSERT_TRUE(ut_is_in_named_system(unit3,"SI"));
    CU_ASSERT_EQUAL(ut_get_status(), UT_SUCCESS);

    CU_ASSERT_FALSE(ut_is_in_named_system(unit1,"US"));
    CU_ASSERT_EQUAL(ut_get_status(), UT_UNKNOWN);	// This is UNKNOWN since "US" has not been added
    CU_ASSERT_FALSE(ut_is_in_named_system(unit2,"US"));
    CU_ASSERT_EQUAL(ut_get_status(), UT_UNKNOWN);	// This is UNKNOWN since "US" has not been added
    CU_ASSERT_FALSE(ut_is_in_named_system(unit3,"US"));
    CU_ASSERT_EQUAL(ut_get_status(), UT_UNKNOWN);	// This is UNKNOWN since "US" has not been added
#ifndef AUTO_CREATE_NAMED_SYSTEMS
    CU_ASSERT_EQUAL(ut_add_unit_to_named_system(unit1,"US"), UT_UNKNOWN);
    CU_ASSERT_EQUAL(ut_add_named_system(system, "US", UT_ASCII), UT_SUCCESS);
#endif
    CU_ASSERT_EQUAL(ut_add_unit_to_named_system(unit1,"US"), UT_SUCCESS);
    CU_ASSERT_TRUE(ut_is_in_named_system(unit1,"US"));
    CU_ASSERT_EQUAL(ut_get_status(), UT_SUCCESS);
    CU_ASSERT_FALSE(ut_is_in_named_system(unit2,"US"));
    CU_ASSERT_EQUAL(ut_get_status(), UT_SUCCESS);
    CU_ASSERT_TRUE(ut_is_in_named_system(unit3,"US"));
    CU_ASSERT_EQUAL(ut_get_status(), UT_SUCCESS);
    CU_ASSERT_EQUAL(ut_add_unit_to_named_system(unit2,"US"), UT_SUCCESS);
    CU_ASSERT_TRUE(ut_is_in_named_system(unit1,"SI"));
    CU_ASSERT_EQUAL(ut_get_status(), UT_SUCCESS);
    CU_ASSERT_FALSE(ut_is_in_named_system(unit2,"SI"));
    CU_ASSERT_EQUAL(ut_get_status(), UT_SUCCESS);
    CU_ASSERT_TRUE(ut_is_in_named_system(unit3,"SI"));
    CU_ASSERT_EQUAL(ut_get_status(), UT_SUCCESS);
    CU_ASSERT_TRUE(ut_is_in_named_system(unit1,"US"));
    CU_ASSERT_EQUAL(ut_get_status(), UT_SUCCESS);
    CU_ASSERT_TRUE(ut_is_in_named_system(unit2,"US"));
    CU_ASSERT_EQUAL(ut_get_status(), UT_SUCCESS);
    CU_ASSERT_TRUE(ut_is_in_named_system(unit3,"US"));
    CU_ASSERT_EQUAL(ut_get_status(), UT_SUCCESS);

    ut_string_list* namedSystems;
    CU_ASSERT_PTR_NULL(ut_get_named_systems_for_unit( NULL ));
    CU_ASSERT_EQUAL(ut_get_status(), UT_BAD_ARG);
    namedSystems = ut_get_named_systems_for_unit( unit4 );
    CU_ASSERT_PTR_NOT_NULL(namedSystems);
    CU_ASSERT_EQUAL(ut_string_list_length(namedSystems),0);
    ut_string_list_free( namedSystems );
    namedSystems = ut_get_named_systems_for_unit( unit1 );
    CU_ASSERT_PTR_NOT_NULL(namedSystems);
    CU_ASSERT_EQUAL(ut_string_list_length(namedSystems),2);
    if (ut_string_list_element(namedSystems,0))
	CU_ASSERT_STRING_EQUAL(ut_string_list_element(namedSystems,0), "SI");
    if (ut_string_list_element(namedSystems,1))
	CU_ASSERT_STRING_EQUAL(ut_string_list_element(namedSystems,1), "US");
    ut_string_list_free( namedSystems );
    namedSystems = ut_get_named_systems_for_unit( unit2 );
    CU_ASSERT_PTR_NOT_NULL(namedSystems);
    CU_ASSERT_EQUAL(ut_string_list_length(namedSystems),1);
    if (ut_string_list_element(namedSystems,0))
	CU_ASSERT_STRING_EQUAL(ut_string_list_element(namedSystems,0), "US");
    ut_string_list_free( namedSystems );

    CU_ASSERT_EQUAL(ut_remove_unit_from_named_system(NULL,NULL), UT_BAD_ARG);
    CU_ASSERT_EQUAL(ut_remove_unit_from_named_system(NULL,"NoSuchSystem"), UT_BAD_ARG);
    CU_ASSERT_EQUAL(ut_remove_unit_from_named_system(NULL,"US"), UT_BAD_ARG);
    CU_ASSERT_EQUAL(ut_remove_unit_from_named_system(unit2,NULL), UT_BAD_ARG);
    CU_ASSERT_EQUAL(ut_remove_unit_from_named_system(unit2,"NoSuchSystem"), UT_UNKNOWN);
    CU_ASSERT_EQUAL(ut_remove_unit_from_named_system(unit4,"US"), UT_SUCCESS);
    CU_ASSERT_EQUAL(ut_remove_unit_from_named_system(unit2,"US"), UT_SUCCESS);
    CU_ASSERT_EQUAL(ut_remove_unit_from_named_system(unit2,"US"), UT_SUCCESS);
    CU_ASSERT_TRUE(ut_is_in_named_system(unit1,"SI"));
    CU_ASSERT_FALSE(ut_is_in_named_system(unit2,"SI"));
    CU_ASSERT_TRUE(ut_is_in_named_system(unit3,"SI"));
    CU_ASSERT_FALSE(ut_is_in_named_system(unit4,"SI"));
    CU_ASSERT_TRUE(ut_is_in_named_system(unit1,"US"));
    CU_ASSERT_FALSE(ut_is_in_named_system(unit2,"US"));
    CU_ASSERT_TRUE(ut_is_in_named_system(unit3,"US"));
    CU_ASSERT_FALSE(ut_is_in_named_system(unit4,"US"));

    ut_free(unit1);
    ut_free(unit2);
    ut_free(unit3);
    ut_free(unit4);
    ut_free_system(system);
}

static void
test_named_system_xml(void)
{
    ut_system*	system	= NULL;
    ut_string_list* namedSystems = NULL;
    ut_unit* unit;

    system = ut_read_xml("./xmlSuccesses/named_systems.xml");
    CU_ASSERT_NOT_EQUAL(ut_get_status(), UT_PARSE);
    CU_ASSERT_EQUAL(ut_get_status(), UT_SUCCESS);
    CU_ASSERT_PTR_NOT_NULL(system);

    namedSystems = ut_get_named_systems( system );
    CU_ASSERT_EQUAL(ut_get_status(), UT_SUCCESS);
    CU_ASSERT_PTR_NOT_NULL(namedSystems);
    CU_ASSERT_EQUAL(ut_string_list_length(namedSystems),4);
    if (ut_string_list_element(namedSystems,0))
	CU_ASSERT_STRING_EQUAL(ut_string_list_element(namedSystems,0), "avoirdupois");
    if (ut_string_list_element(namedSystems,1))
	CU_ASSERT_STRING_EQUAL(ut_string_list_element(namedSystems,1), "SI");
    if (ut_string_list_element(namedSystems,2))
	CU_ASSERT_STRING_EQUAL(ut_string_list_element(namedSystems,2), "SI-base");
    if (ut_string_list_element(namedSystems,3))
	CU_ASSERT_STRING_EQUAL(ut_string_list_element(namedSystems,3), "US");
    ut_string_list_free(namedSystems);

    namedSystems = ut_get_named_system_aliases(system, NULL);
    CU_ASSERT_EQUAL(ut_get_status(), UT_SUCCESS);
    CU_ASSERT_PTR_NOT_NULL(namedSystems);
    CU_ASSERT_EQUAL(ut_string_list_length(namedSystems),4);
    if (ut_string_list_element(namedSystems,0))
	CU_ASSERT_STRING_EQUAL(ut_string_list_element(namedSystems,0), "avoirdupois");
    if (ut_string_list_element(namedSystems,1))
	CU_ASSERT_STRING_EQUAL(ut_string_list_element(namedSystems,1), "SI");
    if (ut_string_list_element(namedSystems,2))
	CU_ASSERT_STRING_EQUAL(ut_string_list_element(namedSystems,2), "SI-base");
    if (ut_string_list_element(namedSystems,3))
	CU_ASSERT_STRING_EQUAL(ut_string_list_element(namedSystems,3), "US");
    ut_string_list_free(namedSystems);

    namedSystems = ut_get_named_system_aliases(system, "");
    CU_ASSERT_EQUAL(ut_get_status(), UT_SUCCESS);
    CU_ASSERT_PTR_NOT_NULL(namedSystems);
    CU_ASSERT_EQUAL(ut_string_list_length(namedSystems),4);
    if (ut_string_list_element(namedSystems,0))
	CU_ASSERT_STRING_EQUAL(ut_string_list_element(namedSystems,0), "avoirdupois");
    if (ut_string_list_element(namedSystems,1))
	CU_ASSERT_STRING_EQUAL(ut_string_list_element(namedSystems,1), "SI");
    if (ut_string_list_element(namedSystems,2))
	CU_ASSERT_STRING_EQUAL(ut_string_list_element(namedSystems,2), "SI-base");
    if (ut_string_list_element(namedSystems,3))
	CU_ASSERT_STRING_EQUAL(ut_string_list_element(namedSystems,3), "US");
    ut_string_list_free(namedSystems);

    namedSystems = ut_get_named_system_aliases(system, "us");
    CU_ASSERT_PTR_NOT_NULL(namedSystems);
    CU_ASSERT_EQUAL(ut_string_list_length(namedSystems),1);
    if (ut_string_list_element(namedSystems,0))
	CU_ASSERT_STRING_EQUAL(ut_string_list_element(namedSystems,0), "US");
    ut_string_list_free(namedSystems);

    unit = ut_get_unit_by_name(system,"kilogram");
    CU_ASSERT_EQUAL(ut_get_status(), UT_SUCCESS);
    CU_ASSERT_PTR_NOT_NULL(unit);
    CU_ASSERT_TRUE(ut_is_in_named_system(unit,"si"));
    CU_ASSERT_TRUE(ut_is_in_named_system(unit,"si-BaSe"));
    CU_ASSERT_FALSE(ut_is_in_named_system(unit,"us"));
    CU_ASSERT_FALSE(ut_is_in_named_system(unit,"avoirdupois"));
    CU_ASSERT_FALSE(ut_is_in_named_system(unit,"noSuchSystem"));
    namedSystems = ut_get_named_systems_for_unit(unit);
    CU_ASSERT_EQUAL(ut_get_status(), UT_SUCCESS);
    CU_ASSERT_PTR_NOT_NULL(namedSystems);
    CU_ASSERT_EQUAL(ut_string_list_length(namedSystems),2);
    if (ut_string_list_element(namedSystems,0))
	CU_ASSERT_STRING_EQUAL(ut_string_list_element(namedSystems,0), "SI");
    if (ut_string_list_element(namedSystems,1))
	CU_ASSERT_STRING_EQUAL(ut_string_list_element(namedSystems,1), "SI-base");
    ut_string_list_free(namedSystems);
    ut_free(unit);

    // systems="avoirdupois, US "
    // >pound<
    // >avoirdupois_pound<
    unit = ut_get_unit_by_name(system,"avoirdupois_pound");
    CU_ASSERT_EQUAL(ut_get_status(), UT_SUCCESS);
    CU_ASSERT_PTR_NOT_NULL(unit);
    CU_ASSERT_FALSE(ut_is_in_named_system(unit,"si"));
    CU_ASSERT_FALSE(ut_is_in_named_system(unit,"si-BaSe"));
    CU_ASSERT_TRUE(ut_is_in_named_system(unit,"us"));
    CU_ASSERT_TRUE(ut_is_in_named_system(unit,"avoirdupois"));
    CU_ASSERT_FALSE(ut_is_in_named_system(unit,"noSuchSystem"));
    namedSystems = ut_get_named_systems_for_unit(unit);
    CU_ASSERT_EQUAL(ut_get_status(), UT_SUCCESS);
    CU_ASSERT_PTR_NOT_NULL(namedSystems);
    CU_ASSERT_EQUAL(ut_string_list_length(namedSystems),2);
    if (ut_string_list_element(namedSystems,0))
	CU_ASSERT_STRING_EQUAL(ut_string_list_element(namedSystems,0), "avoirdupois");
    if (ut_string_list_element(namedSystems,1))
	CU_ASSERT_STRING_EQUAL(ut_string_list_element(namedSystems,1), "US");
    ut_string_list_free(namedSystems);
    ut_free(unit);

    unit = ut_get_unit_by_symbol(system,"ppb");
    CU_ASSERT_EQUAL(ut_get_status(), UT_SUCCESS);
    CU_ASSERT_PTR_NOT_NULL(unit);
    CU_ASSERT_FALSE(ut_is_in_named_system(unit,"si"));
    CU_ASSERT_NOT_EQUAL(ut_get_status(), UT_UNKNOWN);
    CU_ASSERT_EQUAL(ut_get_status(), UT_SUCCESS);
    CU_ASSERT_FALSE(ut_is_in_named_system(unit,"si-BaSe"));
    CU_ASSERT_NOT_EQUAL(ut_get_status(), UT_UNKNOWN);
    CU_ASSERT_EQUAL(ut_get_status(), UT_SUCCESS);
    CU_ASSERT_FALSE(ut_is_in_named_system(unit,"us"));
    CU_ASSERT_NOT_EQUAL(ut_get_status(), UT_UNKNOWN);
    CU_ASSERT_EQUAL(ut_get_status(), UT_SUCCESS);
    CU_ASSERT_FALSE(ut_is_in_named_system(unit,"avoirdupois"));
    CU_ASSERT_NOT_EQUAL(ut_get_status(), UT_UNKNOWN);
    CU_ASSERT_EQUAL(ut_get_status(), UT_SUCCESS);
    CU_ASSERT_FALSE(ut_is_in_named_system(unit,"noSuchSystem"));
    CU_ASSERT_EQUAL(ut_get_status(), UT_UNKNOWN);
    namedSystems = ut_get_named_systems_for_unit(unit);
    CU_ASSERT_NOT_EQUAL(ut_get_status(), UT_BAD_ARG);
    CU_ASSERT_NOT_EQUAL(ut_get_status(), UT_UNKNOWN);
    CU_ASSERT_EQUAL(ut_get_status(), UT_SUCCESS);
    CU_ASSERT_PTR_NOT_NULL(namedSystems);
    CU_ASSERT_EQUAL(ut_string_list_length(namedSystems),0);
    ut_string_list_free(namedSystems);
    ut_free(unit);

    ut_free_system(system);
}

static void
test_ut_unit_from_named_system_convertible_with_unit(void)
{
    // Create a controlled unit system...
    ut_system*	system	= NULL;
    ut_unit*	kilometer	= NULL;
    ut_unit*	meter		= NULL;
    ut_unit*	centimeter	= NULL;
    ut_unit*	millimeter	= NULL;
    ut_unit*	mile		= NULL;
    ut_unit*	yard		= NULL;
    ut_unit*	foot		= NULL;
    ut_unit*	inch		= NULL;
    ut_unit*	liter		= NULL;
    ut_unit*	milliliter	= NULL;
    ut_unit*	gallon		= NULL;
    ut_unit*	quart		= NULL;
    ut_unit*	pint		= NULL;
    ut_unit*	ounce		= NULL;
    ut_unit*	teaspoon	= NULL;
    ut_unit*	testUnit	= NULL;

    CU_ASSERT_PTR_NULL(system);
    CU_ASSERT_PTR_NULL(ut_unit_from_named_system_convertible_with_unit(NULL,meter));
    CU_ASSERT_EQUAL(ut_get_status(), UT_BAD_ARG);
    CU_ASSERT_PTR_NULL(ut_unit_from_named_system_convertible_with_unit("",meter));
    CU_ASSERT_EQUAL(ut_get_status(), UT_BAD_ARG);
    CU_ASSERT_PTR_NULL(ut_unit_from_named_system_convertible_with_unit("US",NULL));
    CU_ASSERT_EQUAL(ut_get_status(), UT_BAD_ARG);
    system = ut_new_system();
    CU_ASSERT_PTR_NOT_NULL(system);
    CU_ASSERT_PTR_NULL(ut_unit_from_named_system_convertible_with_unit(NULL,meter));
    CU_ASSERT_EQUAL(ut_get_status(), UT_BAD_ARG);
    CU_ASSERT_PTR_NULL(ut_unit_from_named_system_convertible_with_unit("",meter));
    CU_ASSERT_EQUAL(ut_get_status(), UT_BAD_ARG);
    CU_ASSERT_PTR_NULL(ut_unit_from_named_system_convertible_with_unit("US",NULL));
    CU_ASSERT_EQUAL(ut_get_status(), UT_BAD_ARG);

    CU_ASSERT_EQUAL( ut_add_named_system(system,"SI",UT_ASCII), UT_SUCCESS);
    CU_ASSERT_EQUAL( ut_add_named_system(system,"US",UT_ASCII), UT_SUCCESS);
    CU_ASSERT_EQUAL( ut_add_named_system(system,"p1",UT_ASCII), UT_SUCCESS);
    CU_ASSERT_PTR_NULL(ut_unit_from_named_system_convertible_with_unit(NULL,meter));
    CU_ASSERT_EQUAL(ut_get_status(), UT_BAD_ARG);
    CU_ASSERT_PTR_NULL(ut_unit_from_named_system_convertible_with_unit("",meter));
    CU_ASSERT_EQUAL(ut_get_status(), UT_BAD_ARG);
    CU_ASSERT_PTR_NULL(ut_unit_from_named_system_convertible_with_unit("US",NULL));
    CU_ASSERT_EQUAL(ut_get_status(), UT_BAD_ARG);

    meter = ut_new_base_unit(system);
    CU_ASSERT_PTR_NOT_NULL(meter);
    CU_ASSERT_PTR_NULL(ut_unit_from_named_system_convertible_with_unit(NULL,meter));
    CU_ASSERT_EQUAL(ut_get_status(), UT_BAD_ARG);
    CU_ASSERT_PTR_NULL(ut_unit_from_named_system_convertible_with_unit("",meter));
    CU_ASSERT_EQUAL(ut_get_status(), UT_BAD_ARG);
    CU_ASSERT_PTR_NULL(ut_unit_from_named_system_convertible_with_unit("US",NULL));
    CU_ASSERT_EQUAL(ut_get_status(), UT_BAD_ARG);
    CU_ASSERT_EQUAL( ut_map_name_to_unit("meter",UT_ASCII,meter), UT_SUCCESS);
    CU_ASSERT_EQUAL( ut_map_symbol_to_unit("m",UT_ASCII,meter), UT_SUCCESS);
    CU_ASSERT_EQUAL( ut_map_unit_to_name(meter,"meter",UT_ASCII), UT_SUCCESS);
    CU_ASSERT_EQUAL( ut_map_unit_to_symbol(meter,"m",UT_ASCII), UT_SUCCESS);
    CU_ASSERT_EQUAL( ut_add_unit_to_named_system(meter,"SI"), UT_SUCCESS);
    kilometer = ut_scale( 1000.0, meter );
    CU_ASSERT_PTR_NOT_NULL(kilometer);
    CU_ASSERT_EQUAL( ut_add_unit_to_named_system(kilometer,"SI"), UT_SUCCESS);
    centimeter = ut_scale( 0.01, meter );
    CU_ASSERT_PTR_NOT_NULL(centimeter);
    CU_ASSERT_EQUAL( ut_add_unit_to_named_system(centimeter,"SI"), UT_SUCCESS);
    millimeter = ut_scale( 0.001, meter );
    CU_ASSERT_PTR_NOT_NULL(millimeter);
    CU_ASSERT_EQUAL( ut_add_unit_to_named_system(millimeter,"SI"), UT_SUCCESS);
    inch = ut_scale( 2.54, centimeter );
    CU_ASSERT_PTR_NOT_NULL(inch);
    CU_ASSERT_EQUAL( ut_map_name_to_unit("inch",UT_ASCII,inch), UT_SUCCESS);
    CU_ASSERT_EQUAL( ut_map_symbol_to_unit("in",UT_ASCII,inch), UT_SUCCESS);
    CU_ASSERT_EQUAL( ut_map_unit_to_name(inch,"inch",UT_ASCII), UT_SUCCESS);
    CU_ASSERT_EQUAL( ut_map_unit_to_symbol(inch,"in",UT_ASCII), UT_SUCCESS);
    CU_ASSERT_EQUAL( ut_add_unit_to_named_system(inch,"US"), UT_SUCCESS);
    foot = ut_scale( 12.0, inch );
    CU_ASSERT_PTR_NOT_NULL(foot);
    CU_ASSERT_EQUAL( ut_map_name_to_unit("foot",UT_ASCII,foot), UT_SUCCESS);
    CU_ASSERT_EQUAL( ut_map_symbol_to_unit("ft",UT_ASCII,foot), UT_SUCCESS);
    CU_ASSERT_EQUAL( ut_map_unit_to_name(foot,"foot",UT_ASCII), UT_SUCCESS);
    CU_ASSERT_EQUAL( ut_map_unit_to_symbol(foot,"ft",UT_ASCII), UT_SUCCESS);
    CU_ASSERT_EQUAL( ut_add_unit_to_named_system(foot,"US"), UT_SUCCESS);
    yard = ut_scale( 3.0, foot );
    CU_ASSERT_PTR_NOT_NULL(yard);
    CU_ASSERT_EQUAL( ut_map_name_to_unit("yard",UT_ASCII,yard), UT_SUCCESS);
    CU_ASSERT_EQUAL( ut_map_symbol_to_unit("yd",UT_ASCII,yard), UT_SUCCESS);
    CU_ASSERT_EQUAL( ut_map_unit_to_name(yard,"yard",UT_ASCII), UT_SUCCESS);
    CU_ASSERT_EQUAL( ut_map_unit_to_symbol(yard,"yd",UT_ASCII), UT_SUCCESS);
    CU_ASSERT_EQUAL( ut_add_unit_to_named_system(yard,"US"), UT_SUCCESS);
    mile = ut_scale( 5280, foot );
    CU_ASSERT_PTR_NOT_NULL(mile);
    CU_ASSERT_EQUAL( ut_map_name_to_unit("mile",UT_ASCII,mile), UT_SUCCESS);
    CU_ASSERT_EQUAL( ut_map_symbol_to_unit("mi",UT_ASCII,mile), UT_SUCCESS);
    CU_ASSERT_EQUAL( ut_map_unit_to_name(mile,"mi",UT_ASCII), UT_SUCCESS);
    CU_ASSERT_EQUAL( ut_map_unit_to_symbol(mile,"yd",UT_ASCII), UT_SUCCESS);
    CU_ASSERT_EQUAL( ut_add_unit_to_named_system(mile,"US"), UT_SUCCESS);

    liter = ut_new_base_unit(system);
    CU_ASSERT_PTR_NOT_NULL(liter);
    CU_ASSERT_EQUAL( ut_map_name_to_unit("liter",UT_ASCII,liter), UT_SUCCESS);
    CU_ASSERT_EQUAL( ut_map_symbol_to_unit("l",UT_ASCII,liter), UT_SUCCESS);
    CU_ASSERT_EQUAL( ut_map_unit_to_name(liter,"liter",UT_ASCII), UT_SUCCESS);
    CU_ASSERT_EQUAL( ut_map_unit_to_symbol(liter,"l",UT_ASCII), UT_SUCCESS);
    CU_ASSERT_EQUAL( ut_add_unit_to_named_system(liter,"SI"), UT_SUCCESS);

    CU_ASSERT_PTR_NULL(ut_unit_from_named_system_convertible_with_unit("BadSystem",meter));
    CU_ASSERT_EQUAL(ut_get_status(), UT_UNKNOWN);

    testUnit = ut_unit_from_named_system_convertible_with_unit("US",meter);
    CU_ASSERT_EQUAL(ut_get_status(), UT_SUCCESS);
    CU_ASSERT_PTR_NOT_NULL(testUnit);
    CU_ASSERT_EQUAL(ut_compare(testUnit,yard), 0);
    ut_free(testUnit);

    testUnit = ut_unit_from_named_system_convertible_with_unit("US",centimeter);
    CU_ASSERT_EQUAL(ut_get_status(), UT_SUCCESS);
    CU_ASSERT_PTR_NOT_NULL(testUnit);
    CU_ASSERT_EQUAL(ut_compare(testUnit,inch), 0);
    ut_free(testUnit);

    testUnit = ut_unit_from_named_system_convertible_with_unit("US",kilometer);
    CU_ASSERT_EQUAL(ut_get_status(), UT_SUCCESS);
    CU_ASSERT_PTR_NOT_NULL(testUnit);
    CU_ASSERT_EQUAL(ut_compare(testUnit,mile), 0);
    ut_free(testUnit);

    testUnit = ut_unit_from_named_system_convertible_with_unit("SI",kilometer);
    CU_ASSERT_EQUAL(ut_get_status(), UT_SUCCESS);
    CU_ASSERT_PTR_NOT_NULL(testUnit);
    CU_ASSERT_EQUAL(ut_compare(testUnit,kilometer), 0);
    ut_free(testUnit);

    testUnit = ut_unit_from_named_system_convertible_with_unit("SI",mile);
    CU_ASSERT_EQUAL(ut_get_status(), UT_SUCCESS);
    CU_ASSERT_PTR_NOT_NULL(testUnit);
    CU_ASSERT_EQUAL(ut_compare(testUnit,kilometer), 0);
    ut_free(testUnit);

    testUnit = ut_unit_from_named_system_convertible_with_unit("SI",liter);
    CU_ASSERT_EQUAL(ut_get_status(), UT_SUCCESS);
    CU_ASSERT_PTR_NOT_NULL(testUnit);
    CU_ASSERT_EQUAL(ut_compare(testUnit,liter), 0);
    ut_free(testUnit);

    CU_ASSERT_PTR_NOT_NULL(liter);
    testUnit = ut_unit_from_named_system_convertible_with_unit("US",liter);
    CU_ASSERT_EQUAL(ut_get_status(), UT_SUCCESS);
    CU_ASSERT_NOT_EQUAL(ut_get_status(), UT_UNKNOWN);
    CU_ASSERT_NOT_EQUAL(ut_get_status(), UT_BAD_ARG);
    CU_ASSERT_PTR_NULL(testUnit);

    // Should find the unit closest in magnitude to an inch in my
    // private unit group (p1)...
    //
    // The code goes through every potential destination unit,
    // gets a converter from the queried unit to the destination,
    // converts 1.0 query units to destination units,
    // returns the destination whose magnitude is closest to 1.0.
    //
    // So if the query unit is an inch and it is converted to
    // miles it will be 0.000016 miles, if it is converted to
    // centimeters it will be 2.54 centimeters.
    // The magnitude is calculated by taking the abs(log10()) so
    // .000016 is -4.79 and 2.54 is 0.404 and it will therefore
    // pick centimeters (0.4 < 4.79).
    //
    // As an enhancement I could look at trying to maintain
    // values with positive exponents (i.e., I would rather have
    // 100 millimeters than .1 meters). For this I might need to
    // know the value that is being converted... I would also need
    // some sort of rule to decide if 1,000,000 is better than .1
    // or not. To do this, maybe three additional parameters:
    // value, max, and min?
    CU_ASSERT_EQUAL( ut_add_unit_to_named_system(      mile,"p1"), UT_SUCCESS);
    CU_ASSERT_EQUAL( ut_add_unit_to_named_system(centimeter,"p1"), UT_SUCCESS);
    testUnit = ut_unit_from_named_system_convertible_with_unit("p1",inch);
    CU_ASSERT_EQUAL(ut_get_status(), UT_SUCCESS);
    CU_ASSERT_PTR_NOT_NULL(testUnit);
    CU_ASSERT_NOT_EQUAL(ut_compare(testUnit,mile), 0);
    CU_ASSERT_EQUAL(ut_compare(testUnit,centimeter), 0);
    ut_free(testUnit);

    ut_free( kilometer);
    ut_free(     meter);
    ut_free(centimeter);
    ut_free(millimeter);
    ut_free(      mile);
    ut_free(      yard);
    ut_free(      foot);
    ut_free(      inch);
    ut_free(     liter);
    ut_free(milliliter);
    ut_free(    gallon);
    ut_free(     quart);
    ut_free(      pint);
    ut_free(     ounce);
    ut_free(  teaspoon);
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
	    CU_ADD_TEST(testSuite, test_string_list);
	    CU_ADD_TEST(testSuite, test_named_system);
	    CU_ADD_TEST(testSuite, test_named_system_name_getting);
	    CU_ADD_TEST(testSuite, test_bitmap);
	    CU_ADD_TEST(testSuite, test_named_system_registry);
	    CU_ADD_TEST(testSuite, test_named_system_registry_location);
	    CU_ADD_TEST(testSuite, test_named_system_public_interface);
	    CU_ADD_TEST(testSuite, test_named_system_xml);
	    CU_ADD_TEST(testSuite, test_ut_unit_from_named_system_convertible_with_unit);
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
