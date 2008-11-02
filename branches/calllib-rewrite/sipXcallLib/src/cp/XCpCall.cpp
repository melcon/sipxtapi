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

XCpCall::XCpCall(const UtlString& sId,
                 SipUserAgent& rSipUserAgent,
                 CpMediaInterfaceFactory& rMediaInterfaceFactory,
                 OsMsgQ& rCallManagerQueue)
: XCpAbstractCall(sId, rSipUserAgent, rMediaInterfaceFactory, rCallManagerQueue)
{

}

XCpCall::~XCpCall()
{
   waitUntilShutDown();
}

/* ============================ MANIPULATORS ============================== */

OsStatus XCpCall::connect(const UtlString& sSipCallId,
                          SipDialog& sSipDialog,
                          const UtlString& toAddress,
                          const UtlString& fullLineUrl,
                          const UtlString& locationHeader,
                          CP_CONTACT_ID contactId)
{
   // TODO: implement
   return OS_FAILED;
}

OsStatus XCpCall::acceptConnection(const UtlString& locationHeader,
                                   CP_CONTACT_ID contactId)
{
   // TODO: implement
   return OS_FAILED;
}

OsStatus XCpCall::rejectConnection()
{
   // TODO: implement
   return OS_FAILED;
}

OsStatus XCpCall::redirectConnection(const UtlString& sRedirectSipUri)
{
   // TODO: implement
   return OS_FAILED;
}

OsStatus XCpCall::answerConnection()
{
   // TODO: implement
   return OS_FAILED;
}

OsStatus XCpCall::dropConnection(const SipDialog& sSipDialog)
{
   // TODO: implement
   return OS_FAILED;
}

OsStatus XCpCall::dropConnection(UtlBoolean bDestroyCall)
{
   // TODO: implement
   return OS_FAILED;
}

OsStatus XCpCall::transferBlind(const SipDialog& sSipDialog,
                                const UtlString& sTransferSipUri)
{
   // TODO: implement
   return OS_FAILED;
}

OsStatus XCpCall::holdConnection(const SipDialog& sSipDialog)
{
   // TODO: implement
   return OS_FAILED;
}

OsStatus XCpCall::holdConnection()
{
   // TODO: implement
   return OS_FAILED;
}

OsStatus XCpCall::unholdConnection(const SipDialog& sSipDialog)
{
   // TODO: implement
   return OS_FAILED;
}

OsStatus XCpCall::unholdConnection()
{
   // TODO: implement
   return OS_FAILED;
}

OsStatus XCpCall::muteInputConnection(const SipDialog& sSipDialog)
{
   // TODO: implement
   return OS_FAILED;
}

OsStatus XCpCall::unmuteInputConnection(const SipDialog& sSipDialog)
{
   // TODO: implement
   return OS_FAILED;
}

OsStatus XCpCall::limitCodecPreferences(CP_AUDIO_BANDWIDTH_ID audioBandwidthId,
                                        const UtlString& sAudioCodecs,
                                        CP_VIDEO_BANDWIDTH_ID videoBandwidthId,
                                        const UtlString& sVideoCodecs)
{
   // TODO: implement
   return OS_FAILED;
}

OsStatus XCpCall::renegotiateCodecsConnection(const SipDialog& sSipDialog,
                                              CP_AUDIO_BANDWIDTH_ID audioBandwidthId,
                                              const UtlString& sAudioCodecs,
                                              CP_VIDEO_BANDWIDTH_ID videoBandwidthId,
                                              const UtlString& sVideoCodecs)
{
   // TODO: implement
   return OS_FAILED;
}

OsStatus XCpCall::sendInfo(const SipDialog& sSipDialog,
                           const UtlString& sContentType,
                           const char* pContent,
                           const size_t nContentLength)
{
   // TODO: implement
   return OS_FAILED;
}

/* ============================ ACCESSORS ================================= */

/* ============================ INQUIRY =================================== */

XCpAbstractCall::DialogMatchEnum XCpCall::hasSipDialog(const SipDialog& sSipDialog) const
{
   // TODO: implement
   return XCpAbstractCall::MISMATCH;
}

int XCpCall::getCallCount() const
{
   // TODO: implement
   return 0;
}

OsStatus XCpCall::getCallSipCallId(UtlString& sSipCallId) const
{
   // TODO: implement
   sSipCallId.remove(0);
   return OS_FAILED;
}

OsStatus XCpCall::getRemoteUserAgent(const SipDialog& sSipDialog,
                                     UtlString& userAgent) const
{
   // TODO: implement
   userAgent.remove(0);

   return OS_NOT_FOUND;
}

OsStatus XCpCall::getMediaConnectionId(int& mediaConnID) const
{
   // TODO: implement
   mediaConnID = -1;

   return OS_INVALID;
}

OsStatus XCpCall::getSipDialog(const SipDialog& sSipDialog,
                               SipDialog& dialog) const
{
   // TODO: implement
   dialog = SipDialog(); // assign empty SipDialog

   return OS_NOT_FOUND;
}

/* //////////////////////////// PROTECTED ///////////////////////////////// */

/* //////////////////////////// PRIVATE /////////////////////////////////// */

/* ============================ FUNCTIONS ================================= */
