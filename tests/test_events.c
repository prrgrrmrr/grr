#include "test_events.h"
#include "logging.h"

void handler1(void *sender, const struct GrrEventData data) {}
void handler2(void *sender, const struct GrrEventData data) {}
void handler3(void *sender, const struct GrrEventData data) {}
void handler4(void *sender, const struct GrrEventData data) {}
void handler5(void *sender, const struct GrrEventData data) {}
void handler6(void *sender, const struct GrrEventData data) {}
void handler7(void *sender, const struct GrrEventData data) {}
void handler8(void *sender, const struct GrrEventData data) {}
void handler9(void *sender, const struct GrrEventData data) {}

void test_Grr_subscribe() {
  int countDoubling = 0;
  Grr_eventHandler handlers[] = {handler1, handler2, handler3,
                                 handler4, handler5, handler6,
                                 handler7, handler8, handler9};
  for (int i = 0; i < sizeof(handlers) / sizeof(Grr_eventHandler); i++) {
    Grr_u32 size = Grr_getEventHandlerSize(GRR_WINDOW_RESIZED);
    Grr_u32 cap = Grr_getEventHandlerCapacity(GRR_WINDOW_RESIZED);
    assert(size <= cap);
    if (size == cap)
      countDoubling++;
    Grr_subscribe(GRR_WINDOW_RESIZED, handlers[i]);
  }
  assert(countDoubling == 5);
  assert(Grr_getEventHandlerCapacity(GRR_WINDOW_RESIZED) == 16);
  assert(Grr_getEventHandlerSize(GRR_WINDOW_RESIZED) ==
         sizeof(handlers) / sizeof(Grr_eventHandler));
  Grr_eventHandler *internalHandlers =
      Grr_getEventHandlerArray(GRR_WINDOW_RESIZED);
  for (Grr_u16 i = 0; i < Grr_getEventHandlerSize(GRR_WINDOW_RESIZED); i++) {
    assert(handlers[i] == internalHandlers[i]);
  }
  GRR_LOG_INFO("PASSED test_Grr_subscribe\n");
}