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
#include <cp/msg/AcAudioToneStartMsg.h>

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

AcAudioToneStartMsg::AcAudioToneStartMsg(int iToneId,
                                         UtlBoolean bLocal,
                                         UtlBoolean bRemote)
: AcCommandMsg(AC_AUDIO_TONE_START)
, m_iToneId(iToneId)
, m_bLocal(bLocal)
, m_bRemote(bRemote)
{

}

AcAudioToneStartMsg::~AcAudioToneStartMsg()
{

}

OsMsg* AcAudioToneStartMsg::createCopy(void) const
{
   return new AcAudioToneStartMsg(m_iToneId, m_bLocal, m_bRemote);
}

/* ============================ MANIPULATORS ============================== */

/* ============================ ACCESSORS ================================= */

/* ============================ INQUIRY =================================== */

/* //////////////////////////// PROTECTED ///////////////////////////////// */

/* //////////////////////////// PRIVATE /////////////////////////////////// */

/* ============================ FUNCTIONS ================================= */

