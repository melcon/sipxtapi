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

#ifndef SipLine_h__
#define SipLine_h__

// SYSTEM INCLUDES
// APPLICATION INCLUDES
#include <utl/UtlHashBag.h>
#include <utl/UtlCopyableContainable.h>
#include <utl/UtlString.h>
#include "HttpMessage.h"
#include "net/Url.h"

// DEFINES
// MACROS
// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
// STRUCTS
// TYPEDEFS

class SipLineCredential;

/**
 * Class representing a SIP line. Has identity URI, credentials, contact associated with it.
 * Currently only 1 credential per realm is supported of any type.
 * It is not thread safe.
 */
class SipLine : public UtlCopyableContainable
{
   /* //////////////////////////// PUBLIC //////////////////////////////////// */
public:
   static const UtlContainableType TYPE;    /** < Class type used for runtime checking */

   /** Line states */
   typedef enum
   {
      LINE_STATE_UNKNOWN  = 0,
      LINE_STATE_REGISTERED,  ///< successful registration, registration no expired on server
      LINE_STATE_DISABLED,    ///< not registering
      LINE_STATE_FAILED,      ///< failed registration
      LINE_STATE_PROVISIONED, ///< don't send registration, but enabled because server provisioned it.
      LINE_STATE_TRYING,      ///< registration message sent, awaiting response
      LINE_STATE_EXPIRED      ///< registration expired on server
   } LineStates;

   /* ============================ CREATORS ================================== */
   ///@name Creators
   //@{

   SipLine(const Url& userEnteredUrl = "", ///< url with parameters like transport=tls
           const Url& identityUri = "", ///< uri, doesn't contain headerParameters or fieldParameters, display name or brackets
           LineStates state = LINE_STATE_UNKNOWN);

   virtual ~SipLine();
   SipLine(const SipLine& rSipLine);
   SipLine& operator=(const SipLine& rSipLine);

   //@}

   /* ============================ MANIPULATORS ============================== */
   ///@name Manipulators
   //@{

   /**
    * Get the ContainableType for a UtlContainable-derived class.
    */
   virtual UtlContainableType getContainableType() const;

   /** Calculate a hash code for this object. */
   virtual unsigned hash() const;

   /** Compare this object to another object. */
   virtual int compareTo(UtlContainable const *compareContainable) const;

   /** Creates a copy of object */
   virtual UtlCopyableContainable* clone() const;

   /**
    * Constructs line identity uri from given url. <>, field parameters and display name
    * will be cut off. This is used in general for converting To/From header field into line uri
    * for line lookup.
    */
   static Url getLineUri(const Url& url);

   /**
    * Constructs line identity uri from given string.
    */
   static Url getLineUri(const UtlString& sUrl);

   //@}

   /* ============================ ACCESSORS ================================= */
   ///@name Accessors
   //@{

   /** Gets 12 characters uniquely identifying this line */
   UtlString getLineId() const;

   /** Gets state of line */
   LineStates getState() const;

   /** Sets state of line. Changing state doesn't trigger any actions. */
   void setState(LineStates state);

   /** Gets UserId part of userEnteredUrl */
   UtlString getUserId() const;

   /** Gets the user entered url used to construct line. */
   Url getUserEnteredUrl() const;

   /**
    * Gets line identityUri. It doesn't fieldParameters, display name or brackets.
    * But it may contain url parameters like transport=tcp
    */
   Url getIdentityUri() const;

   /** Gets canonical line url. It is basically UserEnteredUrl + host:port from identityUri*/
   Url getCanonicalUrl() const;

   /** Sets line proxy servers. If not set, default proxy servers might be used. */
   UtlString getProxyServers() const;
   void setProxyServers(const UtlString& proxyServers);

   /** Returns number of credentials stored in line */
   int getNumOfCredentials() const;

   /** Adds credentials for this line */
   UtlBoolean addCredential(const UtlString& strRealm,
                            const UtlString& strUserID,
                            const UtlString& strPassword,
                            const UtlString& strType);

   /** Adds credentials for this line */
   UtlBoolean addCredential(const SipLineCredential& lineCredential);

   /** Gets credentials for given realm. Type is currently ignored. */
   UtlBoolean getCredential(const UtlString& type,
                            const UtlString& realm,
                            UtlString& userID,
                            UtlString& passwordMD5Digest) const;

   /**
    * Gets credentials for given realm. This method can be used to access
    * original clear text password from SipLineCredentials.
    */
   UtlBoolean getCredential(const UtlString& type,
                            const UtlString& realm,
                            SipLineCredential& lineCredentials) const;

   /** Removes credentials for a particular realm */
   UtlBoolean removeCredential(const UtlString& type,
                               const UtlString& realm
                               );

   /** Removes all credentials for this line */
   void removeAllCredentials();

   /** Set the preferred host/ip for the contact in subsequent registers */
   void setPreferredContact(const UtlString& contactAddress, int contactPort);
   
   /** 
    * Get Preferred host/ip for the contact in subsequent registers.
    */
   Url getPreferredContactUri() const;

   //@}
   /* ============================ INQUIRY =================================== */
   ///@name Inquiry
   //@{

   /** Whether credentials for this realm already exist. */
   UtlBoolean realmExists(const UtlString& realm) const;

   //@}

   /* //////////////////////////// PROTECTED ///////////////////////////////// */
protected:

   Url m_identityUri; /// <line key which is the canonical URL stripped of display name, angle brackets and field parameters (basically the URI).
   Url m_userEnteredUrl; ///< user entered URL string (could be incomplete "sip:444@")
   Url m_canonicalUrl; ///< The canonical URL which is the URL string with the host and port filled in if they were missing.

   UtlString m_lineId; ///< 12 characters uniquely identifying line. Part of contact as LINE parameter
   LineStates m_currentState; ///< current state of line
   Url m_preferredContactUri; ///< contact that will be used in SIP messages
   UtlString m_proxyServers; ///< SIP proxy servers address:port, separated by ,

   mutable UtlHashBag m_credentials; ///< bag of SipLineCredential

   void copyCredentials(const SipLine& rSipLine);
   void generateLineID(UtlString& lineId);
   Url buildLineContact(const UtlString& address = NULL, int port = PORT_NONE);

   /* //////////////////////////// PRIVATE /////////////////////////////////// */
private:
};

#endif // SipLine_h__
