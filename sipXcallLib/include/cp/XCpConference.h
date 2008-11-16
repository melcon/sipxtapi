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

#ifndef XCpConference_h__
#define XCpConference_h__

// SYSTEM INCLUDES
// APPLICATION INCLUDES
#include <utl/UtlSList.h>
#include <cp/XCpAbstractCall.h>

// DEFINES
// MACROS
// EXTERNAL FUNCTIONS
// CONSTANTS
// STRUCTS
// TYPEDEFS
// MACROS
// FORWARD DECLARATIONS
class AcConnectMsg;
class AcDropConnectionMsg;
class AcDropAllConnectionsMsg;
class AcTransferBlindMsg;
class AcHoldConnectionMsg;
class AcHoldAllConnectionsMsg;
class AcUnholdConnectionMsg;
class AcUnholdAllConnectionsMsg;
class AcRenegotiateCodecsMsg;
class AcRenegotiateCodecsAllMsg;
class AcSendInfoMsg;

/**
 * XCpConference wraps several XSipConnections realizing conference functionality. XCpConference
 * is designed to hold multiple XSipConnections, sharing single media session. Conference cannot
 * accept inbound calls, those are routed to XCpCall automatically. Therefore methods related to
 * inbound call processing are not implemented.
 *
 * Call management functions are asynchronous, and post message to the OsServerTask thread. This is
 * useful because sending a sip message can take more time, and we don't want to block the call manager
 * during that time. Also audio functionality can take several 10ms, or even 100ms if media task is
 * getting overloaded.
 * Audio functionality is synchronous, except for focus gain/yield. Focus gain/yield always needs to go through
 * XCpCallManager, since we need a single place to manage focus. It will also ensure that no attept to gain
 * focus is made before media interface is created.
 *
 * Locking strategy:
 * @see XCpAbstractCall
 * - local members that require locking are always after comment mentioning which mutex needs to be locked
 * - const members and pointers/references set from constructor do not require locking, they are deleted by
 *   caller at suitable time
 *
 * This class should have absolute minimum knowledge about sip, all sip communication is done in XSipConnection.
 */
class XCpConference : public XCpAbstractCall
{
   /* //////////////////////////// PUBLIC //////////////////////////////////// */
public:
   /* ============================ CREATORS ================================== */

   XCpConference(const UtlString& sId,
                 SipUserAgent& rSipUserAgent,
                 CpMediaInterfaceFactory& rMediaInterfaceFactory,
                 OsMsgQ& rCallManagerQueue,
                 CpCallStateEventListener* pCallEventListener = NULL,
                 SipInfoStatusEventListener* pInfoStatusEventListener = NULL,
                 SipSecurityEventListener* pSecurityEventListener = NULL,
                 CpMediaEventListener* pMediaEventListener = NULL);

   virtual ~XCpConference();

   /* ============================ MANIPULATORS ============================== */

   /** Connects call to given address. Uses supplied sip call-id. */
   virtual OsStatus connect(const UtlString& sipCallId,
                            SipDialog& sipDialog,
                            const UtlString& toAddress,
                            const UtlString& fromAddress,
                            const UtlString& locationHeader,
                            CP_CONTACT_ID contactId);

   /**
    * Always fails, as conference cannot accept inbound call. Instead, add calls
    * to conference.
    */
   virtual OsStatus acceptConnection(const UtlString& locationHeader,
                                     CP_CONTACT_ID contactId);

   /**
    * Always fails, as conference cannot reject inbound connections.
    */
   virtual OsStatus rejectConnection();

   /**
   * Always fails, as conference cannot redirect inbound connections.
   */
   virtual OsStatus redirectConnection(const UtlString& sRedirectSipUrl);

   /**
   * Always fails, as conference cannot answer inbound connections.
   */
   virtual OsStatus answerConnection();

   /**
   * Disconnects given call with given sip call-id
   *
   * The appropriate disconnect signal is sent (e.g. with SIP BYE or CANCEL).  The connection state
   * progresses to disconnected and the connection is removed.
   */
   virtual OsStatus dropConnection(const SipDialog& sipDialog, UtlBoolean bDestroyConference = FALSE);

   /** Disconnects all calls */
   OsStatus dropAllConnections(UtlBoolean bDestroyConference = FALSE);

   /** Blind transfer given call to sTransferSipUri. Works for simple call and call in a conference */
   virtual OsStatus transferBlind(const SipDialog& sipDialog,
                                  const UtlString& sTransferSipUrl);

   /**
   * Put the specified terminal connection on hold.
   *
   * Change the terminal connection state from TALKING to HELD.
   * (With SIP a re-INVITE message is sent with SDP indicating
   * no media should be sent.)
   */
   virtual OsStatus holdConnection(const SipDialog& sipDialog);

   /**
   * Convenience method to put all of the terminal connections in
   * the specified conference on hold.
   */
   OsStatus holdAllConnections();

   /**
   * Convenience method to take the terminal connection off hold.
   *
   * Change the terminal connection state from HELD to TALKING.
   * (With SIP a re-INVITE message is sent with SDP indicating
   * media should be sent.)
   */
   virtual OsStatus unholdConnection(const SipDialog& sipDialog);

   /**
   * Convenience method to take all of the terminal connections in
   * the specified conference off hold.
   *
   * Change the terminal connection state from HELD to TALKING.
   * (With SIP a re-INVITE message is sent with SDP indicating
   * media should be sent.)
   */
   OsStatus unholdAllConnections();

   /**
   * Rebuild codec factory on the fly with new audio codec requirements
   * and one specific video codec.  Renegotiate the codecs to be use for the
   * specified terminal connection.
   *
   * This is typically performed after a capabilities change for the
   * terminal connection (for example, addition or removal of a codec type).
   * (Sends a SIP re-INVITE.)
   */
   virtual OsStatus renegotiateCodecsConnection(const SipDialog& sipDialog,
                                                const UtlString& sAudioCodecs,
                                                const UtlString& sVideoCodecs);

   /**
   * Rebuild codec factory on the fly with new audio codec requirements
   * and one specific video codec. Convenience method to renegotiate the codecs
   * for all of the terminal connections in the specified conference.
   *
   * This is typically performed after a capabilities change for the
   * terminal connection (for example, addition or removal of a codec type).
   * (Sends a SIP re-INVITE.)
   */
   OsStatus renegotiateCodecsAllConnections(const UtlString& sAudioCodecs,
                                            const UtlString& sVideoCodecs);

   /** Sends an INFO message to the other party(s) on the call */
   virtual OsStatus sendInfo(const SipDialog& sipDialog,
                             const UtlString& sContentType,
                             const char* pContent,
                             const size_t nContentLength);

   /* ============================ ACCESSORS ================================= */

   /* ============================ INQUIRY =================================== */

   /**
   * Checks if this conference has given sip dialog. Uses strict dialog matching.
   */
   virtual SipDialog::DialogMatchEnum hasSipDialog(const SipDialog& sipDialog) const;

   /** Gets the number of sip connections in this call */
   virtual int getCallCount() const;

   /** Gets sip call-id of conference if its available */
   OsStatus getConferenceSipCallIds(UtlSList& sipCallIdList) const;

   /* //////////////////////////// PROTECTED ///////////////////////////////// */
protected:

   /** Finds connection handling given Sip dialog. Uses strict dialog matching. */
   virtual UtlBoolean findConnection(const SipDialog& sipDialog, OsPtrLock<XSipConnection>& ptrLock) const;

   /** Handles command messages */
   virtual UtlBoolean handleCommandMessage(const AcCommandMsg& rRawMsg);

   /** Handles command messages */
   virtual UtlBoolean handleNotificationMessage(const AcNotificationMsg& rRawMsg);

   /** Handles timer messages */
   virtual UtlBoolean handleTimerMessage(const CpTimerMsg& rRawMsg);

   /** Handler for inbound SipMessageEvent messages. */
   virtual UtlBoolean handleSipMessageEvent(const SipMessageEvent& rSipMsgEvent);

   /* //////////////////////////// PRIVATE /////////////////////////////////// */
private:
   XCpConference(const XCpConference& rhs);

   XCpConference& operator=(const XCpConference& rhs);

   /** Handles message to create new sip connection for call */
   OsStatus handleConnect(const AcConnectMsg& rMsg);
   /** Handles message to drop sip connection */
   OsStatus handleDropConnection(const AcDropConnectionMsg& rMsg);
   /** Handles message to drop all sip connections */
   OsStatus handleDropAllConnections(const AcDropAllConnectionsMsg& rMsg);
   /** Handles message to initiate blind call transfer */
   OsStatus handleTransferBlind(const AcTransferBlindMsg& rMsg);
   /** Handles message to initiate remote hold on sip connection */
   OsStatus handleHoldConnection(const AcHoldConnectionMsg& rMsg);
   /** Handles message to initiate remote hold on all sip connections */
   OsStatus handleHoldAllConnections(const AcHoldAllConnectionsMsg& rMsg);
   /** Handles message to initiate remote unhold on sip connection */
   OsStatus handleUnholdConnection(const AcUnholdConnectionMsg& rMsg);
   /** Handles message to initiate remote unhold on all sip connection */
   OsStatus handleUnholdAllConnections(const AcUnholdAllConnectionsMsg& rMsg);
   /** Handles message to renegotiate codecs for some sip connection */
   OsStatus handleRenegotiateCodecs(const AcRenegotiateCodecsMsg& rMsg);
   /** Handles message to renegotiate codecs for all sip connections */
   OsStatus handleRenegotiateCodecsAll(const AcRenegotiateCodecsAllMsg& rMsg);
   /** Handles message to send SIP INFO to on given sip connection */
   OsStatus handleSendInfo(const AcSendInfoMsg& rMsg);

   /** Finds the correct connection by mediaConnectionId and fires media event for it. */
   virtual void fireSipXMediaConnectionEvent(CP_MEDIA_EVENT event,
                                             CP_MEDIA_CAUSE cause,
                                             CP_MEDIA_TYPE type,
                                             int mediaConnectionId,
                                             intptr_t pEventData1,
                                             intptr_t pEventData2);

   /** Fires given media interface event to listeners. */
   virtual void fireSipXMediaInterfaceEvent(CP_MEDIA_EVENT event,
                                            CP_MEDIA_CAUSE cause,
                                            CP_MEDIA_TYPE type,
                                            intptr_t pEventData1,
                                            intptr_t pEventData2);

   // begin of members requiring m_memberMutex
   UtlSList m_sipConnections;
   // end of members requiring m_memberMutex
};

#endif // XCpConference_h__
