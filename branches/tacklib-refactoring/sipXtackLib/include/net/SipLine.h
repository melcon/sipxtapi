//
// Copyright (C) 2004-2006 SIPfoundry Inc.
// Licensed by SIPfoundry under the LGPL license.
//
// Copyright (C) 2004-2006 Pingtel Corp.  All rights reserved.
// Licensed to SIPfoundry under a Contributor Agreement.
//
// $$
///////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_SIPLINE_H__6AD8439B_AA8A_4F09_B5C4_44A3BA9C7AC6__INCLUDED_)
#define AFX_SIPLINE_H__6AD8439B_AA8A_4F09_B5C4_44A3BA9C7AC6__INCLUDED_

// SYSTEM INCLUDES
//#include <...>

// APPLICATION INCLUDES
#include <utl/UtlHashBag.h>
#include "HttpMessage.h"
#include "net/Url.h"


// DEFINES
// MACROS
// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
// STRUCTS
// TYPEDEFS

class SipLineCredentials;

//STATE TRANSITION//
/*
                                                        |----1/2refresh time--|
                                                        |--------------refresh time-------------|
 ---------             ------------           --------          ---------
 | TRYING| --------->  |REGISTERED| --------->|FAILED| -------->|EXPIRED|
 ---------             ------------           --------          ---------
        |                                                       |       ^                                                               ^
        |                                                       |___|
        |                                                           |                                                                    |
    |______________________failed the first time____________________|

*/

/**
 * Class representing a SIP line. Has identity URI, credentials, contact associated with it.
 */
class SipLine
{
   /* //////////////////////////// PUBLIC //////////////////////////////////// */
public:

   //utility functions
   typedef enum
   {
      LINE_STATE_UNKNOWN  = 0,
      LINE_STATE_REGISTERED,  ///< sucessfull registeration , registration no expried on server
      LINE_STATE_DISABLED,    ///< not registering
      LINE_STATE_FAILED,      ///< failed registration
      LINE_STATE_PROVISIONED, ///< dont send registration , but enabled because server provisioned it.
      LINE_STATE_TRYING,      ///< registration message sent , awaiting response
      LINE_STATE_EXPIRED      ///< registration expried on server
   } LineStates;

   /* ============================ CREATORS ================================== */
   ///@name Creators
   //@{

   SipLine(const Url& userEnteredUrl = "", ///< url with parameters like transport=tls
           const Url& identityUri = "", ///< uri, without parameters
           const UtlString& user = "",
           int state = LINE_STATE_UNKNOWN);

   virtual ~SipLine();
   SipLine(const SipLine& rSipLine);
   SipLine& operator=(const SipLine& rSipLine);

   //@}

   /* ============================ MANIPULATORS ============================== */
   ///@name Manipulators
   //@{
   //@}

   /* ============================ ACCESSORS ================================= */
   ///@name Accessors
   //@{
   UtlString getLineId() const;

   int getState() const;
   void setState(int state);

   //who does the line belong to
   UtlString getUser() const;
   void setUser(const UtlString& user);

   //line identily or name - just the Uri
   void getIdentityAndUrl(Url &identityUri, Url &userEnteredUrl) const;
   void setIdentityAndUrl(Url identityUri, Url userEnteredUrl);
   Url getUserEnteredUrl() const;
   Url getIdentity() const;
   Url getCanonicalUrl() const;

   //for use from GUI
   int getNumOfCredentials() const;

   UtlBoolean addCredentials(const UtlString& strRealm,
                             const UtlString& strUserID,
                             const UtlString& strPassword,
                             const UtlString& strType);
   UtlBoolean addCredentials(const SipLineCredentials& lineCredentials);

   ///< Gets credentials for given realm. Type is currently ignored.
   UtlBoolean getCredentials(const UtlString& type,
                             const UtlString& realm,
                             UtlString& userID,
                             UtlString& MD5_token) const;

   UtlBoolean getCredentials(const UtlString& type,
                             const UtlString& realm,
                             SipLineCredentials& lineCredentials) const;

   UtlBoolean getAllCredentials(int maxEnteries,
                                int& actualEnteries,
                                UtlString realm[],
                                UtlString userId[],
                                UtlString type[],
                                UtlString passdigest[]) const;

   //removes credetials for a particular realm
   UtlBoolean removeCredential(const UtlString& realm);
   //removes all credentials for this line
   void removeAllCredentials();

   void setPreferredContact(const UtlString& contactAddress, int contactPort);
   //: Set the preferred host/ip for the contact in subsequent registers
   UtlBoolean getPreferredContactUri(Url& preferredContactUri) const;
   ///< Get Preferred host/ip for the contact in subsequent registers. Returns true if contact contains hostname.

   //@}
   /* ============================ INQUIRY =================================== */
   ///@name Inquiry
   //@{

   UtlBoolean isDeviceLine() const;
   //: Determine if this line is a device line.  Presently, a line is
   //  considered a device line if it's user is "Device"

   ///< Whether credentials for this realm already exist.
   UtlBoolean isDuplicateRealm(const UtlString& realm) const;

   //@}

   /* //////////////////////////// PROTECTED ///////////////////////////////// */
protected:

   Url m_identityUri; /// <line key which is the canonical URL stripped of display name, angle brackets and field parameters (basically the URI).
   Url m_userEnteredUrl; ///< user entered URL string (could be incomplete "sip:444@")
   Url m_canonicalUrl; ///< The canonical URL which is the URL string with the host and port filled in if they were missing.

   UtlString m_user;
   UtlString m_lineId;
   int m_currentState;
   Url m_preferredContactUri;

   mutable UtlHashBag m_credentials;

   void copyCredentials(const SipLine& rSipLine);
   void generateLineID(UtlString& lineId);
   Url buildLineContact(const UtlString& address = NULL, int port = PORT_NONE);

   /* //////////////////////////// PRIVATE /////////////////////////////////// */
private:
};

#endif // !defined(AFX_SIPLINE_H__6AD8439B_AA8A_4F09_B5C4_44A3BA9C7AC6__INCLUDED_)
