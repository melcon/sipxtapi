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

//:Class short description which may consist of multiple lines (note the ':')
// Class detailed description which may extend to multiple lines
class SipLineMgr : public OsServerTask
{
/* //////////////////////////// PUBLIC //////////////////////////////////// */
public:

/* ============================ CREATORS ================================== */

   SipLineMgr(const char* authenticationScheme = HTTP_DIGEST_AUTHENTICATION);
     //:Default constructor

   SipLineMgr(const SipLineMgr& rSipLineMgr);
     //:Copy constructor

   virtual ~SipLineMgr();
     //:Destructor

/* ============================ MANIPULATORS ============================== */

   void StartLineMgr();

   UtlBoolean initializeRefreshMgr( SipRefreshMgr * refreshMgr );

   void setDefaultContactUri(const Url& contactUri);

   UtlBoolean addLine(SipLine& line,
                      UtlBoolean doEnable = TRUE);

   void deleteLine(const Url& identity);

   void setDefaultOutboundLine( const Url& outboundLine );

   UtlBoolean enableLine(const Url& lineURI);

   void disableLine(const Url& identity,
                    UtlBoolean onStartup = FALSE,
                    const UtlString& lineId ="");

   void lineHasBeenUnregistered(const Url& identity);

   UtlBoolean buildAuthenticatedRequest(const SipMessage* response /*[in]*/,
                                       const SipMessage* request /*[in]*/,
                                       SipMessage* newAuthRequest /*[out]*/);

   //
   // Line Manipulators
   //

   void setFirstLineAsDefaultOutBound();

   void setCallHandlingForLine(const Url& identity, UtlBoolean useCallHandling= TRUE);

   void setAutoEnableForLine(const Url& identity, UtlBoolean isAutoEnable = TRUE);

   void setStateForLine(const Url& identity, int state);

   void setVisibilityForLine(const Url& identity, UtlBoolean Visibility = TRUE);

   void setUserForLine(const Url& identity, const UtlString User);

   void setUserEnteredUrlForLine(const Url& identity, UtlString sipUrl);

   UtlBoolean addCredentialForLine(
        const Url& identity,
        const UtlString strRealm,
        const UtlString strUserID,
        const UtlString strPasswd,
        const UtlString type);

   UtlBoolean deleteCredentialForLine(const Url& identity,
                                     const UtlString strRealm );

   //:Removes all SIP message observers for the given message/queue observer
   //!param: messageQueue - All observers dispatching to this message queue
   //        will be removed if the pObserverData is NULL or matches.
   //!param: pObserverData - If null, all observers that match the message
   //        queue will be removed.  Otherwise, only observers that match
   //        both the message queue and observer data will be removed.
   //!returns TRUE if one or more observers are removed otherwise FALSE.

        void notifyChangeInLineProperties(Url& identity);

   void notifyChangeInOutboundLine(Url& identity);

   //
   // Serialization Manipulators
   //

/* ============================ ACCESSORS ================================= */

   void getDefaultOutboundLine( UtlString &rOutBoundLine );

   UtlBoolean getLine(
       const UtlString& toUrl,
       const UtlString& localContact,
       SipLine& sipline ) const;
    //:Get the line identified by the designated To and Local Contact URLs.
    //
    //!returns The line identified by the designated To and Local Contact
    //         URLs or NULL if not found.

   UtlBoolean getLines(size_t maxLines /*[in]*/,
                       size_t& actualLines /*[out]*/,
                       SipLine* lines[]/*[in/out]*/) const;

   UtlBoolean getLines(size_t maxLines /*[in]*/,
                       size_t& actualLines /*[in/out]*/,
                       SipLine lines[]/*[in/out]*/) const;

   int getNumLines () const;
   //:Get the current number of lines.

   int getNumOfCredentialsForLine( const Url& identity ) const;

   UtlBoolean getCredentialListForLine(
        const Url& identity,
        int maxEnteries,
        int &actualEnteries,
        UtlString realmList[],
        UtlString userIdList[],
        UtlString typeList[],
        UtlString passTokenList[] );

   UtlBoolean getCallHandlingForLine( const Url& identity ) const;

   UtlBoolean getEnableForLine(const Url& identity) const;

   int getStateForLine(const Url& identity ) const;

   UtlBoolean getVisibilityForLine(const Url& identity ) const;

   UtlBoolean getUserForLine(const Url& identity, UtlString &User) const;

   UtlBoolean getUserEnteredUrlForLine( const Url& identity, UtlString &sipUrl) const;

   UtlBoolean getCanonicalUrlForLine(const Url& identity, UtlString &sipUrl) const ;

/* ============================ INQUIRY =================================== */

   UtlBoolean isUserIdDefined( const SipMessage* request /*[in]*/) const;


/* //////////////////////////// PROTECTED ///////////////////////////////// */
protected:

   SipLineMgr& operator=(const SipLineMgr& rhs);
     //:Assignment operator

    UtlBoolean handleMessage(OsMsg& eventMessage);

    void removeFromList(SipLine* line);

    void addLineToList(SipLine& line);

    SipLine* getLineforAuthentication(
        const SipMessage* request /*[in]*/,
        const SipMessage* response /*[in]*/,
        UtlBoolean isIncomingRequest = FALSE) const;

/* //////////////////////////// PRIVATE /////////////////////////////////// */
private:
    void dumpLines();

    UtlBoolean mIsStarted;
    UtlString mAuthenticationScheme;

    SipRefreshMgr* mpRefreshMgr;
    Url mOutboundLine;
    Url mDefaultContactUri;

    UtlHashBag mMessageObservers;
    OsRWMutex mObserverMutex;

    // line list and temp line lists
    mutable SipLineList  sLineList;
};

/* ============================ INLINE METHODS ============================ */

#endif  // _SipLineMgr_h_
