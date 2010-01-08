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

// SYSTEM INCLUDES
// APPLICATION INCLUDES
#include <cp/msg/AcLimitCodecPreferencesMsg.h>

// DEFINES
// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
// STATIC VARIABLE INITIALIZATIONS
// MACROS
// GLOBAL VARIABLES
// GLOBAL FUNCTIONS

/* //////////////////////////// PUBLIC //////////////////////////////////// */

/* ============================ CREATORS ================================== */

AcLimitCodecPreferencesMsg::AcLimitCodecPreferencesMsg(CP_AUDIO_BANDWIDTH_ID audioBandwidthId,
                                                       const UtlString& sAudioCodecs,
                                                       CP_VIDEO_BANDWIDTH_ID videoBandwidthId,
                                                       const UtlString& sVideoCodecs)
: AcCommandMsg(AC_LIMIT_CODEC_PREFERENCES)
, m_audioBandwidthId(audioBandwidthId)
, m_sAudioCodecs(sAudioCodecs)
, m_videoBandwidthId(videoBandwidthId)
, m_sVideoCodecs(sVideoCodecs)
{

}

AcLimitCodecPreferencesMsg::~AcLimitCodecPreferencesMsg()
{

}

OsMsg* AcLimitCodecPreferencesMsg::createCopy(void) const
{
   return new AcLimitCodecPreferencesMsg(m_audioBandwidthId, m_sAudioCodecs,
      m_videoBandwidthId, m_sVideoCodecs);
}

/* ============================ MANIPULATORS ============================== */

/* ============================ ACCESSORS ================================= */

/* ============================ INQUIRY =================================== */

/* //////////////////////////// PROTECTED ///////////////////////////////// */

/* //////////////////////////// PRIVATE /////////////////////////////////// */

/* ============================ FUNCTIONS ================================= */

