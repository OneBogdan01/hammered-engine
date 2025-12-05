#pragma once

#ifdef HM_EDITOR
#include <tracy/Tracy.hpp>

// Zone scoping
#define HM_ZONE_SCOPED ZoneScoped
#define HM_ZONE_SCOPED_N(name) ZoneScopedN(name)
#define HM_ZONE_SCOPED_C(color) ZoneScopedC(color)
#define HM_ZONE_SCOPED_NC(name, color) ZoneScopedNC(name, color)
#define HM_ZONE_TEXT(text, len) ZoneText(text, len)
#define HM_ZONE_VALUE(value) ZoneValue(value)

// Frame marks
#define HM_FRAME_MARK FrameMark
#define HM_FRAME_MARK_N(name) FrameMarkNamed(name)
#define HM_FRAME_MARK_START(name) FrameMarkStart(name)
#define HM_FRAME_MARK_END(name) FrameMarkEnd(name)

// Plotting values over time
#define HM_PLOT(name, val) TracyPlot(name, val)
#define HM_PLOT_CONFIG(name, type, step, fill, color) \
  TracyPlotConfig(name, type, step, fill, color)

// Messages/logging
#define HM_MESSAGE(text, len) TracyMessage(text, len)
#define HM_MESSAGE_L(text) TracyMessageL(text)
#define HM_MESSAGE_C(text, len, color) TracyMessageC(text, len, color)

// Memory tracking
#define HM_ALLOC(ptr, size) TracyAlloc(ptr, size)
#define HM_FREE(ptr) TracyFree(ptr)

// Lock profiling
#define HM_LOCKABLE(type, var) TracyLockable(type, var)
#define HM_LOCKABLE_N(type, var, name) TracyLockableN(type, var, name)

#else

// Zone scoping
#define HM_ZONE_SCOPED
#define HM_ZONE_SCOPED_N(name)
#define HM_ZONE_SCOPED_C(color)
#define HM_ZONE_SCOPED_NC(name, color)
#define HM_ZONE_TEXT(text, len)
#define HM_ZONE_VALUE(value)

// Frame marks
#define HM_FRAME_MARK
#define HM_FRAME_MARK_N(name)
#define HM_FRAME_MARK_START(name)
#define HM_FRAME_MARK_END(name)

// Plotting
#define HM_PLOT(name, val)
#define HM_PLOT_CONFIG(name, type, step, fill, color)

// Messages
#define HM_MESSAGE(text, len)
#define HM_MESSAGE_L(text)
#define HM_MESSAGE_C(text, len, color)

// Memory
#define HM_ALLOC(ptr, size)
#define HM_FREE(ptr)

// Locks
#define HM_LOCKABLE(type, var) type var
#define HM_LOCKABLE_N(type, var, name) type var

#endif
