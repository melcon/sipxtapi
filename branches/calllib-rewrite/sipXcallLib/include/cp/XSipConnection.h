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

#ifndef XSipConnection_h__
#define XSipConnection_h__

// SYSTEM INCLUDES
// APPLICATION INCLUDES
#include <os/OsMutex.h>
#include <os/OsRWMutex.h>
#include <os/OsSyncBase.h>
#include <utl/UtlContainable.h>
#include <net/SipInfoStatusEventListener.h>
#include <net/SipSecurityEventListener.h>
#include <cp/CpDefs.h>
#include <cp/XSipConnectionContext.h>
#include <cp/XSipConnectionEventSink.h>
#include <cp/state/SipConnectionStateMachine.h>
#include <cp/state/SipConnectionStateObserver.h>
#include <cp/CpNatTraversalConfig.h>

// DEFINES
// MACROS
// EXTERNAL FUNCTIONS
// CONSTANTS
// STRUCTS
// TYPEDEFS
// MACROS
// FORWARD DECLARATIONS
class CpMediaInterfaceProvider;
class SipUserAgent;
class CpMediaEvent;
class CpCallStateEvent;
class CpCallStateEventListener;
class SipInfoStatusEventListener;
class SipSecurityEventListener;
class CpMediaEventListener;

/**
 * XSipConnection is responsible for SIP communication.
 *
 * All manipulators except OsSyncBase methods must be called from single thread only.
 * Inquiry and accessor methods may be called from multiple threads.
 *
 * Result codes from connect, acceptConnection etc. which are executed in state machine
 * only mean that the operation was handled. It doesn't mean the operation succeeded.
 * Failures should be logged, and perhaps a sipxtapi event should be fired.
 */
class XSipConnection : public UtlContainable, public OsSyncBase, public SipConnectionStateObserver, public XSipConnectionEventSink
{
   /* //////////////////////////// PUBLIC //////////////////////////////////// */
public:
   static const UtlContainableType TYPE;   /** < Class type used for runtime checking */ 

   /* ============================ CREATORS ================================== */

   XSipConnection(const UtlString& sAbstractCallId,
                  const SipDialog& sipDialog,
                  SipUserAgent& rSipUserAgent,
                  CpMediaInterfaceProvider& pMediaInterfaceProvider,
                  const CpNatTraversalConfig& natTraversalConfig,
                  CpCallStateEventListener* pCallEventListener = NULL,
                  SipInfoStatusEventListener* pInfoStatusEventListener = NULL,
                  SipSecurityEventListener* pSecurityEventListener = NULL,
                  CpMediaEventListener* pMediaEventListener = NULL);

   virtual ~XSipConnection();

   /* ============================ MANIPULATORS ============================== */

   /** Acquires exclusive lock on instance. Use only when deleting. It is never released. */
   virtual OsStatus acquireExclusive();

   /** Handles sipx media event */
   virtual void handleSipXMediaEvent(CP_MEDIA_EVENT event,
                                     CP_MEDIA_CAUSE cause,
                                     CP_MEDIA_TYPE  type,
                                     intptr_t pEventData1 = 0,
                                     intptr_t pEventData2 = 0);

   /** Handles sipx call event */
   virtual void handleSipXCallEvent(CP_CALLSTATE_EVENT eventCode,
                                    CP_CALLSTATE_CAUSE causeCode,
                                    const UtlString& sOriginalSessionCallId = NULL,
                                    int sipResponseCode = 0,
                                    const UtlString& sResponseText = NULL);

   /**
    * Connects call to given address. Sip callId is not supplied here, because
    * sip connection has a callId assigned to its during creation.
    */
   OsStatus connect(const UtlString& toAddress,
                    const UtlString& fromAddress,
                    const UtlString& locationHeader,
                    CP_CONTACT_ID contactId);

   /** 
   * Accepts inbound call connection.
   *
   * Progress the connection from the OFFERING state to the
   * RINGING state. This causes a SIP 180 Ringing provisional
   * response to be sent.
   */
   OsStatus acceptConnection(const UtlString& locationHeader,
                             CP_CONTACT_ID contactId);

   /**
   * Reject the incoming connection.
   *
   * Progress the connection from the OFFERING state to
   * the FAILED state with the cause of busy. With SIP this
   * causes a 486 Busy Here response to be sent.
   */
   OsStatus rejectConnection();

   /**
   * Redirect the incoming connection.
   *
   * Progress the connection from the OFFERING state to
   * the FAILED state. This causes a SIP 302 Moved
   * Temporarily response to be sent with the specified
   * contact URI.
   */
   OsStatus redirectConnection(const UtlString& sRedirectSipUrl);

   /**
   * Answer the incoming terminal connection.
   *
   * Progress the connection from the OFFERING or RINGING state
   * to the ESTABLISHED state and also creating the terminal
   * connection (with SIP a 200 OK response is sent).
   */
   OsStatus answerConnection();

   /** Disconnects call */
   OsStatus dropConnection(UtlBoolean bDestroyCall = FALSE);

   /** Blind transfer given call to sTransferSipUri. */
   OsStatus transferBlind(const UtlString& sTransferSipUrl);

   /**
   * Put the specified terminal connection on hold.
   *
   * Change the terminal connection state from TALKING to HELD.
   * (With SIP a re-INVITE message is sent with SDP indicating
   * no media should be sent.)
   */
   OsStatus holdConnection();

   /**
   * Convenience method to take the terminal connection off hold.
   *
   * Change the terminal connection state from HELD to TALKING.
   * (With SIP a re-INVITE message is sent with SDP indicating
   * media should be sent.)
   */
   OsStatus unholdConnection();

   /**
   * Enables discarding of inbound RTP at bridge for given call
   * or conference. Useful for server applications without mic/speaker.
   * DTMF on given call will still be decoded.
   */
   OsStatus muteInputConnection();

   /**
   * Disables discarding of inbound RTP for given call
   * or conference. Useful for server applications without mic/speaker.
   */
   OsStatus unmuteInputConnection();

   /**
   * Rebuild codec factory on the fly with new audio codec requirements
   * and one specific video codec.  Renegotiate the codecs to be use for the
   * specified terminal connection.
   *
   * This is typically performed after a capabilities change for the
   * terminal connection (for example, addition or removal of a codec type).
   * (Sends a SIP re-INVITE.)
   */
   OsStatus renegotiateCodecsConnection(const UtlString& sAudioCodecs,
                                        const UtlString& sVideoCodecs);

   /** Sends an INFO message to the other party(s) on the call */
   OsStatus sendInfo(const UtlString& sContentType,
                     const char* pContent,
                     const size_t nContentLength);

   /* ============================ ACCESSORS ================================= */

   /**
   * Calculate a unique hash code for this object.  If the equals
   * operator returns true for another object, then both of those
   * objects must return the same hashcode.
   */    
   virtual unsigned hash() const;

   /**
   * Get the ContainableType for a UtlContainable derived class.
   */
   virtual UtlContainableType getContainableType() const;

   /**
   * Gets a copy of connection sip dialog.
   */
   void getSipDialog(SipDialog& sSipDialog) const;

   /**
    * Gets sip call-id of the connection.
    */
   void getSipCallId(UtlString& sSipCallId) const;

   /**
    * Gets user agent of the remote party if known.
    */
   void getRemoteUserAgent(UtlString& sRemoteUserAgent) const;

   /** Gets internal id of media connection for connection. */
   int getMediaConnectionId() const;

   /** Gets id of parent abstract call */
   void getAbstractCallId(UtlString& sAbstractCallId) const;

   /** Gets Url of the remote connection party field parameters if present (tag)*/
   void getRemoteAddress(UtlString& sRemoteAddress) const;

   /* ============================ INQUIRY =================================== */

   /**
   * Compare the this object to another like-objects.  Results for 
   * designating a non-like object are undefined.
   *
   * @returns 0 if equal, < 0 if less then and >0 if greater.
   */
   virtual int compareTo(UtlContainable const* inVal) const;

   /**
   * Checks if this call has given sip dialog.
   */
   SipDialog::DialogMatchEnum compareSipDialog(const SipDialog& sSipDialog) const;

   /* //////////////////////////// PROTECTED ///////////////////////////////// */
protected:

   /* //////////////////////////// PRIVATE /////////////////////////////////// */
private:
   XSipConnection(const XSipConnection& rhs);

   XSipConnection& operator=(const XSipConnection& rhs);

   /**
   * Called when we enter new state. This is typically called after we handle
   * SipMessageEvent, resulting in new state transition.
   */
   virtual void handleStateEntry(ISipConnectionState::StateEnum state);

   /**
   * Called when we progress to new state, before old state is destroyed.
   */
   virtual void handleStateExit(ISipConnectionState::StateEnum state);

   /** Sets common properties on media event */
   void prepareMediaEvent(CpMediaEvent& event, CP_MEDIA_CAUSE cause, CP_MEDIA_TYPE type);

   /** Sets common properties on call event */
   void prepareCallStateEvent(CpCallStateEvent& event,
                              CP_CALLSTATE_CAUSE eMinor,
                              const UtlString& sOriginalSessionCallId = NULL,
                              int sipResponseCode = 0,
                              const UtlString& sResponseText = NULL);

   /** Fire info status event */
   virtual void fireSipXInfoStatusEvent(CP_INFOSTATUS_EVENT event,
                                        SIPXTACK_MESSAGE_STATUS status,
                                        const UtlString& sResponseText,
                                        int responseCode = 0);

   /** Fire security event */
   virtual void fireSipXSecurityEvent(SIPXTACK_SECURITY_EVENT event,
                                      SIPXTACK_SECURITY_CAUSE cause,
                                      const UtlString& sSRTPkey,
                                      void* pCertificate,
                                      size_t nCertificateSize,
                                      const UtlString& sSubjAltName,
                                      const UtlString& sSessionCallId,
                                      const UtlString& sRemoteAddress);

   /** Fires sipx media event to media listener */
   virtual void fireSipXMediaEvent(CP_MEDIA_EVENT event,
                                   CP_MEDIA_CAUSE cause,
                                   CP_MEDIA_TYPE  type,
                                   intptr_t pEventData1 = 0,
                                   intptr_t pEventData2 = 0);

   /** Fires sipx call event to call event listener */
   virtual void fireSipXCallEvent(CP_CALLSTATE_EVENT eventCode,
                                  CP_CALLSTATE_CAUSE causeCode,
                                  const UtlString& sOriginalSessionCallId = NULL,
                                  int sipResponseCode = 0,
                                  const UtlString& sResponseText = NULL);

   /** Block until the sync object is acquired. Timeout is not supported! */
   virtual OsStatus acquire(const OsTime& rTimeout = OsTime::OS_INFINITY);

   /** Conditionally acquire the semaphore (i.e., don't block) */
   virtual OsStatus tryAcquire();

   /** Release the sync object */
   virtual OsStatus release();

   // not thread safe, must be used from single thread only
   SipConnectionStateMachine m_stateMachine; ///< state machine for handling commands and SipMessageEvents
   // needs special external locking
   XSipConnectionContext& m_rSipConnectionContext; ///< contains stateful information about sip connection.
   // thread safe
   SipUserAgent& m_rSipUserAgent; // for sending sip messages
   CpMediaInterfaceProvider& m_rMediaInterfaceProvider; ///< media interface provider
   // thread safe, set only once
   CpCallStateEventListener* m_pCallEventListener;
   SipInfoStatusEventListener* m_pInfoStatusEventListener;
   SipSecurityEventListener* m_pSecurityEventListener;
   CpMediaEventListener* m_pMediaEventListener;
   const CpNatTraversalConfig m_natTraversalConfig; ///< NAT traversal configuration

   mutable OsRWMutex m_instanceRWMutex; ///< mutex for guarding instance against deletion from XCpAbstractCall
};

#endif // XSipConnection_h__
