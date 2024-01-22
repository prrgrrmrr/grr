#ifndef GRR_TEST_EVENTS_H
#define GRR_TEST_EVENTS_H

#include "events.h"
#include <assert.h>

extern Grr_u32 Grr_getEventHandlerSize(GRR_EVENT_TYPE type);
extern Grr_u32 Grr_getEventHandlerCapacity(GRR_EVENT_TYPE type);
extern Grr_eventHandler *Grr_getEventHandlerArray(GRR_EVENT_TYPE type);

void test_Grr_subscribe();

#endif