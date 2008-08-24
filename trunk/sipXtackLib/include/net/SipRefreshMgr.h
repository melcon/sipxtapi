//
// Copyright (C) 2004-2006 SIPfoundry Inc.
// Licensed by SIPfoundry under the LGPL license.
//
// Copyright (C) 2004-2006 Pingtel Corp.  All rights reserved.
// Licensed to SIPfoundry under a Contributor Agreement.
//
// $$
///////////////////////////////////////////////////////////////////////////////


#ifndef SIPREFRESHMGR_H
#define SIPREFRESHMGR_H

// SYSTEM INCLUDES

// APPLICATION INCLUDES
#include "os/OsServerTask.h"
#include "utl/UtlRandom.h"
#include "utl/UtlHashMap.h"
#include "net/SipMessage.h"
#include "net/SipTcpServer.h"
#include "net/SipUdpServer.h"
#include "net/SipMessageList.h"
#include "net/SipMessageEvent.h"
#include "net/SipCallIdGenerator.h"
#include "net/SipLine.h"


// DEFINES
#define DEFAULT_PERCENTAGE_TIMEOUT 48 //48%
#define FAILED_PERCENTAGE_TIMEOUT 24 //24%

// MACROS
// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
// STRUCTS
// TYPEDEFS
// FORWARD DECLARATIONS
class SipUserAgent;
class SipLineMgr;
class SipLineStateEventListener;

class SipRefreshMgr : public OsServerTask
{
public:
    SipRefreshMgr(SipLineStateEventListener *listener = NULL);

    virtual ~SipRefreshMgr();

    //INITIALIZE
    UtlBoolean init(SipUserAgent *ptrToMyAgent,
                    int defaultRegistryTimeout = 3600);

    void startRefreshMgr();
    
    /**
     * Mutator for the mDefaultRegistryPeriodMember
     */
    void setRegistryPeriod(int periodInSeconds);
    
    void addMessageObserver (
        OsMsgQ& messageQueue,
        const char* sipMethod = NULL,
        UtlBoolean wantRequests = TRUE,
        UtlBoolean wantResponses = TRUE,
        UtlBoolean wantIncoming = TRUE,
        UtlBoolean wantOutGoing = FALSE,
        const char* eventName = NULL,
        void* observerData = NULL );

    //: Add a SIP message observer for SIP messages meeting the filter criteria
    //! param: messageQueue - the queue on which an SipMessageEvent is dispatched
    //! param: sipMethod - the specific method type of the requests or responses to be observed.  NULL or a null string indicates all methods.
    //! param: wantRequests - want to observe SIP requests
    //! param: wantResponses - want to observe SIP responses
    //! param: wantIncoming - want to observe SIP messages originating from the network.
    //! param: wantOutGoing - want to observe SIP messages originating from locally.
    //! param: eventName - want to observer SUBSCRIBE or NOTIFY requests having the given event type
    //! param: observerData - data to be attached to SIP messages queued on the observer

    //REGISTER METHODS
    UtlBoolean newRegisterMsg (
        const Url& fromUrl,
        const Url& contactUri,
        int registryPeriodSeconds = -1);

    void reRegisterAll();

    void reRegister ( const Url& fromUrl );

    void unRegisterUser (
        const Url& fromUrl,
        const UtlBoolean& onStartup = FALSE,
        const UtlString& lineid ="" );
     
    void setLineMgr(SipLineMgr* const lineMgr);
    //: Sets a pointer to the line manager
    
    void dumpMessageLists(UtlString& results) ;
      //:Appends the message contents of both the mRegisterList and 
      // mSubscribeList
  
    virtual UtlBoolean handleMessage( OsMsg& eventMessage );

protected:
    SipLineMgr* mpLineMgr;
    // the line manager object that uses this refresh manager
    
    void queueMessageToObservers (
        SipMessageEvent& event,
        const char* method);

    void rescheduleAfterTime (
        SipMessage* message,
        int percentage = DEFAULT_PERCENTAGE_TIMEOUT );

    void sendToObservers (
        const OsMsg& eventMessage,
        SipMessage * registerRequest );

    OsStatus sendRequest (
        SipMessage& registerRequest,
        const char *method);

    void rescheduleRequest (
        SipMessage* registerRequest,
        int secondsFromNow,
        const char* method,
        int percentage = DEFAULT_PERCENTAGE_TIMEOUT,
        UtlBoolean sendImmediate = FALSE );

    void processOKResponse (
        SipMessage* registerResponse,
        SipMessage* registerRequest );

    void parseContactFields (
        SipMessage* message,
        SipMessage* sipRequest,
        int& expireVal );

    void processResponse(
        const OsMsg& eventMessage,
        SipMessage* registerRequest);

    void createTagNameValuePair(UtlString& tagNamevaluePair);
    UtlString createTagValue();

    // register
    void registerUrl(const Url& fromUrl,
                     const Url& toUrl,
                     const Url& requestUri,
                     const UtlString& contactUrl,
                     const UtlString& callId,
                     int registerPeriod = -1);

    UtlBoolean isDuplicateRegister( 
        const Url& url,
        SipMessage& oldMessage );

    UtlBoolean isDuplicateRegister( const Url& url );

    void addToRegisterList( SipMessage* message);

    UtlBoolean removeFromRegisterList( SipMessage* message );
     //: Returns TRUE if message was found and removed from list.
     //: Message is NOT deleted.  

    UtlString buildContactField(const Url& registerToField,
                                const UtlString& lineId = "",
                                const Url* pPreferredContactUri = NULL);
    
    void removeAllFromRequestList(SipMessage* response);
    //: Removes all prior request records for this response
    //: from the SipMessageLists (mRegisterList & mSubscribeList)
    
    void removeAllFromRequestList(SipMessage* response, SipMessageList* pRequestList);
    //: Removes all prior request records for this response
    //: from the passed-in SipMessageList
    
    UtlBoolean isExpiresZero(SipMessage* pRequest) ;
      //: Is the expires field set to zero for the specified msg?
       
    // register
    int mDefaultRegistryPeriod;
    SipMessageList mRegisterList;
    OsRWMutex mRegisterListMutexR;
    OsRWMutex mRegisterListMutexW;

    // events
    SipLineStateEventListener* mLineListener;

    // common
    SipCallIdGenerator mCallIdGenerator;
    UtlBoolean mIsStarted;
    UtlHashBag mMessageObservers;
    OsRWMutex mObserverMutex;
    SipUserAgent* mMyUserAgent;
    UtlRandom mRandomNumGenerator;
    UtlHashBag mTimerBag;
};

#endif // SIPREFRESHMGR_H
