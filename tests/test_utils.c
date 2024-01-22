#include "test_utils.h"

void test_Grr_initHashMap() {
  GrrHashMap map;
  Grr_initHashMap(&map);
  for (Grr_u32 i = 0; i < 65532; i++)
    assert(map.entries[i].empty);
  GRR_LOG_INFO("PASSED test_Grr_initHashMap\n");
}

void test_Grr_hashMapGet() {
  GrrHashMap map;
  Grr_initHashMap(&map);

  GrrHashMapValue e;
  e.i32 = -5;
  Grr_hashMapPut(&map, "test", e, INT32);
  GrrType t;
  assert(Grr_hashMapGet(&map, "test", &t)->i32 == -5);
  assert(t == INT32);
  GrrHashMapValue x;
  x.boolean = false;
  Grr_hashMapPut(&map, "test", x, BOOLEAN);
  GRR_LOG_INFO("PASSED test_Grr_hashMapGet\n");
}

void test_Grr_initList() {
  GrrList list;
  Grr_initList(&list);
  assert(list.count == 0);
  GRR_LOG_INFO("PASSED test_Grr_initList\n");
}

void test_Grr_listPushBack() {
  // Test pushing another list
  GrrList list;
  Grr_initList(&list);
  GrrHashMapValue v;
  v.list = (GrrList *)malloc(sizeof(GrrList));
  Grr_initList(v.list);
  Grr_listPushBack(&list, v, LIST);
  GrrType t;
  assert(list.count == 1);                                    // Parent list
  assert(Grr_listGetAtIndex(&list, 0, &t)->list->count == 0); // Child list
  GrrHashMapValue w;
  w.map = (GrrHashMap *)malloc(sizeof(GrrHashMap));
  Grr_listPushBack(Grr_listGetAtIndex(&list, 0, &t)->list, w, HASH_MAP);
  Grr_freeList(
      &list); // Should free parent and child lists (including hashmap), should
              // see 3 'free' prints if GRR_DEBUG is enabled
  assert(list.count == 0);
  GRR_LOG_INFO("PASSED test_Grr_listPushBack\n");
}

void test_Grr_listGetAtIndex() {
  GrrList list;
  Grr_initList(&list);
  GrrType t;
  assert(NULL == Grr_listGetAtIndex(&list, 0, &t)); // Empty
  for (Grr_u32 i = 0; i <= LIST_CHUNK_MAX; i++) {
    GrrHashMapValue v;
    v.i32 = i;
    Grr_listPushBack(&list, v, INT32);
  }
  assert(LIST_CHUNK_MAX == Grr_listGetAtIndex(&list, LIST_CHUNK_MAX, &t)->i32);
  assert(INT32 == t);
  GRR_LOG_INFO("PASSED test_Grr_listGetAtIndex\n");
}