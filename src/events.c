#include "events.h"
#include "logging.h"
#include <stdlib.h>

Grr_eventHandler *Grr_eventHandlers[GRR_EVENT_TYPE_MAX];
Grr_u64 Grr_eventHandlerCapacitiesAndSizes
    [GRR_EVENT_TYPE_MAX]; // (capacity, size) in a 64-bit unsigned integer

#define GRR_EVENT_HANDLER_CAPACITY(type)                                       \
  (Grr_u32)(Grr_eventHandlerCapacitiesAndSizes[type] >> 32)

#define GRR_EVENT_HANDLER_SIZE(type)                                           \
  (Grr_u32)(Grr_eventHandlerCapacitiesAndSizes[type] & 0xFFFFFFFF)

#define GRR_EVENT_HANDLER_SET_CAPACITY(type, capacity)                         \
  Grr_eventHandlerCapacitiesAndSizes[type] =                                   \
      (Grr_eventHandlerCapacitiesAndSizes[type] & 0xFFFFFFFF) |                \
      ((Grr_u64)(capacity) << 32)

#define GRR_EVENT_HANDLER_SET_SIZE(type, size)                                 \
  Grr_eventHandlerCapacitiesAndSizes[type] =                                   \
      (Grr_eventHandlerCapacitiesAndSizes[type] &                              \
       ((Grr_u64)0xFFFFFFFF << 32)) |                                          \
      (size)

void Grr_initializeEvents() {
  GRR_LOG_INFO("Initialize events\n");
  atexit(Grr_freeEvents);
}

Grr_bool Grr_subscribe(GRR_EVENT_TYPE type, Grr_eventHandler handler) {
  if (type < 0 || type >= GRR_EVENT_TYPE_MAX) {
    GRR_LOG_ERROR("Cannot subscribe to unknown event type %d\n", type);
    return false;
  }

  if (handler == NULL) {
    GRR_LOG_ERROR("Cannot subscribe NULL event handler\n");
    return false;
  }

  Grr_u32 capacity = GRR_EVENT_HANDLER_CAPACITY(type),
          size = GRR_EVENT_HANDLER_SIZE(type);

  if (handler != NULL) {
    for (Grr_u32 i = 0; i < size; i++) {
      if (handler == Grr_eventHandlers[type][i]) {
        GRR_LOG_ERROR("Cannot subscribe handler twice to same event type\n");
        return false;
      }
    }
  } else {
    GRR_LOG_ERROR("Passed NULL event handler!\n");
    return false;
  }

  if (capacity == 0) {
    // Make room for 1 handler initially
    Grr_eventHandlers[type] =
        (Grr_eventHandler *)malloc(sizeof(Grr_eventHandler));
    if (Grr_eventHandlers[type] != NULL)
      GRR_EVENT_HANDLER_SET_CAPACITY(type, 1);
  } else {
    if (size == capacity) {
      // Double capacity if no more space
      Grr_eventHandler *h = (Grr_eventHandler *)malloc(
          sizeof(Grr_eventHandler) * (capacity << 1));

      if (h != NULL) {
        // TODO: double check this
        GRR_EVENT_HANDLER_SET_CAPACITY(type, capacity << 1);
        memcpy(h, Grr_eventHandlers[type], sizeof(Grr_eventHandler) * capacity);
        free(Grr_eventHandlers[type]);
      } else {
        GRR_LOG_CRITICAL("Failed to allocate double capacity for handles\n");
        return false;
      }
      Grr_eventHandlers[type] = h;
    }
  }

  if (Grr_eventHandlers[type] == NULL) {
    GRR_LOG_ERROR("Failed to allocate memory for event handler\n");
    return false;
  }

  Grr_eventHandlers[type][size] = handler;
  GRR_EVENT_HANDLER_SET_SIZE(type, size + 1);
  return true;
}

Grr_bool Grr_unsubscribe(GRR_EVENT_TYPE type, Grr_eventHandler handler) {
  if (type < 0 || type >= GRR_EVENT_TYPE_MAX) {
    GRR_LOG_ERROR("Cannot unsubscribe from unknown event type %d\n", type);
    return false;
  }

  if (handler != NULL) {
    Grr_u32 size = GRR_EVENT_HANDLER_SIZE(type);
    for (Grr_u32 i = 0; i < size; i++) {
      if (handler == Grr_eventHandlers[type][i]) {
        Grr_eventHandlers[type][i] = NULL;
        return true;
      }
    }
  }

  GRR_LOG_ERROR("Cannot unsubscribe non-subscribed event handler\n");
  return false;
}

Grr_bool Grr_sendEvent(GRR_EVENT_TYPE type, void *sender,
                       const GrrEventData data) {
  if (type < 0 || type >= GRR_EVENT_TYPE_MAX) {
    GRR_LOG_ERROR("Cannot send unknown event type %d\n", type);
    return false;
  }

  Grr_u32 size = GRR_EVENT_HANDLER_SIZE(type);

  for (Grr_u32 i = 0; i < size; i++) {
    Grr_eventHandler handler = Grr_eventHandlers[type][i];
    if (handler) {
      handler(sender, data);
    } else {
    }
  }

  return true;
}

void Grr_freeEvents() {
  GRR_LOG_INFO("Free events\n");
  for (Grr_u32 i = 0; i < GRR_EVENT_TYPE_MAX; i++) {
    Grr_eventHandler *block = Grr_eventHandlers[i];
    if (block)
      free(block);
  }
}

// Helpers for facilitating testing of events
Grr_u32 Grr_getEventHandlerSize(GRR_EVENT_TYPE type) {
  return GRR_EVENT_HANDLER_SIZE(type);
}

Grr_u32 Grr_getEventHandlerCapacity(GRR_EVENT_TYPE type) {
  return GRR_EVENT_HANDLER_CAPACITY(type);
}

Grr_eventHandler *Grr_getEventHandlerArray(GRR_EVENT_TYPE type) {
  return Grr_eventHandlers[type];
}

Grr_string Grr_keyToString(GRR_KEY key) {
  const Grr_string keyNames[] = {
      "GRR_KEY_A",
      "GRR_KEY_S",
      "GRR_KEY_D",
      "GRR_KEY_F",
      "GRR_KEY_H",
      "GRR_KEY_G",
      "GRR_KEY_Z",
      "GRR_KEY_X",
      "GRR_KEY_C",
      "GRR_KEY_V",
      "GRR_KEY_B",
      "GRR_KEY_Q",
      "GRR_KEY_W",
      "GRR_KEY_E",
      "GRR_KEY_R",
      "GRR_KEY_Y",
      "GRR_KEY_T",
      "GRR_KEY_1",
      "GRR_KEY_2",
      "GRR_KEY_3",
      "GRR_KEY_4",
      "GRR_KEY_6",
      "GRR_KEY_5",
      "GRR_KEY_EQUALS",
      "GRR_KEY_9",
      "GRR_KEY_7",
      "GRR_KEY_HYPHEN",
      "GRR_KEY_8",
      "GRR_KEY_0",
      "GRR_KEY_CLOSE_BRACKET",
      "GRR_KEY_O",
      "GRR_KEY_U",
      "GRR_KEY_OPEN_BRACKET",
      "GRR_KEY_I",
      "GRR_KEY_P",
      "GRR_KEY_RETURN",
      "GRR_KEY_",
      "GRR_KEY_L",
      "GRR_KEY_J",
      "GRR_KEY_APOSTROPHE",
      "GRR_KEY_K",
      "GRR_KEY_SEMICOLON",
      "GRR_KEY_BACKSLASH",
      "GRR_KEY_COMMA",
      "GRR_KEY_SLASH",
      "GRR_KEY_N",
      "GRR_KEY_M",
      "GRR_KEY_PERIOD",
      "GRR_KEY_TAB",
      "GRR_KEY_SPACE",
      "GRR_KEY_TILDE",
      "GRR_KEY_DELETE",
      "GRR_KEY_ESC",
      "GRR_KEY_CMD",
      "GRR_KEY_SHIFT",
      "GRR_KEY_CAPSLOCK",
      "GRR_KEY_OPT",
      "GRR_KEY_CTL",
      "GRR_KEY_FUNCTION",
      "GRR_KEY_NUMPAD_PERIOD",
      "GRR_KEY_NUMPAD_MUL",
      "GRR_KEY_NUMPAD_PLUS",
      "GRR_KEY_NUMPAD_DIV",
      "GRR_KEY_NUMPAD_ENTER",
      "GRR_KEY_NUMPAD_MINUS",
      "GRR_KEY_NUMPAD_EQUALS",
      "GRR_KEY_NUMPAD_0",
      "GRR_KEY_NUMPAD_1",
      "GRR_KEY_NUMPAD_2",
      "GRR_KEY_NUMPAD_3",
      "GRR_KEY_NUMPAD_4",
      "GRR_KEY_NUMPAD_5",
      "GRR_KEY_NUMPAD_6",
      "GRR_KEY_NUMPAD_7",
      "GRR_KEY_NUMPAD_8",
      "GRR_KEY_NUMPAD_9",
      "GRR_KEY_F5",
      "GRR_KEY_F6",
      "GRR_KEY_F7",
      "GRR_KEY_F3",
      "GRR_KEY_F8",
      "GRR_KEY_F9",
      "GRR_KEY_F11",
      "GRR_KEY_F13",
      "GRR_KEY_F14",
      "GRR_KEY_F10",
      "GRR_KEY_F12",
      "GRR_KEY_F15",
      "GRR_KEY_HELP",
      "GRR_KEY_HOME",
      "GRR_KEY_PAGE_UP",
      "GRR_KEY_DEL",
      "GRR_KEY_F4",
      "GRR_KEY_END",
      "GRR_KEY_F2",
      "GRR_KEY_PAGE_DOWN",
      "GRR_KEY_F1",
      "GRR_KEY_ARROW_LEFT",
      "GRR_KEY_ARROW_RIGHT",
      "GRR_KEY_ARROW_DOWN",
      "GRR_KEY_ARROW_UP",
      "GRR_KEY_OTHER",
      "GRR_KEY_MAX",
  };
  if (key < 0 || key >= GRR_KEY_MAX)
    return "Uknown key";
  return keyNames[key];
}