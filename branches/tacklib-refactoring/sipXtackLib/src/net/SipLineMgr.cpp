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

#include <stdio.h>
#include <stdlib.h>

// APPLICATION INCLUDES
#include "utl/UtlHashBagIterator.h"
#include "os/OsDateTime.h"
#include "os/OsQueuedEvent.h"
#include "os/OsTimer.h"
#include "os/OsEventMsg.h"
#include "os/OsConfigDb.h"
#include "os/OsRWMutex.h"
#include "os/OsReadLock.h"
#include "os/OsWriteLock.h"
#include "os/OsLock.h"
#include "net/Url.h"
#include "net/SipLineMgr.h"
#include "net/SipObserverCriteria.h"
#include "net/SipUserAgent.h"
#include "net/SipMessage.h"
#include "net/NetMd5Codec.h"
#include "net/SipRefreshMgr.h"

// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
// STATIC INITIALIZERS

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

SipLineMgr::SipLineMgr(SipRefreshMgr *refershMgr)
: OsServerTask("SipLineMgr-%d")
, m_pRefreshMgr(refershMgr)
, m_mutex(OsMutex::Q_FIFO)
{
}

SipLineMgr::~SipLineMgr()
{
   OsSysLog::add(FAC_LINE_MGR, PRI_INFO, "SipLineMgr is shutting down...");

   dumpLines();
   waitUntilShutDown();

   // refresh manager is not deleted by us
   m_pRefreshMgr = NULL;
}

void SipLineMgr::startLineMgr()
{
   if(!isStarted())
   {
      OsSysLog::add(FAC_LINE_MGR, PRI_INFO, "SipLineMgr is starting...");
      // start the thread
      start();
   }
}

UtlBoolean SipLineMgr::handleMessage(OsMsg &eventMessage)
{
   UtlBoolean messageProcessed = FALSE;

   int msgType = eventMessage.getMsgType();
   int msgSubType = eventMessage.getMsgSubType();

   // do nothing with the message

   return  messageProcessed;
}

UtlBoolean SipLineMgr::addLine(SipLine& line)
{
   OsLock lock(m_mutex);

   if (m_listList.add(line))
   {
      syslog(FAC_LINE_MGR, PRI_INFO, "SipLineMgr::addLine added line: %s",
         line.getIdentityUri().toString().data());

      return TRUE;
   }

   return FALSE;
}

UtlBoolean SipLineMgr::deleteLine(const Url& lineUri)
{
   OsLock lock(m_mutex);

   if (m_listList.remove(lineUri))
   {
      syslog(FAC_LINE_MGR, PRI_INFO, "SipLineMgr::deleteLine deleted line: %s",
         lineUri.toString().data());

      return TRUE;
   }

   return FALSE;
}

UtlBoolean SipLineMgr::registerLine(const Url& lineURI)
{
   Url canonicalUrl;
   Url preferredContact;

   {
      OsLock lock(m_mutex); // scoped lock

      SipLine *pLine = m_listList.getLine(lineURI);

      if (!pLine)
      {
         syslog(FAC_LINE_MGR, PRI_ERR, "unable to enable line (not found): %s",
            lineURI.toString().data()) ;
         return FALSE;
      }

      pLine->setState(SipLine::LINE_STATE_TRYING);
      canonicalUrl = pLine->getCanonicalUrl();
      preferredContact = pLine->getPreferredContactUri();
      pLine = NULL;
   }

   if (!m_pRefreshMgr->newRegisterMsg(canonicalUrl, preferredContact, -1))
   {
      //duplicate ...call reregister
      m_pRefreshMgr->reRegister(lineURI);
   }

   syslog(FAC_LINE_MGR, PRI_INFO, "enabled line: %s",
      lineURI.toString().data());

   return TRUE;
}

UtlBoolean SipLineMgr::unregisterLine(const Url& lineURI)
{
   SipLine::LineStates lineState = SipLine::LINE_STATE_UNKNOWN;

   {
      OsLock lock(m_mutex); // scoped lock

      SipLine *pLine = m_listList.getLine(lineURI);
      if (!pLine)
      {
         syslog(FAC_LINE_MGR, PRI_ERR, "SipLineMgr::unregisterLine unable to disable line (not found): %s",
            lineURI.toString().data());
         return FALSE;
      }

      lineState = pLine->getState();
      pLine = NULL;
   }

   if (lineState == SipLine::LINE_STATE_REGISTERED ||
       lineState == SipLine::LINE_STATE_TRYING)
   {
      m_pRefreshMgr->unRegisterUser(lineURI);
      syslog(FAC_LINE_MGR, PRI_INFO, "SipLineMgr::unregisterLine disabled line: %s",
         lineURI.toString().data());
      return TRUE;
   }

   return FALSE;
}

void SipLineMgr::getLineCopies(UtlSList& lineList) const
{
   OsLock lock(m_mutex); // scoped lock
   m_listList.getLineCopies(lineList);
}

int SipLineMgr::getNumLines() const
{
   OsLock lock(m_mutex); // scoped lock
   return m_listList.getLinesCount();
}

SipLine* SipLineMgr::getLineforAuthentication(const SipMessage* request,
                                              const SipMessage* response,
                                              UtlBoolean isIncomingRequest) const
{
   SipLine* line = NULL;
   UtlString lineId;
   Url      toFromUrl;
   UtlString toFromUri;
   UtlString userId;

   UtlString nonce;
   UtlString opaque;
   UtlString realm;
   UtlString scheme;
   UtlString algorithm;
   UtlString qop;

   // Get realm and scheme (hard way but not too expensive)
   if (response != NULL)
   {
      if (!response->getAuthenticationData(&scheme, &realm, &nonce, &opaque, &algorithm, &qop, SipMessage::PROXY))
      {
         if (!response->getAuthenticationData(&scheme, &realm, &nonce, &opaque, &algorithm, &qop, SipMessage::SERVER))
         {
            // Report inability to get auth criteria
            UtlString callId ;
            UtlString method ;
            int sequenceNum ;

            response->getCallIdField(&callId);
            response->getCSeqField(&sequenceNum, &method);
            OsSysLog::add(FAC_LINE_MGR, PRI_ERR, "unable get auth data for message:\ncallid=%s\ncseq=%d\nmethod=%s",
               callId.data(), sequenceNum, method.data()) ;
         }
         else
         {
            OsSysLog::add(FAC_AUTH, PRI_DEBUG, "SERVER auth request:scheme=%s\nrealm=%s\nnounce=%s\nopaque=%s\nalgorithm=%s\nqop=%s",
               scheme.data(), realm.data(), nonce.data(), opaque.data(), algorithm.data(), qop.data()) ;
         }
      }
      else
      {
         OsSysLog::add(FAC_AUTH, PRI_DEBUG, "PROXY auth request:scheme=%s\nrealm=%s\nnounce=%s\nopaque=%s\nalgorithm=%s\nqop=%s",
            scheme.data(), realm.data(), nonce.data(), opaque.data(), algorithm.data(), qop.data()) ;
      }
   }

   // Get the LineID and userID
   if(isIncomingRequest)
   {
      //check line id in request uri
      UtlString requestUri;
      request->getRequestUri(&requestUri);
      UtlString temp;
      temp.append("<");
      temp.append(requestUri);
      temp.append(">");
      Url requestUriUrl(temp);
      requestUriUrl.getUrlParameter(SIP_LINE_IDENTIFIER , lineId);
      requestUriUrl.getUserId(userId);
   }
   else
   {
      //check line ID in contact
      UtlString contact;
      request->getContactEntry(0, &contact);
      Url contactUrl(contact);
      contactUrl.getUrlParameter(SIP_LINE_IDENTIFIER , lineId);
      contactUrl.getUserId(userId);
   }

   // Get the fromURL
   request->getFromUrl(toFromUrl);
   toFromUrl.removeFieldParameters();
   toFromUrl.setDisplayName("");
   toFromUrl.removeAngleBrackets();


   UtlString emptyRealm(NULL);

   if (line == NULL)
   {
      line = m_listList.findLine(lineId, toFromUrl, userId.data());
      if (line == NULL)
      {
         line = m_listList.findLine(lineId, toFromUrl, userId.data());
      }
   }

   if (line == NULL)
   {
      // Get the toURL
      request->getToUrl(toFromUrl);
      toFromUrl.removeFieldParameters();
      toFromUrl.setDisplayName("");
      toFromUrl.removeAngleBrackets();

      if (line == NULL)
      {
         line = m_listList.findLine(lineId, toFromUrl, userId.data());
         if (line == NULL)
         {
            line = m_listList.findLine(lineId, toFromUrl, userId.data());
         }
      }
   }

   if (line == NULL)
   {
      // Log the failure
      OsSysLog::add(FAC_AUTH, PRI_ERR, "line manager is unable to find auth credentials:\nuser=%s\nrealm=%s\nlineid=%s",
         userId.data(), realm.data(), lineId.data()) ;
   }
   else
   {
      // Log the SUCCESS
      OsSysLog::add(FAC_AUTH, PRI_INFO, "line manager found matching auth credentials:\nuser=%s\nrealm=%s\nlineid=%s",
         userId.data(), realm.data(), lineId.data()) ;
   }

   return line;
}

UtlBoolean SipLineMgr::isUserIdDefined(const SipMessage* request) const
{
   SipLine* pLine = getLineforAuthentication(request, NULL, TRUE);
   if(pLine)
      return TRUE;
   else
      return FALSE;
}


UtlBoolean SipLineMgr::buildAuthenticatedRequest(
   const SipMessage* response /*[in]*/,
   const SipMessage* request /*[in]*/,
   SipMessage* newAuthRequest /*[out]*/)
{
   UtlBoolean createdResponse = FALSE;
   // Get the userId and password from the DB for the URI
   int sequenceNum;
   int authorizationEntity = HttpMessage::SERVER;
   UtlString uri;
   UtlString method;
   UtlString nonce;
   UtlString opaque;
   UtlString realm;
   UtlString scheme;
   UtlString algorithm;
   UtlString qop;
   UtlString callId;
   UtlString stale;

   response->getCSeqField(&sequenceNum, &method);
   response->getCallIdField(&callId) ;
   int responseCode = response->getResponseStatusCode();

   // Use the To uri as key to user and password to use
   if(responseCode == HTTP_UNAUTHORIZED_CODE)
   {
      authorizationEntity = HttpMessage::SERVER;
   } else if(responseCode == HTTP_PROXY_UNAUTHORIZED_CODE)
   {
      // For proxy we use the uri for the key to userId and password
      authorizationEntity = HttpMessage::PROXY;
   }

   // Get the digest authentication info. needed to create
   // a request with credentials
   response->getAuthenticationData(
      &scheme, &realm, &nonce, &opaque,
      &algorithm, &qop, authorizationEntity, &stale);

   UtlBoolean alreadyTriedOnce = FALSE;
   int requestAuthIndex = 0;
   UtlString requestUser;
   UtlString requestRealm;

   // if scheme is basic , we dont support it anymore and we
   //should not sent request again because the password has been
   //converterd to digest already and the BASIC authentication will fail anyway
   if(scheme.compareTo(HTTP_BASIC_AUTHENTICATION, UtlString::ignoreCase) == 0)
   {
      alreadyTriedOnce = TRUE; //so that we never send request with basic authenticatiuon

      // Log error
      OsSysLog::add(FAC_AUTH, PRI_ERR, "line manager is unable to handle basic auth:\ncallid=%s\nmethod=%s\ncseq=%d\nrealm=%s",
         callId.data(), method.data(), sequenceNum, realm.data()) ;
   }
   else
   {
      // if stale flag is TRUE, according to RFC2617 we may wish to retry so we do
      // According to RFC2617: "The server should only set stale to TRUE
      // if it receives a request for which the nonce is invalid but with a
      // valid digest for that nonce" - so there shouldn't be infinite auth loop
      if (stale.compareTo("TRUE", UtlString::ignoreCase) != 0)
      {
         // Check to see if we already tried to send the credentials
         while(request->getDigestAuthorizationData(
            &requestUser, &requestRealm,
            NULL, NULL, NULL, NULL,
            authorizationEntity, requestAuthIndex) )
         {
            if(realm.compareTo(requestRealm) == 0)
            {
               alreadyTriedOnce = TRUE;
               break;
            }
            requestAuthIndex++;
         }
      }
   }
   // Find the line that sent the request that was challenged
   Url fromUrl;
   UtlString fromUri;
   UtlString userID;
   UtlString passToken;
   UtlBoolean credentialFound = FALSE;
   SipLine* line = NULL;
   //if challenged for an unregister request - then get credentials from temp lsit of lines
   int expires;
   int contactIndexCount = 0;
   UtlString contactEntry;

   if (!request->getExpiresField(&expires))
   {
      while ( request->getContactEntry(contactIndexCount , &contactEntry ))
      {
         UtlString expireStr;
         Url contact(contactEntry);
         contact.getFieldParameter(SIP_EXPIRES_FIELD, expireStr);
         expires = atoi(expireStr.data());
         if( expires == 0)
            break;
         contactIndexCount++;
      }
   }

   line = getLineforAuthentication(request, response, FALSE);
   if(line)
   {
      if(line->getCredential(scheme, realm, userID, passToken))
      {
         credentialFound = TRUE;
      }
   }

   if( !alreadyTriedOnce && credentialFound )
   {
      if ( line->getCredential(scheme, realm, userID, passToken))
      {
         OsSysLog::add(FAC_AUTH, PRI_INFO, "found auth credentials for:\nlineId:%s\ncallid=%s\nscheme=%s\nmethod=%s\ncseq=%d\nrealm=%s",
            fromUri.data(), callId.data(), scheme.data(), method.data(), sequenceNum, realm.data()) ;

         // Construct a new request with authorization and send it
         // the Sticky DNS fields will be copied by the copy constructor
         *newAuthRequest = *request;
         // Reset the transport parameters
         newAuthRequest->resetTransport();
         // Get rid of the via as another will be added.
         newAuthRequest->removeLastVia();
         if(scheme.compareTo(HTTP_DIGEST_AUTHENTICATION, UtlString::ignoreCase) == 0)
         {
            UtlString responseHash;
            int nonceCount;
            // create the authorization in the request
            request->getRequestUri(&uri);

            // :TBD: cheat and use the cseq instead of a real nonce-count
            request->getCSeqField(&nonceCount, &method);
            nonceCount = (nonceCount + 1) / 2;

            request->getRequestMethod(&method);

            // Use unique tokens which are constant for this
            // session to generate a cnonce
            Url fromUrl;
            UtlString cnonceSeed;
            UtlString fromTag;
            UtlString cnonce;
            request->getCallIdField(&cnonceSeed);
            request->getFromUrl(fromUrl);
            fromUrl.getFieldParameter("tag", fromTag);
            cnonceSeed.append(fromTag);
            cnonceSeed.append("blablacnonce"); // secret
            NetMd5Codec::encode(cnonceSeed, cnonce);

            // Get the digest of the body
            const HttpBody* body = request->getBody();
            UtlString bodyDigest;
            const char* bodyString = "";
            if(body)
            {
               int len;
               body->getBytes(&bodyString, &len);
               if(bodyString == NULL)
                  bodyString = "";
            }

            NetMd5Codec::encode(bodyString, bodyDigest);

            // Build the Digest hash response
            HttpMessage::buildMd5Digest(
               passToken.data(),
               algorithm.data(),
               nonce.data(),
               cnonce.data(),
               nonceCount,
               qop.data(),
               method.data(),
               uri.data(),
               bodyDigest.data(),
               &responseHash);

            newAuthRequest->setDigestAuthorizationData(
               userID.data(),
               realm.data(),
               nonce.data(),
               uri.data(),
               responseHash.data(),
               algorithm.data(),
               cnonce.data(),
               opaque.data(),
               qop.data(),
               nonceCount,
               authorizationEntity);

         }

         // This is a new version of the message so increment the sequence number
         newAuthRequest->incrementCSeqNumber();

         // If the first hop of this message is strict routed,
         // we need to add the request URI host back in the route
         // field so that the send will work correctly
         //
         //  message is:
         //      METHOD something
         //      Route: xyz, abc
         //
         //  change to xyz:
         //      METHOD something
         //      Route: something, xyz, abc
         //
         //  which sends as:
         //      METHOD something
         //      Route: xyz, abc
         //
         // But if the first hop is loose routed:
         //
         //  message is:
         //      METHOD something
         //      Route: xyz;lr, abc
         //
         // leave URI and routes alone
         if ( newAuthRequest->isClientMsgStrictRouted() )
         {
            UtlString requestUri;
            newAuthRequest->getRequestUri(&requestUri);
            newAuthRequest->addRouteUri(requestUri);
         }
         createdResponse = TRUE;
      }
      else
      {
         OsSysLog::add(FAC_AUTH, PRI_ERR, "could not find auth credentials for:\nlineId:%s\ncallid=%s\nscheme=%s\nmethod=%s\ncseq=%d\nrealm=%s",
            fromUri.data(), callId.data(), scheme.data(), method.data(), sequenceNum, realm.data()) ;
      }
   }
   line = NULL;

   // Else we already tried to provide authentication
   // Or we do not have a userId and password for this uri
   // Let this error message throught to the application
   return( createdResponse );
}

UtlBoolean SipLineMgr::addCredentialForLine(const Url& lineUri,
                                            const UtlString& strRealm,
                                            const UtlString& strUserID,
                                            const UtlString& strPasswd,
                                            const UtlString& type)
{
   OsLock lock(m_mutex); // scoped lock

   SipLine *pLine = m_listList.getLine(lineUri);
   if (!pLine)
   {
      return FALSE;
   }

   return pLine->addCredential(strRealm , strUserID, strPasswd, type);
}

UtlBoolean SipLineMgr::deleteCredentialForLine(const Url& lineUri,
                                               const UtlString& strRealm,
                                               const UtlString& type)
{
   OsLock lock(m_mutex); // scoped lock

   SipLine *pLine = m_listList.getLine(lineUri);
   if (!pLine)
   {
      return FALSE;
   }

   return pLine->removeCredential(type, strRealm);
}

UtlBoolean SipLineMgr::setStateForLine(const Url& lineUri,
                                       SipLine::LineStates state)
{
   OsLock lock(m_mutex); // scoped lock

   SipLine *pLine = m_listList.getLine(lineUri);
   if (!pLine)
   {
      return FALSE;
   }

   pLine->setState(state);
   return TRUE;
}

void SipLineMgr::dumpLines()
{
   // external lock is assumed
   m_listList.dumpLines();
}

