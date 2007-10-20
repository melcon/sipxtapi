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

#include <time.h>
#include <assert.h>

// APPLICATION INCLUDES
#include <os/OsUtil.h>
#include <os/OsConfigDb.h>
#include <os/OsEventMsg.h>
#include <os/OsTimer.h>
#include <os/OsQueuedEvent.h>
#include <os/OsEvent.h>
#include <os/OsReadLock.h>
#include <os/OsWriteLock.h>
#include <cp/CallManager.h>
#include <utl/UtlRegex.h>
#include <net/SipMessageEvent.h>
#include <net/SipUserAgent.h>
#include <net/SdpCodecFactory.h>
#include <net/Url.h>
#include <net/SipSession.h>
#include <net/SipDialog.h>
#include <net/SipLineMgr.h>
#include <net/NameValueTokenizer.h>
#include <sdp/SdpCodec.h>
#include <cp/CpIntMessage.h>
#include <cp/CpMultiStringMessage.h>
#include <cp/Connection.h>
#include <cp/CpPeerCall.h>
#include <cp/CpCallStateEventListener.h>
#include <mi/CpMediaInterfaceFactory.h>
#include "tapi/SipXTransport.h"


// TO_BE_REMOVED
#ifndef EXCLUDE_STREAMING
#include "mp/MpPlayer.h"
#include "mp/MpStreamMsg.h"
#include "mp/MpStreamPlayer.h"
#include "mp/MpStreamQueuePlayer.h"
#include "mp/MpStreamPlaylistPlayer.h"
#include <mp/MpMediaTask.h> 
#endif

// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
#define MAXIMUM_CALLSTATE_LOG_SIZE 100000
#define CALL_STATUS_FIELD "status"
#define SEND_KEY '#'
char CONVERT_TO_STR[17] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
'*', '#', 'A', 'B', 'C', 'D', 'F'};
/*      _________________________
0--9                0--9
*                     10
#                     11
A--D              12--15
Flash                 16
*/

// STATIC VARIABLE INITIALIZATIONS

/* //////////////////////////// PUBLIC //////////////////////////////////// */

/* ============================ CREATORS ================================== */

// Constructor
CallManager::CallManager(UtlBoolean isRequredUserIdMatch,
                         SipLineMgr* pLineMgrTask,
                         UtlBoolean isEarlyMediaFor180Enabled,
                         SdpCodecFactory* pCodecFactory,
                         int rtpPortStart,
                         int rtpPortEnd,
                         const char* localAddress,
                         const char* publicAddress,
                         SipUserAgent* userAgent,
                         int sipSessionReinviteTimer,
                         CpCallStateEventListener* pCallEventListener,
                         SipInfoStatusEventListener* pInfoStatusEventListener,
                         SipSecurityEventListener* pSecurityEventListener,
                         CpMediaEventListener* pMediaEventListener,
                         PtMGCP* mgcpStackTask,
                         const char* defaultCallExtension,
                         int availableBehavior,
                         const char* unconditionalForwardUrl,
                         int forwardOnNoAnswerSeconds,
                         const char* forwardOnNoAnswerUrl,
                         int busyBehavior,
                         const char* sipForwardOnBusyUrl,
                         OsConfigDb* pConfigDb,
                         CallTypes phonesetOutgoingCallProtocol,
                         int numDialPlanDigits,
                         int offeringDelay,
                         const char* locale,
                         int inviteExpireSeconds,
                         int expeditedIpTos,
                         int maxCalls,
                         CpMediaInterfaceFactory* pMediaFactory) 
                         : CpCallManager("CallManager-%d", "c",
                         rtpPortStart, rtpPortEnd, localAddress, publicAddress)
                         , mIsEarlyMediaFor180(TRUE)
                         , mpMediaFactory(NULL)
                         , m_pCallEventListener(pCallEventListener)
                         , m_pInfoStatusEventListener(pInfoStatusEventListener)
                         , m_pSecurityEventListener(pSecurityEventListener)
                         , m_pMediaEventListener(pMediaEventListener)
{
    OsStackTraceLogger(FAC_CP, PRI_DEBUG, "CallManager");

    dialing = FALSE;
    mOffHook = FALSE;
    speakerOn = FALSE;
    flashPending = FALSE;
    mIsEarlyMediaFor180 = isEarlyMediaFor180Enabled;
    mNumDialPlanDigits = numDialPlanDigits;
    mnTotalIncomingCalls = 0;
    mnTotalOutgoingCalls = 0;
    mMaxCalls = maxCalls ;
    
    if (pMediaFactory)
    {
        mpMediaFactory = pMediaFactory;
    }
    else
    {
        assert(false);
    }

    // Instruct the factory to use the specified port range
    mpMediaFactory->setRtpPortRange(rtpPortStart, rtpPortEnd) ;

    mLineAvailableBehavior = availableBehavior;
    mOfferedTimeOut = offeringDelay;
    mNoAnswerTimeout = forwardOnNoAnswerSeconds;
    if(forwardOnNoAnswerUrl)
    {
        mForwardOnNoAnswer = forwardOnNoAnswerUrl;
        if (mNoAnswerTimeout < 0)
            mNoAnswerTimeout = 24;  // default
    }
    if(unconditionalForwardUrl)
        mForwardUnconditional = unconditionalForwardUrl;
    mLineBusyBehavior = busyBehavior;
    if(sipForwardOnBusyUrl)
    {
        mSipForwardOnBusy.append(sipForwardOnBusyUrl);
    }
#ifdef TEST
    OsSysLog::add(FAC_CP, PRI_DEBUG, "SIP forward on busy URL: %s\nSIP unconditional forward URL: %s\nSIP no answer timeout:%d URL: %s\n",
        mSipForwardOnBusy.data(), mForwardUnconditional.data(),
        forwardOnNoAnswerSeconds, mForwardOnNoAnswer.data());
#endif

    mLocale = locale ? locale : "";

    if (inviteExpireSeconds > 0 && inviteExpireSeconds < CP_MAXIMUM_RINGING_EXPIRE_SECONDS)
        mInviteExpireSeconds = inviteExpireSeconds;
    else
        mInviteExpireSeconds = CP_MAXIMUM_RINGING_EXPIRE_SECONDS;

    mpLineMgrTask = pLineMgrTask;
    mIsRequredUserIdMatch = isRequredUserIdMatch;
    mExpeditedIpTos = expeditedIpTos;

    // Register with the SIP user agent
    sipUserAgent = userAgent;
    if(sipUserAgent)
    {
        sipUserAgent->addMessageObserver(*(this->getMessageQueue()),
            SIP_INVITE_METHOD,
            TRUE, // want to get requests
            TRUE, // and responses
            TRUE, // Incoming messages
            FALSE); // Don't want to see out going messages
        sipUserAgent->addMessageObserver(*(this->getMessageQueue()),
            SIP_BYE_METHOD,
            TRUE, // want to get requests
            TRUE, // and responses
            TRUE, // Incoming messages
            FALSE); // Don't want to see out going messages
        sipUserAgent->addMessageObserver(*(this->getMessageQueue()),
            SIP_CANCEL_METHOD,
            TRUE, // want to get requests
            TRUE, // and responses
            TRUE, // Incoming messages
            FALSE); // Don't want to see out going messages
        sipUserAgent->addMessageObserver(*(this->getMessageQueue()),
            SIP_ACK_METHOD,
            TRUE, // want to get requests
            FALSE, // no such thing as a ACK response
            TRUE, // Incoming messages
            FALSE); // Don't want to see out going messages
        sipUserAgent->addMessageObserver(*(this->getMessageQueue()),
            SIP_REFER_METHOD,
            TRUE, // want to get requests
            TRUE, // and responses
            TRUE, // Incoming messages
            FALSE); // Don't want to see out going messages
        sipUserAgent->addMessageObserver(*(this->getMessageQueue()),
            SIP_OPTIONS_METHOD,
            FALSE, // don't want to get requests
            TRUE, // do want responses
            TRUE, // Incoming messages
            FALSE); // Don't want to see out going messages
        sipUserAgent->addMessageObserver(*(this->getMessageQueue()),
            SIP_NOTIFY_METHOD,
            TRUE, // do want to get requests
            TRUE, // do want responses
            TRUE, // Incoming messages
            FALSE); // Don't want to see out going messages
        sipUserAgent->addMessageObserver(*(this->getMessageQueue()),
            SIP_INFO_METHOD,
            TRUE, // do want to get requests
            TRUE, // do want responses
            TRUE, // Incoming messages
            FALSE); // Don't want to see out going messages

        // Allow the "replaces" extension, because CallManager
        // implements the INVITE-with-Replaces logic.
        sipUserAgent->allowExtension(SIP_REPLACES_EXTENSION);

        int sipExpireSeconds = sipUserAgent->getDefaultExpiresSeconds();
        if (mInviteExpireSeconds > sipExpireSeconds) mInviteExpireSeconds = sipExpireSeconds;

    }
    mSipSessionReinviteTimer = sipSessionReinviteTimer;

    if(defaultCallExtension)
    {
        mOutboundLine = defaultCallExtension;
    }

    // MGCP stack
    mpMgcpStackTask = mgcpStackTask;

    infocusCall = NULL;
    mOutGoingCallType = phonesetOutgoingCallProtocol;
    mLocalAddress = localAddress;

#ifdef TEST_PRINT
    OsSysLog::add(FAC_CP, PRI_DEBUG, "CallManager: localAddress: %s mLocalAddress: %s publicAddress: %s mPublicAddress %s\n",
        localAddress, mLocalAddress.data(), publicAddress,
        mPublicAddress.data());
#endif

    mpCodecFactory = pCodecFactory;

#ifdef TEST_PRINT
    // Default the log on
    startCallStateLog();
#else
    // Disable the message log
    stopCallStateLog();
#endif

    // Pre-allocate all of the history memory to minimze fragmentation
    for(int h = 0; h < CP_CALL_HISTORY_LENGTH ; h++)
        mCallManagerHistory[h].capacity(256);

    mMessageEventCount = -1 ;
    mStunPort = PORT_NONE ;
    mStunKeepAlivePeriodSecs = 0 ;

    mTurnPort = PORT_NONE ;
    mTurnKeepAlivePeriodSecs = 0 ;
}

// Copy constructor
CallManager::CallManager(const CallManager& rCallManager) :
CpCallManager("CallManager-%d", "call")
{
}

// Destructor
CallManager::~CallManager()
{
    OsStackTraceLogger stackLogger(FAC_CP, PRI_DEBUG, "~CallManager");
    while(getCallStackSize())
    {
        delete popCall();
    }

    waitUntilShutDown();   

    // do not delete the codecFactory it is not owned here
}

/* ============================ MANIPULATORS ============================== */

void CallManager::setOutboundLine(const char* lineUrl)
{
    mOutboundLine = lineUrl ? lineUrl : "";
}


UtlBoolean CallManager::handleMessage(OsMsg& eventMessage)
{
    int msgType = eventMessage.getMsgType();
    int msgSubType = eventMessage.getMsgSubType();
    UtlBoolean messageProcessed = TRUE;
    UtlString holdCallId;
    UtlBoolean messageConsumed = FALSE;
    CpMediaInterface* pMediaInterface;

    switch(msgType)
    {
    case OsMsg::PHONE_APP:
        switch(msgSubType)
        {
        case CP_SIP_MESSAGE:
            {
                OsWriteLock lock(mCallListMutex);
                CpCall* handlingCall = NULL;

                handlingCall = findHandlingCall(eventMessage);

                // This message does not belong to any of the calls
                // If this is an invite for a new call
                // Currently only one call can exist
                if(!handlingCall)
                {
                    UtlString callId;
                    if(msgSubType == CP_SIP_MESSAGE)
                    {
                        const SipMessage* sipMsg = ((SipMessageEvent&)eventMessage).getMessage();
                        if(sipMsg)
                        {
                           UtlString method;
                           sipMsg->getRequestMethod(&method);
                           // always allocate call ID, as we want CpPeerCall to have
                           // different Id than SipConnection
                           getNewCallId(&callId);
                           OsSysLog::add(FAC_CP, PRI_DEBUG, "Message callid: %s\n", callId.data());
                        }

                        /////////////////
                        UtlBoolean isUserValid = FALSE;
                        UtlString method;
                        sipMsg->getRequestMethod(&method);

                        if(mpLineMgrTask && mIsRequredUserIdMatch &&
                            method.compareTo(SIP_INVITE_METHOD,UtlString::ignoreCase) == 0)
                        {
                            isUserValid = mpLineMgrTask->isUserIdDefined(sipMsg);
                            if( !isUserValid)
                            {
                                //no such user - return 404
                                SipMessage noSuchUserResponse;
                                noSuchUserResponse.setResponseData(sipMsg,
                                    SIP_NOT_FOUND_CODE,
                                    SIP_NOT_FOUND_TEXT);
                                sipUserAgent->send(noSuchUserResponse);
                            }
                        }
                        else
                        {
                            isUserValid = TRUE;
                        }
                        ////////////////

                        if( isUserValid && CpPeerCall::shouldCreateCall(
                            *sipUserAgent, eventMessage, *mpCodecFactory))
                        {
                            // If this call would exceed the limit that we have been
                            // given for calls to handle simultaneously,
                            // send a BUSY_HERE SIP (486) message back to the sender.
                            if(getCallStackSize() >= mMaxCalls)
                            {
                                OsSysLog::add(FAC_CP, PRI_DEBUG, "CallManager::handleMessage - The call stack size as reached it's limit of %d", mMaxCalls);
                                if( (sipMsg->isResponse() == FALSE) &&
                                    (method.compareTo(SIP_ACK_METHOD,UtlString::ignoreCase) != 0) )

                                {
                                    SipMessage busyHereResponse;
                                    busyHereResponse.setInviteBusyData(sipMsg);
                                    sipUserAgent->send(busyHereResponse);
                                }
                            }
                            else
                            {
                                // Create a new SIP call
                                int numCodecs;
                                SdpCodec** codecArray = NULL;
                                getCodecs(numCodecs, codecArray);
                                OsSysLog::add(FAC_CP, PRI_DEBUG, "Creating new call for incoming SIP message\n");

                                UtlString publicAddress;
                                int publicPort;
                                //always use sipUserAgent public address, not the mPublicAddress of this call manager.
                                sipUserAgent->getViaInfo(OsSocket::UDP, publicAddress, publicPort, NULL, NULL);

                                UtlString localAddress;
                                int port;
                                UtlString adapterName;

                                localAddress = sipMsg->getLocalIp();

                                getContactAdapterName(adapterName, localAddress);

                                SIPX_CONTACT_ADDRESS contact;
                                sipUserAgent->getContactDb().getRecordForAdapter(contact, adapterName.data(), CONTACT_LOCAL);
                                port = contact.iPort;

                                pMediaInterface = mpMediaFactory->createMediaInterface(
									NULL,
                                    NULL, 
                                    localAddress, numCodecs, codecArray, 
                                    mLocale.data(), mExpeditedIpTos, mStunServer, 
                                    mStunPort, mStunKeepAlivePeriodSecs, mTurnServer,
                                    mTurnPort, mTurnUsername, mTurnPassword,
                                    mTurnKeepAlivePeriodSecs, isIceEnabled());


                                int inviteExpireSeconds;
                                if (sipMsg->getExpiresField(&inviteExpireSeconds) && inviteExpireSeconds > 0)
                                {
                                    if (inviteExpireSeconds > mInviteExpireSeconds)
                                        inviteExpireSeconds = mInviteExpireSeconds;
                                }
                                else
                                    inviteExpireSeconds = mInviteExpireSeconds;

                                handlingCall = new CpPeerCall(mIsEarlyMediaFor180,
                                    this,
                                    pMediaInterface,
                                    m_pCallEventListener,
                                    m_pInfoStatusEventListener,
                                    m_pSecurityEventListener,
                                    m_pMediaEventListener,
                                    aquireCallIndex(),
                                    callId.data(),
                                    sipUserAgent,
                                    mSipSessionReinviteTimer,
                                    mOutboundLine.data(),
                                    mOfferedTimeOut,
                                    mLineAvailableBehavior,
                                    mForwardUnconditional.data(),
                                    mLineBusyBehavior,
                                    mSipForwardOnBusy.data(),
                                    mNoAnswerTimeout,
                                    mForwardOnNoAnswer.data(),
                                    inviteExpireSeconds);
								// temporary
								pMediaInterface->setInterfaceNotificationQueue(handlingCall->getMessageQueue());

                                for (int i = 0; i < numCodecs; i++)
                                {
                                    delete codecArray[i];
                                }
                                delete[] codecArray;
                            }
                        }
                    }

                    // If we created a new call
                    if(handlingCall)
                    {
                        handlingCall->start();
                        // addToneListener(callId.data(), 0);

                        //if(infocusCall == NULL)
                        //{
                        //    infocusCall = handlingCall;
                        //    infocusCall->inFocus();
                        //}
                        //else
                        // {
                        // Push the new call on the stack
                        pushCall(handlingCall);
                        // }

                        //handlingCall->startMetaEvent( getNewMetaEventId(),
                        //                                        PtEvent::META_CALL_STARTING,
                        //                                        0,
                        //                                        0);
                    }
                }

                // Pass on the message if there is a call to process
                if(handlingCall)
                {
                    handlingCall->postMessage(eventMessage);
                    messageProcessed = TRUE;
                }
            }
            break;

        case CP_CALL_EXITED:
            {
                CpCall* call;
                ((CpIntMessage&)eventMessage).getIntData((int&) call);

                OsSysLog::add(FAC_CP, PRI_DEBUG, "Call EXITING message received: %p infofocus: %p\r\n", 
                        (void*)call, (void*) infocusCall);

                call->stopMetaEvent();

                mCallListMutex.acquireWrite() ;                                                
                releaseCallIndex(call->getCallIndex());
                if(infocusCall == call)
                {
                    // The infocus call is not in the mCallList -- no need to 
                    // remove, but we should tell the call that it is not 
                    // longer in focus.
                    call->outOfFocus();                    
                }
                else
                {
                    call = removeCall(call);
                }
                mCallListMutex.releaseWrite() ;

                if(call)
                {
                    delete call;                        
                }

                messageProcessed = TRUE;
                break;
            }

        case CP_DIAL_STRING:
            {
                OsWriteLock lock(mCallListMutex);
                if(infocusCall && dialing)
                {
                    //OsSysLog::add(FAC_CP, PRI_DEBUG, "CallManager::processMessage posting dial string to infocus call\n");
                    ((CpMultiStringMessage&)eventMessage).getString1Data(mDialString) ;
                    infocusCall->postMessage(eventMessage);
                }
                dialing = FALSE;
                messageProcessed = TRUE;
                break;
            }

        case CP_YIELD_FOCUS:
            {
                CpCall* call;
                ((CpIntMessage&)eventMessage).getIntData((int&) call);

                OsSysLog::add(FAC_CP, PRI_DEBUG, "Call YIELD FOCUS message received: %p\r\n", (void*)call);
                OsSysLog::add(FAC_CP, PRI_DEBUG, "infocusCall: %p\r\n", infocusCall);
                yieldFocus(call);
                messageConsumed = TRUE;
                messageProcessed = TRUE;
                break;
            }
        case CP_GET_FOCUS:
            {
                CpCall* call;
                ((CpIntMessage&)eventMessage).getIntData((int&) call);
                OsSysLog::add(FAC_CP, PRI_DEBUG, "Call GET FOCUS message received: %p\r\n", (void*)call);
                OsSysLog::add(FAC_CP, PRI_DEBUG, "infocusCall: %p\r\n", infocusCall);
                doGetFocus(call);
                messageConsumed = TRUE;
                messageProcessed = TRUE;
                break;
            }
        case CP_CREATE_CALL:
            {
                UtlString callId;
                int metaEventId = ((CpMultiStringMessage&)eventMessage).getInt1Data();
                int metaEventType = ((CpMultiStringMessage&)eventMessage).getInt2Data();
                int numCalls = ((CpMultiStringMessage&)eventMessage).getInt3Data();
                UtlBoolean assumeFocusIfNoInfocusCall = ((CpMultiStringMessage&)eventMessage).getInt4Data();
                const char* metaEventCallIds[4];
                UtlString metaCallId0;
                UtlString metaCallId1;
                UtlString metaCallId2;
                UtlString metaCallId3;

                ((CpMultiStringMessage&)eventMessage).getString1Data(callId);
                ((CpMultiStringMessage&)eventMessage).getString2Data(metaCallId0);
                ((CpMultiStringMessage&)eventMessage).getString3Data(metaCallId1);
                ((CpMultiStringMessage&)eventMessage).getString4Data(metaCallId2);
                ((CpMultiStringMessage&)eventMessage).getString5Data(metaCallId3);

                OsSysLog::add(FAC_CP, PRI_DEBUG, "CallManager:: create call %s\n", callId.data());

                metaEventCallIds[0] = metaCallId0.data();
                metaEventCallIds[1] = metaCallId1.data();
                metaEventCallIds[2] = metaCallId2.data();
                metaEventCallIds[3] = metaCallId3.data();
                doCreateCall(callId, metaEventId, metaEventType,
                    numCalls, metaEventCallIds, assumeFocusIfNoInfocusCall);

                messageProcessed = TRUE;
                break;
            }

        case CP_CONNECT:
            {
                UtlString callId;
                UtlString addressUrl;
                UtlString desiredConnectionCallId ;
                UtlString locationHeader;
                SIPX_CONTACT_ID contactId;
                ((CpMultiStringMessage&)eventMessage).getString1Data(callId);
                ((CpMultiStringMessage&)eventMessage).getString2Data(addressUrl);
                ((CpMultiStringMessage&)eventMessage).getString4Data(desiredConnectionCallId);
                ((CpMultiStringMessage&)eventMessage).getString5Data(locationHeader);
                contactId = (SIPX_CONTACT_ID) ((CpMultiStringMessage&)eventMessage).getInt1Data();
                void* pDisplay = (void*) ((CpMultiStringMessage&)eventMessage).getInt2Data();
                void* pSecurity = (void*) ((CpMultiStringMessage&)eventMessage).getInt3Data();
                int bandWidth = ((CpMultiStringMessage&)eventMessage).getInt4Data();
                SIPX_TRANSPORT_DATA* pTransport = (SIPX_TRANSPORT_DATA*)((CpMultiStringMessage&)eventMessage).getInt5Data();
                RtpTransportOptions rtpTransportFlags = (RtpTransportOptions)((CpMultiStringMessage&)eventMessage).getInt6Data();

                const char* locationHeaderData = (locationHeader.length() == 0) ? NULL : locationHeader.data();

                doConnect(callId.data(), addressUrl.data(), desiredConnectionCallId.data(), contactId, pDisplay, pSecurity, 
                          locationHeaderData, bandWidth, pTransport, rtpTransportFlags) ;
                messageProcessed = TRUE;
                break;
            }
        case CP_ENABLE_STUN:
            {
                UtlString stunServer ;
                int iRefreshPeriod ;
                int iStunPort ;
                OsNotification* pNotification ;


                CpMultiStringMessage& enableStunMessage = (CpMultiStringMessage&)eventMessage;
                enableStunMessage.getString1Data(stunServer) ;
                iStunPort = enableStunMessage.getInt1Data() ;
                iRefreshPeriod = enableStunMessage.getInt2Data() ;                
                pNotification = (OsNotification*) enableStunMessage.getInt3Data() ;

                doEnableStun(stunServer, iStunPort, iRefreshPeriod, pNotification) ;
                break ;
            }
        case CP_ENABLE_TURN:
            {
                UtlString turnServer ;
                UtlString turnUsername ;
                UtlString turnPassword ;
                int iTurnPort ;
                int iRefreshPeriod ;

                CpMultiStringMessage& enableStunMessage = (CpMultiStringMessage&)eventMessage;
                enableStunMessage.getString1Data(turnServer) ;
                enableStunMessage.getString2Data(turnUsername) ;
                enableStunMessage.getString3Data(turnPassword) ;
                iTurnPort = enableStunMessage.getInt1Data() ;
                iRefreshPeriod = enableStunMessage.getInt2Data() ;                

                doEnableTurn(turnServer, iTurnPort, turnUsername, turnPassword, iRefreshPeriod) ;
                break ;
            }
        case CP_ANSWER_CONNECTION:
        case CP_DROP:
        case CP_BLIND_TRANSFER:
        case CP_CONSULT_TRANSFER:
        case CP_CONSULT_TRANSFER_ADDRESS:
        case CP_TRANSFER_CONNECTION:
        case CP_TRANSFER_CONNECTION_STATUS:
        case CP_TRANSFEREE_CONNECTION:
        case CP_TRANSFEREE_CONNECTION_STATUS:
        case CP_HOLD_TERM_CONNECTION:
        case CP_HOLD_ALL_TERM_CONNECTIONS:
        case CP_UNHOLD_ALL_TERM_CONNECTIONS:
        case CP_UNHOLD_TERM_CONNECTION:
        case CP_RENEGOTIATE_CODECS_CONNECTION:
        case CP_SILENT_REMOTE_HOLD:
        case CP_RENEGOTIATE_CODECS_ALL_CONNECTIONS:
        case CP_UNHOLD_LOCAL_TERM_CONNECTION:
        case CP_HOLD_LOCAL_TERM_CONNECTION:
        case CP_START_TONE_CONNECTION:
        case CP_STOP_TONE_CONNECTION:
        case CP_PLAY_AUDIO_TERM_CONNECTION:        
        case CP_STOP_AUDIO_TERM_CONNECTION:
        case CP_PAUSE_AUDIO_PLAYBACK_CONNECTION:
        case CP_RESUME_AUDIO_PLAYBACK_CONNECTION:
        case CP_PLAY_AUDIO_CONNECTION:        
        case CP_STOP_AUDIO_CONNECTION:
        case CP_RECORD_AUDIO_CONNECTION_START:
        case CP_RECORD_AUDIO_CONNECTION_STOP:
        case CP_REFIRE_MEDIA_EVENT:
        case CP_PLAY_BUFFER_TERM_CONNECTION:
        case CP_GET_NUM_CONNECTIONS:
        case CP_GET_CONNECTIONS:
        case CP_GET_CALLED_ADDRESSES:
        case CP_GET_CALLING_ADDRESSES:
        case CP_IS_LOCAL_TERM_CONNECTION:
        case CP_ACCEPT_CONNECTION:
        case CP_REJECT_CONNECTION:
        case CP_REDIRECT_CONNECTION:
        case CP_DROP_CONNECTION:
        case CP_FORCE_DROP_CONNECTION:
        case CP_OFFERING_EXPIRED:
        case CP_RINGING_EXPIRED:
        case CP_GET_SESSION:
        case CP_CANCEL_TIMER:
        case CP_SET_OUTBOUND_LINE:
        case CP_GET_MEDIA_CONNECTION_ID:
        case CP_GET_MEDIA_ENERGY_LEVELS:
        case CP_GET_CALL_MEDIA_ENERGY_LEVELS:
        case CP_GET_CAN_ADD_PARTY:
        case CP_SPLIT_CONNECTION:
        case CP_JOIN_CONNECTION:
        case CP_TRANSFER_OTHER_PARTY_HOLD:
        case CP_TRANSFER_OTHER_PARTY_JOIN:
        case CP_TRANSFER_OTHER_PARTY_UNHOLD:
        case CP_LIMIT_CODEC_PREFERENCES:
        case CP_OUTGOING_INFO:
        case CP_GET_USERAGENT:
            // Forward the message to the call
            {
                UtlString callId;
                ((CpMultiStringMessage&)eventMessage).getString1Data(callId);
                OsReadLock lock(mCallListMutex);
                CpCall* call = findHandlingCall(callId);
                if(!call)
                {
                    // The call might have been terminated by asynchronous events.
                    // But output a debugging message, so the programmer can check
                    // to see that the CallId was valid in the past.
                    OsSysLog::add(FAC_CP, PRI_DEBUG, "Cannot find CallId: %s to post message: %d\n",
                        callId.data(), msgSubType);
                    if( msgSubType == CP_GET_NUM_CONNECTIONS ||
                        msgSubType == CP_GET_CONNECTIONS ||
                        msgSubType == CP_GET_CALLED_ADDRESSES ||
                        msgSubType == CP_GET_CALLING_ADDRESSES ||
                        msgSubType == CP_IS_LOCAL_TERM_CONNECTION ||
                        msgSubType == CP_GET_SESSION ||
                        msgSubType == CP_GET_MEDIA_CONNECTION_ID ||
                        msgSubType == CP_GET_MEDIA_ENERGY_LEVELS ||
                        msgSubType == CP_GET_CALL_MEDIA_ENERGY_LEVELS ||
                        msgSubType == CP_GET_CAN_ADD_PARTY ||
                        msgSubType == CP_RECORD_AUDIO_CONNECTION_START ||
                        msgSubType == CP_RECORD_AUDIO_CONNECTION_STOP ||
                        msgSubType == CP_OUTGOING_INFO ||
                        msgSubType == CP_GET_USERAGENT)
                    {
                        // Get the OsProtectedEvent and signal it to go away
                        OsProtectedEvent* eventWithoutCall = (OsProtectedEvent*)
                            ((CpMultiStringMessage&)eventMessage).getInt1Data();
                        if (eventWithoutCall)
                        {
                            OsSysLog::add(FAC_CP, PRI_ERR, "CallManager::handleMessage Received a message subtype %d request on invalid callId '%s'; signaled event in message\n",
                                msgSubType, callId.data());

                            // Test if already signaled here and
                            // releasing the event if it is.
                            if(OS_ALREADY_SIGNALED == eventWithoutCall->signal(0))
                            {
                                OsProtectEventMgr* eventMgr =
                                    OsProtectEventMgr::getEventMgr();
                                eventMgr->release(eventWithoutCall);
                                eventWithoutCall = NULL;
                            }
                        }
                        else
                        {
                            OsSysLog::add(FAC_CP, PRI_ERR, "CallManager::handleMessage Received a message subtype %d request on invalid callId '%s'; no event to signal\n",
                                msgSubType, callId.data());
                        }
                    }
                }
                else
                {
                    call->postMessage(eventMessage);
                }
                messageProcessed = TRUE;
                break;
            }

        default:
            {
                OsSysLog::add(FAC_CP, PRI_ERR, "Unknown PHONE_APP CallManager message subtype: %d\n", msgSubType);
                messageProcessed = TRUE;
                break;
            }
        }
        break;

        // Timer event expired
    case OsMsg::OS_EVENT:
        // Pull out the OsMsg from the user data and post it
        if(msgSubType == OsEventMsg::NOTIFY)
        {
            OsMsg* timerMsg;
            OsTimer* timer;

            ((OsEventMsg&)eventMessage).getUserData((int&)timerMsg);
            ((OsEventMsg&)eventMessage).getEventData((int&)timer);

            if(timer)
            {
#ifdef TEST_PRINT
                int eventMessageType = timerMsg->getMsgType();
                int eventMessageSubType = timerMsg->getMsgSubType();
                OsSysLog::add(FAC_CP, PRI_DEBUG, "CallManager::handleMessage deleting timer for message type: %d %d\n",
                    eventMessageType, eventMessageSubType);
#endif
                //timer->stop();
                // timer gets deleted where it was started, this enables
                // us to delete timers safely from other class, where
                // we keep pointer to it
                timer = NULL;
            }
            if(timerMsg)
            {
                postMessage(*timerMsg);
                delete timerMsg;
                timerMsg = NULL;
            }
        }
        messageProcessed = TRUE;
        break;
    default:
        {
            OsSysLog::add(FAC_CP, PRI_ERR, "Unknown TYPE %d of CallManager message subtype: %d\n", msgType, msgSubType);
            messageProcessed = TRUE;
            break;
        }
    }

    return(messageProcessed);
}


void CallManager::requestShutdown()
{
    // Need to put a Mutex on the call stack


    UtlSListIterator iterator(callStack);
    CpCall* call = NULL;
    UtlInt* callCollectable;

    while(! callStack.isEmpty() && ! iterator.atLast())
    {
        callCollectable = (UtlInt*) iterator();
        if(callCollectable)
        {
            call = (CpCall*) callCollectable->getValue();
            call->requestShutdown();
        }
    }

    {
        OsReadLock lock(mCallListMutex);
        if(infocusCall)
        {
            infocusCall->requestShutdown();
        }
    }

    // Pass the shut down to itself
    OsServerTask::requestShutdown();
    yield();

}

void CallManager::createCall(UtlString* callId,
                             int metaEventId,
                             int metaEventType,
                             int numCalls,
                             const char* callIds[],
                             UtlBoolean assumeFocusIfNoInfocusCall)
{
    if(callId->isNull())
    {
        getNewCallId(callId);
    }
    OsSysLog::add(FAC_CP, PRI_DEBUG, "CallManager::createCall new Id: %s\n", callId->data());
    CpMultiStringMessage callMessage(CP_CREATE_CALL,
        callId->data(),
        numCalls >= 1 ? callIds[0] : NULL,
        numCalls >= 2 ? callIds[1] : NULL,
        numCalls >= 3 ? callIds[2] : NULL,
        numCalls >= 4 ? callIds[3] : NULL,
        metaEventId,
        metaEventType,
        numCalls,
        assumeFocusIfNoInfocusCall);
    postMessage(callMessage);
    mnTotalOutgoingCalls++;

}


OsStatus CallManager::getCalls(UtlSList& callIdList)
{
   OsStatus returnCode = OS_SUCCESS;
   CpCall* call = NULL;
   UtlString callId;
   UtlInt* callCollectable;

   OsReadLock lock(mCallListMutex);
   // Get the callId for the infocus call
   if (infocusCall)
   {
      // get IDs of call
      if (infocusCall->getConnectionCallIds(callIdList) != OS_SUCCESS)
      {
         // call doesn't have any sessionCallIds, append callId of CpPeerCall
         infocusCall->getCallId(callId);
         callIdList.append(new UtlString(callId));
      }
   }

   // Get the callId for the calls in the stack
   UtlSListIterator iterator(callStack);
   while(callCollectable = (UtlInt*)iterator())
   {
      call = (CpCall*)callCollectable->getValue();
      if(call)
      {
         // appends session call ids to the list
         if (call->getConnectionCallIds(callIdList) != OS_SUCCESS)
         {
            // call doesn't have any sessionCallIds, append callId of CpPeerCall
            call->getCallId(callId);
            callIdList.append(new UtlString(callId));
         }
      }
   }

#ifdef TEST_PRINT
    OsSysLog::add(FAC_CP, PRI_DEBUG, "CallManager:getCalls numCalls = %d\n ", numCalls) ;
#endif
    return returnCode;
}

void CallManager::getRemoteUserAgent(const char* callId, const char* remoteAddress,
                                     UtlString& userAgent)
{
    OsProtectEventMgr* eventMgr = OsProtectEventMgr::getEventMgr();
    OsProtectedEvent* agentSet = eventMgr->alloc();
    OsTime maxEventTime(CP_MAX_EVENT_WAIT_SECONDS, 0);
    UtlString* pUserAgent = new UtlString;
    agentSet->setIntData((int) pUserAgent);
    CpMultiStringMessage getAgentMessage(CP_GET_USERAGENT, callId, remoteAddress, NULL,
        NULL, NULL, (int)agentSet);
    postMessage(getAgentMessage);

    // Wait until the call manager sets the callIDs
    if(agentSet->wait(0, maxEventTime) == OS_SUCCESS)
    {
        userAgent = *pUserAgent;
        delete pUserAgent;
        eventMgr->release(agentSet);
    }
    else
    {
        OsSysLog::add(FAC_CP, PRI_ERR, "CallManager::getRemoteUserAgent TIMED OUT\n");

        // If the event has already been signalled, clean up
        if(OS_ALREADY_SIGNALED == agentSet->signal(0))
        {
            delete pUserAgent;
            eventMgr->release(agentSet);
        }
    }
}

PtStatus CallManager::connect(const char* callId,
                              const char* toAddressString,
                              const char* fromAddressString,
                              const char* desiredCallIdString,
                              SIPX_CONTACT_ID contactId,
                              const void* pDisplay,
                              const void* pSecurity,
                              const char* locationHeader,
                              const int bandWidth,
                              SIPX_TRANSPORT_DATA* pTransportData,
                              const RTP_TRANSPORT rtpTransportOptions)
{
    UtlString toAddressUrl(toAddressString ? toAddressString : "");
    UtlString fromAddressUrl(fromAddressString ? fromAddressString : "");
    UtlString desiredCallId(desiredCallIdString ? desiredCallIdString : "") ;
    
    // create a copy of the transport data
    SIPX_TRANSPORT_DATA* pTransportDataCopy = NULL;
    if (pTransportData)
    {
        pTransportDataCopy = new SIPX_TRANSPORT_DATA;
        memcpy(pTransportDataCopy, pTransportData, sizeof(SIPX_TRANSPORT_DATA));
    }

    PtStatus returnCode = validateAddress(toAddressUrl);
    if(returnCode == PT_SUCCESS)
    {
        CpMultiStringMessage callMessage(CP_CONNECT, callId,
            toAddressUrl, fromAddressUrl, desiredCallId, locationHeader,
            contactId, (int)pDisplay, (int)pSecurity, 
            (int)bandWidth, (int)pTransportDataCopy, rtpTransportOptions);
        postMessage(callMessage);
    }
    else
    {
       if (m_pCallEventListener)
       {
          m_pCallEventListener->OnDisconnected(CpCallStateEvent(NULL, callId, SipSession(), toAddressString, CALLSTATE_CAUSE_BAD_ADDRESS));
       }
    }
    return(returnCode);
}

void CallManager::drop(const char* callId)
{
    CpMultiStringMessage callMessage(CP_DROP, callId);
    postMessage(callMessage);
}

UtlBoolean CallManager::sendInfo(const char*  callId, 
                                 const char*  szRemoteAddress,
                                 const char*  szContentType,
                                 const size_t nContentLength,
                                 const char*  szContent)
{
    UtlBoolean bRC = false ;
    OsProtectEventMgr* eventMgr = OsProtectEventMgr::getEventMgr();
    OsProtectedEvent* pSuccessEvent = eventMgr->alloc();
    OsTime maxEventTime(CP_MAX_EVENT_WAIT_SECONDS, 0);


    CpMultiStringMessage infoMessage(CP_OUTGOING_INFO, 
            callId, 
            szRemoteAddress,
            szContentType, 
            UtlString(szContent, nContentLength),
            NULL, 
            (int) pSuccessEvent) ;
    postMessage(infoMessage);

    if (pSuccessEvent->wait(0, maxEventTime) == OS_SUCCESS)
    {
        int success ;
        pSuccessEvent->getEventData(success);
        bRC = success ;
        eventMgr->release(pSuccessEvent);
    }
    else
    {
        if(OS_ALREADY_SIGNALED == pSuccessEvent->signal(0))
        {
            eventMgr->release(pSuccessEvent);
        }
    }

    return bRC ;
}

// Transfer an individual participant from one end point to another using 
// REFER w/replaces.
PtStatus CallManager::transfer(const char* sourceCallId, 
                               const char* sourceAddress, 
                               const char* targetCallId,
                               const char* targetAddress) 
{
    PtStatus returnCode =  PT_SUCCESS;


    // Place connections on hold
    CpMultiStringMessage sourceHold(CP_HOLD_TERM_CONNECTION, sourceCallId, sourceAddress);
    postMessage(sourceHold);
    CpMultiStringMessage targetHold(CP_HOLD_TERM_CONNECTION, targetCallId, targetAddress);
    postMessage(targetHold);
    
    // Construct the replaces header info
    // SIP alert: this is SIP specific and should not be in CallManager
    UtlString fromAddress;
    getFromField(targetCallId, targetAddress, fromAddress) ;

    // Add the Replaces header info to the consultative URL
    UtlString replacesField;
    SipMessage::buildReplacesField(replacesField, targetCallId, fromAddress, 
            targetAddress);

    Url transferTargetUrl(targetAddress);
    transferTargetUrl.removeFieldParameters() ;
    transferTargetUrl.setHeaderParameter(SIP_REPLACES_FIELD, replacesField.data());
    UtlString transferTargetUrlString;
    transferTargetUrl.toString(transferTargetUrlString);

    // Tell the original call to complete the consultative transfer
    CpMultiStringMessage consultTransfer(CP_CONSULT_TRANSFER_ADDRESS,
    sourceCallId, sourceAddress, targetCallId, targetAddress, transferTargetUrlString);

    postMessage(consultTransfer);

    return returnCode ;
}




// Split szSourceAddress from szSourceCallId and join it to the specified 
// target call id.  The source call/connection MUST be on hold prior
// to initiating the split/join.
PtStatus CallManager::split(const char* szSourceCallId,
                            const char* szSourceAddress,
                            const char* szTargetCallId) 
{
    PtStatus status = PT_FAILED ;

    OsProtectEventMgr* eventMgr = OsProtectEventMgr::getEventMgr();
    OsProtectedEvent* splitSuccess = eventMgr->alloc();
    OsTime maxEventTime(CP_MAX_EVENT_WAIT_SECONDS, 0);

    CpMultiStringMessage splitMessage(CP_SPLIT_CONNECTION, 
            szSourceCallId, 
            szSourceAddress,
            szTargetCallId, 
            NULL, 
            NULL,
            (int) splitSuccess);
    postMessage(splitMessage);

    // Wait until the call sets the number of connections
    if(splitSuccess->wait(0, maxEventTime) == OS_SUCCESS)
    {
        int success ;
        splitSuccess->getEventData(success);
        eventMgr->release(splitSuccess);

        if (success)
        {
            status = PT_SUCCESS  ;
        } 
    }
    else
    {
        OsSysLog::add(FAC_CP, PRI_ERR, "CallManager::split TIMED OUT\n");
        // If the event has already been signalled, clean up
        if(OS_ALREADY_SIGNALED == splitSuccess->signal(0))
        {
            eventMgr->release(splitSuccess);
        }
    }

    return status ;
}


// Blind transfer?
PtStatus CallManager::transfer_blind(const char* callId, const char* transferToUrl,
                               UtlString* targetConnectionCallId,
                               UtlString* targetConnectionAddress)
{
    UtlString transferTargetUrl(transferToUrl ? transferToUrl : "");

    PtStatus returnCode = validateAddress(transferTargetUrl);

    if(returnCode == PT_SUCCESS)
    {
        if(targetConnectionAddress)
            *targetConnectionAddress = transferToUrl;
        UtlString targetCallId;
        getNewCallId(&targetCallId);
        if(targetConnectionCallId)
            *targetConnectionCallId = targetCallId;

        // CP_BLIND_TRANSFER (i.e. two call blind transfer)
        CpMultiStringMessage transferMessage(CP_BLIND_TRANSFER,
            callId, transferTargetUrl, targetCallId.data(), NULL, NULL,
            getNewMetaEventId());

        postMessage(transferMessage);
    }
    return(returnCode);
}

void CallManager::audioPlay(const char* callId, const char* audioUrl, UtlBoolean repeat, UtlBoolean local, UtlBoolean remote, UtlBoolean mixWithMic, int downScaling)
{
    CpMultiStringMessage startToneMessage(CP_PLAY_AUDIO_TERM_CONNECTION,
        callId, audioUrl, NULL, NULL, NULL,
        repeat, local, remote, mixWithMic, downScaling);

    postMessage(startToneMessage);
}

void CallManager::audioStop(const char* callId)
{
    CpMultiStringMessage stopAudioMessage(CP_STOP_AUDIO_TERM_CONNECTION, callId);
    postMessage(stopAudioMessage);
}

void CallManager::pauseAudioPlayback(const UtlString& callId, const UtlString& szRemoteAddress)
{
   CpMultiStringMessage stopAudioMessage(CP_PAUSE_AUDIO_PLAYBACK_CONNECTION, callId, szRemoteAddress);
   postMessage(stopAudioMessage);
}

void CallManager::resumeAudioPlayback(const UtlString& callId, const UtlString& szRemoteAddress)
{
   CpMultiStringMessage stopAudioMessage(CP_RESUME_AUDIO_PLAYBACK_CONNECTION, callId, szRemoteAddress);
   postMessage(stopAudioMessage);
}

void CallManager::toneChannelStart(const char* callId, const char* szRemoteAddress, int toneId, UtlBoolean local, UtlBoolean remote)
{
    CpMultiStringMessage startToneMessage(CP_START_TONE_CONNECTION,
        callId, szRemoteAddress, NULL, NULL, NULL,
        toneId, local, remote);

    postMessage(startToneMessage);
}

void CallManager::toneChannelStop(const char* callId, const char* szRemoteAddress)
{
    CpMultiStringMessage stopToneMessage(CP_STOP_TONE_CONNECTION, 
        callId, szRemoteAddress);
    postMessage(stopToneMessage);
}

void CallManager::audioChannelPlay(const char* callId, const char* szRemoteAddress, const char* audioUrl, UtlBoolean repeat, UtlBoolean local, UtlBoolean remote, UtlBoolean mixWithMic, int downScaling, void* pCookie)
{
    CpMultiStringMessage startPlayMessage(CP_PLAY_AUDIO_CONNECTION,
        callId, szRemoteAddress, audioUrl, NULL, NULL,
        repeat, local, remote, mixWithMic, downScaling, (intptr_t)pCookie);

    postMessage(startPlayMessage);
}

void CallManager::audioChannelStop(const char* callId, const char* szRemoteAddress)
{
    CpMultiStringMessage stopAudioMessage(CP_STOP_AUDIO_CONNECTION, 
        callId, szRemoteAddress);
    postMessage(stopAudioMessage);
}


OsStatus CallManager::audioChannelRecordStart(const char* callId, const char* szRemoteAddress, const char* szFile) 
{
    OsStatus status = OS_FAILED ;

    OsProtectEventMgr* eventMgr = OsProtectEventMgr::getEventMgr();
    OsProtectedEvent* pEvent = eventMgr->alloc();
    OsTime maxEventTime(CP_MAX_EVENT_WAIT_SECONDS, 0);

    CpMultiStringMessage message(CP_RECORD_AUDIO_CONNECTION_START, 
            callId, 
            szRemoteAddress,
            szFile, 
            NULL, 
            NULL,
            (int) pEvent);
    postMessage(message);

    // Wait for error response
    if(pEvent->wait(0, maxEventTime) == OS_SUCCESS)
    {
        int success ;
        pEvent->getEventData(success);
        eventMgr->release(pEvent);

        if (success)
        {
            status = OS_SUCCESS  ;
        } 
    }
    else
    {
        OsSysLog::add(FAC_CP, PRI_ERR, "CallManager::audioChannelRecordStart TIMED OUT\n");
        // If the event has already been signalled, clean up
        if(OS_ALREADY_SIGNALED == pEvent->signal(0))
        {
            eventMgr->release(pEvent);
        }
    }

    return status ;
}


OsStatus CallManager::audioChannelRecordStop(const char* callId, const char* szRemoteAddress) 
{
    OsStatus status = OS_FAILED ;

    OsProtectEventMgr* eventMgr = OsProtectEventMgr::getEventMgr();
    OsProtectedEvent* pEvent = eventMgr->alloc();
    OsTime maxEventTime(CP_MAX_EVENT_WAIT_SECONDS, 0);

    CpMultiStringMessage message(CP_RECORD_AUDIO_CONNECTION_STOP, 
            callId, 
            szRemoteAddress,
            NULL,
            NULL, 
            NULL,
            (int) pEvent);

    postMessage(message);

    // Wait for error response
    if(pEvent->wait(0, maxEventTime) == OS_SUCCESS)
    {
        int success ;
        pEvent->getEventData(success);
        eventMgr->release(pEvent);

        if (success)
        {
            status = OS_SUCCESS  ;
        } 
    }
    else
    {
        OsSysLog::add(FAC_CP, PRI_ERR, "CallManager::audioChannelRecordStop TIMED OUT\n");
        // If the event has already been signalled, clean up
        if(OS_ALREADY_SIGNALED == pEvent->signal(0))
        {
            eventMgr->release(pEvent);
        }
    }

    return status ;
}


void CallManager::bufferPlay(const char* callId, int audioBuf, int bufSize, int type, UtlBoolean repeat, UtlBoolean local, UtlBoolean remote, void* pCookie)
{
    CpMultiStringMessage startToneMessage(CP_PLAY_BUFFER_TERM_CONNECTION,
       callId, NULL, NULL, NULL, NULL,
       NULL, repeat, local, remote, audioBuf, bufSize, type, (intptr_t)pCookie);

    postMessage(startToneMessage);
}


void CallManager::setOutboundLineForCall(const char* callId, const char* address, SIPX_CONTACT_TYPE eType)
{
    CpMultiStringMessage outboundLineMessage(CP_SET_OUTBOUND_LINE, callId, 
            address, NULL, NULL, NULL, (int) eType);

    postMessage(outboundLineMessage);
}

void CallManager::acceptConnection(const char* callId,
                                   const char* address, 
                                   SIPX_CONTACT_ID contactId,
                                   const void* hWnd,
                                   const void* security,
                                   const char* locationHeader,
                                   const int bandWidth,
                                   UtlBoolean sendEarlyMedia)
{
    CpMultiStringMessage acceptMessage(CP_ACCEPT_CONNECTION, callId, address, NULL, NULL, locationHeader, (int) contactId, 
                                       (int) hWnd, (int) security, (int)bandWidth, sendEarlyMedia);
    postMessage(acceptMessage);
}

void CallManager::rejectConnection(const char* callId, const char* address)
{
    CpMultiStringMessage acceptMessage(CP_REJECT_CONNECTION, callId, address);
    postMessage(acceptMessage);
}

PtStatus CallManager::redirectConnection(const char* callId, const char* address,
                                         const char* forwardAddress)
{
    UtlString forwardAddressUrl(forwardAddress ? forwardAddress : "");
    PtStatus returnCode = validateAddress(forwardAddressUrl);

    if(returnCode == PT_SUCCESS)
    {
        CpMultiStringMessage acceptMessage(CP_REDIRECT_CONNECTION, callId, address,
            forwardAddressUrl.data());
        postMessage(acceptMessage);
    }
    return(returnCode);
}

void CallManager::dropConnection(const char* callId, const char* address)
{
    CpMultiStringMessage acceptMessage(CP_DROP_CONNECTION, callId, address);
    postMessage(acceptMessage);
}

OsStatus CallManager::getConnections(const char* callId, int maxConnections,
                                     int& numConnections, UtlString addresses[])
{
    OsProtectEventMgr* eventMgr = OsProtectEventMgr::getEventMgr();
    UtlSList* addressList = new UtlSList;
    OsProtectedEvent* numConnectionsSet = eventMgr->alloc();
    numConnectionsSet->setIntData((int) addressList);
    OsTime maxEventTime(CP_MAX_EVENT_WAIT_SECONDS, 0);
    OsStatus returnCode = OS_WAIT_TIMEOUT;
    CpMultiStringMessage getNumMessage(CP_GET_CONNECTIONS, callId, NULL, NULL,
        NULL, NULL,
        (int)numConnectionsSet);
    postMessage(getNumMessage);

    // Wait until the call sets the number of connections
    if(numConnectionsSet->wait(0, maxEventTime) == OS_SUCCESS)
    {
        {
            int addressIndex = 0;
            UtlSListIterator iterator(*addressList);
            UtlString* addressCollectable;
            addressCollectable = (UtlString*)iterator();
            returnCode = OS_SUCCESS;

            while (addressCollectable)
            {
                if(addressIndex >= maxConnections)
                {
                    returnCode = OS_LIMIT_REACHED;
                    break;
                }
                addresses[addressIndex] = *addressCollectable;
                addressIndex++;
                addressCollectable = (UtlString*)iterator();
            }
            numConnections = addressIndex;
        }

#ifdef TEST_PRINT_EVENT
        OsSysLog::add(FAC_CP, PRI_DEBUG, "CallManager::getConnections %d connections\n",
            numConnections);
#endif

        addressList->destroyAll();
        delete addressList;
        eventMgr->release(numConnectionsSet);
    }
    else
    {
        OsSysLog::add(FAC_CP, PRI_ERR, "CallManager::getConnections TIMED OUT\n");
        // If the event has already been signalled, clean up
        if(OS_ALREADY_SIGNALED == numConnectionsSet->signal(0))
        {
            addressList->destroyAll();
            delete addressList;
            eventMgr->release(numConnectionsSet);
        }
        numConnections = 0;

    }

    return(returnCode);
}

OsStatus CallManager::getCalledAddresses(const char* callId, int maxConnections,
                                         int& numConnections, UtlString addresses[])
{
    OsProtectEventMgr* eventMgr = OsProtectEventMgr::getEventMgr();
    UtlSList* addressList = new UtlSList;
    OsProtectedEvent* numConnectionsSet = eventMgr->alloc();
    numConnectionsSet->setIntData((int) addressList);
    OsTime maxEventTime(CP_MAX_EVENT_WAIT_SECONDS, 0);
    OsStatus returnCode = OS_WAIT_TIMEOUT;
    CpMultiStringMessage getNumMessage(CP_GET_CALLED_ADDRESSES, callId, NULL,
        NULL, NULL, NULL,
        (int)numConnectionsSet);
    postMessage(getNumMessage);

    // Wait until the call sets the number of connections
    if(numConnectionsSet->wait(0, maxEventTime) == OS_SUCCESS)
    {
        int addressIndex = 0;
        {  // set the iterator scope
            UtlSListIterator iterator(*addressList);
            UtlString* addressCollectable;
            addressCollectable = (UtlString*)iterator();
            returnCode = OS_SUCCESS;

            while (addressCollectable)
            {
                if(addressIndex >= maxConnections)
                {
                    returnCode = OS_LIMIT_REACHED;
                    break;
                }
                addresses[addressIndex] = *addressCollectable;
                addressIndex++;
                addressCollectable = (UtlString*)iterator();
            }
            numConnections = addressIndex;
        } // end of interator scope
#ifdef TEST_PRINT_EVENT
        OsSysLog::add(FAC_CP, PRI_DEBUG, "CallManager::getCalledAddresses %d addresses\n",
            numConnections);
#endif

        addressList->destroyAll();
        delete addressList;
        eventMgr->release(numConnectionsSet);
    }
    else
    {
        OsSysLog::add(FAC_CP, PRI_ERR, "CallManager::getCalledAddresses TIMED OUT\n");
        // If the event has already been signalled, clean up
        if(OS_ALREADY_SIGNALED == numConnectionsSet->signal(0))
        {
            addressList->destroyAll();
            delete addressList;
            eventMgr->release(numConnectionsSet);
        }
        numConnections = 0;

    }

    return(returnCode);
}

OsStatus CallManager::getCallingAddresses(const char* callId, int maxConnections,
                                          int& numConnections, UtlString addresses[])
{
    OsProtectEventMgr* eventMgr = OsProtectEventMgr::getEventMgr();
    UtlSList* addressList = new UtlSList;
    OsProtectedEvent* numConnectionsSet = eventMgr->alloc();
    numConnectionsSet->setIntData((int) addressList);
    OsTime maxEventTime(CP_MAX_EVENT_WAIT_SECONDS, 0);
    OsStatus returnCode = OS_WAIT_TIMEOUT;
    CpMultiStringMessage getNumMessage(CP_GET_CALLING_ADDRESSES, callId, NULL,
        NULL, NULL, NULL,
        (int)numConnectionsSet);
    postMessage(getNumMessage);

    // Wait until the call sets the number of connections
    if(numConnectionsSet->wait(0, maxEventTime) == OS_SUCCESS)
    {
        int addressIndex = 0;
        UtlSListIterator iterator(*addressList);
        UtlString* addressCollectable;
        addressCollectable = (UtlString*)iterator();
        returnCode = OS_SUCCESS;

        while (addressCollectable)
        {
            if(addressIndex >= maxConnections)
            {
                returnCode = OS_LIMIT_REACHED;
                break;
            }
            addresses[addressIndex] = *addressCollectable;
            addressIndex++;
            addressCollectable = (UtlString*)iterator();
        }
        numConnections = addressIndex;

#ifdef TEST_PRINT_EVENT
        OsSysLog::add(FAC_CP, PRI_DEBUG, "CallManager::getCallingAddresses %d addresses\n",
            numConnections);
#endif

        addressList->destroyAll();
        delete addressList;
        eventMgr->release(numConnectionsSet);
    }
    else
    {
        OsSysLog::add(FAC_CP, PRI_ERR, "CallManager::getCalledAddresses TIMED OUT");
        // If the event has already been signalled, clean up
        if(OS_ALREADY_SIGNALED == numConnectionsSet->signal(0))
        {
            addressList->destroyAll();
            delete addressList;
            eventMgr->release(numConnectionsSet);
        }
        numConnections = 0;

    }

    return(returnCode);
}

OsStatus CallManager::getFromField(const char* callId,
                                   const char* address,
                                   UtlString& fromField)
{
    SipSession session;
    OsStatus status = getSession(callId, address, session);

    if(status == OS_SUCCESS)
    {
        Url fromUrl;
        session.getFromUrl(fromUrl);
        fromUrl.toString(fromField);

#ifdef TEST_PRINT_EVENT
        OsSysLog::add(FAC_CP, PRI_DEBUG, "CallManager::getFromField %s\n", fromField.data());
#endif
    }

    else
    {
        fromField.remove(0);
    }

    return(status);
}

void CallManager::limitCodecPreferences(const char* callId,
                                        const char* remoteAddr,
                                        const int audioBandwidth,
                                        const int videoBandwidth,
                                        const char* szVideoCodecName)
{
    CpMultiStringMessage message(CP_LIMIT_CODEC_PREFERENCES, 
            callId, 
            remoteAddr,
            szVideoCodecName,
            NULL, 
            NULL,
            (int) audioBandwidth,
            (int) videoBandwidth);
    postMessage(message);
}

void CallManager::limitCodecPreferences(const char* callId,
                                        const int audioBandwidth,
                                        const int videoBandwidth,
                                        const char* szVideoCodecName)
{
    CpMultiStringMessage message(CP_LIMIT_CODEC_PREFERENCES, 
            callId,
            NULL,
            szVideoCodecName,
            NULL, 
            NULL,
            (int) audioBandwidth,
            (int) videoBandwidth);
    postMessage(message);
}

OsStatus CallManager::getSession(const char* callId,
                                 const char* address,
                                 SipSession& session)
{
   OsSysLog::add(FAC_CP, PRI_DEBUG, "CallManager::getSession callId = '%s', address = '%s'",
                 callId, address);
    SipSession* sessionPtr = new SipSession;
#ifdef TEST_PRINT
    OsSysLog::add(FAC_CP, PRI_DEBUG, "CallManager::getSession allocated session: 0x%x",
        sessionPtr);
#endif
    OsProtectEventMgr* eventMgr = OsProtectEventMgr::getEventMgr();
    OsProtectedEvent* getSessionEvent = eventMgr->alloc();
    getSessionEvent->setIntData((int) sessionPtr);
    OsTime maxEventTime(CP_MAX_EVENT_WAIT_SECONDS, 0);
    OsStatus returnCode = OS_WAIT_TIMEOUT;
    CpMultiStringMessage getFieldMessage(CP_GET_SESSION, callId, address,
        NULL, NULL, NULL,
        (int)getSessionEvent);
    postMessage(getFieldMessage);

    // Wait until the call sets the number of connections
    if(getSessionEvent->wait(0, maxEventTime) == OS_SUCCESS)
    {
        returnCode = OS_SUCCESS;

        session = *sessionPtr;

        OsSysLog::add(FAC_CP, PRI_DEBUG, "CallManager::getSession deleting session: %p",
            sessionPtr);

        delete sessionPtr;
        sessionPtr = NULL;
        eventMgr->release(getSessionEvent);
    }
    else
    {
        OsSysLog::add(FAC_CP, PRI_ERR, "CallManager::getSession TIMED OUT\n");
        // If the event has already been signalled, clean up
        if(OS_ALREADY_SIGNALED == getSessionEvent->signal(0))
        {
#ifdef TEST_PRINT
            OsSysLog::add(FAC_CP, PRI_DEBUG, "CallManager::getSession deleting timed out session: 0x%x",
                sessionPtr);
#endif
            delete sessionPtr;
            sessionPtr = NULL;

            eventMgr->release(getSessionEvent);
        }
    }
    return(returnCode);
}


OsStatus CallManager::getSipDialog(const char* callId,
                                   const char* address,
                                   SipDialog& dialog)
{
    OsStatus returnCode = OS_SUCCESS;

    // For now, this is only the warp for SipSession and we need to
    // re-implement it when SipSession is deprecated.
    SipSession ssn;

    returnCode = getSession(callId, address, ssn);
    // Copy all the contents into the SipDialog
    if (returnCode == OS_SUCCESS)
    {
       UtlString call;
       ssn.getCallId(call);
       dialog.setCallId(call);
       
       Url url;
       ssn.getFromUrl(url);
       dialog.setLocalField(url);

       ssn.getToUrl(url);
       dialog.setRemoteField(url);

       ssn.getLocalContact(url);
       dialog.setLocalContact(url);

       ssn.getRemoteContact(url);
       dialog.setRemoteContact(url);

       UtlString uValue;
       ssn.getInitialMethod(uValue);
       dialog.setInitialMethod(uValue);

       ssn.getLocalRequestUri(uValue);
       dialog.setLocalRequestUri(uValue);

       ssn.getRemoteRequestUri(uValue);
       dialog.setRemoteRequestUri(uValue);

       dialog.setLastLocalCseq(ssn.getLastFromCseq());
       dialog.setLastRemoteCseq(ssn.getLastToCseq());
    }
    
    return(returnCode);
}

void CallManager::answerTerminalConnection(const char* callId, const char* address, const char* terminalId,
                                           const void* pDisplay, const void* pSecurity)
{
    SIPX_VIDEO_DISPLAY* pDisplayCopy = NULL;
    SIPX_SECURITY_ATTRIBUTES* pSecurityCopy = NULL;
    
    if (pDisplay)
    {
        pDisplayCopy = new SIPX_VIDEO_DISPLAY(*(SIPX_VIDEO_DISPLAY*)pDisplay);
    }
    if (pSecurity)
    {
        pSecurityCopy = new SIPX_SECURITY_ATTRIBUTES(*(SIPX_SECURITY_ATTRIBUTES*)pSecurity);
    }
    
    CpMultiStringMessage callConnectionMessage(CP_ANSWER_CONNECTION, callId, address, NULL, NULL, NULL, (int)pDisplayCopy, (int)pSecurityCopy);
    postMessage(callConnectionMessage);
    mnTotalIncomingCalls++;

}

void CallManager::holdTerminalConnection(const char* callId, const char* address, const char* terminalId)
{
    CpMultiStringMessage holdMessage(CP_HOLD_TERM_CONNECTION, callId, address, terminalId);
    postMessage(holdMessage);
}

void CallManager::holdAllTerminalConnections(const char* callId)
{
    CpMultiStringMessage holdMessage(CP_HOLD_ALL_TERM_CONNECTIONS, callId);
    postMessage(holdMessage);
}

void CallManager::holdLocalTerminalConnection(const char* callId)
{
    CpMultiStringMessage holdMessage(CP_HOLD_LOCAL_TERM_CONNECTION, callId);
    postMessage(holdMessage);
}

void CallManager::unholdLocalTerminalConnection(const char* callId)
{    
    CpMultiStringMessage holdMessage(CP_UNHOLD_LOCAL_TERM_CONNECTION, callId);
    postMessage(holdMessage);
}

void CallManager::unholdAllTerminalConnections(const char* callId)
{
    // Unhold all of the remote connections
    CpMultiStringMessage unholdMessage(CP_UNHOLD_ALL_TERM_CONNECTIONS, callId);
    postMessage(unholdMessage);

    unholdLocalTerminalConnection(callId) ;
}

void CallManager::unholdTerminalConnection(const char* callId, const char* address, const char* terminalId)
{
    CpMultiStringMessage unholdMessage(CP_UNHOLD_TERM_CONNECTION, callId, address, terminalId);
    postMessage(unholdMessage);
}

void CallManager::renegotiateCodecsTerminalConnection(const char* callId, const char* address, const char* terminalId)
{
    CpMultiStringMessage unholdMessage(CP_RENEGOTIATE_CODECS_CONNECTION, callId, address, terminalId);
    postMessage(unholdMessage);
}

void CallManager::silentRemoteHold(const char* callId)
{
    CpMultiStringMessage silentRemoteHoldMessage(CP_SILENT_REMOTE_HOLD, callId);
    postMessage(silentRemoteHoldMessage);
}

void CallManager::renegotiateCodecsAllTerminalConnections(const char* callId)
{
    CpMultiStringMessage renegotiateMessage(CP_RENEGOTIATE_CODECS_ALL_CONNECTIONS, callId);
    postMessage(renegotiateMessage);
}

// Assignment operator
CallManager& CallManager::operator=(const CallManager& rhs)
{
    if (this == &rhs)            // handle the assignment to self case
        return *this;

    return *this;
}

// Set the maximum number of calls to admit to the system.
void CallManager::setMaxCalls(int maxCalls)
{
    mMaxCalls = maxCalls;
}

// Enable STUN for NAT/Firewall traversal
void CallManager::enableStun(const char* szStunServer, 
                             int iServerPort,
                             int iKeepAlivePeriodSecs, 
                             OsNotification* pNotification)
{
    CpMultiStringMessage enableStunMessage(CP_ENABLE_STUN, szStunServer, NULL, 
            NULL, NULL, NULL, iServerPort, iKeepAlivePeriodSecs, (int) pNotification) ;
    postMessage(enableStunMessage);
}


void CallManager::enableTurn(const char* szTurnServer,
                             int iTurnPort,
                             const char* szUsername,
                             const char* szPassword,
                             int iKeepAlivePeriodSecs) 
{
    CpMultiStringMessage enableTurnMessage(CP_ENABLE_TURN, szTurnServer, szUsername,
            szPassword, NULL, NULL, iTurnPort, iKeepAlivePeriodSecs) ;
    postMessage(enableTurnMessage);
}

/* ============================ ACCESSORS ================================= */

UtlBoolean CallManager::changeCallFocus(CpCall* callToTakeFocus)
{
    OsWriteLock lock(mCallListMutex);
    UtlBoolean focusChanged = FALSE;

    if(callToTakeFocus != infocusCall)
    {
        focusChanged = TRUE;
        if(callToTakeFocus)
        {
            callToTakeFocus = removeCall(callToTakeFocus);
            if(callToTakeFocus) callToTakeFocus->inFocus();
        }
        if(infocusCall)
        {
            // Temporary fix so that focus change has happened
            delay(20);


            infocusCall->outOfFocus();
            pushCall(infocusCall);
        }
        infocusCall = callToTakeFocus;
    }
    return(focusChanged);
}

void CallManager::pushCall(CpCall* call)
{
    callStack.insertAt(0, new UtlInt((int) call));
}

CpCall* CallManager::popCall()
{
    CpCall* call = NULL;
    UtlInt* callCollectable = (UtlInt*) callStack.get();
    if(callCollectable)
    {
        call = (CpCall*) callCollectable->getValue();
        delete callCollectable;
        callCollectable = NULL;
    }
    return(call);
}

CpCall* CallManager::removeCall(CpCall* call)
{
    UtlInt matchCall((int)call);
    UtlInt* callCollectable = (UtlInt*) callStack.remove(&matchCall);
    if(callCollectable)
    {
        call = (CpCall*) callCollectable->getValue();
#ifdef TEST_PRINT
        OsSysLog::add(FAC_CP, PRI_DEBUG, "Found and removed call from stack: %X\r\n", call);
#endif
        delete callCollectable;
        callCollectable = NULL;
    }
    else
    {
        OsSysLog::add(FAC_CP, PRI_DEBUG, "Failed to find call to remove from stack\r\n");
        call = NULL;
    }
    return(call);
}

int CallManager::getCallStackSize()
{
    return(callStack.entries());
}

CpCall* CallManager::findHandlingCall(const char* callId)
{
    CpCall* handlingCall = NULL;

    if(infocusCall)
    {
        if(infocusCall->hasCallId(callId))
        {
            handlingCall = infocusCall;
        }
    }

    if(!handlingCall)
    {
        UtlSListIterator iterator(callStack);
        UtlInt* callCollectable;
        CpCall* call;
        callCollectable = (UtlInt*)iterator();
        while(callCollectable &&
            !handlingCall)
        {
            call = (CpCall*)callCollectable->getValue();
            if(call && call->hasCallId(callId))
            {
                handlingCall = call;
            }
            callCollectable = (UtlInt*)iterator();
        }

    }

    return(handlingCall);
}

CpCall* CallManager::findHandlingCall(const OsMsg& eventMessage)
{
    CpCall* handlingCall = NULL;
    CpCall::handleWillingness handlingWeight = CpCall::CP_WILL_NOT_HANDLE;
    CpCall::handleWillingness thisCallHandlingWeight;

    if(infocusCall)
    {
        handlingWeight = infocusCall->willHandleMessage(eventMessage);
        if(handlingWeight != CpCall::CP_WILL_NOT_HANDLE)
        {
            handlingCall = infocusCall;
        }
    }

    if(handlingWeight != CpCall::CP_DEFINITELY_WILL_HANDLE)
    {
        UtlSListIterator iterator(callStack);
        UtlInt* callCollectable;
        CpCall* call;
        callCollectable = (UtlInt*)iterator();
        while(callCollectable)
        {
            call = (CpCall*)callCollectable->getValue();
            if(call)
            {
                thisCallHandlingWeight =
                    call->willHandleMessage(eventMessage);

                if(thisCallHandlingWeight > handlingWeight)
                {
                    handlingWeight = thisCallHandlingWeight;
                    handlingCall = call;
                }

                if(handlingWeight == CpCall::CP_DEFINITELY_WILL_HANDLE)
                {
                    break;
                }
            }
            callCollectable = (UtlInt*)iterator();
        }

    }

    return(handlingCall);
}

void CallManager::startCallStateLog()
{
    mCallStateLogEnabled = TRUE;
#ifdef TEST_PRINT
    OsSysLog::add(FAC_CP, PRI_DEBUG, "Call State LOGGING ENABLED\n");
#endif
}

void CallManager::stopCallStateLog()
{
    mCallStateLogEnabled = FALSE;
#ifdef TEST_PRINT
    OsSysLog::add(FAC_CP, PRI_DEBUG, "Call State LOGGING DISABLED\n");
#endif
}

PtStatus CallManager::validateAddress(UtlString& address)
{
    PtStatus returnCode = PT_SUCCESS;

    // Check that we are adhering to one of the address schemes
    // Currently we only support SIP URLs so everything must map
    // to a SIP URL

    RegEx ip4Address("^[0-9]+\\.[0-9]+\\.[0-9]+\\.[0-9]+$");

    // If it is all digits
    RegEx allDigits("^[0-9*]+$");

    if(allDigits.Search(address.data()))
    {
        // There must be a valid default SIP host address (SIP_DIRECTORY_SERVER)
        UtlString directoryServerAddress;
        if(sipUserAgent)
        {
            int port;
            UtlString protocol;
            sipUserAgent->getDirectoryServer(0,&directoryServerAddress, &port,
                &protocol);
        }

        // If there is no host or there is an invalid IP4 address
        // We do not validate DNS host names here so that we do not block
        if(   directoryServerAddress.isNull() // no host
            || (   ip4Address.Search(directoryServerAddress.data())
            && !OsSocket::isIp4Address(directoryServerAddress)
            ))
        {
            returnCode = PT_INVALID_SIP_DIRECTORY_SERVER;
        }

        else
        {
            address.append("@");
            //OsSysLog::add(FAC_CP, PRI_DEBUG, "CallManager::transfer adding @\n");
        }
    }

    // If it is not all digits it must be a SIP URL
    else
    {
        Url addressUrl(address.data());
        UtlString urlHost;
        addressUrl.getHostAddress(urlHost);
        if(urlHost.isNull())
        {
            returnCode = PT_INVALID_SIP_URL;
        }
        else
        {
            // If the host name is an IP4 address check that it is valid
            if(   ip4Address.Search(urlHost.data())
                && !OsSocket::isIp4Address(urlHost)
                )
            {
                returnCode = PT_INVALID_IP_ADDRESS;
            }

            else
            {
                // It is illegal to have a tag in the
                // To field of an initial INVITE
                addressUrl.removeFieldParameter("tag");
                addressUrl.toString(address);
            }
        }
    }
    return(returnCode);
}

// Get the current number of calls in the system and the maximum number of
// calls to be admitted to the system.
void CallManager::getCalls(int& currentCalls, int& maxCalls)
{
    currentCalls = getCallStackSize();
    maxCalls = mMaxCalls;
}

int CallManager::getMediaConnectionId(const char* szCallId, const char* szRemoteAddress, void** ppInstData)
{
    int connectionId = -1;
    OsProtectEventMgr* eventMgr = OsProtectEventMgr::getEventMgr();
    OsProtectedEvent* getIdEvent = eventMgr->alloc();
    OsTime maxEventTime(CP_MAX_EVENT_WAIT_SECONDS, 0);
    CpMultiStringMessage getIdMessage(CP_GET_MEDIA_CONNECTION_ID, szCallId, szRemoteAddress, NULL, NULL, NULL, (int) getIdEvent, (int) ppInstData);
    postMessage(getIdMessage);

    // Wait until the call sets the number of connections
    if(getIdEvent->wait(0, maxEventTime) == OS_SUCCESS)
    {
        getIdEvent->getEventData(connectionId);
        eventMgr->release(getIdEvent);
    }
    else
    {
        OsSysLog::add(FAC_CP, PRI_ERR, "CallManager::getMediaConnectionId TIMED OUT\n");
        // If the event has already been signalled, clean up
        if(OS_ALREADY_SIGNALED == getIdEvent->signal(0))
        {
            eventMgr->release(getIdEvent);
        }
        connectionId = -1;
    }
    return connectionId;
}


UtlBoolean CallManager::getAudioEnergyLevels(const char*   szCallId, 
                                             const char*   szRemoteAddress,
                                             int&          iInputEnergyLevel,
                                             int&          iOutputEnergyLevel,
                                             int&          nContributors,
                                             unsigned int* pContributorSRCIds,
                                             int*          pContributorEngeryLevels)
{
    UtlBoolean bSuccess = false ;

    OsProtectEventMgr* eventMgr = OsProtectEventMgr::getEventMgr();
    OsProtectedEvent* getELEvent = eventMgr->alloc();
    OsTime maxEventTime(CP_MAX_EVENT_WAIT_SECONDS, 0);
    CpMultiStringMessage getELMessage(CP_GET_MEDIA_ENERGY_LEVELS, 
            szCallId, szRemoteAddress, NULL, NULL, NULL, 
            (int) getELEvent,
            (int) &iInputEnergyLevel, 
            (int) &iOutputEnergyLevel, 
            (int) &nContributors,
            (int) pContributorSRCIds,
            (int) pContributorEngeryLevels);
    postMessage(getELMessage);

    // Wait until the call sets the number of connections
    if(getELEvent->wait(0, maxEventTime) == OS_SUCCESS)
    {
        int status ;
        getELEvent->getEventData(status);
        eventMgr->release(getELEvent);

        bSuccess = (UtlBoolean) status ;
    }
    else
    {
        OsSysLog::add(FAC_CP, PRI_ERR, "CallManager::getAudioEnergyLevels TIMED OUT\n");
        // If the event has already been signalled, clean up
        if(OS_ALREADY_SIGNALED == getELEvent->signal(0))
        {
            eventMgr->release(getELEvent);
        }
        bSuccess = false ;
    }
    return bSuccess ;
}

UtlBoolean CallManager::getAudioEnergyLevels(const char*   szCallId,                                            
                                             int&          iInputEnergyLevel,
                                             int&          iOutputEnergyLevel) 
{
    UtlBoolean bSuccess = false ;

    OsProtectEventMgr* eventMgr = OsProtectEventMgr::getEventMgr();
    OsProtectedEvent* getELEvent = eventMgr->alloc();
    OsTime maxEventTime(CP_MAX_EVENT_WAIT_SECONDS, 0);
    CpMultiStringMessage getELMessage(CP_GET_CALL_MEDIA_ENERGY_LEVELS, 
            szCallId, NULL, NULL, NULL, NULL, 
            (int) getELEvent,
            (int) &iInputEnergyLevel, 
            (int) &iOutputEnergyLevel);
    postMessage(getELMessage);

    // Wait until the call sets the number of connections
    if(getELEvent->wait(0, maxEventTime) == OS_SUCCESS)
    {
        int status ;
        getELEvent->getEventData(status);
        eventMgr->release(getELEvent);

        bSuccess = (UtlBoolean) status ;
    }
    else
    {
        OsSysLog::add(FAC_CP, PRI_ERR, "CallManager::getAudioEnergyLevels TIMED OUT\n");
        // If the event has already been signalled, clean up
        if(OS_ALREADY_SIGNALED == getELEvent->signal(0))
        {
            eventMgr->release(getELEvent);
        }
        bSuccess = false ;
    }
    return bSuccess ;
}

// Can a new connection be added to the specified call?  This method is 
// delegated to the media interface.
UtlBoolean CallManager::canAddConnection(const char* szCallId)
{
    UtlBoolean bCanAdd = FALSE ;

    OsProtectEventMgr* eventMgr = OsProtectEventMgr::getEventMgr();
    OsProtectedEvent* getIdEvent = eventMgr->alloc();
    OsTime maxEventTime(CP_MAX_EVENT_WAIT_SECONDS, 0);
    CpMultiStringMessage getIdMessage(CP_GET_CAN_ADD_PARTY, szCallId, NULL, NULL, NULL, NULL, (int) getIdEvent);
    postMessage(getIdMessage);

    // Wait until the call sets the number of connections
    if(getIdEvent->wait(0, maxEventTime) == OS_SUCCESS)
    {
        int eventData ;
        getIdEvent->getEventData(eventData);
        eventMgr->release(getIdEvent);
        bCanAdd = (UtlBoolean) eventData ;

    }
    else
    {
        OsSysLog::add(FAC_CP, PRI_ERR, "CallManager::canAddConnection TIMED OUT\n");
        // If the event has already been signalled, clean up
        if(OS_ALREADY_SIGNALED == getIdEvent->signal(0))
        {
            eventMgr->release(getIdEvent);
        }        
    }

    return bCanAdd ;
   
}

// Gets the media interface factory used by the call manager
CpMediaInterfaceFactory* CallManager::getMediaInterfaceFactory() 
{
    return mpMediaFactory;
}

/* ============================ INQUIRY =================================== */


/* //////////////////////////// PRIVATE /////////////////////////////////// */

// used for outgoing calls
void CallManager::doCreateCall(const char* callId,
                               int metaEventId,
                               int metaEventType,
                               int numMetaEventCalls,
                               const char* metaEventCallIds[],
                               UtlBoolean assumeFocusIfNoInfocusCall)
{
    OsWriteLock lock(mCallListMutex);

    CpCall* call = findHandlingCall(callId);
    if(call)
    {
        // This is generally bad.  The call should not exist.
        OsSysLog::add(FAC_CP, PRI_ERR, "doCreateCall cannot create call. CallId: %s already exists.\n",
            callId);
    }
    else
    {
        if(mOutGoingCallType == SIP_CALL)
        {
            int numCodecs;
            SdpCodec** codecArray = NULL;
#ifdef TEST_PRINT
            OsSysLog::add(FAC_CP, PRI_DEBUG, "CallManager::doCreateCall getting codec array copy\n");
#endif
            getCodecs(numCodecs, codecArray);
#ifdef TEST_PRINT
            OsSysLog::add(FAC_CP, PRI_DEBUG, "CallManager::doCreateCall got %d codecs, creating CpPhoneMediaInterface\n",
                numCodecs);
#endif
            UtlString publicAddress;
            int publicPort;
            //always use sipUserAgent public address, not the mPublicAddress of this call manager.
            sipUserAgent->getViaInfo(OsSocket::UDP, publicAddress, publicPort, NULL, NULL);

            UtlString localAddress;
            int dummyPort;
            
            sipUserAgent->getLocalAddress(&localAddress, &dummyPort, TRANSPORT_UDP);
            CpMediaInterface* mediaInterface = mpMediaFactory->createMediaInterface(
				NULL,
                publicAddress.data(), localAddress.data(),
                numCodecs, codecArray, mLocale.data(), mExpeditedIpTos,
                mStunServer, mStunPort, mStunKeepAlivePeriodSecs, 
                mTurnServer, mTurnPort, mTurnUsername, mTurnPassword, 
                mTurnKeepAlivePeriodSecs, isIceEnabled());

            OsSysLog::add(FAC_CP, PRI_DEBUG, "Creating new SIP Call, mediaInterface: 0x%08x\n", (int)mediaInterface);
            call = new CpPeerCall(mIsEarlyMediaFor180,
                this,
                mediaInterface,
                m_pCallEventListener,
                m_pInfoStatusEventListener,
                m_pSecurityEventListener,
                m_pMediaEventListener,
                aquireCallIndex(),
                callId,
                sipUserAgent,
                mSipSessionReinviteTimer,
                mOutboundLine.data(),
                mOfferedTimeOut,
                mLineAvailableBehavior,
                mForwardUnconditional.data(),
                mLineBusyBehavior,
                mSipForwardOnBusy.data(),
                mNoAnswerTimeout,
                mForwardOnNoAnswer.data());
			// temporary
			mediaInterface->setInterfaceNotificationQueue(call->getMessageQueue());
            // Short term kludge: createCall invoked, this
            // implys the phone is off hook
            call->start();

            if(metaEventId > 0)
            {
                call->setMetaEvent(metaEventId, metaEventType,
                    numMetaEventCalls, metaEventCallIds);
            }
            else
            {
                int type = (metaEventType != PtEvent::META_EVENT_NONE) ? metaEventType : PtEvent::META_CALL_STARTING;
                call->startMetaEvent(getNewMetaEventId(), type, numMetaEventCalls, metaEventCallIds);
            }
            
            // Make this call infocus if there currently is not infocus call
            if(!infocusCall && assumeFocusIfNoInfocusCall)
            {
                infocusCall = call;
                infocusCall->inFocus(0);
            }
            // Other wise add this call to the stack
            else
            {
                pushCall(call);
            }

            for (int i = 0; i < numCodecs; i++)
            {
                delete codecArray[i];
            }
            delete[] codecArray;
        }
    }
}


void CallManager::doConnect(const char* callId,
                            const char* addressUrl, 
                            const char* desiredConnectionCallId, 
                            SIPX_CONTACT_ID contactId,
                            const void* pDisplay,
                            const void* pSecurity,
                            const char* locationHeader,
                            const int bandWidth,
                            SIPX_TRANSPORT_DATA* pTransport,
                            const RtpTransportOptions rtpTransportOptions)
{
    OsWriteLock lock(mCallListMutex);
    CpCall* call = findHandlingCall(callId);
    if(!call)
    {
        // This is generally bad.  The call should exist.
        OsSysLog::add(FAC_CP, PRI_ERR, "doConnect cannot find CallId: %s\n", callId);
    }
    else
    {
        // For now just send the call a dialString
        CpMultiStringMessage dialStringMessage(CP_DIAL_STRING, addressUrl, desiredConnectionCallId, NULL, NULL, locationHeader,
                                               contactId, (int)pDisplay, (int)pSecurity, bandWidth, (int) pTransport, rtpTransportOptions) ;
        call->postMessage(dialStringMessage);
        call->setLocalConnectionState(PtEvent::CONNECTION_ESTABLISHED);
        call->stopMetaEvent();
    }
}

void CallManager::doEnableStun(const UtlString& stunServer, 
                               int              iStunPort,
                               int              iKeepAlivePeriodSecs, 
                               OsNotification*  pNotification)
{
    mStunServer = stunServer ;
    mStunPort = iStunPort ;
    mStunKeepAlivePeriodSecs = iKeepAlivePeriodSecs ;

    if (sipUserAgent) 
    {
        sipUserAgent->enableStun(mStunServer, mStunPort, mStunKeepAlivePeriodSecs, pNotification) ;
    }
}


void CallManager::doEnableTurn(const UtlString& turnServer, 
                               int              iTurnPort,
                               const UtlString& turnUsername,
                               const UtlString& szTurnPassword,
                               int              iKeepAlivePeriodSecs)
{
    mTurnServer = turnServer ;
    mTurnPort = iTurnPort ;
    mTurnUsername = turnUsername ;
    mTurnPassword = szTurnPassword ;
    mTurnKeepAlivePeriodSecs = iKeepAlivePeriodSecs ;

    bool bEnabled = (mTurnServer.length() > 0) && portIsValid(mTurnPort) ;
    sipUserAgent->getContactDb().enableTurn(bEnabled) ;
}




void CallManager::getCodecs(int& numCodecs, SdpCodec**& codecArray)
{
    mpCodecFactory->getCodecs(numCodecs,
        codecArray);

}

void CallManager::doGetFocus(CpCall* call)
{
    // OsWriteLock lock(mCallListMutex);
    //if(call && infocusCall != call)
    //{
    if (call)
    {
        changeCallFocus(call);
    }
    //}
}

void CallManager::onCallDestroy(CpCall* call)
{
    if (call)
    {
        call->stopMetaEvent();

        mCallListMutex.acquireWrite() ;                                                
        releaseCallIndex(call->getCallIndex());
        if(infocusCall == call)
        {
            // The infocus call is not in the mCallList -- no need to 
            // remove, but we should tell the call that it is not 
            // longer in focus.
            call->outOfFocus();                    
        }
        else
        {
            call = removeCall(call);
        }
        mCallListMutex.releaseWrite() ;
    }
}

void CallManager::yieldFocus(CpCall* call)
{
    OsWriteLock lock(mCallListMutex);
    if(infocusCall == call)
    {
        infocusCall->outOfFocus();
        pushCall(infocusCall);
        infocusCall = NULL;
    }
}

UtlBoolean CallManager::isFocusTaken()
{
   // no lock is needed
   return (infocusCall != NULL);
}
/* ============================ FUNCTIONS ================================= */
