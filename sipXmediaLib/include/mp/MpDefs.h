//
// Copyright (C) 2004-2006 SIPfoundry Inc.
// Licensed by SIPfoundry under the LGPL license.
//
// Copyright (C) 2004-2006 Pingtel Corp.  All rights reserved.
// Licensed to SIPfoundry under a Contributor Agreement.
//
// Copyright (C) 2007 Jaroslav Libak
// Licensed under the LGPL license.
// $$
///////////////////////////////////////////////////////////////////////////////

#ifndef MpDefs_h__
#define MpDefs_h__

// SYSTEM INCLUDES
// APPLICATION INCLUDES
// DEFINES

/**
 * Maximum number of media connections to flowgraph. Limits the number of participants in a conference.
 * We may have multiple flowgraphs, so this value doesn't limit the total number of calls.
 */
#define MAX_CONNECTIONS 100

/**
 * Sampling rate of flowgraph in Hz. We don't allow custom sampling rate for each flowgraph, since
 * then we would need to do much more resampling which is already quite expensive. MpMediaTask,
 * MpAudioDriverManager will use this sampling rate.
 */
#define SAMPLES_PER_SECOND 8000

/**
* Number of samples per frame. We use 10ms frames in flowgraph. For 8000Hz sampling rate, should be 80.
* For 16Khz, should be 160. Samples are 16bit - 2 bytes.
*
* Keep this value at SAMPLES_PER_SECOND/100, so that we use 10ms frames.
*/
#define SAMPLES_PER_FRAME (SAMPLES_PER_SECOND/100)

// MACROS
// EXTERNAL FUNCTIONS
// CONSTANTS
// STRUCTS
// TYPEDEFS
// MACROS
// FORWARD DECLARATIONS

#endif // MpDefs_h__
