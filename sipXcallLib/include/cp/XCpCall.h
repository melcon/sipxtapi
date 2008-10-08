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

/**
 * XCpCall wraps XSipConnection realizing all call functionality.
 */
class XCpCall : public XCpAbstractCall
{
   /* //////////////////////////// PUBLIC //////////////////////////////////// */
public:
   /* ============================ CREATORS ================================== */

   XCpCall(const UtlString& sId);

   virtual ~XCpCall();

   /* ============================ MANIPULATORS ============================== */

   /** Connects call to given address. Uses supplied sip call-id. */
   virtual OsStatus connect(const UtlString& sSipCallId,
                            const UtlString& toAddress,
                            const UtlString& lineURI,
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
   virtual OsStatus redirectConnection(const UtlString& sRedirectSipUri);

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
   virtual OsStatus dropConnection(const UtlString& sSipCallId,
                                   const UtlString& sLocalTag,
                                   const UtlString& sRemoteTag);

   /** Disconnects call without knowing the sip call-id*/
   OsStatus dropConnection();

   /** Sends an INFO message to the other party(s) on the call */
   virtual OsStatus sendInfo(const UtlString& sSipCallId,
                             const UtlString& sLocalTag,
                             const UtlString& sRemoteTag,
                             const UtlString& sContentType,
                             const UtlString& sContentEncoding,
                             const UtlString& sContent);

   /* ============================ ACCESSORS ================================= */

   /* ============================ INQUIRY =================================== */

   /**
   * Checks if this call has given sip dialog.
   */
   virtual UtlBoolean hasSipDialog(const UtlString& sSipCallId,
                                   const UtlString& sLocalTag = NULL,
                                   const UtlString& sRemoteTag = NULL) const;

   /** Gets the number of sip connections in this call */
   virtual int getCallCount() const;

   /** Gets sip call-id of call if its available */
   OsStatus getCallSipCallId(UtlString& sSipCallId) const;

   /** Gets audio energy levels for call */
   virtual OsStatus getAudioEnergyLevels(int& iInputEnergyLevel,
                                         int& iOutputEnergyLevel) const;

   /** Gets remote user agent for call or conference */
   virtual OsStatus getRemoteUserAgent(const UtlString& sSipCallId,
                                       const UtlString& sLocalTag,
                                       const UtlString& sRemoteTag,
                                       UtlString& userAgent) const;

   /** Gets internal id of media connection for given call or conference. Only for unit tests */
   virtual OsStatus getMediaConnectionId(int& mediaConnID) const;

   /** Gets copy of SipDialog for given call */
   virtual OsStatus getSipDialog(const UtlString& sSipCallId,
                                 const UtlString& sLocalTag,
                                 const UtlString& sRemoteTag,
                                 SipDialog& dialog) const;

   /* //////////////////////////// PROTECTED ///////////////////////////////// */
protected:

   /* //////////////////////////// PRIVATE /////////////////////////////////// */
private:
   XCpCall(const XCpCall& rhs);

   XCpCall& operator=(const XCpCall& rhs);
};

#endif // XCpCall_h__
