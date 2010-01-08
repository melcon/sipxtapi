//
// Copyright (C) 2005-2007 SIPez LLC.
// Licensed to SIPfoundry under a Contributor Agreement.
// 
// Copyright (C) 2004-2007 SIPfoundry Inc.
// Licensed by SIPfoundry under the LGPL license.
//
// Copyright (C) 2004-2006 Pingtel Corp.  All rights reserved.
// Licensed to SIPfoundry under a Contributor Agreement.
//
// $$
///////////////////////////////////////////////////////////////////////////////

// Author: Daniel Petrie dpetrie AT SIPez DOT com

// SYSTEM INCLUDES
#include "os/OsDefs.h"

// APPLICATION INCLUDES
#include "utl/UtlRegex.h"
#include <os/OsReadLock.h>
#include <os/OsWriteLock.h>
#include <os/OsProtectEvent.h>
#include "os/OsQueuedEvent.h"
#include "os/OsTimer.h"
#include "os/OsTime.h"
#include "os/OsDateTime.h"
#include "os/OsEventMsg.h"
#include "os/OsIntPtrMsg.h"
#include <cp/CpPeerCall.h>
#include <cp/CpCallManager.h>
#include <cp/CpIntMessage.h>
#include <cp/CpMultiStringMessage.h>
#include "cp/CpNotificationMsgDef.h"
#include <cp/SipConnection.h>
#include <cp/CpGhostConnection.h>
#include <cp/CpCallStateEventListener.h>
#include <mi/CpMediaInterface.h>
#include <net/SipMessageEvent.h>
#include <net/SipUserAgent.h>
#include <net/SipLineProvider.h>
#include <net/NameValueTokenizer.h>
#include <net/Url.h>
#include "net/SmimeBody.h"
#include "ptapi/PtCall.h"
#include <ptapi/PtConnection.h>
#include <ptapi/PtTerminalConnection.h>

// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
#define CALL_STATUS_FIELD       "status"
#define CALL_DELETE_DELAY_SECS  2     // Number of seconds between a drop 
// request (call) and call deletion
// (call manager)
#ifdef _WIN32
#   define CALL_CONTROL_TONES
#endif

// STATIC VARIABLE INITIALIZATIONS

/* //////////////////////////// PUBLIC //////////////////////////////////// */

/* ============================ CREATORS ================================== */

// Constructor
CpPeerCall::CpPeerCall(UtlBoolean isEarlyMediaFor180Enabled,
                       CpCallManager* callManager,
                       const UtlString& sMediaInterfaceLocalIp,
                       CpCallStateEventListener* pCallEventListener,
                       SipInfoStatusEventListener* pInfoStatusEventListener,
                       SipSecurityEventListener* pSecurityEventListener,
                       CpMediaEventListener* pMediaEventListener,
                       int callIndex,
                       const char* callId,
                       SipUserAgent* sipUA, 
                       int sipSessionReinviteTimer,
                       int offeringDelayMilliSeconds,
                       int availableBehavior, 
                       const char* forwardUnconditionalUrl,
                       int busyBehavior, 
                       const char* forwardOnBusyUrl,
                       int forwardOnNoAnswerMilliSeconds, 
                       const char* forwardOnNoAnswerUrl,
                       int ringingExpireSeconds)
                       : CpCall(callManager, NULL, callIndex, callId)
                       , mConnectionMutex(OsRWMutex::Q_PRIORITY)
                       , mIsEarlyMediaFor180(TRUE)
                       , mpSecurity(NULL)
                       , m_pCallEventListener(pCallEventListener)
                       , m_pInfoStatusEventListener(pInfoStatusEventListener)
                       , m_pSecurityEventListener(pSecurityEventListener)
                       , m_pMediaEventListener(pMediaEventListener)
{
   // SIP and Peer to Peer call intialization
   sipUserAgent = sipUA;
   mIsEarlyMediaFor180 = isEarlyMediaFor180Enabled;
   mSipSessionReinviteTimer = sipSessionReinviteTimer;
   offeringDelay = offeringDelayMilliSeconds;
   lineAvailableBehavior = availableBehavior;
   if(lineAvailableBehavior == Connection::FORWARD_UNCONDITIONAL &&
      forwardUnconditionalUrl != NULL)
   {
      forwardUnconditional.append(forwardUnconditionalUrl);
   }
   lineBusyBehavior = busyBehavior;
   if(lineBusyBehavior == Connection::FORWARD_ON_BUSY &&
      forwardOnBusyUrl != NULL)
   {
      forwardOnBusy.append(forwardOnBusyUrl);
   }
   if(forwardOnNoAnswerUrl != NULL && strlen(forwardOnNoAnswerUrl) > 0)
   {
      if ( forwardOnNoAnswerMilliSeconds > -1)
         noAnswerTimeout = forwardOnNoAnswerMilliSeconds;
      else
         noAnswerTimeout = 24;        // default

      forwardOnNoAnswer.append(forwardOnNoAnswerUrl);
   }

   else
   {
      noAnswerTimeout = ringingExpireSeconds;
   }

   mDialMode = ADD_PARTY;
   setCallType(CP_NORMAL_CALL);
   mbRequestedDrop = false;
   eLastMajor = (SIPX_CALLSTATE_EVENT) -1;
   eLastMinor = (SIPX_CALLSTATE_CAUSE) -1;

   // send a message to ourselves to create media interface
   CpMultiStringMessage createInterfaceMsg(CpCallManager::CP_CREATE_MEDIA_INTERFACE, sMediaInterfaceLocalIp);
   postMessage(createInterfaceMsg);
}

// Copy constructor
CpPeerCall::CpPeerCall(const CpPeerCall& rCpPeerCall) :
mConnectionMutex(OsRWMutex::Q_PRIORITY),
mpSecurity(NULL)
{
}

// Destructor
CpPeerCall::~CpPeerCall()
{
   // Notify the call manager of this object's impending demise
   if (mpManager)
   {
      mpManager->onCallDestroy(this);
   }
   waitUntilShutDown(); // shutdown so that connections do not try to process messages
   Connection* connection = NULL;
   while ((connection = (Connection*) mConnections.get()))
   {
      delete connection;
      connection = NULL;
   }
}

/* ============================ MANIPULATORS ============================== */

// Assignment operator
CpPeerCall& CpPeerCall::operator=(const CpPeerCall& rhs)
{
   if (this == &rhs)            // handle the assignment to self case
      return *this;

   return *this;
}

// Handles the processing of a CallManager::CP_DIAL_STRING message
UtlBoolean CpPeerCall::handleDialString(OsMsg* pEventMessage)
{
   UtlString remoteAddress;
   UtlString localAddress;
   UtlString desiredCallId;
   UtlString remoteHostName;
   UtlString locationHeader;
   SIPX_CONTACT_ID contactId;

   ((CpMultiStringMessage*)pEventMessage)->getString1Data(remoteAddress);
   ((CpMultiStringMessage*)pEventMessage)->getString2Data(desiredCallId);
   ((CpMultiStringMessage*)pEventMessage)->getString3Data(localAddress);
   ((CpMultiStringMessage*)pEventMessage)->getString5Data(locationHeader);
   contactId = (SIPX_CONTACT_ID) ((CpMultiStringMessage*)pEventMessage)->getInt1Data();
   void* pDisplay = (void*) ((CpMultiStringMessage*)pEventMessage)->getInt2Data();
   void* pSecurity = (void*) ((CpMultiStringMessage*)pEventMessage)->getInt3Data();
   int bandWidth = ((CpMultiStringMessage*)pEventMessage)->getInt4Data();
   SIPX_TRANSPORT_DATA* pTransport = (SIPX_TRANSPORT_DATA*)((CpMultiStringMessage*)pEventMessage)->getInt5Data();
   RTP_TRANSPORT rtpTransportOptions = (RTP_TRANSPORT)((CpMultiStringMessage*)pEventMessage)->getInt6Data();
   const char* locationHeaderData = (locationHeader.length() == 0) ? NULL : locationHeader.data();

   // If all digits or * assume this is a userid not a server address
   RegEx allDigits("^[0-9*]+$");
   if(allDigits.Search(remoteAddress.data()))
   {
      remoteHostName.append(remoteAddress.data());
      remoteHostName.append('@');
   }
   else
   {
      remoteHostName.append(remoteAddress.data());
   }

   if(!isCallIdSet())
   {
      UtlString callId;
      mpManager->getNewCallId(&callId);
      setCallId(callId.data());
   }

   // Initial two party call or adding a party
   if(mDialMode == ADD_PARTY)
   { 
      if (desiredCallId.length() != 0)
      {
         // Use supplied callId
         addParty(localAddress, remoteHostName, NULL, NULL, desiredCallId.data(), contactId, pDisplay, pSecurity, 
            locationHeaderData, bandWidth, false, NULL, pTransport, rtpTransportOptions);
      }
      else
      {
         // Use default call id
         addParty(localAddress, remoteHostName, NULL, NULL, NULL, contactId, pDisplay, pSecurity,
            locationHeaderData, bandWidth, false, NULL, pTransport, rtpTransportOptions);
      }        
   } 
   delete pTransport;  // (SipConnection should have a copy of it)
   return TRUE;
}

// Handles the processing of a CallManager::CP_BLIND_TRANSFER and
// CallManager::CP_CONSULT_TRANSFER messages
UtlBoolean CpPeerCall::handleTransfer(OsMsg* pEventMessage)
{
   int msgSubType = pEventMessage->getMsgSubType();

   // This message is received by the original call on
   // the transfer controller
   if(getCallType() == CP_NORMAL_CALL)
   { // case variable scope
      setCallType(CP_TRANSFER_CONTROLLER_ORIGINAL_CALL);

      int metaEventId = ((CpMultiStringMessage*)pEventMessage)->getInt1Data();
      UtlString targetCallId;
      ((CpMultiStringMessage*)pEventMessage)->getString3Data(targetCallId);
      setTargetCallId(targetCallId.data());

      UtlString thisCallId;
      getCallId(thisCallId);
      const char* metaCallIds[2];
      metaCallIds[0] = targetCallId.data(); // New call first
      metaCallIds[1] = thisCallId.data();

      // Start the meta event
      startMetaEvent(metaEventId, PtEvent::META_CALL_TRANSFERRING, 2, metaCallIds);

      // Create the target call
      mpManager->createCall(&targetCallId, metaEventId, 
         PtEvent::META_CALL_TRANSFERRING, 2, metaCallIds,
         FALSE);  // Do  not assume focus if there is no infocus call
      // as this is not a real call.  It is a place holder for call
      // state on the remote call

      if(msgSubType == CallManager::CP_BLIND_TRANSFER)
      {
         // Transfer does an implicit hold.
         // The local hold is done here.  The remote hold is
         // done on each connection.
         localHold();
      }

      Connection* connection = NULL;
      UtlString transferTargetAddress;
      ((CpMultiStringMessage*)pEventMessage)->getString2Data(transferTargetAddress);
      OsReadLock lock(mConnectionMutex);
      UtlDListIterator iterator(mConnections);
      while ((connection = dynamic_cast<Connection*>(iterator())))
      {
         // Do the transfer operation on each connection in this call
         UtlBoolean isOk = connection->originalCallTransfer(transferTargetAddress, 
            NULL, targetCallId.data());

         if (!isOk)
         {
            /** SIPXTAPI: TBD **/
         }
         else
         {
            // Send a message to the target call for each transfered
            // connection
            UtlString originalCallId;
            UtlString connectionAddress;
            connection->getCallId(&originalCallId);
            connection->getRemoteAddress(&connectionAddress);
            CpMultiStringMessage transferConnect(CallManager::CP_TRANSFER_CONNECTION,
               targetCallId.data(), 
               transferTargetAddress.data(), 
               originalCallId.data(),
               connectionAddress.data(),
               NULL,
               metaEventId);
            mpManager->postMessage(transferConnect);
         }
      }
   }

   return TRUE;
}

// Handles the processing of a CallManager::CP_CONSULT_TRANSFER_ADDRESS
// message.
UtlBoolean CpPeerCall::handleTransferAddress(OsMsg* pEventMessage)
{
   CpMultiStringMessage* pMessage = (CpMultiStringMessage*) pEventMessage;

   UtlString sourceCallId;
   UtlString sourceAddress;
   UtlString targetCallId;
   UtlString targetAddress;
   UtlString targetUrl;

   // Parse parameters
   pMessage->getString1Data(sourceCallId);
   pMessage->getString2Data(sourceAddress);
   pMessage->getString3Data(targetCallId);
   pMessage->getString4Data(targetAddress);
   pMessage->getString5Data(targetUrl);

   Connection* pConnection = findHandlingConnection(sourceAddress);
   if (pConnection)
   {
      UtlBoolean bRC = pConnection->originalCallTransfer(targetUrl, sourceAddress, targetCallId);
      if (!bRC)
      {

      }
   }

   return TRUE;
}


// Handles the processing of a CallManager::CP_TRANSFER_CONNECTION message
UtlBoolean CpPeerCall::handleTransferConnection(OsMsg* pEventMessage)
{
   UtlString originalCallId;
   UtlString currentOriginalCallId;
   getOriginalCallId(currentOriginalCallId);
   UtlString transferTargetAddress;
   UtlString connectionAddress;
   ((CpMultiStringMessage*)pEventMessage)->getString2Data(transferTargetAddress);
   ((CpMultiStringMessage*)pEventMessage)->getString3Data(originalCallId);
   ((CpMultiStringMessage*)pEventMessage)->getString4Data(connectionAddress);
   // If it is legal for this call to be a transfer target
   if(getCallType() == CP_NORMAL_CALL ||
      (getCallType() == CP_TRANSFER_CONTROLLER_TARGET_CALL &&
      currentOriginalCallId.compareTo(originalCallId) == 0))
   {
      // Set the original call id so that we can send messages
      // back if necessary.
      if(getCallType() == CP_NORMAL_CALL)
      {
         setOriginalCallId(originalCallId.data());
         setCallType(CP_TRANSFER_CONTROLLER_TARGET_CALL);
      }

      // Find the connection
      // Currently do not need to lock as we should not get
      // a connection back and if we do we do nothing with it.
      //OsReadLock lock(mConnectionMutex);
      Connection* connection = findHandlingConnection(connectionAddress);

      // The connection does not exist, for now we can assume
      // this is a blind transfer.  Create a ghost connection
      // and put it in the offering state
      if(! connection)
      {
         UtlString thisCallId;
         getCallId(thisCallId);
         mLocalConnectionState = PtEvent::CONNECTION_ESTABLISHED;
         mLocalTermConnectionState = PtTerminalConnection::TALKING;

         connection = new CpGhostConnection(mpManager, this, 
            thisCallId.data());
         addConnection(connection);
         connection->targetCallBlindTransfer(connectionAddress, NULL);
      }
   }


   /*
   * WARNING: We should creating another connection for the TARGET address.
   *          We are not doing this now, because we don't have any code that
   *          steps through state progressions.
   *
   * TODO: CODE ME
   */

   return TRUE;
}


// Handles the processing of a CallManager::CP_TRANSFEREE_CONNECTION message
UtlBoolean CpPeerCall::handleTransfereeConnection(OsMsg* pEventMessage)
{
   // Message sent to target call on transferee

   UtlString referTo;
   UtlString referredBy;
   UtlString originalCallId;
   UtlString currentOriginalCallId;
   UtlString localAddress;
   getOriginalCallId(currentOriginalCallId);
   UtlString originalConnectionAddress;
   ((CpMultiStringMessage*)pEventMessage)->getString2Data(referTo);
   ((CpMultiStringMessage*)pEventMessage)->getString3Data(referredBy);
   ((CpMultiStringMessage*)pEventMessage)->getString4Data(originalCallId);
   ((CpMultiStringMessage*)pEventMessage)->getString5Data(originalConnectionAddress);
   ((CpMultiStringMessage*)pEventMessage)->getString6Data(localAddress);
   UtlBoolean bOnHold = ((CpMultiStringMessage*)pEventMessage)->getInt1Data();
   RTP_TRANSPORT rtpTransportOptions = (RTP_TRANSPORT) ((CpMultiStringMessage*)pEventMessage)->getInt2Data();

   if(getCallType() == CP_NORMAL_CALL ||
      (getCallType() == CP_TRANSFEREE_TARGET_CALL &&
      currentOriginalCallId.compareTo(originalCallId) == 0))
   {
      if(getCallType() == CP_NORMAL_CALL) setOriginalCallId(originalCallId);
      // Do not need to lock as connection is never touched
      // and addConnection does its own locking
      //OsReadLock lock(mConnectionMutex);
      UtlString cleanReferTo;
      Url referToUrl(referTo);
      referToUrl.removeHeaderParameters();
      referToUrl.toString(cleanReferTo);
      Connection* connection;

      connection = findHandlingConnection(cleanReferTo);
      if(!connection)
      {
         // Create a new connection on this call to connect to the
         // transfer target.
         addParty(localAddress,
            referTo, 
            referredBy, 
            originalConnectionAddress, 
            NULL, 
            0, 
            NULL, 
            NULL, 
            NULL, // locationHeader
            AUDIO_CODEC_BW_DEFAULT, 
            bOnHold, 
            originalCallId);
         // Note: The connection is added to the call in addParty
      }
   }

   return TRUE;
}

// Handles the processing of a CallManager::CP_SIP_MESSAGE message
UtlBoolean CpPeerCall::handleSipMessage(OsMsg* pEventMessage)
{
   UtlBoolean bAddedConnection = FALSE;

   // There are of course small windows between:
   // findHandlingConnection, addConnection and the
   // read lock taken below.  But you cannot have a
   // read or write lock nested in a write lock.
   Connection* connection = 
      findHandlingConnection(*pEventMessage);
   const SipMessage* pSipMsg = ((SipMessageEvent*)pEventMessage)->getMessage();

   UtlString name = getName();
   if(connection == NULL)
   {
      if (SipConnection::shouldCreateConnection(*sipUserAgent, 
         *pEventMessage))
      {
         // extract our localAddress from sip message as we do not know it at this place
         UtlString sRequestUri;
         pSipMsg->getRequestUri(&sRequestUri);
         Url requestUrl(sRequestUri);// convert request uri into url. Request uri cannot have a field tag
         // this is not supposed to be a lineUri. Call manager doesn't know about "lines". This requestUrl
         // needs to keep parameters line transport=tcp from the original request
         connection = new SipConnection(requestUrl.toString(),
            mIsEarlyMediaFor180,
            mpManager,
            this,
            mpMediaInterface, 
            //mpCallUiContext,
            sipUserAgent,
            m_pCallEventListener,
            m_pInfoStatusEventListener,
            m_pSecurityEventListener,
            m_pMediaEventListener,
            offeringDelay, 
            mSipSessionReinviteTimer,
            lineAvailableBehavior, 
            forwardUnconditional.data(),
            lineBusyBehavior, 
            forwardOnBusy.data());
         connection->setBindIPAddress(m_bindIPAddress);
         ((SipConnection*)connection)->setSecurity(mpSecurity);
         UtlString voiceQualityReportTarget;
         if (mpManager->getVoiceQualityReportTarget(voiceQualityReportTarget))
         {
            ((SipConnection*)connection)->setVoiceQualityReportTarget(
               voiceQualityReportTarget);
         }
         addConnection(connection);
         bAddedConnection = TRUE;
      }
      else
      {
         SipConnection::processNewFinalMessage(sipUserAgent, pEventMessage);
      }
   }

   if(connection)
   {
      OsReadLock lock(mConnectionMutex);
      int previousConnectionState = connection->getState();
      //PtTerminalConnection::TerminalConnectionState prevTermConState;
      //int gotPrevTermConState = getUiTerminalConnectionState(prevTermConState);

      //                if (previousConnectionState == Connection::CONNECTION_IDLE)
      //                {
      //                    startMetaEvent( mpManager->getNewMetaEventId(), 
      //                                    PtEvent::META_CALL_STARTING, 
      //                                    0, 
      //                                    0);
      //                }

      connection->processMessage(*pEventMessage, mCallInFocus);

      int currentConnectionState = connection->getState();    
      if ( ((previousConnectionState != currentConnectionState) || 
         (getCallState() == PtCall::IDLE)) &&
         ((currentConnectionState == Connection::CONNECTION_OFFERING) ||
         (currentConnectionState == Connection::CONNECTION_ALERTING)) )
      {
         UtlString responseText;
         connection->getResponseText(responseText);
         setCallState(connection->getResponseCode(), responseText, PtCall::ACTIVE);
         /** SIPXTAPI: TBD **/            
      }

      if (previousConnectionState == Connection::CONNECTION_IDLE && 
         currentConnectionState == Connection::CONNECTION_OFFERING)
      {
         stopMetaEvent(connection->isRemoteCallee());
      }

      // If this call does not have a callId set it
      if(!isCallIdSet() &&
         (currentConnectionState != Connection::CONNECTION_FAILED ||
         currentConnectionState != Connection::CONNECTION_DISCONNECTED ||
         currentConnectionState != Connection::CONNECTION_IDLE))
      {
         UtlString callId;
         connection->getCallId(&callId);
         setCallId(callId.data());
      }

   } // End if we created a new connection or used and existing one            

   // Check if call is dead and drop it if it is
   dropIfDead();

   return TRUE;
}

// Handles the processing of a CallManager::CP_DROP_CONNECTION message
UtlBoolean CpPeerCall::handleDropConnection(OsMsg* pEventMessage)
{
   {
      OsReadLock lock(mConnectionMutex);
      UtlString connectionAddress;
      UtlString sessionCallID;
      ((CpMultiStringMessage*)pEventMessage)->getString1Data(sessionCallID);
      ((CpMultiStringMessage*)pEventMessage)->getString2Data(connectionAddress);

      assert(!sessionCallID.isNull());
      Connection* connection = findHandlingConnection(connectionAddress, sessionCallID);

      if(connection)
      {
         // do not fire the tapi event if it is a ghost connection
         CpGhostConnection* pGhost = NULL;
         pGhost = dynamic_cast<CpGhostConnection*>(connection);
         if (!pGhost)
         {
            connection->fireSipXCallEvent(CALLSTATE_DISCONNECTED, CALLSTATE_CAUSE_NORMAL);
         }
         connection->hangUp();
      }
   }

   // Check if call is dead and drop it if it is
   dropIfDead();

   return TRUE;
}


// Handles the processing of a CallManager::CP_FORCE_DROP_CONNECTION message
UtlBoolean CpPeerCall::handleForceDropConnection(OsMsg* pEventMessage)
{
   {
      OsReadLock lock(mConnectionMutex);
      UtlString connectionAddress;
      ((CpMultiStringMessage*)pEventMessage)->getString2Data(connectionAddress);
      Connection* connection = findHandlingConnection(connectionAddress);

      if(connection)
      {
         connection->forceHangUp();
         /** SIPXTAPI: TBD **/

      }
      mLocalConnectionState = PtEvent::CONNECTION_DISCONNECTED;
      mLocalTermConnectionState = PtTerminalConnection::DROPPED;
   }

   // Check if call is dead and drop it if it is
   dropIfDead();

   return TRUE;
}

// Handles the processing of a CallManager::CP_GET_CALLED_ADDRESSES and
// CallManager::CP_GET_CALLING_ADDRESSES messages
UtlBoolean CpPeerCall::handleGetAddresses(OsMsg* pEventMessage)
{
   int msgSubType = pEventMessage->getMsgSubType();

   int numConnections = 0;
   UtlSList* connectionList;
   OsProtectedEvent* getConnEvent = (OsProtectedEvent*) ((CpMultiStringMessage*)pEventMessage)->getInt1Data();
   getConnEvent->getIntData((int&)connectionList);

   if(getConnEvent)
   {
      // Get the remote connection(s)/address(es)
      { // scope for lock
         Connection* connection = NULL;
         UtlString address;
         OsReadLock lock(mConnectionMutex);
         UtlDListIterator iterator(mConnections);
         while ((connection = dynamic_cast<Connection*>(iterator())))
         {
            if((msgSubType == CallManager::CP_GET_CALLED_ADDRESSES &&
               connection->isRemoteCallee() ) ||
               (msgSubType == CallManager::CP_GET_CALLING_ADDRESSES &&
               !connection->isRemoteCallee()))
            {
               connection->getRemoteAddress(&address);
               connectionList->append(new UtlString(address));
               numConnections++;
            }
         }
      }
      // Signal the caller that we are done.
      // If the event has already been signalled, clean up
      if(OS_ALREADY_SIGNALED == getConnEvent->signal(numConnections))
      {
         // The other end must have timed out on the wait
         connectionList->destroyAll();
         delete connectionList;
         OsProtectEventMgr* eventMgr = OsProtectEventMgr::getEventMgr();
         eventMgr->release(getConnEvent);
      }
   }

   return TRUE;
}


// Handles the processing of a CallManager::CP_ACCEPT_CONNECTION 
// message
UtlBoolean CpPeerCall::handleAcceptConnection(OsMsg* pEventMessage)
{
   UtlString remoteAddress;
   UtlString locationHeader;
   UtlBoolean connectionFound = FALSE;
   ((CpMultiStringMessage*)pEventMessage)->getString2Data(remoteAddress);
   ((CpMultiStringMessage*)pEventMessage)->getString5Data(locationHeader);
   SIPX_CONTACT_ID contactId = (SIPX_CONTACT_ID) ((CpMultiStringMessage*)pEventMessage)->getInt1Data();
   void* hWnd = (void*) ((CpMultiStringMessage*)pEventMessage)->getInt2Data();
   void* security = (void*) ((CpMultiStringMessage*)pEventMessage)->getInt3Data();
   int bandWidth = ((CpMultiStringMessage*)pEventMessage)->getInt4Data();
   UtlBoolean sendEarlyMedia = ((CpMultiStringMessage*)pEventMessage)->getInt5Data();
   const char* locationHeaderData = (locationHeader.length() == 0) ? NULL : locationHeader.data();

   if (hWnd && mpMediaInterface)
   {
      mpMediaInterface->setVideoWindowDisplay(hWnd);
      if (security)
      {
         mpSecurity = (SIPXTACK_SECURITY_ATTRIBUTES*)security;
         mpMediaInterface->setSecurityAttributes(security);
      }
   }

   // This is a bit of a hack/short cut.
   // Find the first remote connection which is in the OFFERING
   // state and assume that it is the connection on
   // which the accept operation is to occur.  The difficulty
   // is that the operation is being called on the local connection
   // but the action must be invoked on the remote connection.  This
   // will need to be fixed for the general case to better support
   // conference calls.  The weird thing from the JTAPI perspective
   // is that the local connection may be ESTABLISHED and an 
   // incoming call (new connection) to join an existing conference 
   // call will make the local connection go to the OFFERING state.

   SipConnection* connection = NULL;
   UtlString address;
   int connectState;
   OsReadLock lock(mConnectionMutex);
   UtlDListIterator iterator(mConnections);

   while ((connection = dynamic_cast<SipConnection*>(iterator())))
   {
      connectState = connection->getState();
      if(connectState == Connection::CONNECTION_OFFERING)
      {
         connection->setContactId(contactId);
         connection->accept(noAnswerTimeout, 
            security, 
            locationHeaderData, 
            bandWidth, 
            sendEarlyMedia);
         connectionFound = TRUE;
         break;
      }
   }

   if(connectionFound)
   {
      //connection
   }
   return TRUE;
}


// Handles the processing of a CallManager::CP_REJECT_CONNECTION 
// message
UtlBoolean CpPeerCall::handleRejectConnection(OsMsg* pEventMessage)
{
   UtlString remoteAddress;
   UtlBoolean connectionFound = FALSE;

   ((CpMultiStringMessage*)pEventMessage)->getString2Data(remoteAddress);

   // This is a bit of a hack/short cut.
   // Find the first remote connection which is in the OFFERING
   // state and assume that it is the connection on
   // which the reject operation is to occur.  The difficulty
   // is that the operation is being called on the local connection
   // but the action must be invoked on the remote connection.  This
   // will need to be fixed for the general case to better support
   // conference calls.  The weird thing from the JTAPI perspective
   // is that the local connection may be ESTABLISHED and an 
   // incoming call (new connection) to join an existing conference 
   // call will make the local connection go to the OFFERING state.
   Connection* connection = NULL;
   UtlString address;
   int connectState;
   {
      OsReadLock lock(mConnectionMutex);
      UtlDListIterator iterator(mConnections);
      while ((connection = dynamic_cast<Connection*>(iterator())))
      {
         connectState = connection->getState();
         if(connectState == Connection::CONNECTION_OFFERING)
         {
            connection->reject();
            connectionFound = TRUE;
            break;
         }
      }
   }

   // Check if call is dead and drop it if it is
   dropIfDead();

   return TRUE;
}


// Handles the processing of a CallManager::CP_REDIRECT_CONNECTION 
// message
UtlBoolean CpPeerCall::handleRedirectConnection(OsMsg* pEventMessage)
{  
   UtlString remoteAddress;
   UtlString forwardAddress;
   UtlBoolean connectionFound = FALSE;
   ((CpMultiStringMessage*)pEventMessage)->getString2Data(remoteAddress);
   ((CpMultiStringMessage*)pEventMessage)->getString3Data(forwardAddress);

   // This is a bit of a hack/short cut.
   // Find the first remote connection which is in the OFFERING
   // or ALERTING state and assume that it is the connection on
   // which the redirect operation is to occur.  The difficulty
   // is that the operation is being called on the local connection
   // but the action must be invoked on the remote connection.  This
   // will need to be fixed for the general case to better support
   // conference calls.  The weird thing from the JTAPI perspective
   // is that the local connection may be ESTABLISHED and an 
   // incoming call (new connection) to join an existing conference 
   // call will make the local connection go to the OFFERING state.
   Connection* connection = NULL;
   UtlString address;
   int connectState;
   {
      OsReadLock lock(mConnectionMutex);
      UtlDListIterator iterator(mConnections);
      while ((connection = dynamic_cast<Connection*>(iterator())))
      {
         connectState = connection->getState();
         if(connectState == Connection::CONNECTION_OFFERING ||
            connectState == Connection::CONNECTION_ALERTING)
         {
            connection->redirect(forwardAddress.data());
            connectionFound = TRUE;
            break;
         }
      }
   }
   // Check if call is dead and drop it if it is
   dropIfDead();

   return TRUE;
}    


// Handles the processing of a CallManager::CP_HOLD_TERM_CONNECTION 
// message
UtlBoolean CpPeerCall::handleHoldTermConnection(OsMsg* pEventMessage)
{
   UtlString address;
   ((CpMultiStringMessage*)pEventMessage)->getString2Data(address);

   OsReadLock lock(mConnectionMutex);
   Connection* connection = findHandlingConnection(address);
   if(connection)
   {
      connection->hold();
   }

   return TRUE;
}


// Handles the processing of a CallManager::CP_HOLD_ALL_TERM_CONNECTIONS 
// message
UtlBoolean CpPeerCall::handleHoldAllTermConnection(OsMsg* pEventMessage)
{        
   // put all the connections on hold

   // The local connection:
   localHold();

   // All of the remote connections  
   OsReadLock lock(mConnectionMutex);
   UtlDListIterator iterator(mConnections);

   Connection* connection = NULL;
   while ((connection = dynamic_cast<Connection*>(iterator())))
   {
      connection->hold();
   }

   return TRUE;
}


// Handles the processing of a CallManager::CP_UNHOLD_TERM_CONNECTION 
// message
UtlBoolean CpPeerCall::handleUnholdTermConnection(OsMsg* pEventMessage)
{
   UtlString address;
   ((CpMultiStringMessage*)pEventMessage)->getString2Data(address);
   OsReadLock lock(mConnectionMutex);
   Connection* connection = findHandlingConnection(address);
   if(connection)
   {
      connection->offHold();
      UtlString remoteAddress;
      connection->getRemoteAddress(&remoteAddress);
      if (mLocalTermConnectionState != PtTerminalConnection::TALKING &&
         mLocalTermConnectionState != PtTerminalConnection::IDLE)
      {
         UtlString responseText;
         connection->getResponseText(responseText);
         postTaoListenerMessage(connection->getResponseCode(), 
            responseText, 
            PtEvent::TERMINAL_CONNECTION_TALKING, 
            TERMINAL_CONNECTION_STATE, 
            PtEvent::CAUSE_UNHOLD, 
            connection->isRemoteCallee(), 
            remoteAddress);
      }
   }
   return TRUE;
}

// Handles the processing of a CallManager::CP_RENEGOTIATE_CODECS_CONNECTION 
// message
UtlBoolean CpPeerCall::handleRenegotiateCodecsConnection(OsMsg* pEventMessage)
{
   UtlString address;
   ((CpMultiStringMessage*)pEventMessage)->getString2Data(address);

   OsReadLock lock(mConnectionMutex);
   Connection* connection = findHandlingConnection(address);
   if(connection)
   {
      connection->renegotiateCodecs();
   }

   return TRUE;
}

// Handles the processing of a CallManager::CP_RENEGOTIATE_CODECS_ALL_CONNECTIONS 
// message
UtlBoolean CpPeerCall::handleRenegotiateCodecsAllConnections(OsMsg* pEventMessage)
{
   Connection* connection = NULL;    
   OsReadLock lock(mConnectionMutex);
   UtlDListIterator iterator(mConnections);
   while ((connection = dynamic_cast<Connection*>(iterator())))
   {
      connection->renegotiateCodecs();
   }

   return TRUE;
}

// Handles the processing of a CallManager::CP_SILENT_REMOTE_HOLD
// message
UtlBoolean CpPeerCall::handleSilentRemoteHold(OsMsg* pEventMessage)
{
   Connection* connection = NULL;    
   OsReadLock lock(mConnectionMutex);
   UtlDListIterator iterator(mConnections);
   while ((connection = dynamic_cast<Connection*>(iterator())))
   {
      connection->silentRemoteHold();
   }

   return TRUE;
}

// Handles the processing of a CallManager::CP_TRANSFER_CONNECTION_STATUS 
// message
UtlBoolean CpPeerCall::handleTransferConnectionStatus(OsMsg* pEventMessage)
{
   // This message is sent to the target call on the
   // transfer controller

   UtlString connectionAddress;
   ((CpMultiStringMessage*)pEventMessage)->getString2Data(connectionAddress);
   int connectionState = ((CpMultiStringMessage*)pEventMessage)->getInt1Data();
   int cause = ((CpMultiStringMessage*)pEventMessage)->getInt2Data();
   {
      // Find the connection and give it the status
      OsReadLock lock(mConnectionMutex);
      Connection* connection = findHandlingConnection(connectionAddress);
      if(connection)
      {
         connection->transferControllerStatus(connectionState, cause);
      }
   }

   // Stop the meta event
   stopMetaEvent();

   // Check if call is dead and drop it if it is
   dropIfDead();

   return TRUE;
}


// Handles the processing of a CallManager::CP_TRANSFEREE_CONNECTION_STATUS 
// message
UtlBoolean CpPeerCall::handleTransfereeConnectionStatus(OsMsg* pEventMessage)
{
   // This message gets sent to the original call on the
   // transferee

   UtlString connectionAddress;
   ((CpMultiStringMessage*)pEventMessage)->getString2Data(connectionAddress);
   int connectionState = ((CpMultiStringMessage*)pEventMessage)->getInt1Data();
   int responseCode = ((CpMultiStringMessage*)pEventMessage)->getInt2Data();
   // Find the connection and give it the status
   {
      OsReadLock lock(mConnectionMutex);
      Connection* connection = findHandlingConnection(connectionAddress);
      if(connection)
         connection->transfereeStatus(connectionState, responseCode);
   }

   // Stop the meta event
   stopMetaEvent();

   // Check if call is dead and drop it if it is
   dropIfDead();

   return TRUE;
}

// Handles the processing of a CallManager::CP_GET_SESSION 
// message
UtlBoolean CpPeerCall::handleGetSession(OsMsg* pEventMessage)
{
   UtlString address;
   UtlString callId;
   ((CpMultiStringMessage*)pEventMessage)->getString1Data(callId);
   ((CpMultiStringMessage*)pEventMessage)->getString2Data(address);
   SipSession* sessionPtr;
   OsProtectedEvent* getFieldEvent = (OsProtectedEvent*) 
      ((CpMultiStringMessage*)pEventMessage)->getInt1Data();
   getFieldEvent->getIntData((int&)sessionPtr);

   OsSysLog::add(FAC_CP, PRI_DEBUG, "CpPeerCall::handleGetSession session: %p for callId %s address %s",
      sessionPtr, callId.data(), address.data());

   // Check whether the tag is set in addresses or not. If so, do not need to use callId
   // for comparison.
   UtlBoolean hasTag = checkForTag(address);

   // Get the remote connection(s)/address(es)
   SipConnection* connection = NULL;
   UtlString localAddress;
   UtlString remoteAddress;
   UtlString connCallId;
   OsReadLock lock(mConnectionMutex);
   UtlDListIterator iterator(mConnections);
   while ((connection = dynamic_cast<SipConnection*>(iterator())))
   {
      connection->getCallId(&connCallId);
      connection->getFromField(&localAddress);
      connection->getToField(&remoteAddress);

      OsSysLog::add(FAC_CP, PRI_DEBUG, "CpPeerCall::handleGetSession looking for the SipSession for %s, %s, %s",
         connCallId.data(), localAddress.data(), remoteAddress.data());

      if ((hasTag && (address.compareTo(localAddress) == 0)) ||
         (hasTag && (address.compareTo(remoteAddress) == 0)) ||
         (callId.compareTo(connCallId) == 0) &&
         (address.compareTo(localAddress) == 0 || address.compareTo(remoteAddress) == 0))
      {
//         SipSession session;
//         connection->getSession(session);
         OsSysLog::add(FAC_CP, PRI_DEBUG, "CpPeerCall::handleGetSession copying session: %p",
            sessionPtr);

//         *sessionPtr = SipSession(session);
         // Signal the caller that we are done.
         break;
      }
   }

   // If the event has already been signalled, clean up
   if(OS_ALREADY_SIGNALED == getFieldEvent->signal(1))
   {
      // The other end must have timed out on the wait
      OsSysLog::add(FAC_CP, PRI_DEBUG,
         "CpPeerCall::handleGetSession deleting session: %p",
         sessionPtr);
/*      delete sessionPtr;
      sessionPtr = NULL;*/

      OsProtectEventMgr* eventMgr = OsProtectEventMgr::getEventMgr();
      eventMgr->release(getFieldEvent);
   }

   return TRUE;
}

// Handles the processing of a CallManager::CP_GET_USERAGENT 
// message
UtlBoolean CpPeerCall::handleGetUserAgent(OsMsg* pEventMessage)
{
   UtlString* pUserAgent = NULL;
   OsProtectedEvent* getAgentEvent = (OsProtectedEvent*) 
      ((CpMultiStringMessage*)pEventMessage)->getInt1Data();
   getAgentEvent->getIntData((int&)pUserAgent);

   CpMultiStringMessage* pMultiMessage = (CpMultiStringMessage*) pEventMessage;

   UtlString remoteAddress;

   pMultiMessage->getString2Data(remoteAddress);

   Connection* connection = NULL;
   OsReadLock lock(mConnectionMutex);
   UtlDListIterator iterator(mConnections);
   while ((connection = dynamic_cast<Connection*>(iterator())))
   {
      UtlString connectionRemoteAddress;

      connection->getRemoteAddress(&connectionRemoteAddress);
      if (remoteAddress.isNull() || connectionRemoteAddress == remoteAddress)
      {
         connection->getRemoteUserAgent(pUserAgent);
      }
   }    
   // Signal the caller that we are done.
   // If the event has already been signalled, clean up
   if(OS_ALREADY_SIGNALED == getAgentEvent->signal(1))
   {
      // The other end must have timed out on the wait
      OsProtectEventMgr* eventMgr = OsProtectEventMgr::getEventMgr();
      eventMgr->release(getAgentEvent);
   }

   return TRUE;
}

// Enumerate possible contact addresses
void CpPeerCall::getLocalContactAddresses( SIPX_CONTACT_ADDRESS contacts[], 
                                          size_t nMaxContacts, 
                                          size_t& nActualContacts)
{
   UtlString ipAddress;
   int port;

   // Order: Local, NAT, configure
   nActualContacts = 0;

   if (    (nActualContacts < nMaxContacts) && 
      (sipUserAgent->getLocalAddress(&ipAddress, &port, TRANSPORT_UDP)))
   {
      contacts[nActualContacts].eContactType = CONTACT_LOCAL;
      SAFE_STRNCPY(contacts[nActualContacts].cIpAddress, ipAddress.data(), 32);
      contacts[nActualContacts].iPort = port;
      contacts[nActualContacts].eTransportType = TRANSPORT_UDP;
      nActualContacts++;
   }

   if (    (nActualContacts < nMaxContacts) && 
      (sipUserAgent->getLocalAddress(&ipAddress, &port, TRANSPORT_TCP)))
   {
      contacts[nActualContacts].eContactType = CONTACT_LOCAL;
      SAFE_STRNCPY(contacts[nActualContacts].cIpAddress, ipAddress.data(), 32);
      contacts[nActualContacts].iPort = port;
      contacts[nActualContacts].eTransportType = TRANSPORT_TCP;
      nActualContacts++;
   }

   if (    (nActualContacts < nMaxContacts) && 
      (sipUserAgent->getLocalAddress(&ipAddress, &port, TRANSPORT_TLS)))
   {
      contacts[nActualContacts].eContactType = CONTACT_LOCAL;
      SAFE_STRNCPY(contacts[nActualContacts].cIpAddress, ipAddress.data(), 32);
      contacts[nActualContacts].iPort = port;
      contacts[nActualContacts].eTransportType = TRANSPORT_TLS;
      nActualContacts++;
   }

   if (    (nActualContacts < nMaxContacts) && 
      (sipUserAgent->getNatMappedAddress(&ipAddress, &port)))
   {
      contacts[nActualContacts].eContactType = CONTACT_NAT_MAPPED;
      SAFE_STRNCPY(contacts[nActualContacts].cIpAddress, ipAddress.data(), 32);
      contacts[nActualContacts].iPort = port;
      contacts[nActualContacts].eTransportType = TRANSPORT_UDP;
      nActualContacts++;
   }

   if (    (nActualContacts < nMaxContacts) && 
      (sipUserAgent->getConfiguredPublicAddress(&ipAddress, &port)))
   {
      contacts[nActualContacts].eContactType = CONTACT_CONFIG;
      SAFE_STRNCPY(contacts[nActualContacts].cIpAddress, ipAddress.data(), 32);
      contacts[nActualContacts].iPort = port;
      contacts[nActualContacts].eTransportType = TRANSPORT_UDP;
      nActualContacts++;
   }
}

// Handles the processing of a CallManager::CP_CANCEL_TIMER 
// message
UtlBoolean CpPeerCall::handleCancelTimer(OsMsg* pEventMessage)
{
   // Find the connection to be transitioned out of OFFERING
   UtlString address;
   ((CpMultiStringMessage*)pEventMessage)->getString2Data(address);
   Connection*  connection = findHandlingConnection(address);

   if(connection)
   {
      connection->forceHangUp();
      dropIfDead();
   }

   return TRUE;
}


// Handles the processing of a CallManager::CP_OFFERING_EXPIRED 
// message
UtlBoolean CpPeerCall::handleOfferingExpired(OsMsg* pEventMessage)
{
   // Find the connection to be transitioned out of OFFERING
   UtlString address;
   ((CpMultiStringMessage*)pEventMessage)->getString2Data(address);
   OsReadLock lock(mConnectionMutex);
   Connection*  connection = findHandlingConnection(address);

   if(connection)
   {
      int connectionState = connection->getState();

      // If we do have a timeout, note it to both the
      // console and syslog
      if (connectionState == Connection::CONNECTION_OFFERING)
      {
         UtlString    msg;            
//         SipSession  session;
         Url         urlFrom, urlTo;
         UtlString    callId, from, to;

//         connection->getSession(session);
/*         session.getCallId(callId);
         session.getFromUrl(urlFrom);
         urlFrom.toString(from);
         session.getToUrl(urlTo);
         urlTo.toString(to);*/

         msg = "CP_OFFERING_EXPIRED for address: " + address;
         msg += "\n\tHandling CallId: " + callId;
         msg += "\n\tHandling From: " + from;
         msg += "\n\tHandling To: " + to;

         OsSysLog::add(FAC_CP, PRI_ERR, "%s", msg.data());
      }

      // If the call is in focus when the offering
      // timer expired, assume we will accept the call
      if(mCallInFocus &&
         connectionState == Connection::CONNECTION_OFFERING)
      {
         // Unconditional forwarding is on
         if(lineAvailableBehavior == 
            Connection::FORWARD_UNCONDITIONAL &&
            !forwardUnconditional.isNull())
         {
            UtlString forwardAddressUrl(forwardUnconditional.data());
            if (PT_SUCCESS == mpManager->validateAddress(forwardAddressUrl))
               connection->redirect(forwardAddressUrl.data());
            forwardAddressUrl = NULL;
         }

         // Otherwise accept the call
         else
         {
            connection->accept(noAnswerTimeout);
         }
      }

      // The call is out of focus reject the offer
      else if(connectionState == Connection::CONNECTION_OFFERING)
      {
         // If forward on busy is enabled
         if(lineBusyBehavior == Connection::FORWARD_ON_BUSY &&
            !forwardOnBusy.isNull())
         {
            UtlString forwardAddressUrl(forwardOnBusy.data());
            if (PT_SUCCESS == mpManager->validateAddress(forwardAddressUrl))
               connection->redirect(forwardAddressUrl.data());
            forwardAddressUrl = NULL;
         }
         // Otherwise reject the call
         else
         {
            connection->reject();
         }
      }
   }
   return TRUE;
}


// Handles the processing of a CallManager::CP_RINGING_EXPIRED 
// message
UtlBoolean CpPeerCall::handleRingingExpired(OsMsg* pEventMessage)
{
   // Find the connection to be transitioned out of OFFERING
   UtlString address;
   ((CpMultiStringMessage*)pEventMessage)->getString2Data(address);
   OsReadLock lock(mConnectionMutex);
   Connection*  connection = findHandlingConnection(address);

   // We got here because forward on no answer is enabled
   // and the timer expired.
   if(connection &&
      connection->getState() == 
      Connection::CONNECTION_ALERTING) 
   {      
      if (lineAvailableBehavior == Connection::FORWARD_ON_NO_ANSWER &&
         !forwardOnNoAnswer.isNull())
      {
         UtlString forwardAddressUrl(forwardOnNoAnswer.data());
         if (PT_SUCCESS == mpManager->validateAddress(forwardAddressUrl))
            connection->redirect(forwardAddressUrl.data());
         forwardAddressUrl = NULL;
      }
      // We now drop the call if no one picks up this call after so long
      else
      {
         connection->reject();
      }
   }

   return TRUE;
}


// Handles the processing of a CallManager::CP_UNHOLD_ALL_TERM_CONNECTIONS
// message
UtlBoolean CpPeerCall::handleUnholdAllTermConnections(OsMsg* pEventMessage)
{
   Connection* connection = NULL;
   OsReadLock lock(mConnectionMutex);
   UtlDListIterator iterator(mConnections);

   while ((connection = dynamic_cast<Connection*>(iterator())))
   {
      connection->offHold();

      if (mLocalTermConnectionState != PtTerminalConnection::TALKING &&
         mLocalTermConnectionState != PtTerminalConnection::IDLE)
      {
         UtlString responseText;
         UtlString remoteAddress;

         connection->getResponseText(responseText);      
         connection->getRemoteAddress(&remoteAddress);

         postTaoListenerMessage(connection->getResponseCode(), responseText, PtEvent::TERMINAL_CONNECTION_TALKING, TERMINAL_CONNECTION_STATE, PtEvent::CAUSE_UNHOLD, connection->isRemoteCallee(), remoteAddress);

         // connection->fireSipXEvent(CALLSTATE_CONNECTED, CALLSTATE_CAUSE_NORMAL);
      }
   }

   return TRUE;
}


// Handles the processing of a CallManager::CP_UNHOLD_LOCAL_TERM_CONNECTION
// message
UtlBoolean CpPeerCall::handleUnholdLocalTermConnection(OsMsg* pEventMessage)
{
   // Post a message to the callManager to change focus
   //    CpIntMessage localHoldMessage(CallManager::CP_GET_FOCUS, (int)this);

   mpManager->doGetFocus(this);    
   // mpManager->postMessage(localHoldMessage);
   mLocalHeld = FALSE;

   return TRUE;
}


// Handles the processing of a CallManager::CP_HOLD_LOCAL_TERM_CONNECTION
// message
UtlBoolean CpPeerCall::handleHoldLocalTermConnection(OsMsg* pEventMessage)
{
   // Post a message to the callManager to change focus
   CpIntMessage localHoldMessage(CallManager::CP_YIELD_FOCUS, (int)this);
   mpManager->postMessage(localHoldMessage);
   mLocalHeld = TRUE;

   return TRUE;
}


UtlBoolean CpPeerCall::handleCallMessage(OsMsg& eventMessage)
{
   int msgSubType = eventMessage.getMsgSubType();
   UtlBoolean processedMessage = TRUE;
   CpMultiStringMessage* multiStringMessage = (CpMultiStringMessage*)&eventMessage;

   switch(msgSubType)
   {
   case CallManager::CP_SIP_MESSAGE:
      handleSipMessage(&eventMessage);
      break;

   case CallManager::CP_DROP_CONNECTION:
      handleDropConnection(&eventMessage);
      break;

   case CallManager::CP_FORCE_DROP_CONNECTION:
      handleForceDropConnection(&eventMessage);
      break;

   case CallManager::CP_DIAL_STRING:
      handleDialString(&eventMessage);
      break;

   case CallManager::CP_OUTGOING_INFO:
      handleSendInfo(&eventMessage);
      break;

   case CallManager::CP_ANSWER_CONNECTION:
      {
         CpMultiStringMessage* pMessage = (CpMultiStringMessage*)&eventMessage;
         const void* pDisplay = (void*) pMessage->getInt1Data();
         offHook(pDisplay);
         break;
      }
   case CallManager::CP_BLIND_TRANSFER:
   case CallManager::CP_CONSULT_TRANSFER:
      handleTransfer(&eventMessage);
      break;

   case CallManager::CP_CONSULT_TRANSFER_ADDRESS:
      handleTransferAddress(&eventMessage);
      break;

   case CallManager::CP_TRANSFER_CONNECTION:
      handleTransferConnection(&eventMessage);
      break;

   case CallManager::CP_TRANSFER_CONNECTION_STATUS:
      handleTransferConnectionStatus(&eventMessage);
      break;

   case CallManager::CP_TRANSFEREE_CONNECTION:
      handleTransfereeConnection(&eventMessage);
      break;

   case CallManager::CP_TRANSFEREE_CONNECTION_STATUS:
      handleTransfereeConnectionStatus(&eventMessage);
      break;

   case CallManager::CP_GET_MEDIA_CONNECTION_ID:
      handleGetMediaConnectionId(&eventMessage);
      break;

   case CallManager::CP_LIMIT_CODEC_PREFERENCES:
      handleLimitCodecPreferences(&eventMessage);
      break;

   case CallManager::CP_GET_MEDIA_ENERGY_LEVELS:
      handleGetMediaEnergyLevels(&eventMessage);
      break;

   case CallManager::CP_GET_CALL_MEDIA_ENERGY_LEVELS:
      handleGetCallMediaEnergyLevels(&eventMessage);
      break;

   case CallManager::CP_GET_CAN_ADD_PARTY:
      handleGetCanAddParty(&eventMessage);
      break;

   case CallManager::CP_SPLIT_CONNECTION:
      handleSplitConnection(&eventMessage);
      break;

   case CallManager::CP_JOIN_CONNECTION:
      handleJoinConnection(&eventMessage);
      break;

   case CallManager::CP_GET_CALLED_ADDRESSES:
   case CallManager::CP_GET_CALLING_ADDRESSES:
      handleGetAddresses(&eventMessage);
      break;

   case CallManager::CP_GET_SESSION:
      handleGetSession(&eventMessage);
      break;

   case CallManager::CP_ACCEPT_CONNECTION:
      handleAcceptConnection(&eventMessage);
      break;

   case CallManager::CP_REJECT_CONNECTION:
      handleRejectConnection(&eventMessage);
      break;

   case CallManager::CP_REDIRECT_CONNECTION:
      handleRedirectConnection(&eventMessage);
      break;

   case CallManager::CP_HOLD_TERM_CONNECTION:
      handleHoldTermConnection(&eventMessage);
      break;

   case CallManager::CP_HOLD_ALL_TERM_CONNECTIONS:
      handleHoldAllTermConnection(&eventMessage);
      break;

   case CallManager::CP_UNHOLD_TERM_CONNECTION:
      handleUnholdTermConnection(&eventMessage);
      break;

   case CallManager::CP_RENEGOTIATE_CODECS_CONNECTION:
      handleRenegotiateCodecsConnection(&eventMessage);
      break;

   case CallManager::CP_RENEGOTIATE_CODECS_ALL_CONNECTIONS:
      handleRenegotiateCodecsAllConnections(&eventMessage);
      break;

   case CallManager::CP_CANCEL_TIMER:
      handleCancelTimer(&eventMessage);
      break;

   case CallManager::CP_OFFERING_EXPIRED:
      handleOfferingExpired(&eventMessage);
      break;

   case CallManager::CP_RINGING_EXPIRED:
      handleRingingExpired(&eventMessage);
      break;

   case CallManager::CP_UNHOLD_ALL_TERM_CONNECTIONS:
      handleUnholdAllTermConnections(&eventMessage);
      break;

   case CallManager::CP_UNHOLD_LOCAL_TERM_CONNECTION:
      handleUnholdLocalTermConnection(&eventMessage);
      break;

   case CallManager::CP_HOLD_LOCAL_TERM_CONNECTION:
      handleHoldLocalTermConnection(&eventMessage);
      break;

   case CallManager::CP_GET_USERAGENT:
      handleGetUserAgent( &eventMessage );
      break;

   case CallManager::CP_START_TONE_CONNECTION:        
      {
         UtlString remoteAddress;
         ((CpMultiStringMessage&)eventMessage).getString2Data(remoteAddress);
         int toneId = ((CpMultiStringMessage&)eventMessage).getInt1Data();
         int local = ((CpMultiStringMessage&)eventMessage).getInt2Data();
         int remote =  ((CpMultiStringMessage&)eventMessage).getInt3Data();

         Connection* connection = findHandlingConnection(remoteAddress);
         if (connection && mpMediaInterface)
         {   
            int connectionId = connection->getConnectionId();
//            mpMediaInterface->startChannelTone(connectionId, toneId, local, remote);
         }                
      }
      break;            
   case CallManager::CP_STOP_TONE_CONNECTION:
      {
         UtlString remoteAddress;
         ((CpMultiStringMessage&)eventMessage).getString2Data(remoteAddress);

         Connection* connection = findHandlingConnection(remoteAddress);
         if (connection && mpMediaInterface)
         {   
            int connectionId = connection->getConnectionId();
            //mpMediaInterface->stopChannelTone(connectionId);
         }                
      }
      break;
   case CallManager::CP_PLAY_AUDIO_CONNECTION:        
      {
         UtlString remoteAddress;
         UtlString url;
         ((CpMultiStringMessage&)eventMessage).getString2Data(remoteAddress);
         ((CpMultiStringMessage&)eventMessage).getString3Data(url);
         int repeat = ((CpMultiStringMessage&)eventMessage).getInt1Data();
         int local = ((CpMultiStringMessage&)eventMessage).getInt2Data();
         int remote =  ((CpMultiStringMessage&)eventMessage).getInt3Data();
         UtlBoolean mixWithMic = ((CpMultiStringMessage&)eventMessage).getInt4Data();
         int downScaling = ((CpMultiStringMessage&)eventMessage).getInt5Data();
         void* pCookie = (void*)((CpMultiStringMessage&)eventMessage).getInt6Data();

         Connection* connection = findHandlingConnection(remoteAddress);
         if (connection && mpMediaInterface)
         {   
            int connectionId = connection->getConnectionId();
/*            mpMediaInterface->playChannelAudio(connectionId, url, repeat, 
               local, remote, mixWithMic, downScaling, pCookie);*/
         }     
         else if (mpMediaInterface)
         {
            mpMediaInterface->playAudio(url, repeat, local, remote, 
               mixWithMic, downScaling, pCookie);
         }

      }

      break;
   case CallManager::CP_STOP_AUDIO_CONNECTION:
      {
         UtlString remoteAddress;
         ((CpMultiStringMessage&)eventMessage).getString2Data(remoteAddress);

         Connection* connection = findHandlingConnection(remoteAddress);
         if (connection && mpMediaInterface)
         {   
            int connectionId = connection->getConnectionId();
//            mpMediaInterface->stopChannelAudio(connectionId);
         }
      }
      break;
   case CallManager::CP_RECORD_AUDIO_CONNECTION_START:
      {
         UtlString remoteAddress;
         UtlString file;
         ((CpMultiStringMessage&)eventMessage).getString2Data(remoteAddress);
         ((CpMultiStringMessage&)eventMessage).getString3Data(file);
         OsProtectedEvent* pEvent = (OsProtectedEvent*) 
            ((CpMultiStringMessage&)eventMessage).getInt1Data();
         UtlBoolean bSuccess = false;

         Connection* connection = findHandlingConnection(remoteAddress);
         if (connection && mpMediaInterface)
         {   
            int connectionId = connection->getConnectionId();
            if (mpMediaInterface->recordChannelAudio(connectionId, file))
            {
               bSuccess = true;
            }
         }

         // If the event has already been signalled, clean up
         if(pEvent && OS_ALREADY_SIGNALED == pEvent->signal(bSuccess))
         {
            // The other end must have timed out on the wait
            OsProtectEventMgr* eventMgr = OsProtectEventMgr::getEventMgr();
            eventMgr->release(pEvent);
         }
      }
      break;

   case CallManager::CP_RECORD_AUDIO_CONNECTION_STOP:
      {
         UtlString remoteAddress;
         ((CpMultiStringMessage&)eventMessage).getString2Data(remoteAddress);
         OsProtectedEvent* pEvent = (OsProtectedEvent*) 
            ((CpMultiStringMessage&)eventMessage).getInt1Data();
         UtlBoolean bSuccess = false;

         Connection* connection = findHandlingConnection(remoteAddress);
         if (connection && mpMediaInterface)
         {   
            int connectionId = connection->getConnectionId();
            if (mpMediaInterface->stopRecordChannelAudio(connectionId))
            {
               bSuccess = true;
            }
         }

         // If the event has already been signalled, clean up
         if(pEvent && OS_ALREADY_SIGNALED == pEvent->signal(bSuccess))
         {
            // The other end must have timed out on the wait
            OsProtectEventMgr* eventMgr = OsProtectEventMgr::getEventMgr();
            eventMgr->release(pEvent);
         }
      }
      break;
   case CallManager::CP_REFIRE_MEDIA_EVENT:
      {
         UtlString remoteAddress;
         ((CpMultiStringMessage&)eventMessage).getString2Data(remoteAddress);
         int event = ((CpMultiStringMessage&)eventMessage).getInt1Data();
         CP_MEDIA_CAUSE cause = (CP_MEDIA_CAUSE)((CpMultiStringMessage&)eventMessage).getInt2Data();
         CP_MEDIA_TYPE type = (CP_MEDIA_TYPE)((CpMultiStringMessage&)eventMessage).getInt3Data();

         Connection* connection = findHandlingConnection(remoteAddress);
         if (connection)
         {
            connection->fireSipXMediaEvent(
               (CP_MEDIA_EVENT) event,
               (CP_MEDIA_CAUSE) cause,
               (CP_MEDIA_TYPE) type,
               NULL);            
         }
      }
      break;

   case CallManager::CP_TRANSFER_OTHER_PARTY_JOIN:
      handleTransferOtherPartyJoin(&eventMessage);
      break;

   case CallManager::CP_TRANSFER_OTHER_PARTY_HOLD:
      handleTransferOtherPartyHold(&eventMessage);
      break;

   case CallManager::CP_TRANSFER_OTHER_PARTY_UNHOLD:
      handleTransferOtherPartyUnhold(&eventMessage);
      break;

   case CallManager::CP_MUTE_INPUT_TERM_CONNECTION:
      handleMuteInputTermConnection(&eventMessage);
      break;
   case CallManager::CP_CREATE_MEDIA_INTERFACE:
      handleCreateMediaInterface(&eventMessage);
      break;
   default:
      processedMessage = FALSE;
      break;
   }

   return(processedMessage);
}

// fires given media event to all SipConnections
void CpPeerCall::forkSipXMediaEvent(CP_MEDIA_EVENT event,
                                    CP_MEDIA_CAUSE cause,
                                    CP_MEDIA_TYPE type,
                                    intptr_t pEventData1,
                                    intptr_t pEventData2)
{
   Connection* connection = NULL;
   OsReadLock lock(mConnectionMutex);
   UtlDListIterator iterator(mConnections);

   while ((connection = dynamic_cast<Connection*>(iterator())))
   {
      connection->fireSipXMediaEvent(event, cause, type, pEventData1, pEventData2);
   }
}

void CpPeerCall::fireSipXMediaEvent(CP_MEDIA_EVENT event,
                                    CP_MEDIA_CAUSE cause,
                                    CP_MEDIA_TYPE type,
                                    int mediaConnectionId,
                                    intptr_t pEventData1,
                                    intptr_t pEventData2)

{
   Connection* connection = NULL;
   OsReadLock lock(mConnectionMutex);
   UtlDListIterator iterator(mConnections);

   while ((connection = dynamic_cast<Connection*>(iterator())))
   {
      if (connection->getConnectionId() == mediaConnectionId)
      {
         connection->fireSipXMediaEvent(event, cause, type, pEventData1, pEventData2);
         break;
      }
   }
}

// we forward message handling to the corresponding SipMessage
UtlBoolean CpPeerCall::handleConnectionNotfMessage(OsMsg& eventMessage)
{
   OsIntPtrMsg* pMsg = (OsIntPtrMsg*)&eventMessage;
   CpNotificationMsgMedia media = (CpNotificationMsgMedia)pMsg->getMsgSubType();
   CpNotificationMsgType type = (CpNotificationMsgType)pMsg->getData1();
   int mediaConnectionId = pMsg->getData2();
   intptr_t pData1 = pMsg->getData3();
   intptr_t pData2 = pMsg->getData4();

   switch(type)
   {
   case CP_NOTIFICATION_DTMF_INBAND:
      fireSipXMediaEvent(CP_MEDIA_REMOTE_DTMF, CP_MEDIA_CAUSE_DTMF_INBAND, (CP_MEDIA_TYPE)media, mediaConnectionId, pData1, pData2);
      break;
   case CP_NOTIFICATION_DTMF_RFC2833:
      fireSipXMediaEvent(CP_MEDIA_REMOTE_DTMF, CP_MEDIA_CAUSE_DTMF_RFC2833, (CP_MEDIA_TYPE)media, mediaConnectionId, pData1, pData2);
      break;
/*   case CP_NOTIFICATION_DTMF_SIPINFO:
      fireSipXMediaEvent(CP_MEDIA_REMOTE_DTMF, CP_MEDIA_CAUSE_DTMF_SIPINFO, (CP_MEDIA_TYPE)media, mediaConnectionId, pData1, pData2);
      break;*/
   default:
      assert(false);
   }

   return TRUE;
}


UtlBoolean CpPeerCall::handleInterfaceNotfMessage(OsMsg& eventMessage)
{
   OsIntPtrMsg* pMsg = (OsIntPtrMsg*)&eventMessage;
   CpNotificationMsgMedia media = (CpNotificationMsgMedia)pMsg->getMsgSubType();
   CpNotificationMsgType type = (CpNotificationMsgType)pMsg->getData1();
   intptr_t pData1 = pMsg->getData2();
   intptr_t pData2 = pMsg->getData3();

   switch(type)
   {
   case CP_NOTIFICATION_START_PLAY_FILE:
      forkSipXMediaEvent(CP_MEDIA_PLAYFILE_START, CP_MEDIA_CAUSE_NORMAL, (CP_MEDIA_TYPE)media, pData1, pData2);
      break;
   case CP_NOTIFICATION_STOP_PLAY_FILE:
      forkSipXMediaEvent(CP_MEDIA_PLAYFILE_STOP, CP_MEDIA_CAUSE_NORMAL, (CP_MEDIA_TYPE)media, pData1, pData2);
      break;
   case CP_NOTIFICATION_START_PLAY_BUFFER:
      forkSipXMediaEvent(CP_MEDIA_PLAYBUFFER_START, CP_MEDIA_CAUSE_NORMAL, (CP_MEDIA_TYPE)media, pData1, pData2);
      break;
   case CP_NOTIFICATION_STOP_PLAY_BUFFER:
      forkSipXMediaEvent(CP_MEDIA_PLAYBUFFER_STOP, CP_MEDIA_CAUSE_NORMAL, (CP_MEDIA_TYPE)media, pData1, pData2);
      break;
   case CP_NOTIFICATION_PAUSE_PLAYBACK:
      forkSipXMediaEvent(CP_MEDIA_PLAYBACK_PAUSED, CP_MEDIA_CAUSE_NORMAL, (CP_MEDIA_TYPE)media, pData1, pData2);
      break;
   case CP_NOTIFICATION_RESUME_PLAYBACK:
      forkSipXMediaEvent(CP_MEDIA_PLAYBACK_RESUMED, CP_MEDIA_CAUSE_NORMAL, (CP_MEDIA_TYPE)media, pData1, pData2);
      break;
   case CP_NOTIFICATION_RECORDING_STARTED:
      forkSipXMediaEvent(CP_MEDIA_RECORDING_START, CP_MEDIA_CAUSE_NORMAL, (CP_MEDIA_TYPE)media, pData1, pData2);
      break;
   case CP_NOTIFICATION_RECORDING_STOPPED:
      forkSipXMediaEvent(CP_MEDIA_RECORDING_STOP, CP_MEDIA_CAUSE_NORMAL, (CP_MEDIA_TYPE)media, pData1, pData2);
      break;
   default:
      assert(false);
   }

   return TRUE;
}

UtlBoolean CpPeerCall::handleSendInfo(OsMsg* pEventMessage)
{
   CpMultiStringMessage& infoMessage = (CpMultiStringMessage&) *pEventMessage;
   UtlString callId;
   UtlString remoteAddress;
   UtlString contentType;
   UtlString sContent;
   bool bSuccess = false;

   OsProtectedEvent* pEvent = (OsProtectedEvent*) infoMessage.getInt1Data();
   infoMessage.getString1Data(callId);
   infoMessage.getString2Data(remoteAddress);
   infoMessage.getString3Data(contentType);
   infoMessage.getString4Data(sContent);

   UtlString connectionCallId;
   Connection* connection = NULL;
   OsReadLock lock(mConnectionMutex);
   UtlDListIterator iterator(mConnections);

   while ((connection = dynamic_cast<Connection*>(iterator())))
   {
      UtlString connectionRemoteAddress;
      connection->getRemoteAddress(&connectionRemoteAddress);
      if (connectionRemoteAddress == remoteAddress)
      {
         if (connection->canSendInfo())
         {
            connection->sendInfo(contentType, sContent); 
            bSuccess = true;
         }
         break;
      }
   }

   // If the event has already been signalled, clean up
   if(pEvent && OS_ALREADY_SIGNALED == pEvent->signal(bSuccess))
   {
      // The other end must have timed out on the wait
      OsProtectEventMgr* eventMgr = OsProtectEventMgr::getEventMgr();
      eventMgr->release(pEvent);
   }

   return true;
}

UtlBoolean CpPeerCall::handleGetMediaConnectionId(OsMsg* pEventMessage)
{
   int connectionId = -1;
   CpMultiStringMessage* pMultiMessage = (CpMultiStringMessage*) pEventMessage;
   OsProtectedEvent* event = (OsProtectedEvent*) 
      ((CpMultiStringMessage*)pEventMessage)->getInt1Data();

   UtlString callId;
   UtlString remoteAddress;

   pMultiMessage->getString1Data(callId);
   pMultiMessage->getString2Data(remoteAddress);
   void** ppInstData = (void**)pMultiMessage->getInt2Data();

   Connection* connection = NULL;
   OsReadLock lock(mConnectionMutex);
   UtlDListIterator iterator(mConnections);
   while ((connection = dynamic_cast<Connection*>(iterator())))
   {
      UtlString connectionRemoteAddress;

      connection->getRemoteAddress(&connectionRemoteAddress);
      if (connectionRemoteAddress == remoteAddress)
      {
         connectionId = connection->getConnectionId();
         if ((ppInstData) && (connectionId != -1))
         {
            *ppInstData = connection->getMediaInterfacePtr();
         }
         break;
      }
   }    

   // If the event has already been signalled, clean up
   if(event && OS_ALREADY_SIGNALED == event->signal(connectionId))
   {
      // The other end must have timed out on the wait
      OsProtectEventMgr* eventMgr = OsProtectEventMgr::getEventMgr();
      eventMgr->release(event);
   }

   return true;
}

UtlBoolean CpPeerCall::handleLimitCodecPreferences(OsMsg* pEventMessage)
{
   int connectionId = -1;
   CpMultiStringMessage* pMultiMessage = (CpMultiStringMessage*) pEventMessage;

   UtlString callId;
   UtlString remoteAddress;
   UtlString videoCodec;
   int audioBandwidth;
   int videoBandwidth;

   pMultiMessage->getString1Data(callId);
   pMultiMessage->getString2Data(remoteAddress);
   pMultiMessage->getString3Data(videoCodec);

   audioBandwidth = pMultiMessage->getInt1Data();
   videoBandwidth = pMultiMessage->getInt2Data();

   void* pInstData = NULL;

   Connection* connection = NULL;
   OsReadLock lock(mConnectionMutex);
   UtlDListIterator iterator(mConnections);
   while ((connection = dynamic_cast<Connection*>(iterator())))
   {
      UtlString connectionRemoteAddress;

      connection->getRemoteAddress(&connectionRemoteAddress);
      if (remoteAddress.isNull() || connectionRemoteAddress == remoteAddress)
      {
         connectionId = connection->getConnectionId();
         if (connectionId != -1)
         {
            pInstData = connection->getMediaInterfacePtr();

            if (pInstData != NULL)
            {
/*               ((CpMediaInterface*)pInstData)->rebuildCodecFactory(connectionId, 
                  audioBandwidth, 
                  videoBandwidth, 
                  videoCodec);*/
            }
         }
      }
   }    
   return true;
}

// Handles the processing of a CP_GET_MEDIA_ENERGY_LEVELS message
UtlBoolean CpPeerCall::handleGetMediaEnergyLevels(OsMsg* pEventMessage)
{
   CpMultiStringMessage* pMultiMessage = (CpMultiStringMessage*) pEventMessage;
   UtlString callId;
   UtlString remoteAddress;
   OsProtectedEvent* event;
   int* piInputEnergyLevel;
   int* piOutputEnergyLevel;
   int* pnContributors;
   unsigned int* pContributorSRCIds;
   int* pContributorEngeryLevels;
   UtlBoolean bSucccess = false;

   pMultiMessage->getString1Data(callId);
   pMultiMessage->getString2Data(remoteAddress);       
   event = (OsProtectedEvent*) pMultiMessage->getInt1Data();
   piInputEnergyLevel = (int*) pMultiMessage->getInt2Data();
   piOutputEnergyLevel = (int*) pMultiMessage->getInt3Data();
   pnContributors = (int*) pMultiMessage->getInt4Data();
   pContributorSRCIds = (unsigned int*) pMultiMessage->getInt5Data();
   pContributorEngeryLevels = (int*) pMultiMessage->getInt6Data();

   Connection* connection = NULL;
   OsReadLock lock(mConnectionMutex);
   UtlDListIterator iterator(mConnections);
   while ((connection = dynamic_cast<Connection*>(iterator())))
   {
      UtlString connectionRemoteAddress;        
      connection->getRemoteAddress(&connectionRemoteAddress);
      if (connectionRemoteAddress == remoteAddress)
      {
         int connectionId = connection->getConnectionId();
         CpMediaInterface* pInterface = connection->getMediaInterfacePtr();
         if ((pInterface != NULL) && (connectionId >= 0))
         {
            if (pInterface->getAudioEnergyLevels(
               connectionId,
               *piInputEnergyLevel,
               *piOutputEnergyLevel,
               *pnContributors,
               pContributorSRCIds,
               pContributorEngeryLevels) == OS_SUCCESS)
            {
               bSucccess = true;
            }
         }
         break;
      }
   }

   // If the event has already been signalled, clean up
   if(event && OS_ALREADY_SIGNALED == event->signal(bSucccess))
   {
      // The other end must have timed out on the wait
      OsProtectEventMgr* eventMgr = OsProtectEventMgr::getEventMgr();
      eventMgr->release(event);
   }

   return true;
}   

// Handles the processing of a CP_GET_CALL_MEDIA_ENERGY_LEVELS message
UtlBoolean CpPeerCall::handleGetCallMediaEnergyLevels(OsMsg* pEventMessage)
{
   UtlBoolean bSuccess = false;
   CpMultiStringMessage* pMultiMessage = (CpMultiStringMessage*) pEventMessage;
   UtlString callId;
   UtlString remoteAddress;
   OsProtectedEvent* event;
   int* piInputEnergyLevel;
   int* piOutputEnergyLevel;

   pMultiMessage->getString1Data(callId);
   event = (OsProtectedEvent*) pMultiMessage->getInt1Data();
   piInputEnergyLevel = (int*) pMultiMessage->getInt2Data();
   piOutputEnergyLevel = (int*) pMultiMessage->getInt3Data();

   if (mpMediaInterface)
   {
      mpMediaInterface->getAudioEnergyLevels(*piInputEnergyLevel, *piOutputEnergyLevel);
      bSuccess = true;
   }

   // If the event has already been signalled, clean up
   if(event && OS_ALREADY_SIGNALED == event->signal(bSuccess))
   {
      // The other end must have timed out on the wait
      OsProtectEventMgr* eventMgr = OsProtectEventMgr::getEventMgr();
      eventMgr->release(event);
   }

   return true;
}

UtlBoolean CpPeerCall::handleGetCanAddParty(OsMsg* pEventMessage)
{
   UtlBoolean bCanAdd = FALSE;
   CpMultiStringMessage* pMultiMessage = (CpMultiStringMessage*) pEventMessage;
   OsProtectedEvent* event = (OsProtectedEvent*) 
      ((CpMultiStringMessage*)pEventMessage)->getInt1Data();

   UtlString callId;    
   pMultiMessage->getString1Data(callId);

   if (mpMediaInterface)
   {
      bCanAdd = mpMediaInterface->canAddParty();
   }

   // If the event has already been signalled, clean up
   if(event && OS_ALREADY_SIGNALED == event->signal(bCanAdd))
   {
      // The other end must have timed out on the wait
      OsProtectEventMgr* eventMgr = OsProtectEventMgr::getEventMgr();
      eventMgr->release(event);
   }

   return true;
}

// Handles the processing of a CP_SPLIT_CONNECTION message
UtlBoolean CpPeerCall::handleSplitConnection(OsMsg* pEventMessage)
{    
   UtlString sourceCallId;
   UtlString remoteAddress;
   UtlString targetCallId;
   CpMultiStringMessage* pMultiMessage = (CpMultiStringMessage*) pEventMessage;
   pMultiMessage->getString1Data(sourceCallId);
   pMultiMessage->getString2Data(remoteAddress);
   pMultiMessage->getString3Data(targetCallId);
   OsProtectedEvent* pEvent = (OsProtectedEvent*) pMultiMessage->getInt1Data();
   UtlBoolean bAutoUnhold = (UtlBoolean) pMultiMessage->getInt2Data();

   OsSysLog::add(FAC_CP, PRI_INFO, "Splitting connection %s from %s to %s",
      remoteAddress.data(), sourceCallId.data(), targetCallId.data());

   Connection* pConnection = findHandlingConnection(remoteAddress);
   if (pConnection != NULL)
   {
      OsWriteLock lock(mConnectionMutex); // TODO: Move write lock above and inline findHandling

      // Call must be on hold prior to split/join.
      if (pConnection->isHeld())
      {
         pConnection->prepareForSplit();
         removeConnection(pConnection);           

         CpMultiStringMessage joinMessage(CallManager::CP_JOIN_CONNECTION,   
            targetCallId,
            remoteAddress,
            NULL,
            NULL,
            NULL,
            (int) pConnection,
            (int) pEvent,
            (int) bAutoUnhold);
         mpManager->postMessage(joinMessage);
      }
      else
      {
         if (pEvent)
         {
            pEvent->signal(FALSE);
         }
      }
   }
   else
   {
      if (pEvent)
      {
         pEvent->signal(FALSE);
      }
   }

   return true; 
}


// Handles the processing of a CP_JOIN_CONNECTION message
UtlBoolean CpPeerCall::handleJoinConnection(OsMsg* pEventMessage)
{
   UtlString remoteAddress;
   UtlString sourceCallId;

   CpMultiStringMessage* pMultiMessage = (CpMultiStringMessage*) pEventMessage;
   Connection* pConnection = (Connection*) pMultiMessage->getInt1Data();
   OsProtectedEvent* pEvent = (OsProtectedEvent*) pMultiMessage->getInt2Data();

   UtlBoolean bAutoUnhold = (UtlBoolean) pMultiMessage->getInt3Data();
   pMultiMessage->getString1Data(sourceCallId);
   pMultiMessage->getString2Data(remoteAddress);

   OsSysLog::add(FAC_CP, PRI_INFO, "Joining connection %s to %s (unhold=%d)",
      remoteAddress.data(), sourceCallId.data(), bAutoUnhold);

   pConnection->prepareForJoin(this, NULL, mpMediaInterface);
   addConnection(pConnection);

   if (bAutoUnhold)
   {
      pConnection->offHold();
   }

   if (pEvent)
   {
      pEvent->signal(TRUE);
   }

   return true; 
}

// Handles the processing of a CP_TRANSFER_OTHER_PARTY_HOLD message
UtlBoolean CpPeerCall::handleTransferOtherPartyHold(OsMsg* pEventMessage) 
{
   UtlString targetCallId;
   UtlString remoteAddress;
   Connection* pConnection;

   CpMultiStringMessage* pMultiMessage = (CpMultiStringMessage*) pEventMessage;
   pMultiMessage->getString1Data(targetCallId);
   pMultiMessage->getString2Data(remoteAddress);

   UtlDListIterator iterator(mConnections);
   while ((pConnection = dynamic_cast<Connection*>(iterator())))
   {
      Url remoteUrl(remoteAddress);

      if (!pConnection->isSameRemoteAddress(remoteUrl))
      {
         if (pConnection->isHeld())
         {
            pConnection->setTransferHeld(false);
         }
         else
         {
            pConnection->setTransferHeld(true);
            pConnection->hold();                
         }
      }
   }

   return true;
}

// Handles the processing of a CP_TRANSFER_OTHER_PARTY_UNHOLD message
UtlBoolean CpPeerCall::handleTransferOtherPartyUnhold(OsMsg* pEventMessage) 
{
   UtlString targetCallId;
   UtlString remoteAddress;
   Connection* pConnection;

   CpMultiStringMessage* pMultiMessage = (CpMultiStringMessage*) pEventMessage;
   pMultiMessage->getString1Data(targetCallId);
   pMultiMessage->getString2Data(remoteAddress);

   UtlDListIterator iterator(mConnections);
   while ((pConnection = dynamic_cast<Connection*>(iterator())))
   {
      Url remoteUrl(remoteAddress);

      if (!pConnection->isSameRemoteAddress(remoteUrl))
      {
         if (pConnection->isTransferHeld())
         {
            pConnection->offHold();
         }
         pConnection->setTransferHeld(false);
      }
   }

   return true;
}

// Handles the processing of a CP_MUTE_INPUT_TERM_CONNECTION message
UtlBoolean CpPeerCall::handleMuteInputTermConnection(OsMsg* pEventMessage)
{
   UtlString targetCallId;
   UtlString remoteAddress;
   Connection* pConnection = NULL;

   CpMultiStringMessage* pMultiMessage = (CpMultiStringMessage*) pEventMessage;
   pMultiMessage->getString1Data(targetCallId);
   pMultiMessage->getString2Data(remoteAddress);
   UtlBoolean mute = (UtlBoolean)pMultiMessage->getInt1Data();
   OsEvent* pEvent = (OsEvent*)pMultiMessage->getInt2Data();
   OsStatus res = OS_FAILED;

   pConnection = findHandlingConnection(remoteAddress);
   if (pConnection)
   {
      if (pConnection->muteInput(mute))
      {
         res = OS_SUCCESS;
      }
   }

   if (pEvent)
   {
      pEvent->signal(res);
   }

   return true;
}

// Handles the processing of a CP_CREATE_MEDIA_INTERFACE message
UtlBoolean CpPeerCall::handleCreateMediaInterface(OsMsg* pEventMessage)
{
   if (mpManager)
   {
      CpMultiStringMessage* pMsg = dynamic_cast<CpMultiStringMessage*>(pEventMessage);
      if (!mpMediaInterface && pMsg)
      {
         UtlString localIPAddress;
         pMsg->getString1Data(localIPAddress);

         mpMediaInterface = mpManager->createMediaInterface(localIPAddress);
         mpMediaInterface->setInterfaceNotificationQueue(getMessageQueue());
      }
   }
   return TRUE;
}

// Handles the processing of a CP_TRANSFER_OTHER_PARTY_JOIN message
UtlBoolean CpPeerCall::handleTransferOtherPartyJoin(OsMsg* pEventMessage) 
{
   UtlString sourceCallId;
   UtlString remoteAddress;
   UtlString targetCallId;
   Connection* pConnection;

   CpMultiStringMessage* pMultiMessage = (CpMultiStringMessage*) pEventMessage;
   pMultiMessage->getString1Data(sourceCallId);
   pMultiMessage->getString2Data(remoteAddress);
   pMultiMessage->getString3Data(targetCallId);    

   UtlDListIterator iterator(mConnections);
   while ((pConnection = dynamic_cast<Connection*>(iterator())))
   {
      Url remoteUrl(remoteAddress);

      if (!pConnection->isSameRemoteAddress(remoteUrl))
      {
         UtlString connRemoteAddress;
         if (pConnection->getRemoteAddress(&connRemoteAddress))
         {
            UtlBoolean bAutoUnhold = pConnection->isTransferHeld();

            CpMultiStringMessage msg(CpCallManager::CP_SPLIT_CONNECTION, 
               sourceCallId, connRemoteAddress, targetCallId, 
               NULL, NULL, 0, bAutoUnhold);

            mpManager->postMessage(msg);
         }
      }
   }

   return true; 
}


Connection* CpPeerCall::addParty(const UtlString& localAddress,
                                 const UtlString& remoteAddress,
                                 const char* callController,
                                 const char* originalCallConnectionAddress,
                                 const char* newCallId,
                                 SIPX_CONTACT_ID contactId,
                                 const void* pDisplay,
                                 const void* pSecurity,
                                 const char* locationHeader,
                                 const int bandWidth,
                                 UtlBoolean bOnHold,
                                 const char* originalCallId,
                                 SIPX_TRANSPORT_DATA* pTransport,
                                 const RTP_TRANSPORT rtpTransportOptions)
{
   SipConnection* connection = NULL;
   UtlString sTransferTargetAddress;

   connection = new SipConnection(localAddress,
      mIsEarlyMediaFor180,
      mpManager,
      this,
      mpMediaInterface,
      sipUserAgent,
      m_pCallEventListener,
      m_pInfoStatusEventListener,
      m_pSecurityEventListener,
      m_pMediaEventListener,
      offeringDelay, 
      mSipSessionReinviteTimer);
   connection->setBindIPAddress(m_bindIPAddress);
   if (pSecurity)
   {
      connection->setSecurity((SIPXTACK_SECURITY_ATTRIBUTES*)pSecurity);
   }

   UtlString voiceQualityReportTarget;
   if (mpManager->getVoiceQualityReportTarget(voiceQualityReportTarget))
   {
      connection->setVoiceQualityReportTarget(voiceQualityReportTarget);
   }

   connection->setExternalTransport(pTransport);

   connection->setContactId(contactId);
   SIPX_CONTACT_ADDRESS* pContact = NULL;

   // if we are calling someone with a "sips:" schema, 
   // we should assume that we want to use our TLS contact,
   // so we should select it now
   if (remoteAddress.contains("sips:"))
   {
      pContact = sipUserAgent->getContactDb().findByType(CONTACT_LOCAL, TRANSPORT_TLS);
      connection->setContactId(pContact->id);
   }
   if (!pContact)
   {
      pContact = sipUserAgent->getContactDb().find(contactId);
   }
   if (pContact)
   {
      Url url(remoteAddress);
      connection->setContactType(pContact->eContactType, &url);
   }
   else
   {
      Url url(remoteAddress);
      connection->setContactType(CONTACT_AUTO, &url);
   }
   addConnection(connection);

   UtlString callId;
   if (newCallId != NULL)
   {
      callId = newCallId;
   }
   else
   {
      mpManager->getNewCallId(&callId);
   }

   connection->dial(remoteAddress,
      localAddress.data(), 
      callId.data(),
      callController,
      originalCallConnectionAddress, 
      FALSE,
      pDisplay,
      pSecurity,
      locationHeader,
      bandWidth,
      bOnHold,
      originalCallId,
      rtpTransportOptions); 

   return connection;
}

UtlBoolean CpPeerCall::hasCallId(const char* callIdString)
{
   UtlString connectionCallId;
   UtlBoolean foundCallId = FALSE;
   Connection* connection = NULL;
   OsReadLock lock(mConnectionMutex);
   UtlDListIterator iterator(mConnections);

   while ((connection = dynamic_cast<Connection*>(iterator())))
   {
      connection->getCallId(&connectionCallId);
      if(strcmp(callIdString, connectionCallId.data()) == 0)
      {
         foundCallId = TRUE;
         break;
      }

   }

   UtlString callId;
   getCallId(callId);
   if(!foundCallId && callId.compareTo(callIdString) == 0)
   {
      foundCallId = TRUE;
   }

   return(foundCallId);
}

OsStatus CpPeerCall::getConnectionCallIds(UtlSList& pCallIdList)
{
   OsStatus returnCode = OS_NOT_FOUND;
   UtlString connectionCallId;
   Connection* connection = NULL;

   OsReadLock lock(mConnectionMutex);
   UtlDListIterator iterator(mConnections);

   while ((connection = dynamic_cast<Connection*>(iterator())))
   {
      connection->getCallId(&connectionCallId);
      pCallIdList.append(new UtlString(connectionCallId));
      returnCode = OS_SUCCESS;
   }

   return returnCode;
}

void CpPeerCall::inFocus(int talking)
{
   CpCall::inFocus();

   OsReadLock lock(mConnectionMutex);
   Connection* connection = (Connection*) mConnections.first();

   int remoteIsCallee = 1;
   UtlString remoteAddress;
   if(connection)
   {
      UtlString connectionCallId;
      connection->getCallId(&connectionCallId);
      remoteIsCallee = connection->isRemoteCallee();
      connection->getRemoteAddress(&remoteAddress);
   }

   // Notify listeners that the local connection is in focus
   if (!talking)
   {
      int responseCode = 0;
      UtlString responseText;
      if (connection)
      {
         responseCode = connection->getResponseCode();
         connection->getResponseText(responseText);
      }

      if (getCallState() != PtCall::ACTIVE)
      {
         setCallState(responseCode, responseText, PtCall::ACTIVE, PtEvent::CAUSE_NEW_CALL);
      }

      if (mLocalTermConnectionState == PtTerminalConnection::IDLE)
      {
         postTaoListenerMessage(responseCode, responseText, PtEvent::TERMINAL_CONNECTION_CREATED, TERMINAL_CONNECTION_STATE, PtEvent::CAUSE_NEW_CALL, remoteIsCallee, remoteAddress);

         int metaEventId = 0;
         int metaEventType = PtEvent::META_EVENT_NONE;
         int numCalls = 0;
         const UtlString* metaEventCallIds = NULL;
         getMetaEvent(metaEventId, metaEventType, numCalls, 
            &metaEventCallIds);
         if(metaEventType != PtEvent::META_CALL_TRANSFERRING)
            stopMetaEvent();
      }
   }
   else 
   {
      UtlDListIterator iterator(mConnections);
      while ((connection = dynamic_cast<Connection*>(iterator())))
      {
         int cause = 0;
         int state = connection->getState(cause);
         if (state != Connection::CONNECTION_ALERTING || mLocalTermConnectionState == PtTerminalConnection::HELD)
         {
            UtlString responseText;
            connection->getResponseText(responseText);
            postTaoListenerMessage(connection->getResponseCode(), responseText, PtEvent::TERMINAL_CONNECTION_TALKING, TERMINAL_CONNECTION_STATE, PtEvent::CAUSE_UNHOLD, remoteIsCallee, remoteAddress);
         }
      }
   }

   UtlDListIterator iterator(mConnections);
   while ((connection = dynamic_cast<Connection*>(iterator())))
   {
      int cause = 0;
      int state = connection->getState(cause);

      if (state != Connection::CONNECTION_ALERTING)
      {
         if (!connection->isHoldInProgress())
         {
            if (connection->isHeld())
            {
               connection->fireSipXCallEvent(CALLSTATE_REMOTE_HELD, CALLSTATE_CAUSE_NORMAL);
            }
            else
            {
               connection->fireSipXCallEvent(CALLSTATE_CONNECTED, CALLSTATE_CAUSE_NORMAL);
            }
         }
      }
   }    
}

void CpPeerCall::outOfFocus()
{
   CpCall::outOfFocus();

   OsReadLock lock(mConnectionMutex);

   UtlDListIterator iterator(mConnections);
   Connection* connection = NULL;

   while ((connection = dynamic_cast<Connection*>(iterator())))
   {
      if(connection->isHeld())
      {
         int remoteIsCallee = 1;
         UtlString responseText;
         UtlString remoteAddress;
         UtlString connectionCallId;
         connection->getCallId(&connectionCallId);
         remoteIsCallee = connection->isRemoteCallee();
         connection->getRemoteAddress(&remoteAddress);
         connection->getResponseText(responseText);
         connection->outOfFocus();

         // Notify listeners that the local connection is out of focus
         postTaoListenerMessage(connection->getResponseCode(), responseText, PtEvent::TERMINAL_CONNECTION_HELD, TERMINAL_CONNECTION_STATE, PtEvent::CAUSE_NORMAL, remoteIsCallee, remoteAddress);
      }

      if (!connection->isHoldInProgress() && !mDropping)
      {
         if (connection->isHeld())
         {
            connection->fireSipXCallEvent(CALLSTATE_HELD, CALLSTATE_CAUSE_NORMAL);
         }
         else
         {
            connection->fireSipXCallEvent(CALLSTATE_BRIDGED, CALLSTATE_CAUSE_NORMAL);
         }
      }
   }
}

void CpPeerCall::onHook()
{
   Connection* connection = NULL;

   // Take this call out of focus right away
   CpIntMessage localHoldMessage(CallManager::CP_YIELD_FOCUS, (int) this);
   mpManager->postMessage(localHoldMessage);
   {
      OsReadLock lock(mConnectionMutex);
      UtlDListIterator iterator(mConnections);

      while ((connection = dynamic_cast<Connection*>(iterator())))
      {
         connection->hangUp();
         connection->setMediaInterface(NULL);

         // do not fire the taip event if it is a ghost connection
         CpGhostConnection* pGhost = NULL;
         pGhost = dynamic_cast<CpGhostConnection*>(connection);

         //          causes double CALLSTATE_DISCONNECTED events
         if (!pGhost)
         {
            connection->fireSipXCallEvent(CALLSTATE_DISCONNECTED, CALLSTATE_CAUSE_NORMAL);
         }
      }
   }

   dropIfDead();
}

void CpPeerCall::hangUp(const char* callId, 
                        const char* toTag,
                        const char* fromTag)
{
   Connection* connection = findHandlingConnection(callId, 
      toTag,
      fromTag,
      FALSE);
   if(connection)
   {
      connection->hangUp();
   }
}


// Get the connection for the connection identified by the designated callId,
// toTag, and fromTag.  If the connection cannot be found a UtlBoolean value of
// false is returned.
UtlBoolean CpPeerCall::getConnectionState(const char* callId, 
                                          const char* toTag,
                                          const char* fromTag,
                                          int&        state,
                                          UtlBoolean   strictCompare)
{
   UtlBoolean bRC = FALSE;

   Connection* connection = findHandlingConnection(callId, toTag, fromTag, strictCompare);
   if(connection)
   {
      state = connection->getState();
      bRC = TRUE;
   }

   return bRC;
}


void CpPeerCall::dropIfDead()
{
   int localConnectionState;    

   // If all the connections are dead, drop the call
   if (mDropping && !isConnectionLive(&localConnectionState))
   {
      if (mbRequestedDrop)
      {
         return;
      }
      else
      {
         mbRequestedDrop = true;
      }

      setCallState(0, "", PtCall::INVALID);          

      // Signal the manager to Shutdown the task
      // Do this at the very last opportunity
      {
         OsReadLock lock(mConnectionMutex);

         if (mConnections.entries())
         {
            // Notify listeners that call is going to be torn down
            UtlDListIterator iterator(mConnections);
            Connection* connection = NULL;
            while ((connection = dynamic_cast<Connection*>(iterator())))
            {              
               // do not fire the taip event if it is a ghost connection
               CpGhostConnection* pGhost = NULL;
               pGhost = dynamic_cast<CpGhostConnection*>(connection);
               if (!pGhost)
               {
                  connection->fireSipXCallEvent(CALLSTATE_DESTROYED, CALLSTATE_CAUSE_NORMAL);
               }
            }
            // Drop the call immediately
            releaseMediaInterface();
            CpIntMessage ExitMsg(CallManager::CP_CALL_EXITED, (int)this);
            mpManager->postMessage(ExitMsg);
         }
         else
         {
            // Drop the call immediately
            releaseMediaInterface();
            CpIntMessage ExitMsg(CallManager::CP_CALL_EXITED, (int)this);
            mpManager->postMessage(ExitMsg);

            if (m_pCallEventListener)
            {
               // fire DESTROYED event for the CpPeerCall, call is not connected
               UtlString sCallId;
               getCallId(sCallId);
               CpCallStateEvent event(sCallId, NULL, CP_CALLSTATE_CAUSE_NORMAL);

               m_pCallEventListener->OnDestroyed(event);
            }

         }
      }
   }
   else
   {
      dropDeadConnections();
   }
}

void CpPeerCall::dropDeadConnections()
{
   OsWriteLock lock(mConnectionMutex);
   Connection* connection = NULL;
   int         connectionState;
   OsTime      now;

   OsDateTime::getCurTimeSinceBoot(now);
   UtlDListIterator iterator(mConnections);
   while ((connection = dynamic_cast<Connection*>(iterator())))
   {
      // 1. Look for newly disconnected/failed connections and fire off events
      // and mark for deletion.

      int cause = 0;
      connectionState = connection->getState(0, cause);    // get remote connection state
      if (!connection->isMarkedForDeletion() &&
         (connectionState ==  Connection::CONNECTION_DISCONNECTED ||
         connectionState == Connection::CONNECTION_FAILED))
      {
         int localState = connection->getState(1, cause);    // get local state
         if (localState ==  Connection::CONNECTION_DISCONNECTED)
         {
            UtlString responseText;
            connection->getResponseText(responseText);

            // do not fire the taip event if it is a ghost connection
            CpGhostConnection* pGhost = NULL;
            pGhost = dynamic_cast<CpGhostConnection*>(connection);
            if (!pGhost)
            {
               connection->fireSipXCallEvent(CALLSTATE_DISCONNECTED, CALLSTATE_CAUSE_NORMAL);
            }

            postTaoListenerMessage(connection->getResponseCode(), responseText, PtEvent::TERMINAL_CONNECTION_DROPPED, TERMINAL_CONNECTION_STATE);
         }
         else if (localState ==  Connection::CONNECTION_FAILED)
         {
            UtlString responseText;
            connection->getResponseText(responseText);
            postTaoListenerMessage(connection->getResponseCode(), responseText, PtEvent::TERMINAL_CONNECTION_DROPPED, TERMINAL_CONNECTION_STATE);

            CpGhostConnection* pGhost = NULL;
            pGhost = dynamic_cast<CpGhostConnection*>(connection);

            // do not fire the tapi event if it is a ghost connection
            if (!pGhost)
            {
               connection->fireSipXCallEvent(CALLSTATE_DISCONNECTED, CALLSTATE_CAUSE_NORMAL);
            }
         }               

         // Mark the connection for deletion
         connection->markForDeletion();            
      } 

      // 2. Look for connections which could be removed/deleted
      if (connection->isMarkedForDeletion())
      {
         OsTime deleteAfter;
         connection->getDeleteAfter(deleteAfter);
         if (now > deleteAfter)
         {            
            mConnections.destroy(connection);
         }
      }
   }              
}


void CpPeerCall::offHook(const void* pDisplay)
{
   OsSysLog::add(FAC_CP, PRI_DEBUG,"%s-CpPeerCall::offHook\n", mName.data());

   OsReadLock lock(mConnectionMutex);
   Connection* connection = NULL;

   UtlDListIterator iterator(mConnections);
   while ((connection = dynamic_cast<Connection*>(iterator())))
   {
      if(connection &&
         connection->getState() == Connection::CONNECTION_ALERTING)
      {
         connection->answer(pDisplay);
         mLocalConnectionState = PtEvent::CONNECTION_ESTABLISHED;
      }
   }
}

/* ============================ ACCESSORS ================================= */

/* ============================ INQUIRY =================================== */

UtlBoolean CpPeerCall::shouldCreateCall(SipUserAgent& sipUa, OsMsg& eventMessage,
                                        SdpCodecList& codecFactory)
{
   UtlBoolean createCall = FALSE;
   int msgType = eventMessage.getMsgType();
   int msgSubType = eventMessage.getMsgSubType();

   if(msgType == OsMsg::PHONE_APP &&
      msgSubType == CallManager::CP_SIP_MESSAGE)
   {
      createCall = SipConnection::shouldCreateConnection(sipUa, eventMessage,
         &codecFactory);
   }
   // This should work with the connection Factory to decide which
   // type of connection and if the connection should be created.
   return(createCall);
}

CpCall::handleWillingness CpPeerCall::willHandleMessage(const OsMsg& eventMessage)
{
   CpCall::handleWillingness takeTheMessage = CP_WILL_NOT_HANDLE;

   if(eventMessage.getMsgType() == OsMsg::PHONE_APP &&
      eventMessage.getMsgSubType() == CallManager::CP_SIP_MESSAGE)
   {
      const SipMessage* sipMsg = ((SipMessageEvent&)eventMessage).getMessage();

      if(sipMsg)
      {
         int seqNum;
         UtlString seqMethod;
         sipMsg->getCSeqField(&seqNum,&seqMethod);
         UtlString strToField;
         sipMsg->getToField(&strToField);

         // Ignore any reINVITE when attempting to drop a call.
         if (mDropping && seqMethod == "INVITE" && !sipMsg->isResponse())
         {
            if (OsSysLog::willLog(FAC_CP, PRI_WARNING))
            {
               UtlString callId;
               sipMsg->getCallIdField(&callId);                    

               OsSysLog::add(FAC_CP, PRI_WARNING, 
                  "willHandleMessage: Ignoring SIP request for dropping call: %s",
                  callId.data());
            }
         }
         else
         {
            UtlString callId;
            sipMsg->getCallIdField(&callId);
            UtlBoolean thisCallHasCallId = hasCallId(callId.data());
            if(thisCallHasCallId)
            {
               takeTheMessage = CP_DEFINITELY_WILL_HANDLE;
            }

            // Check if this is an INVITE with a Replaces header
            if(takeTheMessage == CP_WILL_NOT_HANDLE && 
               !sipMsg->isResponse())
            {
               UtlString method;
               sipMsg->getRequestMethod(&method);

               if(method.compareTo(SIP_INVITE_METHOD) == 0)
               {
                  // If there is a Replaces header check for a 
                  // match of the Call-Id in the Replaces header
                  UtlString toTag;
                  UtlString fromTag;
                  sipMsg->getReplacesData(callId, toTag, fromTag);
                  UtlBoolean replacesMatchesThisCallId = 
                     hasCallId(callId.data());
                  if(replacesMatchesThisCallId)
                  {
                     takeTheMessage = CP_MAY_HANDLE;
                  }
               }
            }
         }
      }
   }

   return(takeTheMessage);
}

UtlBoolean CpPeerCall::isConnectionLive(int* localConnectionState)
{
   UtlBoolean liveConnections = FALSE;
   OsReadLock lock(mConnectionMutex);
   UtlDListIterator iterator(mConnections);
   Connection* connection = NULL;
   int connectionState;
   if(localConnectionState) 
   {
      *localConnectionState = getLocalConnectionStateFromPt(mLocalConnectionState);
   }


   while ((connection = dynamic_cast<Connection*>(iterator())))
   {
      int cause;
      connectionState = connection->getState(0, cause);    // get remote connection state

      if(localConnectionState && 
         *localConnectionState != Connection::CONNECTION_ESTABLISHED &&
         connectionState == Connection::CONNECTION_DISCONNECTED)
      {
         *localConnectionState = connectionState;
      }

      if(connectionState != Connection::CONNECTION_DISCONNECTED &&
         connectionState != Connection::CONNECTION_FAILED  &&
         connectionState != Connection::CONNECTION_UNKNOWN)
         // Atleast sometimes we do not want to kill the call
         // if there are IDLE connections
         //connectionState != Connection::CONNECTION_IDLE)
      {
         liveConnections = TRUE;
         if(localConnectionState) 
            *localConnectionState = Connection::CONNECTION_ESTABLISHED;
         break;
      }
   }
   return(liveConnections);
}

UtlBoolean CpPeerCall::isConnection(const char* callId, 
                                    const char* toTag,
                                    const char* fromTag)
{
   return(NULL != findHandlingConnection(callId, 
      toTag,
      fromTag,
      FALSE));
}

UtlBoolean CpPeerCall::canDisconnectConnection(Connection* pConnection)
{
   UtlBoolean ret;
   Connection* connection = NULL;
   OsReadLock lock(mConnectionMutex);
   UtlDListIterator iterator(mConnections);
   int cnt = 0;
   int contains = 0;

   while ((connection = dynamic_cast<Connection*>(iterator())))
   {
      cnt++;
      if (connection == pConnection) contains = 1;
   }

   ret = ((cnt >= 1) &&
      contains  && 
      (!mLocalHeld || 
      mLocalTermConnectionState != PtTerminalConnection::HELD));

   return ret;
}


UtlBoolean CpPeerCall::isConnectionLocallyInitiatedRemoteHold(const char* callId, 
                                                              const char* toTag,
                                                              const char* fromTag) 
{
   UtlBoolean bHeld = false;

   Connection* pConnection = findHandlingConnection(callId, toTag, fromTag, true);
   if (pConnection)
   {
      bHeld = pConnection->isLocallyInitiatedRemoteHold();
   }

   return bHeld;
}


/* //////////////////////////// PROTECTED ///////////////////////////////// */
Connection* CpPeerCall::findHandlingConnection(OsMsg& eventMessage)
{
   Connection* connection = NULL;
   OsReadLock lock(mConnectionMutex);
   UtlDListIterator iterator(mConnections);

   if (mConnections.entries())
   {
      while ((connection = dynamic_cast<Connection*>(iterator())))
      {
         if(connection->willHandleMessage(eventMessage)) break;
         connection = NULL;
      }    
   }

   return(connection);
}

Connection* CpPeerCall::findHandlingConnection(const char* callId, 
                                               const char* toTag,
                                               const char* fromTag,
                                               UtlBoolean  strictCompare)
{
   Connection* connection = NULL;
   OsReadLock lock(mConnectionMutex);
   UtlDListIterator iterator(mConnections);

   while ((connection = dynamic_cast<Connection*>(iterator())))
   {
      if(connection->isConnection(callId, 
         toTag,
         fromTag,
         strictCompare)) break;
      connection = NULL;
   }
   return(connection);
}

Connection* CpPeerCall::findHandlingConnection(UtlString& remoteAddress)
{
   OsReadLock lock(mConnectionMutex);
   Connection* connection = NULL;
   UtlDListIterator iterator(mConnections);
   Url remoteUrl(remoteAddress);

   while ((connection = dynamic_cast<Connection*>(iterator())))
   {
      UtlString connectionRemoteAddress;

      connection->getRemoteAddress(&connectionRemoteAddress);
      if (!connectionRemoteAddress.isNull())
      {
         Url connectionAddressUrl(connectionRemoteAddress);

         // This allows remoteUrl to match if it does not have a tag
         if(SipMessage::isSameSession(remoteUrl, connectionAddressUrl)) break;

         // This allows connectionAddressUrl to match if it does not have a tag
         if(SipMessage::isSameSession(connectionAddressUrl, remoteUrl)) break;

         connection = NULL;
      }
   }        
   return(connection);
}

// find connection by remote address and sip Call ID
Connection* CpPeerCall::findHandlingConnection(const UtlString& remoteAddress,
                                               const UtlString& sessionCallId)
{
   OsReadLock lock(mConnectionMutex);
   Connection* connection = NULL;
   UtlDListIterator iterator(mConnections);

   while ((connection = dynamic_cast<Connection*>(iterator())))
   {
      Url remoteUrl(remoteAddress);
      UtlString connectionRemoteAddress;
      UtlString connectionSessionCallId;

      connection->getRemoteAddress(&connectionRemoteAddress);
      connection->getCallId(&connectionSessionCallId);

      if (!connectionRemoteAddress.isNull() &&
         connectionSessionCallId.compareTo(&sessionCallId) == 0)
      {
         Url connectionAddressUrl(connectionRemoteAddress);

         // This allows remoteUrl to match if it does not have a tag
         if(SipMessage::isSameSession(remoteUrl, connectionAddressUrl)) break;

         // This allows connectionAddressUrl to match if it does not have a tag
         if(SipMessage::isSameSession(connectionAddressUrl, remoteUrl)) break;

         connection = NULL;
      }
   }        
   return(connection);
}

void CpPeerCall::addConnection(Connection* connection)
{
   OsWriteLock lock(mConnectionMutex);
   mConnections.append(connection);
}

// Assumed lock is head externally
void CpPeerCall::removeConnection(Connection* connection)
{
   // OsWriteLock lock(mConnectionMutex);
   mConnections.remove(connection);
}

Connection* CpPeerCall::findQueuedConnection()
{
   Connection* connection = NULL;
   OsReadLock lock(mConnectionMutex);
   UtlDListIterator iterator(mConnections);

   while ((connection = dynamic_cast<Connection*>(iterator())))
   {
      if(connection->getState() == Connection::CONNECTION_QUEUED) break;
      connection = NULL;
   }
   return(connection);
}


/* //////////////////////////// PRIVATE /////////////////////////////////// */
UtlBoolean CpPeerCall::checkForTag(UtlString &address)
{
   if (address.compareTo("sip:") == 0)
   {
      return FALSE;
   }

   UtlString tag;
   Url url(address);
   url.getFieldParameter("tag", tag);

   if (tag.isNull())
   {
      return FALSE;
   }
   else
   {
      return TRUE;
   }
}

/* ============================ FUNCTIONS ================================= */

