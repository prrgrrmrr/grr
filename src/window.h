#ifndef WINDOW_H
#define WINDOW_H

#include "events.h"

#if defined(GRR_PLATFORM_MACOS)
#import <AppKit/AppKit.h>
#import <QuartzCore/QuartzCore.h>
#endif

struct GrrWindowInfo {
  // Title
  Grr_string title;

  // Content area on screen
  // From Apple's doc: window server limits window position coordinates to
  // ±16000 and sizes to 10000
  Grr_i16 x;
  Grr_i16 y;
  Grr_u16 w;
  Grr_u16 h;

  // Style mask
  Grr_u32 styleFlags;

  // State
  Grr_bool running;
  Grr_bool suspended; // App paused for whatever reason, so don't

  // Mouse & keyboard states
  Grr_bool buttons[GRR_MOUSE_BUTTON_MAX]; // Pressed mouse buttons
  Grr_bool keys[GRR_KEY_MAX];             // pressed keys

  // Native window handle
#if defined(GRR_PLATFORM_MACOS)
  NSWindow *nativeWindow;
#endif
};

extern struct GrrWindowInfo *windowInfo;

// Window styles
enum {
  GRR_WINDOW_STYLE_DEFAULT = 0,
  GRR_WINDOW_FULL_SCREEN = 1 << 0,
  GRR_WINDOW_STYLE_END = 1 << 1
};

void Grr_initializeWindow(Grr_string title, Grr_i16 x, Grr_i16 y, Grr_u16 w,
                          Grr_u16 h, Grr_u32 mask);
Grr_bool Grr_windowWasInitialized();
void Grr_startGameLoop();
void Grr_stopGameLoop();
void Grr_freeWindow();

#if defined(GRR_PLATFORM_MACOS)
CAMetalLayer *Grr_metalLayer();
#endif

#endif