#if defined(GRR_PLATFORM_MACOS)
// #import <Foundation/Foundation.h>
// #import <Metal/Metal.h>
#import <AvailabilityMacros.h>
#endif

#include "events.h"
#include "logging.h"
#include "vulkan.h"
#include "window.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#if defined(GRR_PLATFORM_MACOS)
// Map macOS keys to platform independent keys
// Note: macOS virtual key codes are hardware-independent but dependent on ANSI
// layout: pressing Q on an AZERTY layout corresponds to ANSI A key code
// Mapping source: https://gist.github.com/eegrok/949034
GRR_KEY Grr_key(unsigned short vKey) {
  switch (vKey) {
  case 0x00:
    return GRR_KEY_A;
  case 0x01:
    return GRR_KEY_S;
  case 0x02:
    return GRR_KEY_D;
  case 0x03:
    return GRR_KEY_F;
  case 0x04:
    return GRR_KEY_H;
  case 0x05:
    return GRR_KEY_G;
  case 0x06:
    return GRR_KEY_Z;
  case 0x07:
    return GRR_KEY_X;
  case 0x08:
    return GRR_KEY_C;
  case 0x09:
    return GRR_KEY_V;
  case 0x0B:
    return GRR_KEY_B;
  case 0x0C:
    return GRR_KEY_Q;
  case 0x0D:
    return GRR_KEY_W;
  case 0x0E:
    return GRR_KEY_E;
  case 0x0F:
    return GRR_KEY_R;
  case 0x10:
    return GRR_KEY_Y;
  case 0x11:
    return GRR_KEY_T;
  case 0x12:
    return GRR_KEY_1;
  case 0x13:
    return GRR_KEY_2;
  case 0x14:
    return GRR_KEY_3;
  case 0x15:
    return GRR_KEY_4;
  case 0x16:
    return GRR_KEY_6;
  case 0x17:
    return GRR_KEY_5;
  case 0x18:
    return GRR_KEY_EQUALS;
  case 0x19:
    return GRR_KEY_9;
  case 0x1A:
    return GRR_KEY_7;
  case 0x1B:
    return GRR_KEY_HYPHEN;
  case 0x1C:
    return GRR_KEY_8;
  case 0x1D:
    return GRR_KEY_0;
  case 0x1E:
    return GRR_KEY_CLOSE_BRACKET;
  case 0x1F:
    return GRR_KEY_O;
  case 0x20:
    return GRR_KEY_U;
  case 0x21:
    return GRR_KEY_OPEN_BRACKET;
  case 0x22:
    return GRR_KEY_I;
  case 0x23:
    return GRR_KEY_P;
  case 0x24:
    return GRR_KEY_RETURN;
  case 0x25:
    return GRR_KEY_L;
  case 0x26:
    return GRR_KEY_J;
  case 0x27:
    return GRR_KEY_APOSTROPHE;
  case 0x28:
    return GRR_KEY_K;
  case 0x29:
    return GRR_KEY_SEMICOLON;
  case 0x2A:
    return GRR_KEY_BACKSLASH;
  case 0x2B:
    return GRR_KEY_COMMA;
  case 0x2C:
    return GRR_KEY_SLASH;
  case 0x2D:
    return GRR_KEY_N;
  case 0x2E:
    return GRR_KEY_M;
  case 0x2F:
    return GRR_KEY_PERIOD;
  case 0x30:
    return GRR_KEY_TAB;
  case 0x31:
    return GRR_KEY_SPACE;
  case 0x32:
    return GRR_KEY_TILDE;
  case 0x33:
    return GRR_KEY_DELETE;
  case 0x35:
    return GRR_KEY_ESC;
  case 0x37:
    return GRR_KEY_CMD;
  case 0x38:
    return GRR_KEY_SHIFT;
  case 0x39:
    return GRR_KEY_CAPSLOCK;
  case 0x3A:
    return GRR_KEY_OPT;
  case 0x3B:
    return GRR_KEY_CTL;
  case 0x41:
    return GRR_KEY_NUMPAD_PERIOD;
  case 0x43:
    return GRR_KEY_NUMPAD_MUL;
  case 0x45:
    return GRR_KEY_NUMPAD_PLUS;
  case 0x4B:
    return GRR_KEY_NUMPAD_DIV;
  case 0x4C:
    return GRR_KEY_NUMPAD_ENTER;
  case 0x4E:
    return GRR_KEY_NUMPAD_MINUS;
  case 0x51:
    return GRR_KEY_NUMPAD_EQUALS;
  case 0x52:
    return GRR_KEY_NUMPAD_0;
  case 0x53:
    return GRR_KEY_NUMPAD_1;
  case 0x54:
    return GRR_KEY_NUMPAD_2;
  case 0x55:
    return GRR_KEY_NUMPAD_3;
  case 0x56:
    return GRR_KEY_NUMPAD_4;
  case 0x57:
    return GRR_KEY_NUMPAD_5;
  case 0x58:
    return GRR_KEY_NUMPAD_6;
  case 0x59:
    return GRR_KEY_NUMPAD_7;
  case 0x5A:
  case 0x5B:
    return GRR_KEY_NUMPAD_8;
  case 0x5C:
    return GRR_KEY_NUMPAD_9;
  case 0x5D:
  case 0x60:
    return GRR_KEY_F5;
  case 0x61:
    return GRR_KEY_F6;
  case 0x62:
    return GRR_KEY_F7;
  case 0x63:
    return GRR_KEY_F3;
  case 0x64:
    return GRR_KEY_F8;
  case 0x65:
    return GRR_KEY_F9;
  case 0x67:
    return GRR_KEY_F11;
  case 0x69:
    return GRR_KEY_F13;
  case 0x6B:
    return GRR_KEY_F14;
  case 0x6D:
    return GRR_KEY_F10;
  case 0x6F:
    return GRR_KEY_F12;
  case 0x71:
    return GRR_KEY_F15;
  case 0x72:
    return GRR_KEY_HELP;
  case 0x73:
    return GRR_KEY_HOME;
  case 0x74:
    return GRR_KEY_PAGE_UP;
  case 0x75:
    return GRR_KEY_DEL;
  case 0x76:
    return GRR_KEY_F4;
  case 0x77:
    return GRR_KEY_END;
  case 0x78:
    return GRR_KEY_F2;
  case 0x79:
    return GRR_KEY_PAGE_DOWN;
  case 0x7A:
    return GRR_KEY_F1;
  case 0x7B:
    return GRR_KEY_ARROW_LEFT;
  case 0x7C:
    return GRR_KEY_ARROW_RIGHT;
  case 0x7D:
    return GRR_KEY_ARROW_DOWN;
  case 0x7E:
    return GRR_KEY_ARROW_UP;

  default:
    return GRR_KEY_OTHER;
  }
}

void Grr_handleModifierKey(NSEventModifierFlags modifierFlags,
                           Grr_u32 modifierKey, GRR_KEY grrKey) {
  // Check for the Shift key
  GrrEventData data;
  if ((modifierFlags & modifierKey) != 0) {
    if (windowInfo->keys[grrKey] == false) {
      windowInfo->keys[grrKey] = true;
      data.keyboard.key = grrKey;
      Grr_sendEvent(GRR_KEY_DOWN, NULL, data);
    }
  } else {
    if (windowInfo->keys[grrKey] == true) {
      windowInfo->keys[grrKey] = false;
      data.keyboard.key = grrKey;
      Grr_sendEvent(GRR_KEY_UP, NULL, data);
    }
  }
}

#endif

// Native window data structres
#if defined(GRR_PLATFORM_MACOS)
@interface GrrApplicationController : NSObject <NSApplicationDelegate>
@end

@implementation GrrApplicationController
@end

// Subclass NSview to have more control on keyboard/mosue events
@interface GrrView : NSView
@end

@implementation GrrView

- (BOOL)wantsUpdateLayer {
  // By default NO
  // Our view is layer-backed (Metal layer) and udpates itself by modifying
  // its layer
  return YES;
}

- (BOOL)isFlipped {
  // By default NO
  // Origin is lower-left corner and positive y coordinates increase upward
  // which is not what we want Most platforms have y coordinates that increase
  // top to bottom which is why we override this property to always return YES
  return YES;
}

- (void)mouseEntered:(NSEvent *)event {
  // GRR_LOG_DEBUG("Mouse Entered\n");
  // Called when the mouse enters the view
}

- (void)mouseExited:(NSEvent *)event {
  // GRR_LOG_ERROR("Mouse Exited\n");
  // Called when the mouse exits the view
}

- (void)mouseDown:(NSEvent *)event {
  // Called when left mouse button is pressed
  NSPoint windowLocation = event.locationInWindow;
  NSPoint viewLocation = [self convertPoint:windowLocation fromView:nil];
  GrrEventData data = {};
  data.mouse.pressed = true;
  data.mouse.x = viewLocation.x;
  data.mouse.y = viewLocation.y;
  data.mouse.button = GRR_MOUSE_BUTTON_LEFT;
  windowInfo->buttons[GRR_MOUSE_BUTTON_LEFT] = true;
  Grr_sendEvent(GRR_MOUSE_BUTTON_DOWN, NULL, data);
}

- (void)mouseUp:(NSEvent *)event {
  // Called when left mouse button is released
  NSPoint windowLocation = event.locationInWindow;
  NSPoint viewLocation = [self convertPoint:windowLocation fromView:nil];
  GrrEventData data = {};
  data.mouse.pressed = false;
  data.mouse.x = viewLocation.x;
  data.mouse.y = viewLocation.y;
  data.mouse.button = GRR_MOUSE_BUTTON_LEFT;
  windowInfo->buttons[GRR_MOUSE_BUTTON_LEFT] = false;
  Grr_sendEvent(GRR_MOUSE_BUTTON_UP, NULL, data);
}

- (void)mouseMoved:(NSEvent *)event {
  // Called when the mouse is moved without any buttons being pressed
  NSPoint windowLocation = [event locationInWindow];
  NSPoint viewLocation = [self convertPoint:windowLocation fromView:nil];
  GrrEventData data = {};
  data.mouse.pressed = false;
  data.mouse.x = viewLocation.x;
  data.mouse.y = viewLocation.y;
  Grr_sendEvent(GRR_MOUSE_MOTION, NULL, data);
}

- (void)rightMouseDown:(NSEvent *)event {
  // Called when right mouse button is pressed
  NSPoint windowLocation = event.locationInWindow;
  NSPoint viewLocation = [self convertPoint:windowLocation fromView:nil];
  GrrEventData data = {};
  data.mouse.pressed = true;
  data.mouse.x = viewLocation.x;
  data.mouse.y = viewLocation.y;
  data.mouse.button = GRR_MOUSE_BUTTON_RIGHT;
  windowInfo->buttons[GRR_MOUSE_BUTTON_RIGHT] = true;
  Grr_sendEvent(GRR_MOUSE_BUTTON_DOWN, NULL, data);
}

- (void)rightMouseUp:(NSEvent *)event {
  // Called when right mouse button is released
  NSPoint windowLocation = event.locationInWindow;
  NSPoint viewLocation = [self convertPoint:windowLocation fromView:nil];
  GrrEventData data = {};
  data.mouse.pressed = false;
  data.mouse.x = viewLocation.x;
  data.mouse.y = viewLocation.y;
  data.mouse.button = GRR_MOUSE_BUTTON_RIGHT;
  windowInfo->buttons[GRR_MOUSE_BUTTON_RIGHT] = false;
  Grr_sendEvent(GRR_MOUSE_BUTTON_DOWN, NULL, data);
}

- (void)scrollWheel:(NSEvent *)event {
  // Called when the user scrolls the scroll wheel
  GrrEventData data = {}; // TODO
  Grr_sendEvent(GRR_MOUSE_SCROLL, NULL, data);
}

- (void)keyDown:(NSEvent *)event {
  // Called when a key is pressed
  GRR_KEY key = Grr_key(event.keyCode);
  GrrEventData data;
  data.keyboard.pressed = true;
  data.keyboard.key = key;
  windowInfo->keys[key] = true;
  Grr_sendEvent(GRR_KEY_DOWN, NULL, data);
}

- (void)keyUp:(NSEvent *)event {
  // Called when a key is released
  GRR_KEY key = Grr_key(event.keyCode);
  GrrEventData data;
  data.keyboard.pressed = false;
  data.keyboard.key = key;
  windowInfo->keys[key] = false;
  Grr_sendEvent(GRR_KEY_UP, NULL, data);
}

- (void)flagsChanged:(NSEvent *)event {
  // Called when modifier key flags change
  // SHIFT, CTL, CMD, OPT, FUNCTION, HELP, NUMPAD, CAPSLOCK
  NSEventModifierFlags modifierFlags = [event modifierFlags];

  Grr_handleModifierKey(modifierFlags, NSEventModifierFlagShift, GRR_KEY_SHIFT);
  Grr_handleModifierKey(modifierFlags, NSEventModifierFlagControl, GRR_KEY_CTL);
  Grr_handleModifierKey(modifierFlags, NSEventModifierFlagCommand, GRR_KEY_CMD);
  Grr_handleModifierKey(modifierFlags, NSEventModifierFlagOption, GRR_KEY_OPT);
  Grr_handleModifierKey(modifierFlags, NSEventModifierFlagFunction,
                        GRR_KEY_FUNCTION);
  Grr_handleModifierKey(modifierFlags, NSEventModifierFlagHelp, GRR_KEY_HELP);

  // Check for the NUMPAD
  if ((modifierFlags & NSEventModifierFlagNumericPad) != 0) {
    NSLog(@"Numeric Pad key is pressed"); // TODO
  }

  // Check for capslock
  GrrEventData data = {};
  if ((modifierFlags & NSEventModifierFlagCapsLock) != 0) {
    windowInfo->keys[GRR_KEY_CAPSLOCK] = true;
    Grr_sendEvent(GRR_CAPSLOCK_ON, NULL, data);
  } else {
    if (windowInfo->keys[GRR_KEY_CAPSLOCK] == true) {
      windowInfo->keys[GRR_KEY_CAPSLOCK] = false;
      Grr_sendEvent(GRR_CAPSLOCK_OFF, NULL, data);
    }
  }
}

@end

// Class that conforms to the NSWindowDelegate protocol to have more control on
// window events
@interface GrrWindowController : NSObject <NSWindowDelegate>
@end

@implementation GrrWindowController

- (BOOL)acceptsFirstResponder {
  // By default NO
  // We want this view to be the first responder of the window so that it
  // handles keyboard/mouse events
  return YES;
}

- (void)windowWillClose:(NSNotification *)notification {
  GRR_LOG_DEBUG("Window will close\n");
}

- (NSSize)windowWillResize:(NSWindow *)sender toSize:(NSSize)frameSize {
  return frameSize; // You can return a modified size if needed
}

- (void)windowDidResize:(NSNotification *)notification {
  GRR_LOG_DEBUG("Window did resize\n");
  Grr_u16 w = windowInfo->nativeWindow.contentView.bounds.size.width;
  Grr_u16 h = windowInfo->nativeWindow.contentView.bounds.size.height;
  GrrEventData data;
  data.window.w = w;
  data.window.h = h;
  GRR_LOG_DEBUG("Size %d %d\n", w, h);
  Grr_sendEvent(GRR_WINDOW_RESIZED, NULL, data);
}

- (void)windowWillMove:(NSNotification *)notification {
  GRR_LOG_DEBUG("Window will move\n");
}

- (void)windowDidMove:(NSNotification *)notification {
  GRR_LOG_DEBUG("Window did move\n");
}

- (void)windowDidBecomeKey:(NSNotification *)notification {
  GRR_LOG_DEBUG("Window did become key\n");
}

- (void)windowWillMiniaturize:(NSNotification *)notification {
  GRR_LOG_DEBUG("Window will miniaturize\n");
}

- (void)windowDidMiniaturize:(NSNotification *)notification {
  GRR_LOG_DEBUG("Window did miniaturize\n");
  GrrEventData data;
  data.window.w = 0;
  data.window.h = 0;
  Grr_sendEvent(GRR_WINDOW_RESIZED, NULL, data);
}

- (void)windowDidDeminiaturize:(NSNotification *)notification {
  GRR_LOG_DEBUG("Window did deminiaturize\n");
  Grr_u16 w = windowInfo->nativeWindow.contentView.bounds.size.width;
  Grr_u16 h = windowInfo->nativeWindow.contentView.bounds.size.height;
  GrrEventData data;
  data.window.w = w;
  data.window.h = h;
  Grr_sendEvent(GRR_WINDOW_RESIZED, NULL, data);
}

- (BOOL)windowShouldClose:(NSWindow *)sender {
  // Return YES if the window should close; NO otherwise
  GrrEventData data = {};
  Grr_sendEvent(GRR_QUIT, NULL, data);
  return YES;
}

- (void)windowDidEnterFullScreen:(NSNotification *)notification {
  GRR_LOG_DEBUG("Window entered full-screen mode\n");
}

- (void)windowDidExitFullScreen:(NSNotification *)notification {
  GRR_LOG_DEBUG("Window exited full-screen mode\n");
}

- (void)windowWillEnterFullScreen:(NSNotification *)notification {
  GRR_LOG_DEBUG("Window will enter full-screen mode\n");
}

- (void)windowWillExitFullScreen:(NSNotification *)notification {
  GRR_LOG_DEBUG("Window will exit full-screen mode\n");
}

@end
#endif

struct GrrWindowInfo *windowInfo = NULL;

void Grr_initializeWindow(Grr_string title, Grr_i16 x, Grr_i16 y, Grr_u16 w,
                          Grr_u16 h, Grr_u32 mask) {
  GRR_LOG_INFO("Initialize window\n");
  // Make sure init is called only once
  if (windowInfo != NULL) {
    GRR_LOG_CRITICAL("Cannot initialize window twice\n");
    exit(EXIT_FAILURE);
  }

  // Native window
#if defined(GRR_PLATFORM_MACOS)
  // Create app, a global variable NSApp holds the app instance
  [NSApplication sharedApplication];

  NSRect rect = NSMakeRect(x, y, w, h);
  GrrView *view = [[GrrView alloc] initWithFrame:rect];
  // View will use Apple's Metal layer as its backing store (order of next two
  // operations is important)
  view.wantsLayer = YES;
  view.layer = [CAMetalLayer layer];
  if (!view.layer) {
    GRR_LOG_CRITICAL("Failed to create metal layer\n");
    exit(EXIT_FAILURE);
  }

  // NSWindowStyleMaskFullScreen is toggled automatically by calls to
  // toggleFullScreen
  NSWindowStyleMask styleMask =
      NSWindowStyleMaskTitled | NSWindowStyleMaskClosable |
      NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable;
  NSWindow *nativeWindow =
      [[NSWindow alloc] initWithContentRect:rect
                                  styleMask:styleMask
                                    backing:NSBackingStoreBuffered
                                      defer:NO];
  [nativeWindow setDelegate:[[GrrWindowController alloc] init]];
  [nativeWindow setContentView:view]; // Content view
                                      // resizes
                                      // automatically
                                      // when window
  // resizes, view’s coordinate system can
  // be changed through its bounds rectangle
  [nativeWindow makeFirstResponder:view];     // View is first to receive
                                              // events
  nativeWindow.acceptsMouseMovedEvents = YES; // By default NO
  if (title != NULL)
    [nativeWindow setTitle:@(title)]; // Title if
                                      // provided
#endif

  // Populate window info object
  windowInfo = (struct GrrWindowInfo *)malloc(sizeof(struct GrrWindowInfo));
  if (errno == ENOMEM) {
    GRR_LOG_ERROR("%s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }
  windowInfo->title = title;
  windowInfo->x = x;
  windowInfo->y = y;
  windowInfo->w = w;
  windowInfo->h = h;
  windowInfo->styleFlags =
      mask & (GRR_WINDOW_STYLE_END - 1); // Silently zero out unrecognized
                                         // style flags
  windowInfo->running = true;
  windowInfo->suspended = false;
  windowInfo->nativeWindow = nativeWindow;

  for (Grr_u16 i = 0; i < GRR_MOUSE_BUTTON_MAX; i++)
    windowInfo->buttons[i] = false;
  for (Grr_u16 i = 0; i < GRR_KEY_MAX; i++)
    windowInfo->keys[i] = false;

    // Start macOS app main event loop (this
    // is not the engine's event loop)
#if defined(GRR_PLATFORM_MACOS)
  [windowInfo->nativeWindow setLevel:NSNormalWindowLevel];
  [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
  // Activate app
#if (defined(MAC_OS_X_VERSION_MIN_REQUIRED) &&                                 \
     MAC_OS_X_VERSION_MIN_REQUIRED > 140000)
  // activateIgnoringOtherApps deprecated on
  // macOS 10.0 to 14.0
  [NSApp activate];
#else
  [NSApp activateIgnoringOtherApps:YES];

  // TODO: if-else to be deleted
  if ([NSApp isActive]) {
    GRR_LOG_DEBUG("Active\n\n");
  } else {
    GRR_LOG_DEBUG("Inactive\n\n");
  }

#endif
  // Show window (make it key window) and
  // bring to front
  [windowInfo->nativeWindow makeKeyAndOrderFront:nil];
  if (mask & GRR_WINDOW_FULL_SCREEN)
    [windowInfo->nativeWindow toggleFullScreen:nil];
  [NSApp finishLaunching];
#endif

  // Mapping between the logical coordinate space of the layer (measured in
  // points) and the physical coordinate space (measured in pixels)
  [view.layer setContentsScale:nativeWindow.backingScaleFactor];

  // CAMetalLayer *metalLayer = (CAMetalLayer *)view.layer;
  // metalLayer.drawableSize = [view convertSizeToBacking:view.bounds.size];

  atexit(Grr_freeWindow);
}

Grr_bool Grr_windowWasInitialized() { return windowInfo != NULL; }

void Grr_startGameLoop() {
#if defined(GRR_PLATFORM_MACOS)
  while (windowInfo->running) {
    // Events
    for (;;) {
      // Pop events from event queue
      NSEvent *event = [NSApp nextEventMatchingMask:NSEventMaskAny
                                          untilDate:nil
                                             inMode:NSDefaultRunLoopMode
                                            dequeue:TRUE];
      if (event == nil)
        break;                 // Event queue is empty, break
      [NSApp sendEvent:event]; // Else, dispatch event to window
    }

    if (!windowInfo->suspended) {
      Grr_drawFrame();
    } else {
      GRR_LOG_DEBUG("Window suspended\n");
    }
  }
#endif
}

void Grr_stopGameLoop() { windowInfo->running = false; }

void Grr_freeWindow() {
  GRR_LOG_INFO("Free window\n");
  if (windowInfo != NULL) {
    free(windowInfo);
  }
}

#if defined(GRR_PLATFORM_MACOS)
CAMetalLayer *Grr_metalLayer() {
  CALayer *layer = [[windowInfo->nativeWindow contentView] layer];
  if (![layer isKindOfClass:[CAMetalLayer class]]) {
    GRR_LOG_CRITICAL("macOS window's content view does not have Metal layer\n");
    return nil;
  }
  CAMetalLayer *metalLayer = (CAMetalLayer *)layer;
  return metalLayer;
}
#endif