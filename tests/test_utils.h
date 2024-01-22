#ifndef GRR_TEST_UTILS_H
#define GRR_TEST_UTILS_H

#include "utils.h"
#include <assert.h>
#include <string.h>

extern void Grr_initHashMap(GrrHashMap *map);
extern void Grr_hashMapPut(GrrHashMap *map, const Grr_string key,
                           const GrrHashMapValue value, const GrrType type);
extern GrrHashMapValue *Grr_hashMapGet(GrrHashMap *map, const Grr_string key,
                                       GrrType *type);

extern void Grr_initList(GrrList *list);

void test_Grr_initHashMap();
void test_Grr_hashMapGet();
void test_Grr_initList();
void test_Grr_listPushBack();
void test_Grr_listGetAtIndex();

#endif