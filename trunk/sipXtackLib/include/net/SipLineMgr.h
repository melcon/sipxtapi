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
#include "net/SipLineEvent.h"
#include "net/HttpMessage.h"

// DEFINES
#define MAX_LINES                       32                  // Max number of lines
#define MAX_CREDENTIALS                 32                  // Max number of credentials per line

#define BASE_PHONESET_LINE_KEY          "PHONESET_LINE."    // Base key for device lines
#define BASE_USER_LINE_KEY              "USER_LINE."        // Base key for user lines
#define USER_DEFAULT_OUTBOUND_LINE      "USER_DEFAULT_OUTBOUND_LINE" // Default outbound line

#define LINE_REGISTRATION_PROVISION     "PROVISION"         // Registration value: Provision
#define LINE_REGISTRATION_REGISTER      "REGISTER"          // Registration value: Register
#define LINE_ALLOW_FORWARDING_ENABLE    "ENABLE"            // Allow Forwarding value: Enable
#define LINE_ALLOW_FORWARDING_DISABLE   "DISABLE"           // Allow Forwarding value: Disable
#define LINE_CONTACT_TYPE_LOCAL         "LOCAL"             // Use local contact/IP
#define LINE_CONTACT_TYPE_NAT_MAPPED    "NAT_MAPPED"        // Use NAT-derived contact/IP

#define LINE_PARAM_LINEID               "LINEID"           // lineID
#define LINE_PARAM_URL                  "URL"               // Line Parameter: identity/url
#define LINE_PARAM_REGISTRATION         "REGISTRATION"      // Line Parameter: registration method
#define LINE_PARAM_ALLOW_FORWARDING     "ALLOW_FORWARDING"  // Line Parameter: allow call forwarding
#define LINE_PARAM_CONTACT_TYPE         "CONTACT_TYPE"      // Contact type (LOCAL or NAT_MAPPED)
#define LINE_PARAM_CREDENTIAL           "CREDENTIAL."       // Line Parameter: credential sub key
#define LINE_PARAM_CREDENTIAL_REALM     "REALM"             // Credential Parameter: realm
#define LINE_PARAM_CREDENTIAL_USERID    "USERID"            // Credential Parameter: userid
#define LINE_PARAM_CREDENTIAL_PASSTOKEN "PASSTOKEN"         // Credential Parameter: Pass token

#define DEFAULT_LINE_PARAM_PHONESET_LINE    "PHONESET_LINE" // Outbound Line Param: Device line
#define DEFAULT_LINE_PARAM_BASE_USER_LINE   "USER_LINE."    // Outbound Line Param: User line

// MACROS
// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
// STRUCTS
// TYPEDEFS
// FORWARD DECLARATIONS
class OsConfigDb;
class SipRefreshMgr;

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

   virtual
   ~SipLineMgr();
     //:Destructor

/* ============================ MANIPULATORS ============================== */

   void StartLineMgr();

   UtlBoolean initializeRefreshMgr( SipRefreshMgr * refreshMgr );

   void setDefaultContactUri(const Url& contactUri);

   void setOwner(const UtlString& owner);

   UtlBoolean addLine(SipLine& line,
                      UtlBoolean doEnable = TRUE);

   void deleteLine(const Url& identity);

   void setDefaultOutboundLine( const Url& outboundLine );

   UtlBoolean enableLine(const Url& lineURI);

   void disableLine(const Url& identity,
                    UtlBoolean onStartup = FALSE,
                    const UtlString& lineId ="");

   void lineHasBeenUnregistered(const Url& identity);

   void enableAllLines();

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

   //
   // Listener/Observer Manipulators
   //

   void addMessageObserver(OsMsgQ& messageQueue,
                            void* observerData = NULL );

   UtlBoolean removeMessageObserver(OsMsgQ& messageQueue,
                                   void* pObserverData = NULL );
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

   void storeLine( OsConfigDb* pConfigDb, UtlString strSubKey, SipLine line);
   //:Stores the specified line to the configuration database under the
   // passed key.
   //!param: (in) pConfigDb - Configuration database to save to.
   //!param: (in) strSubKey - ROOT sub key of configuration. For example
   //        "USER_LINE.1."
   //!param: (in) line - The line to be serialized

   UtlBoolean loadLine(OsConfigDb* pConfigDb, UtlString strSubkey, SipLine& line);
   //:Loads a line from the configuration database given the specified sub key.
   //!param: (in) pConfigDb - Configuration database to loaded from.
   //!param: (in) strSubKey - ROOT sub key of configuration. For example
   //        "USER_LINE.1."
   //!param: (out) line - The line to be serialized
   //!returns: TRUE if successfully loaded otherwise false.

   void purgeLines(OsConfigDb* pConfigDb) ;
   //:Removes all device and user lines from the configuration database.
   //!param: (in) pConfigDb - Configuration database to be cleared.

/* ============================ ACCESSORS ================================= */

   const UtlString& getOwner() const;

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

    void queueMessageToObservers(SipLineEvent& event);

    void removeFromList(SipLine* line);

    void addLineToList(SipLine& line);

    SipLine* getLineforAuthentication(
        const SipMessage* request /*[in]*/,
        const SipMessage* response /*[in]*/,
        const UtlBoolean& isIncomingRequest = FALSE,
        const UtlBoolean& fromTempList = FALSE ) const;

    // temp list for storing credentials for deleted lines
    void removeFromTempList( SipLine* line );

    void addToTempList( SipLine *line );

    void storeCredential(
        OsConfigDb *pConfigDb,
        UtlString strSubKey,
        UtlString strRealm,
        UtlString strUserId,
        UtlString strPassToken,
        UtlString strType );
    //:Stores a single set of credentials under the passed key
    //!param: (in) pConfigDb - Configuration database to saved to.
    //!param: (in) strSubKey - ROOT sub key of configuration. For example
    //        "USER_LINE.1.CREDENTIAL.1."
    //!param: (in) strRealm - Realm for the Credential.
    //!param: (in) strUserId - User ID for the Credential.
    //!param: (in) strPassToken - Pass Token for the Credential.
    //!param: (in) strType - Authentication Type for the Credential.

    UtlBoolean loadCredential(
        OsConfigDb* pConfigDb,
        UtlString strSubKey,
        SipLine& line) ;
    //:Loads a credential from the configuration db which is identified by
    // the specified sub key.
    //!param: (in) pConfigDb - Configuration database to saved to.
    //!param: (in) strSubKey - ROOT sub key of configuration. For example
    //        "USER_LINE.1.CREDENTIAL.1."
    //!param: (in/out) line - The line to populate the credentials into.
    //!returns TRUE if successfully loaded otherwise FALSE.

/* //////////////////////////// PRIVATE /////////////////////////////////// */
private:
    void dumpLines();

    // MsgType categories defined for use by the system
    enum LineMsgTypes
    {
        UNSPECIFIED = 0,
        START_LINE_MGR
    };

    UtlBoolean mIsStarted;
    UtlString mAuthenticationScheme;
    UtlString mAuthenticationRealm;
    OsConfigDb* mpAuthenticationDb;
    OsConfigDb* mpAuthorizationUserIds;
    OsConfigDb* mpAuthorizationPasswords;

    SipRefreshMgr* mpRefreshMgr;
    UtlString mOwner;
    Url mOutboundLine;
    Url mDefaultContactUri;

    UtlHashBag mMessageObservers;
    OsRWMutex mObserverMutex;

    // line list and temp line lists
    mutable SipLineList  sLineList;
    mutable SipLineList  sTempLineList;
};

/* ============================ INLINE METHODS ============================ */

#endif  // _SipLineMgr_h_
