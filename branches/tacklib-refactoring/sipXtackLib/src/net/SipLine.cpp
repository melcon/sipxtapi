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


// SYSTEM INCLUDES

#if defined(_VXWORKS)
#   include <taskLib.h>
#   include <netinet/in.h>
#endif

#include <net/SipMessage.h>
#include <utl/UtlHashBagIterator.h>
#include <net/SipLine.h>
#include <net/Url.h>
#include <net/SipLineCredential.h>
#include <os/OsConfigDb.h>
#include <os/OsRWMutex.h>
#include <os/OsReadLock.h>
#include <os/OsWriteLock.h>
#include <os/OsDateTime.h>
#include <net/NetMd5Codec.h>

// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS

const UtlContainableType SipLine::TYPE = "SipLine";

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

SipLine::SipLine(const Url& userEnteredUrl,
                 const Url& identityUri,
                 LineStates state)
{
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
   m_currentState = state;

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
   // build default contact
   m_preferredContactUri = buildLineContact();
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
      m_identityUri = rSipLine.m_identityUri;
      m_userEnteredUrl = rSipLine.m_userEnteredUrl;
      m_canonicalUrl = rSipLine.m_canonicalUrl;
      m_currentState = rSipLine.m_currentState;
      m_lineId = rSipLine.m_lineId;
      m_preferredContactUri = rSipLine.m_preferredContactUri;
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
      SipLineCredential* credential = NULL;

      while((credential = dynamic_cast<SipLineCredential*>(observerIterator())) != NULL)
      {
         addCredential(credential->getRealm(),
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

SipLine::LineStates SipLine::getState() const
{
    return m_currentState;
}

void SipLine::setState(SipLine::LineStates state)
{
    m_currentState = state;
}

UtlString SipLine::getUserId() const
{
    return m_userEnteredUrl.getUserId();
}

Url SipLine::getUserEnteredUrl() const
{
    return m_userEnteredUrl;
}
Url SipLine::getIdentityUri() const
{
    return m_identityUri;
}

Url SipLine::getCanonicalUrl() const
{
    return m_canonicalUrl;
}

UtlBoolean SipLine::addCredential(const UtlString& strRealm,
                                  const UtlString& strUserID,
                                  const UtlString& strPasswd,
                                  const UtlString& strType)
{
   UtlBoolean isAdded = FALSE;

   if(!realmExists(strRealm))
   {
      SipLineCredential* credential = new SipLineCredential(strRealm , strUserID, strPasswd, strType);
      m_credentials.insert(credential);
      isAdded = TRUE;
   }

   return isAdded;
}

UtlBoolean SipLine::addCredential(const SipLineCredential& lineCredential)
{
   UtlBoolean isAdded = FALSE;

   if(!realmExists(lineCredential.getRealm()))
   {
      SipLineCredential* credential = new SipLineCredential(lineCredential);
      m_credentials.insert(credential);
      isAdded = TRUE;
   }

   return isAdded;
}

int SipLine::getNumOfCredentials() const
{
   return m_credentials.entries();
}

UtlBoolean SipLine::getCredential(const UtlString& type,
                                  const UtlString& realm,
                                  UtlString& userID,
                                  UtlString& passwordMD5Digest) const
{
   SipLineCredential lineCredential;

   UtlBoolean result = getCredential(type, realm, lineCredential);
   if (result)
   {
      userID = lineCredential.getUserId();
      passwordMD5Digest = lineCredential.getPasswordMD5Digest(realm);
   }

   return result;
}

UtlBoolean SipLine::getCredential(const UtlString& type,
                                  const UtlString& realm,
                                  SipLineCredential& lineCredential) const
{
   if (type.isNull())
   {
      return FALSE;
   }

   UtlString emptyRealm(NULL);

   SipLineCredential* credential = dynamic_cast<SipLineCredential*>(m_credentials.find(&realm));
   if (credential && !type.compareTo(credential->getType(), UtlString::matchCase))
   {
      // also type must match
      lineCredential = *credential;
   }
   else // if realm was not found, then return credentials for default = empty realm
   {
      credential = dynamic_cast<SipLineCredential*>(m_credentials.find(&emptyRealm));
      if (credential && !type.compareTo(credential->getType(), UtlString::matchCase))
      {
         // also type must match
         lineCredential = *credential;
      }
   }

   return credential != NULL;
}

UtlBoolean SipLine::removeCredential(const UtlString& type,
                                     const UtlString& realm)
{
   UtlContainable* wasRemoved = m_credentials.removeReference(&realm);
   if(wasRemoved)
   {
      delete wasRemoved;
      wasRemoved = NULL;
      return TRUE;
   }

   return FALSE;
}

void SipLine::removeAllCredentials()
{
    m_credentials.destroyAll();
}

void SipLine::setPreferredContact(const UtlString& contactAddress, int contactPort)
{
   m_preferredContactUri = buildLineContact(contactAddress, contactPort);
}

Url SipLine::getPreferredContactUri() const
{
   return m_preferredContactUri;
}

UtlBoolean SipLine::realmExists(const UtlString& realm) const
{
   SipLineCredential* credential = dynamic_cast<SipLineCredential*>(m_credentials.find(&realm));
   return credential != NULL;
}

void SipLine::generateLineID(UtlString& lineId)
{
   lineId = NetMd5Codec::encode(m_identityUri.toString());

   // Shorten the line Ids to 12 chars (from 32)
   lineId.remove(12);
}

Url SipLine::buildLineContact(const UtlString& address, int port)
{
   Url contactUrl(m_canonicalUrl);

   if (!address.isNull())
   {
      contactUrl.setHostAddress(address);
      contactUrl.setHostPort(port);
   }
   contactUrl.setPassword(NULL);
   contactUrl.setPath(NULL);
   contactUrl.setUrlParameter(SIP_LINE_IDENTIFIER, m_lineId);
   contactUrl.includeAngleBrackets();

   return contactUrl;
}

UtlContainableType SipLine::getContainableType() const
{
   return SipLine::TYPE;
}

unsigned SipLine::hash() const
{
   return m_identityUri.toString().hash();
}

int SipLine::compareTo(UtlContainable const *compareContainable) const
{
   int compareFlag = -1;

   if (compareContainable)
   {
      if (compareContainable->isInstanceOf(UtlString::TYPE) == TRUE)
      {
         // for same type compare by identity uri
         SipLine const *pLine = dynamic_cast<SipLine const *>(compareContainable);
         compareFlag = m_identityUri.toString().compareTo(pLine->getIdentityUri().toString(), UtlString::matchCase);
      }
      else
      {
         // for different type compare by direct hash
         compareFlag = directHash() - compareContainable->directHash();
      }
   }

   return compareFlag;
}

UtlCopyableContainable* SipLine::clone() const
{
   return new SipLine(*this);
}
