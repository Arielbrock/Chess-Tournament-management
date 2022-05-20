#ifndef _UTILS_H_
#define _UTILS_H_

#include "map.h"
#include <stdlib.h>

MapKeyElement copyInt(MapKeyElement key);
void freeSimple(MapKeyElement key);
int compare(MapKeyElement key1, MapKeyElement key2);

/**
 * An integer copy function.
 * Used mainly for keys of almost every map.
*/
MapKeyElement copyInt(MapKeyElement key)
{
    if (key == NULL)
    {
        return NULL;
    }
    MapKeyElement new_key = (MapKeyElement)malloc(sizeof(int));
    if (new_key == NULL)
    {
        return NULL;
    }
    *((int*)new_key) = *((int*)key);
    return new_key;
}

/**
 * A simple free function for unmalloced types.
 * Gets a MapKeyElement but can free various items,
 * such as keys (integer, double), Games, etc.
*/
void freeSimple(MapKeyElement item)
{
    free(item);
}

/**
 * A simple compare function for integers.
 * Returns a positive value if key1 is greater.
 * Returns a negative value if key2 is greater.
 * Returns 0 if keys identical.
*/
int compare(MapKeyElement key1, MapKeyElement key2)
{
    if (key1 == NULL && key2 == NULL) 
    {
        return 0;
    }
    if (key1 == NULL) 
    {
        return *((int*)key2);
    }
    if (key2 == NULL)
    {
        return *((int*)key1);
    }
    return *((int*)key1) - *((int*)key2);
}

#endif
