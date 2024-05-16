/////////////////////////////////////////////////////////////////////////////////
// SerialDebugSetup.h
//
// This file is used to specify the settings that will be used by the SerialDebug
// library.  With it you can set the level of debug prints that will be displayed
// as well as enable simple serial debugging.  See
// https://github.com/JoaoLopesF/SerialDebug   and
// https://randomnerdtutorials.com/software-debugger-arduino-ide-serialdebug-library/
// for more information.
//
// History:
//  - jmcorbett 11-MAY-2024
//    Original code.
//
// Copyright (c) 2024, Joseph M. Corbett
//
/////////////////////////////////////////////////////////////////////////////////
#if !defined SERIAL_DEBUG_SETUP
#define SERIAL_DEBUG_SETUP 1

// Disable all debug ? Good to release builds (production)
// as nothing of SerialDebug is compiled, zero overhead :-).
// For it just uncomment the DEBUG_DISABLED.
#define DEBUG_DISABLED true


// Define the initial debug level here (uncomment to do it)
//  DEBUG_LEVEL_NONE,   // (N) No debug output.
//  DEBUG_LEVEL_ERROR,  // (E) Critical errors.
//  DEBUG_LEVEL_WARN,   // (W) Error conditions but not critical.
//  DEBUG_LEVEL_INFO,   // (I) Information messages.
//  DEBUG_LEVEL_DEBUG,  // (D) Extra information - default level (if not changed).
//  DEBUG_LEVEL_VERBOSE // (V) More information than the usual.
#define DEBUG_INITIAL_LEVEL DEBUG_LEVEL_NONE


// Disable SerialDebug debugger ? No more commands and features as functions and
// globals.  Uncomment this to disable it.
#define DEBUG_DISABLE_DEBUGGER true


// Debug TAG ?
// Useful for debugging any modules.
// For it, each module must have a TAG variable:
// 		const char* TAG = "...";
// Uncomment this to enable it.
// #define DEBUG_USE_TAG true


// Disable auto function name (good if your debug yet contains it)
//#define DEBUG_AUTO_FUNC_DISABLED true


#include "SerialDebug.h" //https://github.com/JoaoLopesF/SerialDebug

#endif // SERIAL_DEBUG_SETUP