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
