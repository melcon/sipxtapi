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
#include <os/OsLock.h>
#include <os/OsPtrLock.h>
#include <net/SipDialog.h>
#include <cp/XCpCall.h>
#include <cp/XSipConnection.h>

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
, m_pSipConnection(NULL)
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

SipDialog::DialogMatchEnum XCpCall::hasSipDialog(const SipDialog& sSipDialog) const
{
   OsLock lock(m_memberMutex);

   if (m_pSipConnection)
   {
      return m_pSipConnection->compareSipDialog(sSipDialog);
   }

   return SipDialog::DIALOG_MISMATCH;
}

int XCpCall::getCallCount() const
{
   return m_pSipConnection != NULL ? 1 : 0;
}

OsStatus XCpCall::getCallSipCallId(UtlString& sSipCallId) const
{
   OsStatus result = OS_INVALID;

   OsPtrLock<XSipConnection> ptrLock; // auto pointer lock
   UtlBoolean resFind = getConnection(ptrLock);
   if (resFind)
   {
      ptrLock->getSipCallId(sSipCallId);
      result = OS_SUCCESS;
   }

   return result;
}

/* //////////////////////////// PROTECTED ///////////////////////////////// */

UtlBoolean XCpCall::findConnection(const SipDialog& sSipDialog, OsPtrLock<XSipConnection>& ptrLock) const
{
   OsLock lock(m_memberMutex);

   if (m_pSipConnection && m_pSipConnection->compareSipDialog(sSipDialog) != SipDialog::DIALOG_MISMATCH)
   {
      // dialog matches
      ptrLock = m_pSipConnection;
      return TRUE;
   }

   return FALSE;
}

UtlBoolean XCpCall::getConnection(OsPtrLock<XSipConnection>& ptrLock) const
{
   OsLock lock(m_memberMutex);

   ptrLock = m_pSipConnection;
   return m_pSipConnection != NULL;
}

/* //////////////////////////// PRIVATE /////////////////////////////////// */

/* ============================ FUNCTIONS ================================= */
