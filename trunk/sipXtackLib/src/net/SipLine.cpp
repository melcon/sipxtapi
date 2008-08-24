//
// Copyright (C) 2004-2006 SIPfoundry Inc.
// Licensed by SIPfoundry under the LGPL license.
//
// Copyright (C) 2004-2006 Pingtel Corp.  All rights reserved.
// Licensed to SIPfoundry under a Contributor Agreement.
//
// $$
///////////////////////////////////////////////////////////////////////////////


// SYSTEM INCLUDES

#if defined(_VXWORKS)
#   include <taskLib.h>
#   include <netinet/in.h>
#endif

#include <utl/UtlHashBagIterator.h>
#include <net/SipLine.h>
#include <net/Url.h>
#include <net/SipLineCredentials.h>
#include <os/OsConfigDb.h>
#include <os/OsRWMutex.h>
#include <os/OsReadLock.h>
#include <os/OsWriteLock.h>
#include <os/OsDateTime.h>
#include <net/NetMd5Codec.h>


// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

SipLine::SipLine(const Url& userEnteredUrl,
                 const Url& identityUri,
                 const UtlString& user,
                 UtlBoolean visible,
                 int state,
                 UtlBoolean isAutoEnabled,
                 UtlBoolean useCallHandling)
{
   if (user.isNull())
   {
      userEnteredUrl.getUserId(m_user);
   }
   else
   {
      m_user = user;
   }

   m_userEnteredUrl = userEnteredUrl;
   if (identityUri.toString().isNull())
   {
      //then get uri from user entered url ...uri is complete in it
      m_identityUri = m_userEnteredUrl.getUri();
   }
   else
   {
      m_identityUri = identityUri;
   }
   m_isVisible = visible;
   m_currentState = state;
   m_isAutoEnabled = isAutoEnabled;
   m_isUsingCallHandling = useCallHandling;

   //construct a complete url from identityUri and userEntered Url.
   m_canonicalUrl = m_userEnteredUrl;
   UtlString address = m_canonicalUrl.getHostAddress();
   if (address.isNull())
   {
      UtlString identityHost = m_identityUri.getHostAddress();
      int identityPort = m_identityUri.getHostPort();
      m_canonicalUrl.setHostAddress(identityHost);
      m_canonicalUrl.setHostPort(identityPort);
   }
   // create new Line Id for this line
   generateLineID(m_lineId);   
}

SipLine::~SipLine()
{
   m_credentials.destroyAll();
}

// Copy constructor
SipLine::SipLine(const SipLine& rSipLine)
{
   *this = rSipLine;
}

SipLine& SipLine::operator=(const SipLine& rSipLine)
{
   if (this == &rSipLine)            // handle the assignment to self case
      return *this;
   else
   {
      m_isVisible = rSipLine.m_isVisible ;
      m_identityUri = rSipLine.m_identityUri ;
      m_userEnteredUrl = rSipLine.m_userEnteredUrl;
      m_canonicalUrl = rSipLine.m_canonicalUrl;
      m_user = rSipLine.m_user ;
      m_currentState = rSipLine.m_currentState ;
      m_isAutoEnabled = rSipLine.m_isAutoEnabled ;
      m_isUsingCallHandling = rSipLine.m_isUsingCallHandling;
      m_lineId = rSipLine.m_lineId;
      m_preferredContactUri = rSipLine.m_preferredContactUri ;
      copyCredentials(rSipLine);
   }
   return *this;
}
//deep copy of credentials
void SipLine::copyCredentials(const SipLine &rSipLine)
{
   if(!m_credentials.isEmpty())
   {
      m_credentials.destroyAll();
   }
   if (!rSipLine.m_credentials.isEmpty())
   {
      UtlHashBagIterator observerIterator(const_cast<UtlHashBag&> (rSipLine.m_credentials));
      SipLineCredentials* credential = NULL;

      while((credential = (SipLineCredentials*) observerIterator()) != NULL)
      {
         addCredentials(credential->getRealm(),
                        credential->getUserId(),
                        credential->getPasswordToken(),
                        credential->getType());
      }
   }
}

UtlString SipLine::getLineId() const
{
   return m_lineId;
}

UtlBoolean SipLine::isDeviceLine() const
{
    return (getUser().compareTo("Device", UtlString::ignoreCase) == 0) ;
}


int SipLine::getState() const
{
    return m_currentState;
}

void SipLine::setState(int state)
{
    m_currentState = state;
}

UtlString SipLine::getUser() const
{
    return m_user;
}

void SipLine::setUser(const UtlString& user)
{
   m_user = user;
}

void SipLine::getIdentityAndUrl(Url &identityUri, Url &userEnteredUrl) const
{
   identityUri = m_identityUri;
   userEnteredUrl = m_userEnteredUrl;
}

void SipLine::setIdentityAndUrl(Url identityUri, Url userEnteredUrl)
{
   m_identityUri = identityUri;
   generateLineID(m_lineId);
   m_userEnteredUrl = userEnteredUrl;
   //construct a complete url from identityUri and userEntered Url.
   m_canonicalUrl = m_userEnteredUrl;
   
   UtlString address = m_canonicalUrl.getHostAddress();
   if (address.isNull())
   {
      UtlString identityHost = m_identityUri.getHostAddress();
      int identityPort = m_identityUri.getHostPort();
      m_canonicalUrl.setHostAddress(identityHost);
      m_canonicalUrl.setHostPort(identityPort);
   }
}

Url SipLine::getUserEnteredUrl() const
{
    return m_userEnteredUrl;
}
Url SipLine::getIdentity() const
{
    return m_identityUri;
}

Url SipLine::getCanonicalUrl() const
{
    return m_canonicalUrl;
}

void SipLine::setAutoEnableStatus(UtlBoolean isAutoEnabled)
{
    m_isAutoEnabled = isAutoEnabled;
}

UtlBoolean SipLine::getAutoEnableStatus() const
{
    return m_isAutoEnabled;
}

void SipLine::setVisibility(UtlBoolean isEnable)
{
    m_isVisible = isEnable;
}

UtlBoolean SipLine::getVisibility() const
{
    return m_isVisible;
}

void SipLine::setCallHandling(UtlBoolean useCallHandling)
{
    m_isUsingCallHandling = useCallHandling;
}

UtlBoolean SipLine::getCallHandling() const
{
    return m_isUsingCallHandling;
}

UtlBoolean SipLine::addCredentials(const UtlString& strRealm,
                                   const UtlString& strUserID,
                                   const UtlString& strPasswd,
                                   const UtlString& strType)
{
   UtlBoolean isAdded = FALSE;

   if(!isDuplicateRealm(strRealm))
   {
      SipLineCredentials* credential = new SipLineCredentials(strRealm , strUserID, strPasswd, strType);
      m_credentials.insert(credential);
      isAdded = TRUE;
   }

   return isAdded;
}

UtlBoolean SipLine::addCredentials(const SipLineCredentials& lineCredentials)
{
   UtlBoolean isAdded = FALSE;

   if(!isDuplicateRealm(lineCredentials.getRealm()))
   {
      SipLineCredentials* credential = new SipLineCredentials(lineCredentials);
      m_credentials.insert(credential);
      isAdded = TRUE;
   }

   return isAdded;
}

int SipLine::getNumOfCredentials() const
{
   return m_credentials.entries();

}

UtlBoolean SipLine::getCredentials(const UtlString& type,
                                   const UtlString& realm,
                                   UtlString& userID,
                                   UtlString& MD5_token)
{
   SipLineCredentials lineCredentials;

   UtlBoolean result = getCredentials(type, realm, lineCredentials);
   userID = lineCredentials.getUserId();
   MD5_token = lineCredentials.getPasswordMD5Digest();

   return result;
}

UtlBoolean SipLine::getCredentials(const UtlString& type,
                                   const UtlString& realm,
                                   SipLineCredentials& lineCredentials)
{
   UtlString emptyRealm(NULL);

   SipLineCredentials* credential = (SipLineCredentials*) m_credentials.find(&realm);
   if (credential)
   {
      lineCredentials = *credential;
   }
   else // if realm was not found, then return credentials for default = empty realm
   {
      credential = (SipLineCredentials*) m_credentials.find(&emptyRealm);
      if (credential)
      {
         lineCredentials = *credential;
      }
   }

   return credential != NULL;
}

void SipLine::removeCredential(const UtlString& realm)
{
   UtlContainable* wasRemoved = m_credentials.removeReference(&realm);
   if(wasRemoved)
   {
      delete wasRemoved;
   }
}

void SipLine::removeAllCredentials()
{
    m_credentials.destroyAll();
}

void SipLine::setPreferredContactUri(const Url& preferredContactUri)
{
    m_preferredContactUri = preferredContactUri;
}

void SipLine::setPreferredContact(const UtlString& contactAddress, int contactPort)
{
   m_preferredContactUri.reset();
   m_preferredContactUri.setHostAddress(contactAddress);
   m_preferredContactUri.setHostPort(contactPort);
}

UtlBoolean SipLine::getPreferredContactUri(Url& preferredContactUri) const
{
    preferredContactUri = m_preferredContactUri;

    return (!preferredContactUri.getHostAddress().isNull());
}

UtlBoolean SipLine::getAllCredentials(int maxEnteries/*[in]*/ ,
                                      int& actualEnteries /*[out/int]*/,
                                      UtlString realm[]/*[out/int]*/,
                                      UtlString userId[]/*[out/int]*/,
                                      UtlString type[]/*[out/int]*/,
                                      UtlString passdigest[]/*[out/int]*/)
{
    int i = 0;

    UtlHashBagIterator observerIterator(m_credentials);
    SipLineCredentials* credential = NULL;

    while((credential = (SipLineCredentials*) observerIterator()) != NULL && i < maxEnteries)
    {     
      userId[i] = credential->getUserId();
      passdigest[i] = credential->getPasswordMD5Digest();
      realm[i] = credential->getRealm();
      type[i] = credential->getType();

      i++;
    }

    actualEnteries = i;
    return  actualEnteries > 0;
}

UtlBoolean SipLine::isDuplicateRealm(const UtlString& realm) const
{
   SipLineCredentials* credential = (SipLineCredentials*) m_credentials.find(&realm);
   return credential != NULL;
}

void SipLine::generateLineID(UtlString& lineId)
{
   lineId = NetMd5Codec::encode(m_identityUri.toString());

   // Shorten the line Ids to 12 chars (from 32)
   lineId.remove(12);
}
