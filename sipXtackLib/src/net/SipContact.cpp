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
// APPLICATION INCLUDES
#include <os/OsDefs.h>
#include <net/Url.h>
#include <net/SipContact.h>
#include <net/SipMessage.h>

// DEFINES
// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
// STATIC VARIABLE INITIALIZATIONS
const UtlContainableType SipContact::TYPE = "SipContact";

// MACROS
// GLOBAL VARIABLES
// GLOBAL FUNCTIONS

/* //////////////////////////// PUBLIC //////////////////////////////////// */

/* ============================ CREATORS ================================== */

SipContact::SipContact(int contactId,
                       SIP_CONTACT_TYPE contactType,
                       SIP_TRANSPORT_TYPE transportType,
                       UtlString ipAddress,
                       int port)
: m_contactId(contactId)
, m_contactType(contactType)
, m_transportType(transportType)
, m_ipAddress(ipAddress)
, m_port(port)
{

}

SipContact::~SipContact()
{

}

SipContact::SipContact(const SipContact& rhs)
: m_contactId(rhs.m_contactId)
, m_contactType(rhs.m_contactType)
, m_transportType(rhs.m_transportType)
, m_ipAddress(rhs.m_ipAddress)
, m_port(rhs.m_port)
{

}

/* ============================ MANIPULATORS ============================== */

UtlContainableType SipContact::getContainableType() const
{
   return SipContact::TYPE;
}

unsigned SipContact::hash() const
{
   return m_contactId;
}

int SipContact::compareTo(UtlContainable const *compareContainable) const
{
   int compareFlag = -1;

   if (compareContainable)
   {
      if (compareContainable->isInstanceOf(SipContact::TYPE) == TRUE)
      {
         SipContact const *pSipContact = dynamic_cast<SipContact const *>(compareContainable);
         if (pSipContact)
         {
            compareFlag = m_contactId - pSipContact->getContactId();
         }
      }
      else
      {
         // for different type compare by direct hash
         compareFlag = directHash() - compareContainable->directHash();
      }
   }

   return compareFlag;
}

UtlCopyableContainable* SipContact::clone() const
{
   return new SipContact(*this);
}

void SipContact::buildContactUri(Url& contactUri) const
{
   SipContact::buildContactUri(contactUri, NULL, m_ipAddress, m_port, m_transportType);
}

void SipContact::buildContactUri(const UtlString& userId, Url& contactUri) const
{
   SipContact::buildContactUri(contactUri, userId, m_ipAddress, m_port, m_transportType);
}

void SipContact::buildContactUri(Url& contactUri,
                                 const UtlString& userId,
                                 const UtlString& ipAddress,
                                 int port /*= PORT_NONE*/,
                                 SIP_TRANSPORT_TYPE transportType /*= SIP_TRANSPORT_UDP*/)
{
   Url uri;
   uri.setUserId(userId);
   uri.setHostAddress(ipAddress.data());
   if (portIsValid(port))
   {
      uri.setHostPort(port);
   }
   uri.setScheme(transportType == SIP_TRANSPORT_TLS ? Url::SipsUrlScheme : Url::SipUrlScheme);

   if (transportType == SIP_TRANSPORT_TCP)
   {
      uri.setUrlParameter(SIP_TRANSPORT, SIP_TRANSPORT_TCP_STR);
   }

   uri.includeAngleBrackets();
   contactUri = uri;
}

/* ============================ ACCESSORS ================================= */

/* ============================ INQUIRY =================================== */

/* //////////////////////////// PROTECTED ///////////////////////////////// */

/* //////////////////////////// PRIVATE /////////////////////////////////// */

/* ============================ FUNCTIONS ================================= */
