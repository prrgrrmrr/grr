#ifndef GRR_EVENTS_H
#define GRR_EVENTS_H

#include "types.h"

typedef enum GRR_MOUSE_BUTTON {
  GRR_MOUSE_BUTTON_LEFT,
  GRR_MOUSE_BUTTON_MIDDLE,
  GRR_MOUSE_BUTTON_RIGHT,
  GRR_MOUSE_BUTTON_OTHER,
  GRR_MOUSE_BUTTON_MAX
} GRR_MOUSE_BUTTON;

// Grr key codes
typedef enum GRR_KEY {
  GRR_KEY_A,
  GRR_KEY_S,
  GRR_KEY_D,
  GRR_KEY_F,
  GRR_KEY_H,
  GRR_KEY_G,
  GRR_KEY_Z,
  GRR_KEY_X,
  GRR_KEY_C,
  GRR_KEY_V,
  GRR_KEY_B,
  GRR_KEY_Q,
  GRR_KEY_W,
  GRR_KEY_E,
  GRR_KEY_R,
  GRR_KEY_Y,
  GRR_KEY_T,
  GRR_KEY_1,
  GRR_KEY_2,
  GRR_KEY_3,
  GRR_KEY_4,
  GRR_KEY_6,
  GRR_KEY_5,
  GRR_KEY_EQUALS,
  GRR_KEY_9,
  GRR_KEY_7,
  GRR_KEY_HYPHEN,
  GRR_KEY_8,
  GRR_KEY_0,
  GRR_KEY_CLOSE_BRACKET,
  GRR_KEY_O,
  GRR_KEY_U,
  GRR_KEY_OPEN_BRACKET,
  GRR_KEY_I,
  GRR_KEY_P,
  GRR_KEY_RETURN,
  GRR_KEY_,
  GRR_KEY_L,
  GRR_KEY_J,
  GRR_KEY_APOSTROPHE,
  GRR_KEY_K,
  GRR_KEY_SEMICOLON,
  GRR_KEY_BACKSLASH,
  GRR_KEY_COMMA,
  GRR_KEY_SLASH,
  GRR_KEY_N,
  GRR_KEY_M,
  GRR_KEY_PERIOD,
  GRR_KEY_TAB,
  GRR_KEY_SPACE,
  GRR_KEY_TILDE,
  GRR_KEY_DELETE,
  GRR_KEY_ESC,
  GRR_KEY_CMD,
  GRR_KEY_SHIFT,
  GRR_KEY_CAPSLOCK,
  GRR_KEY_OPT,
  GRR_KEY_CTL,
  GRR_KEY_FUNCTION,
  GRR_KEY_NUMPAD_PERIOD,
  GRR_KEY_NUMPAD_MUL,
  GRR_KEY_NUMPAD_PLUS,
  GRR_KEY_NUMPAD_DIV,
  GRR_KEY_NUMPAD_ENTER,
  GRR_KEY_NUMPAD_MINUS,
  GRR_KEY_NUMPAD_EQUALS,
  GRR_KEY_NUMPAD_0,
  GRR_KEY_NUMPAD_1,
  GRR_KEY_NUMPAD_2,
  GRR_KEY_NUMPAD_3,
  GRR_KEY_NUMPAD_4,
  GRR_KEY_NUMPAD_5,
  GRR_KEY_NUMPAD_6,
  GRR_KEY_NUMPAD_7,
  GRR_KEY_NUMPAD_8,
  GRR_KEY_NUMPAD_9,
  GRR_KEY_F5,
  GRR_KEY_F6,
  GRR_KEY_F7,
  GRR_KEY_F3,
  GRR_KEY_F8,
  GRR_KEY_F9,
  GRR_KEY_F11,
  GRR_KEY_F13,
  GRR_KEY_F14,
  GRR_KEY_F10,
  GRR_KEY_F12,
  GRR_KEY_F15,
  GRR_KEY_HELP,
  GRR_KEY_HOME,
  GRR_KEY_PAGE_UP,
  GRR_KEY_DEL, // Below help key
  GRR_KEY_F4,
  GRR_KEY_END,
  GRR_KEY_F2,
  GRR_KEY_PAGE_DOWN,
  GRR_KEY_F1,
  GRR_KEY_ARROW_LEFT,
  GRR_KEY_ARROW_RIGHT,
  GRR_KEY_ARROW_DOWN,
  GRR_KEY_ARROW_UP,
  GRR_KEY_OTHER,
  GRR_KEY_MAX
} GRR_KEY;

typedef struct GrrEventData {
  union {
    struct {
      Grr_u16 x, y;
      Grr_bool pressed;
      GRR_MOUSE_BUTTON button;
    } mouse;

    struct {
      GRR_KEY key; // Key code
      Grr_bool pressed;
    } keyboard;

    struct {
      Grr_u16 w;
      Grr_u16 h;
    } window;
  };
} GrrEventData;

typedef enum GRR_EVENT_TYPE {
  GRR_MOUSE_BUTTON_DOWN,
  GRR_MOUSE_BUTTON_UP,
  GRR_MOUSE_MOTION,
  GRR_MOUSE_SCROLL,
  GRR_KEY_DOWN,
  GRR_KEY_UP,
  GRR_WINDOW_RESIZED,
  GRR_CAPSLOCK_ON,
  GRR_CAPSLOCK_OFF,
  GRR_QUIT,
  GRR_EVENT_TYPE_MAX
} GRR_EVENT_TYPE;

typedef void (*Grr_eventHandler)(void *sender, const GrrEventData data);

void Grr_initializeEvents();
Grr_bool Grr_subscribe(GRR_EVENT_TYPE type, Grr_eventHandler handler);
Grr_bool Grr_unsubscribe(GRR_EVENT_TYPE type, Grr_eventHandler handler);
Grr_bool Grr_sendEvent(GRR_EVENT_TYPE type, void *sender,
                       const GrrEventData data);
void Grr_freeEvents();

// Useful for debugging
Grr_string Grr_keyToString(GRR_KEY key);

#endif