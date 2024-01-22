#include "test_events.h"
#include "test_utils.h"
#include <stdlib.h>

int main() {
  // Events
  test_Grr_subscribe();

  // Utils
  test_Grr_initHashMap();
  test_Grr_hashMapGet();

  test_Grr_initList();
  test_Grr_listPushBack();
  test_Grr_listGetAtIndex();

  return EXIT_SUCCESS;
}