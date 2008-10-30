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
#include <cp/XCpAbstractCall.h>

// DEFINES
// MACROS
// EXTERNAL FUNCTIONS
// CONSTANTS
// STRUCTS
// TYPEDEFS
// MACROS
// FORWARD DECLARATIONS

/**
 * XCpConference wraps several XSipConnections realizing conference functionality.
 */
class XCpConference : public XCpAbstractCall
{
   /* //////////////////////////// PUBLIC //////////////////////////////////// */
public:
   /* ============================ CREATORS ================================== */

   XCpConference(const UtlString& sId,
                 SipUserAgent& rSipUserAgent,
                 CpMediaInterfaceFactory& rMediaInterfaceFactory,
                 OsMsgQ& rCallManagerQueue);

   virtual ~XCpConference();

   /* ============================ MANIPULATORS ============================== */

   /** Connects call to given address. Uses supplied sip call-id. */
   virtual OsStatus connect(const UtlString& sSipCallId,
                            SipDialog& sSipDialog,
                            const UtlString& toAddress,
                            const UtlString& fullLineUrl,
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
   virtual OsStatus redirectConnection(const UtlString& sRedirectSipUri);

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
   virtual OsStatus dropConnection(const SipDialog& sSipDialog);

   /** Disconnects all calls */
   OsStatus dropAllConnections(UtlBoolean bDestroyConference = FALSE);

   /** Blind transfer given call to sTransferSipUri. Works for simple call and call in a conference */
   virtual OsStatus transferBlind(const SipDialog& sSipDialog,
                                  const UtlString& sTransferSipUri);

   /**
   * Put the specified terminal connection on hold.
   *
   * Change the terminal connection state from TALKING to HELD.
   * (With SIP a re-INVITE message is sent with SDP indicating
   * no media should be sent.)
   */
   virtual OsStatus holdConnection(const SipDialog& sSipDialog);

   /**
   * Convenience method to put all of the terminal connections in
   * the specified conference on hold.
   */
   OsStatus holdAllConnections();

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
   * Convenience method to take the terminal connection off hold.
   *
   * Change the terminal connection state from HELD to TALKING.
   * (With SIP a re-INVITE message is sent with SDP indicating
   * media should be sent.)
   */
   virtual OsStatus unholdConnection(const SipDialog& sSipDialog);

   /**
   * Enables discarding of inbound RTP for given call
   * or conference. Useful for server applications without mic/speaker.
   */
   virtual OsStatus silentHoldRemoteConnection(const SipDialog& sSipDialog);

   /**
   * Enables discarding of inbound RTP for given conference.
   * Useful for server applications without mic/speaker.
   */
   virtual OsStatus silentHoldRemoteAllConnections();

   /**
   * Disables discarding of inbound RTP for given call
   * or conference. Useful for server applications without mic/speaker.
   */
   virtual OsStatus silentUnholdRemoteConnection(const SipDialog& sSipDialog);

   /**
   * Disables discarding of inbound RTP for given conference.
   * Useful for server applications without mic/speaker.
   */
   virtual OsStatus silentUnholdRemoteAllConnections();

   /**
   * Stops outbound RTP for given call or conference.
   * Useful for server applications without mic/speaker.
   */
   virtual OsStatus silentHoldLocalConnection(const SipDialog& sSipDialog);

   /**
   * Starts outbound RTP for given call or conference.
   * Useful for server applications without mic/speaker.
   */
   virtual OsStatus silentUnholdLocalConnection(const SipDialog& sSipDialog);

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
   virtual OsStatus renegotiateCodecsConnection(const SipDialog& sSipDialog,
                                                CP_AUDIO_BANDWIDTH_ID audioBandwidthId,
                                                const UtlString& sAudioCodecs,
                                                CP_VIDEO_BANDWIDTH_ID videoBandwidthId,
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
   OsStatus renegotiateCodecsAllConnections(CP_AUDIO_BANDWIDTH_ID audioBandwidthId,
                                            const UtlString& sAudioCodecs,
                                            CP_VIDEO_BANDWIDTH_ID videoBandwidthId,
                                            const UtlString& sVideoCodecs);

   /** Sends an INFO message to the other party(s) on the call */
   virtual OsStatus sendInfo(const SipDialog& sSipDialog,
                             const UtlString& sContentType,
                             const char* pContent,
                             const size_t nContentLength);

   /* ============================ ACCESSORS ================================= */

   /* ============================ INQUIRY =================================== */

   /**
   * Checks if this conference has given sip dialog.
   */
   virtual DialogMatchEnum hasSipDialog(const SipDialog& sSipDialog) const;

   /** Gets the number of sip connections in this call */
   virtual int getCallCount() const;

   /** Gets sip call-id of conference if its available */
   OsStatus getConferenceSipCallIds(UtlSList& sipCallIdList) const;

   /** Gets audio energy levels for call */
   virtual OsStatus getAudioEnergyLevels(int& iInputEnergyLevel,
                                         int& iOutputEnergyLevel) const;

   /** Gets remote user agent for call or conference */
   virtual OsStatus getRemoteUserAgent(const SipDialog& sSipDialog,
                                       UtlString& userAgent) const;

   /** Gets internal id of media connection for given call or conference. Only for unit tests */
   virtual OsStatus getMediaConnectionId(int& mediaConnID) const;

   /** Gets copy of SipDialog for given call */
   virtual OsStatus getSipDialog(const SipDialog& sSipDialog,
                                 SipDialog& sOutputSipDialog) const;

   /* //////////////////////////// PROTECTED ///////////////////////////////// */
protected:

   /* //////////////////////////// PRIVATE /////////////////////////////////// */
private:
   XCpConference(const XCpConference& rhs);

   XCpConference& operator=(const XCpConference& rhs);

};

#endif // XCpConference_h__
