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
#include <cp/msg/AcAudioSendDTMFToneMsg.h>

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

AcAudioSendDTMFToneMsg::AcAudioSendDTMFToneMsg(int iToneId,
                                               UtlBoolean bLocal,
                                               UtlBoolean bRemote,
                                               int duration)
: AcCommandMsg(AC_AUDIO_SEND_DTMF_TONE)
, m_iToneId(iToneId)
, m_bLocal(bLocal)
, m_bRemote(bRemote)
, m_duration(duration)
{

}

AcAudioSendDTMFToneMsg::~AcAudioSendDTMFToneMsg()
{

}

OsMsg* AcAudioSendDTMFToneMsg::createCopy(void) const
{
   return new AcAudioSendDTMFToneMsg(m_iToneId, m_bLocal, m_bRemote, m_duration);
}

/* ============================ MANIPULATORS ============================== */

/* ============================ ACCESSORS ================================= */

/* ============================ INQUIRY =================================== */

/* //////////////////////////// PROTECTED ///////////////////////////////// */

/* //////////////////////////// PRIVATE /////////////////////////////////// */

/* ============================ FUNCTIONS ================================= */

