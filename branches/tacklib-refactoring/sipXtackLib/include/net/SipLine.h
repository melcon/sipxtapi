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
 *
 * Line stores internally so called lineUri and fullLineUrl.
 * - LineURI is meant to be used by hashmaps/hashbags to construct hash, basically uniquely identify
 *  a line. It does't contain any parameters or display name. It is used to match inbound requests to a line.
 * - FullLineUrl is modified value passed into constructor stripped of transport parameter and field parameters.
 *   This is not a sip uri but full url including <>, display name. It is meant to be used to construct sip message
 *   from field.
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

   SipLine(const Url& fullLineUrl = "", ///< full line url. field parameters will be cut off
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
    * for line lookup. We deliberately do not adhere strictly to RFC3261 (19.1.4 URI Comparison),
    * because we want line to be transport independent.
    * All parameters are stripped from input url:
    * - transport, method, ttl, user, maddr and also user made ones
    *
    * Line having sip: scheme is not the same as sips:
    */
   static Url getLineUri(const Url& url);

   /**
    * Constructs line identity uri from given string.
    */
   static Url getLineUri(const UtlString& sUrl);

   /**
    * Constructs full line url from given url. Keeps display name, sip uri, but strips
    * transport parameter. Field parameters are removed. Contains <>. Useful for new
    * sip request messages, can be used in from field.
    */
   static Url getFullLineUrl(const Url& url);

   /**
   * Constructs full line url from given url. Keeps display name, sip uri, but strips
   * transport parameter. Field parameters are removed. Contains <>. Useful for new
   * sip request messages, can be used in from field.
   */
   static Url getFullLineUrl(const UtlString& sUrl);
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
   Url getFullLineUrl() const;

   /**
    * Gets line identityUri. It doesn't fieldParameters, display name or brackets.
    */
   Url getLineUri() const;

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

   Url m_lineUri; /// <line key which is the "userEnteredUrl" stripped of display name, angle brackets and all parameters (basically the URI).
   Url m_fullLineUrl; ///< line Url used to construct sip message from field. Doesn't contain transport parameter

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
