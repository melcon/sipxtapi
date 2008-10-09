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
#include <cp/XCpCall.h>
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

XCpCall::XCpCall(const UtlString& sId)
: XCpAbstractCall(sId)
{

}

XCpCall::~XCpCall()
{

}

/* ============================ MANIPULATORS ============================== */

OsStatus XCpCall::connect(const UtlString& sSipCallId,
                          const UtlString& toAddress,
                          const UtlString& lineURI,
                          const UtlString& locationHeader,
                          CP_CONTACT_ID contactId)
{
   return OS_FAILED;
}

OsStatus XCpCall::acceptConnection(const UtlString& locationHeader,
                                   CP_CONTACT_ID contactId)
{
   return OS_FAILED;
}

OsStatus XCpCall::rejectConnection()
{
   return OS_FAILED;
}

OsStatus XCpCall::redirectConnection(const UtlString& sRedirectSipUri)
{
   return OS_FAILED;
}

OsStatus XCpCall::answerConnection()
{
   return OS_FAILED;
}

OsStatus XCpCall::dropConnection(const UtlString& sSipCallId,
                                 const UtlString& sLocalTag,
                                 const UtlString& sRemoteTag)
{
   return OS_FAILED;
}

OsStatus XCpCall::dropConnection()
{
   return OS_FAILED;
}

OsStatus XCpCall::holdConnection(const UtlString& sSipCallId,
                                 const UtlString& sLocalTag,
                                 const UtlString& sRemoteTag)
{
   return OS_FAILED;
}

OsStatus XCpCall::holdConnection()
{
   return OS_FAILED;
}

OsStatus XCpCall::unholdConnection(const UtlString& sSipCallId,
                                   const UtlString& sLocalTag,
                                   const UtlString& sRemoteTag)
{
   return OS_FAILED;
}

OsStatus XCpCall::unholdConnection()
{
   return OS_FAILED;
}

OsStatus XCpCall::silentHoldRemoteConnection(const UtlString& sSipCallId,
                                             const UtlString& sLocalTag,
                                             const UtlString& sRemoteTag)
{
   return OS_FAILED;
}

OsStatus XCpCall::silentUnholdRemoteConnection(const UtlString& sSipCallId,
                                               const UtlString& sLocalTag,
                                               const UtlString& sRemoteTag)
{
   return OS_FAILED;
}

OsStatus XCpCall::silentHoldLocalConnection(const UtlString& sSipCallId,
                                            const UtlString& sLocalTag,
                                            const UtlString& sRemoteTag)
{
   return OS_FAILED;
}

OsStatus XCpCall::silentUnholdLocalConnection(const UtlString& sSipCallId,
                                              const UtlString& sLocalTag,
                                              const UtlString& sRemoteTag)
{
   return OS_FAILED;
}

OsStatus XCpCall::limitCodecPreferences(CP_AUDIO_BANDWIDTH_ID audioBandwidthId,
                                        const UtlString& sAudioCodecs,
                                        CP_VIDEO_BANDWIDTH_ID videoBandwidthId,
                                        const UtlString& sVideoCodecs)
{
   return OS_FAILED;
}

OsStatus XCpCall::renegotiateCodecsConnection(const UtlString& sSipCallId,
                                              const UtlString& sLocalTag,
                                              const UtlString& sRemoteTag,
                                              CP_AUDIO_BANDWIDTH_ID audioBandwidthId,
                                              const UtlString& sAudioCodecs,
                                              CP_VIDEO_BANDWIDTH_ID videoBandwidthId,
                                              const UtlString& sVideoCodecs)
{
   return OS_FAILED;
}

OsStatus XCpCall::sendInfo(const UtlString& sSipCallId,
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

UtlBoolean XCpCall::hasSipDialog(const UtlString& sSipCallId,
                                 const UtlString& sLocalTag,
                                 const UtlString& sRemoteTag) const
{
   return FALSE;
}

int XCpCall::getCallCount() const
{
   return 0;
}

OsStatus XCpCall::getCallSipCallId(UtlString& sSipCallId) const
{
   sSipCallId.remove(0);
   return OS_FAILED;
}

OsStatus XCpCall::getAudioEnergyLevels(int& iInputEnergyLevel, int& iOutputEnergyLevel) const
{
   iInputEnergyLevel = 0;
   iOutputEnergyLevel = 0;

   return OS_FAILED;
}

OsStatus XCpCall::getRemoteUserAgent(const UtlString& sSipCallId,
                                     const UtlString& sLocalTag,
                                     const UtlString& sRemoteTag,
                                     UtlString& userAgent) const
{
   userAgent.remove(0);

   return OS_NOT_FOUND;
}

OsStatus XCpCall::getMediaConnectionId(int& mediaConnID) const
{
   mediaConnID = -1;

   return OS_INVALID;
}

OsStatus XCpCall::getSipDialog(const UtlString& sSipCallId,
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
