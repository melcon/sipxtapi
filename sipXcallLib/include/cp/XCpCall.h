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

#ifndef XCpCall_h__
#define XCpCall_h__

// SYSTEM INCLUDES
// APPLICATION INCLUDES
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
class AcAcceptConnectionMsg;
class AcRejectConnectionMsg;
class AcRedirectConnectionMsg;
class AcAnswerConnectionMsg;
class AcDropConnectionMsg;
class AcTransferBlindMsg;
class AcHoldConnectionMsg;
class AcUnholdConnectionMsg;
class AcLimitCodecPreferencesMsg;
class AcRenegotiateCodecsMsg;
class AcSendInfoMsg;
class AcMuteInputConnectionMsg;
class AcUnmuteInputConnectionMsg;

/**
 * XCpCall wraps XSipConnection realizing all call functionality. XCpCall is designed to hold
 * only single XSipConnection, and corresponds to a sip connection and media session.
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
class XCpCall : public XCpAbstractCall
{
   /* //////////////////////////// PUBLIC //////////////////////////////////// */
public:
   /* ============================ CREATORS ================================== */

   XCpCall(const UtlString& sId,
           SipUserAgent& rSipUserAgent,
           CpMediaInterfaceFactory& rMediaInterfaceFactory,
           OsMsgQ& rCallManagerQueue,
           CpCallStateEventListener* pCallEventListener = NULL,
           SipInfoStatusEventListener* pInfoStatusEventListener = NULL,
           SipSecurityEventListener* pSecurityEventListener = NULL,
           CpMediaEventListener* pMediaEventListener = NULL);

   virtual ~XCpCall();

   /* ============================ MANIPULATORS ============================== */

   /** Connects call to given address. Uses supplied sip call-id. */
   virtual OsStatus connect(const UtlString& sipCallId,
                            SipDialog& sipDialog,
                            const UtlString& toAddress,
                            const UtlString& fromAddress,
                            const UtlString& locationHeader,
                            CP_CONTACT_ID contactId);

   /** 
   * Accepts inbound call connection. Inbound connections can only be part of XCpCall
   *
   * Progress the connection from the OFFERING state to the
   * RINGING state. This causes a SIP 180 Ringing provisional
   * response to be sent.
   */
   virtual OsStatus acceptConnection(const UtlString& locationHeader,
                                     CP_CONTACT_ID contactId);

   /**
   * Reject the incoming connection.
   *
   * Progress the connection from the OFFERING state to
   * the FAILED state with the cause of busy. With SIP this
   * causes a 486 Busy Here response to be sent.
   */
   virtual OsStatus rejectConnection();

   /**
   * Redirect the incoming connection.
   *
   * Progress the connection from the OFFERING state to
   * the FAILED state. This causes a SIP 302 Moved
   * Temporarily response to be sent with the specified
   * contact URI.
   */
   virtual OsStatus redirectConnection(const UtlString& sRedirectSipUrl);

   /**
   * Answer the incoming terminal connection.
   *
   * Progress the connection from the OFFERING or RINGING state
   * to the ESTABLISHED state and also creating the terminal
   * connection (with SIP a 200 OK response is sent).
   */
   virtual OsStatus answerConnection();

   /**
   * Disconnects given call with given sip call-id
   *
   * The appropriate disconnect signal is sent (e.g. with SIP BYE or CANCEL).  The connection state
   * progresses to disconnected and the connection is removed.
   */
   virtual OsStatus dropConnection(const SipDialog& sipDialog, UtlBoolean bDestroyCall = FALSE);

   /** Disconnects call without knowing the sip call-id*/
   OsStatus dropConnection(UtlBoolean bDestroyCall = FALSE);

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
   virtual OsStatus unholdConnection(const SipDialog& sipDialog);

   /**
   * Convenience method to take the terminal connection off hold.
   *
   * Change the terminal connection state from HELD to TALKING.
   * (With SIP a re-INVITE message is sent with SDP indicating
   * media should be sent.)
   */
   virtual OsStatus unholdConnection();

   /**
   * Enables discarding of inbound RTP at bridge for given call
   * or conference. Useful for server applications without mic/speaker.
   * DTMF on given call will still be decoded.
   */
   virtual OsStatus muteInputConnection(const SipDialog& sipDialog);

   /**
   * Disables discarding of inbound RTP for given call
   * or conference. Useful for server applications without mic/speaker.
   */
   virtual OsStatus unmuteInputConnection(const SipDialog& sipDialog);

   /**
   * Rebuild codec factory on the fly with new audio codec requirements
   * and new video codecs. Preferences will be in effect after the next
   * INVITE or re-INVITE. Can be called on empty call or conference to limit
   * codecs for future calls. When called on an established call, hold/unhold
   * or codec renegotiation needs to be triggered to actually change codecs.
   * If used on conference, codecs will be applied to all future calls, and all
   * calls that are unheld.
   */
   virtual OsStatus limitCodecPreferences(CP_AUDIO_BANDWIDTH_ID audioBandwidthId,
                                          const UtlString& sAudioCodecs,
                                          CP_VIDEO_BANDWIDTH_ID videoBandwidthId,
                                          const UtlString& sVideoCodecs);

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
                                                CP_AUDIO_BANDWIDTH_ID audioBandwidthId,
                                                const UtlString& sAudioCodecs,
                                                CP_VIDEO_BANDWIDTH_ID videoBandwidthId,
                                                const UtlString& sVideoCodecs);

   /** Sends an INFO message to the other party(s) on the call */
   virtual OsStatus sendInfo(const SipDialog& sipDialog,
                             const UtlString& sContentType,
                             const char* pContent,
                             const size_t nContentLength);

   /* ============================ ACCESSORS ================================= */

   /* ============================ INQUIRY =================================== */

   /**
   * Checks if this call has given sip dialog. Uses strict dialog matching.
   */
   virtual SipDialog::DialogMatchEnum hasSipDialog(const SipDialog& sipDialog) const;

   /** Gets the number of sip connections in this call */
   virtual int getCallCount() const;

   /** Gets sip call-id of call if its available */
   OsStatus getCallSipCallId(UtlString& sSipCallId) const;

   /* //////////////////////////// PROTECTED ///////////////////////////////// */
protected:

   /**
    * Finds connection handling given Sip dialog. Uses strict dialog matching.
    */
   virtual UtlBoolean findConnection(const SipDialog& sipDialog, OsPtrLock<XSipConnection>& ptrLock) const;

   /** Gets Sip connection of the call if there is any */
   virtual UtlBoolean getConnection(OsPtrLock<XSipConnection>& ptrLock) const;

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
   XCpCall(const XCpCall& rhs);

   XCpCall& operator=(const XCpCall& rhs);

   /** Handles message to create new sip connection for call */
   OsStatus handleConnect(const AcConnectMsg& rMsg);
   /** Handles message to accept inbound sip connection */
   OsStatus handleAcceptConnection(const AcAcceptConnectionMsg& rMsg);
   /** Handles message to reject inbound sip connection */
   OsStatus handleRejectConnection(const AcRejectConnectionMsg& rMsg);
   /** Handles message to redirect inbound sip connection */
   OsStatus handleRedirectConnection(const AcRedirectConnectionMsg& rMsg);
   /** Handles message to answer inbound sip connection */
   OsStatus handleAnswerConnection(const AcAnswerConnectionMsg& rMsg);
   /** Handles message to drop sip connection */
   OsStatus handleDropConnection(const AcDropConnectionMsg& rMsg);
   /** Handles message to initiate blind call transfer */
   OsStatus handleTransferBlind(const AcTransferBlindMsg& rMsg);
   /** Handles message to initiate remote hold on sip connection */
   OsStatus handleHoldConnection(const AcHoldConnectionMsg& rMsg);
   /** Handles message to initiate remote unhold on sip connection */
   OsStatus handleUnholdConnection(const AcUnholdConnectionMsg& rMsg);
   /** Handles message to limit codec preferences for future sip connections */
   OsStatus handleLimitCodecPreferences(const AcLimitCodecPreferencesMsg& rMsg);
   /** Handles message to renegotiate codecs for some sip connection */
   OsStatus handleRenegotiateCodecs(const AcRenegotiateCodecsMsg& rMsg);
   /** Handles message to send SIP INFO to on given sip connection */
   OsStatus handleSendInfo(const AcSendInfoMsg& rMsg);
   /** Handles message to mute inbound RTP in audio bridge */
   OsStatus handleMuteInputConnection(const AcMuteInputConnectionMsg& rMsg);
   /** Handles message to unmute inbound RTP in audio bridge */
   OsStatus handleUnmuteInputConnection(const AcUnmuteInputConnectionMsg& rMsg);

   // needs m_memberMutex locked
   XSipConnection* m_pSipConnection;
};

#endif // XCpCall_h__
