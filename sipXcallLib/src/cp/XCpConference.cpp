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
#include <cp/XCpConference.h>
#include <net/SipDialog.h>

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

XCpConference::XCpConference(const UtlString& sId)
: XCpAbstractCall(sId)
{

}

XCpConference::~XCpConference()
{

}

/* ============================ MANIPULATORS ============================== */

OsStatus XCpConference::sendInfo(const UtlString& sSipCallId,
                                 const UtlString& sLocalTag,
                                 const UtlString& sRemoteTag,
                                 const UtlString& sContentType,
                                 const UtlString& sContentEncoding,
                                 const UtlString& sContent)
{
   return OS_FAILED;
}

/* ============================ ACCESSORS ================================= */

/* ============================ INQUIRY =================================== */

UtlBoolean XCpConference::hasSipDialog(const UtlString& sSipCallId,
                                       const UtlString& sLocalTag /*= NULL*/,
                                       const UtlString& sRemoteTag /*= NULL*/) const
{
   return FALSE;
}

int XCpConference::getCallCount() const
{
   return 0;
}

OsStatus XCpConference::getAudioEnergyLevels(int& iInputEnergyLevel, int& iOutputEnergyLevel) const
{
   iInputEnergyLevel = 0;
   iOutputEnergyLevel = 0;

   return OS_FAILED;
}

OsStatus XCpConference::getRemoteUserAgent(const UtlString& sSipCallId,
                                           const UtlString& sLocalTag,
                                           const UtlString& sRemoteTag,
                                           UtlString& userAgent) const
{
   userAgent.remove(0);

   return OS_NOT_FOUND;
}

OsStatus XCpConference::getMediaConnectionId(int& mediaConnID) const
{
   mediaConnID = -1;

   return OS_INVALID;
}

OsStatus XCpConference::getSipDialog(const UtlString& sSipCallId,
                                     const UtlString& sLocalTag,
                                     const UtlString& sRemoteTag,
                                     SipDialog& dialog) const
{
   dialog = SipDialog(); // assign empty SipDialog

   return OS_NOT_FOUND;
}

/* //////////////////////////// PROTECTED ///////////////////////////////// */

/* //////////////////////////// PRIVATE /////////////////////////////////// */

/* ============================ FUNCTIONS ================================= */
