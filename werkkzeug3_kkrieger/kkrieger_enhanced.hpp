// This file is distributed under a BSD license. See LICENSE.txt for details.

#ifndef __KKRIEGER_ENHANCED_HPP__
#define __KKRIEGER_ENHANCED_HPP__

// Optional enhanced player/dev build switches. Defaults preserve the original
// build path; define KKR_ENHANCED_BUILD=1 or the individual KKR_* macros in a
// project configuration to opt in.
#ifndef KKR_ENHANCED_BUILD
#define KKR_ENHANCED_BUILD 0
#endif

#define KKR_ASPECT_ORIGINAL        0
#define KKR_ASPECT_FULLSCREEN_16X9 1
#define KKR_ASPECT_FULLSCREEN_16X10 2
#define KKR_ASPECT_FULLSCREEN_21X9 3
#define KKR_ASPECT_STRETCH_DEBUG   4

#ifndef KKR_TEXTURE_SCALE_OFFSET
#if KKR_ENHANCED_BUILD
#define KKR_TEXTURE_SCALE_OFFSET 1
#else
#define KKR_TEXTURE_SCALE_OFFSET 0
#endif
#endif

#ifndef KKR_TEXTURE_MAX_EXP
#define KKR_TEXTURE_MAX_EXP 12
#endif

#ifndef KKR_DEFAULT_RESOLUTION_INDEX
#if KKR_ENHANCED_BUILD
#define KKR_DEFAULT_RESOLUTION_INDEX 6
#else
#define KKR_DEFAULT_RESOLUTION_INDEX 1
#endif
#endif

#ifndef KKR_ASPECT_MODE
#if KKR_ENHANCED_BUILD
#define KKR_ASPECT_MODE KKR_ASPECT_FULLSCREEN_16X9
#else
#define KKR_ASPECT_MODE KKR_ASPECT_ORIGINAL
#endif
#endif

#ifndef KKR_OVERLAY_FULLRT_MAX_X_EXP
#if KKR_ENHANCED_BUILD
#define KKR_OVERLAY_FULLRT_MAX_X_EXP 11
#else
#define KKR_OVERLAY_FULLRT_MAX_X_EXP 10
#endif
#endif

#ifndef KKR_OVERLAY_FULLRT_MAX_Y_EXP
#if KKR_ENHANCED_BUILD
#define KKR_OVERLAY_FULLRT_MAX_Y_EXP 11
#else
#define KKR_OVERLAY_FULLRT_MAX_Y_EXP 9
#endif
#endif

#ifndef KKR_PLAYER_TRACE_LOG
#define KKR_PLAYER_TRACE_LOG 0
#endif

#ifndef KKR_ENABLE_AUDIO
#define KKR_ENABLE_AUDIO 0
#endif

#endif
