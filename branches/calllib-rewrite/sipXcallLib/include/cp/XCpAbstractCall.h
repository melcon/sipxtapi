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

#ifndef XCpAbstractCall_h__
#define XCpAbstractCall_h__

// SYSTEM INCLUDES
// APPLICATION INCLUDES
#include <os/OsDefs.h>
#include <os/OsMutex.h>
#include <os/OsSyncBase.h>
#include <os/OsServerTask.h>
#include <utl/UtlContainable.h>
#include <cp/CpDefs.h>
#include <cp/CpAudioCodecInfo.h>
#include <cp/CpVideoCodecInfo.h>

// DEFINES
// MACROS
// EXTERNAL FUNCTIONS
// CONSTANTS
// STRUCTS
// TYPEDEFS
// MACROS
// FORWARD DECLARATIONS
class SipDialog;
class SipUserAgent;
class CpMediaInterfaceFactory;
class CpCallStateEventListener;
class SipInfoStatusEventListener;
class SipSecurityEventListener;
class CpMediaEventListener;

/**
 * XCpAbstractCall is the top class for XCpConference and XCpCall providing
 * common functionality. This class can be stored in Utl containers.
 * Inherits from OsSyncBase, and can be locked externally. Locking the object ensures
 * that its state doesn't change.
 *
 * Most public methods must acquire the object mutex first.
 */
class XCpAbstractCall : public OsServerTask, public UtlContainable, public OsSyncBase
{
   /* //////////////////////////// PUBLIC //////////////////////////////////// */
public:
   static const UtlContainableType TYPE; /** < Class type used for runtime checking */ 

   typedef enum
   {
      ESTABLISHED_MATCH = 0,
      NOT_ESTABLISHED_MATCH,
      MISMATCH
   } DialogMatchEnum;

   /* ============================ CREATORS ================================== */

   XCpAbstractCall(const UtlString& sId,
                   SipUserAgent& rSipUserAgent,
                   CpMediaInterfaceFactory& rMediaInterfaceFactory);

   virtual ~XCpAbstractCall();

   /* ============================ MANIPULATORS ============================== */

   virtual UtlBoolean handleMessage(OsMsg& rRawMsg);

   /** Connects call to given address. Uses supplied sip call-id. */
   virtual OsStatus connect(const UtlString& sSipCallId,
                            SipDialog& sSipDialog,
                            const UtlString& toAddress,
                            const UtlString& fullLineUrl,
                            const UtlString& locationHeader,
                            CP_CONTACT_ID contactId) = 0;

   /** 
   * Accepts inbound call connection. Inbound connections can only be part of XCpCall
   *
   * Progress the connection from the OFFERING state to the
   * RINGING state. This causes a SIP 180 Ringing provisional
   * response to be sent.
   */
   virtual OsStatus acceptConnection(const UtlString& locationHeader,
                                     CP_CONTACT_ID contactId) = 0;

   /**
   * Reject the incoming connection.
   *
   * Progress the connection from the OFFERING state to
   * the FAILED state with the cause of busy. With SIP this
   * causes a 486 Busy Here response to be sent.
   */
   virtual OsStatus rejectConnection() = 0;

   /**
   * Redirect the incoming connection.
   *
   * Progress the connection from the OFFERING state to
   * the FAILED state. This causes a SIP 302 Moved
   * Temporarily response to be sent with the specified
   * contact URI.
   */
   virtual OsStatus redirectConnection(const UtlString& sRedirectSipUri) = 0;

   /**
   * Answer the incoming terminal connection.
   *
   * Progress the connection from the OFFERING or RINGING state
   * to the ESTABLISHED state and also creating the terminal
   * connection (with SIP a 200 OK response is sent).
   */
   virtual OsStatus answerConnection() = 0;

   /**
    * Disconnects given call with given sip call-id
    *
    * The appropriate disconnect signal is sent (e.g. with SIP BYE or CANCEL).  The connection state
    * progresses to disconnected and the connection is removed.
    */
   virtual OsStatus dropConnection(const SipDialog& sSipDialog) = 0;

   /** Blind transfer given call to sTransferSipUri. Works for simple call and call in a conference */
   virtual OsStatus transferBlind(const SipDialog& sSipDialog,
                                  const UtlString& sTransferSipUri) = 0;

   /** Starts DTMF tone on call connection.*/
   OsStatus audioToneStart(int iToneId,
                           UtlBoolean bLocal,
                           UtlBoolean bRemote);

   /** Stops DTMF tone on call connection */
   OsStatus audioToneStop();

   /** Starts playing audio file on call connection */
   OsStatus audioFilePlay(const UtlString& audioFile,
                          UtlBoolean bRepeat,
                          UtlBoolean bLocal,
                          UtlBoolean bRemote,
                          UtlBoolean bMixWithMic = FALSE,
                          int iDownScaling = 100,
                          void* pCookie = NULL);

   /** Starts playing audio buffer on call connection. Passed buffer will be copied internally. */
   OsStatus audioBufferPlay(const void* pAudiobuf,
                            size_t iBufSize,
                            int iType,
                            UtlBoolean bRepeat,
                            UtlBoolean bLocal,
                            UtlBoolean bRemote,
                            void* pCookie = NULL);

   /** Stops playing audio file or buffer on call connection */
   OsStatus audioStop();

   /** Pauses audio playback of file or buffer. */
   OsStatus pauseAudioPlayback();

   /** Resumes audio playback of file or buffer */
   OsStatus resumeAudioPlayback();

   /** Starts recording call or conference. */
   OsStatus audioRecordStart(const UtlString& sFile);

   /** Stops recording call or conference. */
   OsStatus audioRecordStop();

   /**
   * Put the specified terminal connection on hold.
   *
   * Change the terminal connection state from TALKING to HELD.
   * (With SIP a re-INVITE message is sent with SDP indicating
   * no media should be sent.)
   */
   virtual OsStatus holdConnection(const SipDialog& sSipDialog) = 0;

   /**
   * Convenience method to put the local terminal connection on hold.
   * Can be used for both calls and conferences.
   * Microphone will be disconnected from the call or conference, local
   * audio will stop flowing to remote party. Remote party will still
   * be audible to local user.
   */
   OsStatus holdLocalConnection();

   /**
   * Take the specified local terminal connection off hold,.
   *
   * Microphone will be reconnected to the call or conference,
   * and audio will start flowing from local machine to remote party.
   */
   OsStatus unholdLocalConnection();

   /**
   * Convenience method to take the terminal connection off hold.
   *
   * Change the terminal connection state from HELD to TALKING.
   * (With SIP a re-INVITE message is sent with SDP indicating
   * media should be sent.)
   */
   virtual OsStatus unholdConnection(const SipDialog& sSipDialog) = 0;

   /**
   * Enables discarding of inbound RTP for given call
   * or conference. Useful for server applications without mic/speaker.
   */
   virtual OsStatus silentHoldRemoteConnection(const SipDialog& sSipDialog) = 0;

   /**
   * Disables discarding of inbound RTP for given call
   * or conference. Useful for server applications without mic/speaker.
   */
   virtual OsStatus silentUnholdRemoteConnection(const SipDialog& sSipDialog) = 0;

   /**
   * Stops outbound RTP for given call or conference.
   * Useful for server applications without mic/speaker.
   */
   virtual OsStatus silentHoldLocalConnection(const SipDialog& sSipDialog) = 0;

   /**
   * Starts outbound RTP for given call or conference.
   * Useful for server applications without mic/speaker.
   */
   virtual OsStatus silentUnholdLocalConnection(const SipDialog& sSipDialog) = 0;

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
                                          const UtlString& sVideoCodecs) = 0;

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
                                                const UtlString& sVideoCodecs) = 0;


   /** Sends an INFO message to the other party(s) on the call */
   virtual OsStatus sendInfo(const SipDialog& sSipDialog,
                             const UtlString& sContentType,
                             const char* pContent,
                             const size_t nContentLength) = 0;

   /** Block until the sync object is acquired or the timeout expires */
   virtual OsStatus acquire(const OsTime& rTimeout = OsTime::OS_INFINITY);

   /** Conditionally acquire the semaphore (i.e., don't block) */
   virtual OsStatus tryAcquire();

   /** Release the sync object */
   virtual OsStatus release();

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
    * Gets Id of the abstract call.
    */
   UtlString getId() const;

   /** Sets event listener for call events */
   void setCallEventListener(CpCallStateEventListener* val) { m_pCallEventListener = val; }

   /** Sets event listener for sip info events */
   void setInfoStatusEventListener(SipInfoStatusEventListener* val) { m_pInfoStatusEventListener = val; }

   /** Sets event listener for security events */
   void setSecurityEventListener(SipSecurityEventListener* val) { m_pSecurityEventListener = val; }

   /** Sets event listener for media events */
   void setMediaEventListener(CpMediaEventListener* val) { m_pMediaEventListener = val; }

   /* ============================ INQUIRY =================================== */

   /**
   * Compare the this object to another like-objects.  Results for 
   * designating a non-like object are undefined.
   *
   * @returns 0 if equal, < 0 if less then and >0 if greater.
   */
   virtual int compareTo(UtlContainable const* inVal) const;

   /**
    * Checks if this abstract call has given sip dialog.
    */
   virtual DialogMatchEnum hasSipDialog(const SipDialog& sSipDialog) const = 0;

   /** Gets the number of sip connections in this call */
   virtual int getCallCount() const = 0;

   /** Gets audio energy levels for call */
   virtual OsStatus getAudioEnergyLevels(int& iInputEnergyLevel,
                                         int& iOutputEnergyLevel) const = 0;

   /** gets remote user agent for call or conference */
   virtual OsStatus getRemoteUserAgent(const SipDialog& sSipDialog,
                                       UtlString& userAgent) const = 0;

   /** Gets internal id of media connection for given call or conference. Only for unit tests */
   virtual OsStatus getMediaConnectionId(int& mediaConnID) const = 0;

   /** Gets copy of SipDialog for given call */
   virtual OsStatus getSipDialog(const SipDialog& sSipDialog,
                                 SipDialog& sOutputSipDialog) const = 0;

   /* //////////////////////////// PROTECTED ///////////////////////////////// */
protected:
   static const int CALL_MAX_REQUEST_MSGS;
   mutable OsMutex m_memberMutex; ///< mutex for member synchronization

   SipUserAgent& m_rSipUserAgent; // for sending sip messages
   CpMediaInterfaceFactory& m_rMediaInterfaceFactory;
   CpCallStateEventListener* m_pCallEventListener; // listener for firing call events
   SipInfoStatusEventListener* m_pInfoStatusEventListener; // listener for firing info events
   SipSecurityEventListener* m_pSecurityEventListener; // listener for firing security events
   CpMediaEventListener* m_pMediaEventListener; // listener for firing media events
   const UtlString m_sId; ///< unique identifier of the abstract call

   /* //////////////////////////// PRIVATE /////////////////////////////////// */
private:
   XCpAbstractCall(const XCpAbstractCall& rhs);

   XCpAbstractCall& operator=(const XCpAbstractCall& rhs);
};

#endif // XCpAbstractCall_h__
