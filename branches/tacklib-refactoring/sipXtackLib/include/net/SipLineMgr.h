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
// APPLICATION INCLUDES
#include "os/OsServerTask.h"
#include "os/OsMutex.h"
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

   /** Line manager destructor */
   virtual ~SipLineMgr();

   /* ============================ MANIPULATORS ============================== */

   /** Starts line manager server task */
   void startLineMgr();

   /** Adds new sip line to line manager. */
   UtlBoolean addLine(SipLine& line);

   /** Registers line identified by lineURI. */
   UtlBoolean registerLine(const Url& lineURI);

   /** Unregisters line identified by lineURI. */
   UtlBoolean unregisterLine(const Url& lineURI);

   /** Deletes line from line manager */
   UtlBoolean deleteLine(const Url& lineURI);

   /** Deletes all lines */
   void deleteAllLines();

   UtlBoolean buildAuthenticatedRequest(const SipMessage* response,
                                        const SipMessage* request,
                                        SipMessage* newAuthRequest);

   /** Sets state on given line */
   UtlBoolean setStateForLine(const Url& lineUri, SipLine::LineStates state);

   /** Adds new credentials to given line */
   UtlBoolean addCredentialForLine(const Url& lineUri,
                                   const SipLineCredential& credential);

   /** Adds new credentials to given line */
   UtlBoolean addCredentialForLine(const Url& lineUri,
                                   const UtlString& strRealm,
                                   const UtlString& strUserID,
                                   const UtlString& strPasswd,
                                   const UtlString& type);

   /** Deletes credentials for given realm from given line */
   UtlBoolean deleteCredentialForLine(const Url& lineUri,
                                      const UtlString& strRealm,
                                      const UtlString& type);

   /** Deletes all credentials from given line */
   UtlBoolean deleteAllCredentialsForLine(const Url& lineUri);

   /* ============================ ACCESSORS ================================= */

   /** Copies SipLine clones into supplied list */
   void getLineCopies(UtlSList& lineList) const;

   /** Gets SIP lineURIs of all SipLines */
   void getLineUris(UtlSList& lineUris) const;

   /** Gets a copy of given line including credentials. Returns FALSE if line was not found */
   UtlBoolean getLineCopy(const Url& lineUri, SipLine& sipLine) const;

   /** Get the current number of lines. */
   size_t getNumLines() const;

   /* ============================ INQUIRY =================================== */

   UtlBoolean isUserIdDefined(const SipMessage* request) const;


   /* //////////////////////////// PROTECTED ///////////////////////////////// */
protected:

   /** Copy constructor */
   SipLineMgr(const SipLineMgr& rSipLineMgr);

   /** Assignment operator */
   SipLineMgr& operator=(const SipLineMgr& rhs);

   /** Handles message sent to this OsServerTask */
   UtlBoolean handleMessage(OsMsg& eventMessage);

   SipLine* getLineforAuthentication(const SipMessage* request,
                                     const SipMessage* response,
                                     UtlBoolean isIncomingRequest = FALSE) const;

   /* //////////////////////////// PRIVATE /////////////////////////////////// */
private:

   /** Prints all lines in line manager */
   void dumpLines();

   SipRefreshMgr* m_pRefreshMgr; ///< refresh manager, resends line register messages
   mutable OsMutex m_mutex; ///< mutex for concurrent access
   mutable SipLineList m_listList; ///< list of SipLine objects
};

/* ============================ INLINE METHODS ============================ */

#endif  // _SipLineMgr_h_
