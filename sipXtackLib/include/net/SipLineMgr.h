//
// Copyright (C) 2004-2006 SIPfoundry Inc.
// Licensed by SIPfoundry under the LGPL license.
//
// Copyright (C) 2004-2006 Pingtel Corp.  All rights reserved.
// Licensed to SIPfoundry under a Contributor Agreement.
//
// $$
///////////////////////////////////////////////////////////////////////////////

#ifndef _SipLineMgr_h_
#define _SipLineMgr_h_

// SYSTEM INCLUDES
// #include <...>

// APPLICATION INCLUDES
#include "os/OsServerTask.h"
#include "net/SipLine.h"
#include "net/SipLineList.h"

// DEFINES
// MACROS
// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
// STRUCTS
// TYPEDEFS
// FORWARD DECLARATIONS

class SipRefreshMgr;
class HttpMessage;
class SipMessage;

/**
 * Line management class. Responsible for managing addition, removal,
 * configuration of new lines, and firing line events.
 */
class SipLineMgr : public OsServerTask
{
/* //////////////////////////// PUBLIC //////////////////////////////////// */
public:

/* ============================ CREATORS ================================== */

   SipLineMgr(SipRefreshMgr *refershMgr = NULL);

   virtual ~SipLineMgr();

/* ============================ MANIPULATORS ============================== */

   /** Starts line manager server task */
   void startLineMgr();

   /** Adds new sip line to line manager. */
   UtlBoolean addLine(SipLine& line);

   /** Registers line identified by lineURI. */
   UtlBoolean registerLine(const Url& lineURI);

   /** Unregisters line identified by lineURI. */
   UtlBoolean unregisterLine(const Url& identity,
                             const UtlString& lineId ="");

   UtlBoolean deleteLine(const Url& identity);

   UtlBoolean buildAuthenticatedRequest(const SipMessage* response /*[in]*/,
                                       const SipMessage* request /*[in]*/,
                                       SipMessage* newAuthRequest /*[out]*/);

   //
   // Line Manipulators
   //

   void setStateForLine(const Url& lineUri, int state);

   void setUserForLine(const Url& lineUri, const UtlString User);

   void setUserEnteredUrlForLine(const Url& lineUri, UtlString sipUrl);

   UtlBoolean addCredentialForLine(const Url& lineUri,
                                   const UtlString& strRealm,
                                   const UtlString& strUserID,
                                   const UtlString& strPasswd,
                                   const UtlString& type);

   UtlBoolean deleteCredentialForLine(const Url& lineUri,
                                      const UtlString strRealm);

/* ============================ ACCESSORS ================================= */

   UtlBoolean getLines(size_t maxLines,
                       size_t& actualLines,
                       SipLine* lines[]) const;

   UtlBoolean getLines(size_t maxLines,
                       size_t& actualLines,
                       SipLine lines[]) const;

   int getNumLines () const;
   //:Get the current number of lines.

   int getNumOfCredentialsForLine(const Url& identity) const;

   UtlBoolean getCredentialListForLine(
        const Url& identity,
        int maxEnteries,
        int &actualEnteries,
        UtlString realmList[],
        UtlString userIdList[],
        UtlString typeList[],
        UtlString passTokenList[] );

   int getStateForLine(const Url& identity ) const;

   UtlBoolean getUserForLine(const Url& identity, UtlString &User) const;

   UtlBoolean getUserEnteredUrlForLine( const Url& identity, UtlString &sipUrl) const;

   UtlBoolean getCanonicalUrlForLine(const Url& identity, UtlString &sipUrl) const ;

/* ============================ INQUIRY =================================== */

   UtlBoolean isUserIdDefined( const SipMessage* request /*[in]*/) const;


/* //////////////////////////// PROTECTED ///////////////////////////////// */
protected:

   SipLineMgr(const SipLineMgr& rSipLineMgr);

   SipLineMgr& operator=(const SipLineMgr& rhs);
     //:Assignment operator

    UtlBoolean handleMessage(OsMsg& eventMessage);

    void removeFromList(SipLine& line);

    void addLineToList(SipLine& line);

    SipLine* getLineforAuthentication(
        const SipMessage* request /*[in]*/,
        const SipMessage* response /*[in]*/,
        UtlBoolean isIncomingRequest = FALSE) const;

/* //////////////////////////// PRIVATE /////////////////////////////////// */
private:
    void dumpLines();

    UtlBoolean mIsStarted;

    SipRefreshMgr* mpRefreshMgr;
    Url mOutboundLine;

    // line list and temp line lists
    mutable SipLineList  sLineList;
};

/* ============================ INLINE METHODS ============================ */

#endif  // _SipLineMgr_h_
