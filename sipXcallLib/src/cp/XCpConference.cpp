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

XCpConference::XCpConference(const UtlString& sId,
                             SipUserAgent& rSipUserAgent,
                             CpMediaInterfaceFactory& rMediaInterfaceFactory,
                             OsMsgQ& rCallManagerQueue)
: XCpAbstractCall(sId, rSipUserAgent, rMediaInterfaceFactory, rCallManagerQueue)
{

}

XCpConference::~XCpConference()
{
   waitUntilShutDown();
}

/* ============================ MANIPULATORS ============================== */

OsStatus XCpConference::connect(const UtlString& sSipCallId,
                                SipDialog& sSipDialog,
                                const UtlString& toAddress,
                                const UtlString& fullLineUrl,
                                const UtlString& locationHeader,
                                CP_CONTACT_ID contactId)
{
   return OS_FAILED;
}

OsStatus XCpConference::acceptConnection(const UtlString& locationHeader,
                                         CP_CONTACT_ID contactId)
{
   return OS_FAILED;
}

OsStatus XCpConference::rejectConnection()
{
   return OS_FAILED;
}

OsStatus XCpConference::redirectConnection(const UtlString& sRedirectSipUri)
{
   return OS_FAILED;
}

OsStatus XCpConference::answerConnection()
{
   return OS_FAILED;
}

OsStatus XCpConference::dropConnection(const SipDialog& sSipDialog)
{
   return OS_FAILED;
}

OsStatus XCpConference::dropAllConnections(UtlBoolean bDestroyConference)
{
   return OS_FAILED;
}

OsStatus XCpConference::transferBlind(const SipDialog& sSipDialog,
                                      const UtlString& sTransferSipUri)
{
   return OS_FAILED;
}

OsStatus XCpConference::holdConnection(const SipDialog& sSipDialog)
{
   return OS_FAILED;
}

OsStatus XCpConference::holdAllConnections()
{
   return OS_FAILED;
}

OsStatus XCpConference::unholdAllConnections()
{
   return OS_FAILED;
}

OsStatus XCpConference::unholdConnection(const SipDialog& sSipDialog)
{
   return OS_FAILED;
}

OsStatus XCpConference::silentHoldRemoteConnection(const SipDialog& sSipDialog)
{
   return OS_FAILED;
}

OsStatus XCpConference::silentHoldRemoteAllConnections()
{
   return OS_FAILED;
}

OsStatus XCpConference::silentUnholdRemoteConnection(const SipDialog& sSipDialog)
{
   return OS_FAILED;
}

OsStatus XCpConference::silentUnholdRemoteAllConnections()
{
   return OS_FAILED;
}

OsStatus XCpConference::silentHoldLocalConnection(const SipDialog& sSipDialog)
{
   return OS_FAILED;
}

OsStatus XCpConference::silentUnholdLocalConnection(const SipDialog& sSipDialog)
{
   return OS_FAILED;
}

OsStatus XCpConference::limitCodecPreferences(CP_AUDIO_BANDWIDTH_ID audioBandwidthId,
                                              const UtlString& sAudioCodecs,
                                              CP_VIDEO_BANDWIDTH_ID videoBandwidthId,
                                              const UtlString& sVideoCodecs)
{
   return OS_FAILED;
}

OsStatus XCpConference::renegotiateCodecsConnection(const SipDialog& sSipDialog,
                                                    CP_AUDIO_BANDWIDTH_ID audioBandwidthId,
                                                    const UtlString& sAudioCodecs,
                                                    CP_VIDEO_BANDWIDTH_ID videoBandwidthId,
                                                    const UtlString& sVideoCodecs)
{
   return OS_FAILED;
}

OsStatus XCpConference::renegotiateCodecsAllConnections(CP_AUDIO_BANDWIDTH_ID audioBandwidthId,
                                                        const UtlString& sAudioCodecs,
                                                        CP_VIDEO_BANDWIDTH_ID videoBandwidthId,
                                                        const UtlString& sVideoCodecs)
{
   return OS_FAILED;
}

OsStatus XCpConference::sendInfo(const SipDialog& sSipDialog,
                                 const UtlString& sContentType,
                                 const char* pContent,
                                 const size_t nContentLength)
{
   return OS_FAILED;
}

/* ============================ ACCESSORS ================================= */

/* ============================ INQUIRY =================================== */

XCpAbstractCall::DialogMatchEnum XCpConference::hasSipDialog(const SipDialog& sSipDialog) const
{
   return XCpAbstractCall::MISMATCH;
}

int XCpConference::getCallCount() const
{
   return 0;
}

OsStatus XCpConference::getConferenceSipCallIds(UtlSList& sipCallIdList) const
{
   sipCallIdList.destroyAll();
   return OS_FAILED;
}

OsStatus XCpConference::getRemoteUserAgent(const SipDialog& sSipDialog,
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

OsStatus XCpConference::getSipDialog(const SipDialog& sSipDialog,
                                     SipDialog& sOutputSipDialog) const
{
   sOutputSipDialog = SipDialog(); // assign empty SipDialog

   return OS_NOT_FOUND;
}

/* //////////////////////////// PROTECTED ///////////////////////////////// */

/* //////////////////////////// PRIVATE /////////////////////////////////// */

/* ============================ FUNCTIONS ================================= */
