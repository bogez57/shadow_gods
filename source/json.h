#pragma once

/* Json Types: */
#define Json_False 0
#define Json_True 1
#define Json_NULL 2
#define Json_Number 3
#define Json_String 4
#define Json_Array 5
#define Json_Object 6

#ifndef SPINE_JSON_HAVE_PREV
/* Spine doesn't use the "prev" link in the Json sibling lists. */
#define SPINE_JSON_HAVE_PREV 0
#endif

/* The Json structure: */
struct Json
{
    struct Json* next;
#if SPINE_JSON_HAVE_PREV
    struct Json* prev; /* next/prev allow you to walk array/object chains. Alternatively, use getSize/getItem */
#endif
    struct Json* child; /* An array or object item will have a child pointer pointing to a chain of the items in the array/object. */

    int type; /* The type of the item, as above. */
    int size; /* The number of children. */

    const char* valueString; /* The item's string, if type==Json_String */
    int valueInt; /* The item's number, if type==Json_Number */
    float valueFloat; /* The item's number, if type==Json_Number */

    const char* name; /* The item's name string, if this item is the child of, or is in the list of subitems of an object. */
};
