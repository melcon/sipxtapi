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
, mpRefreshMgr(refershMgr)
{
   if (refershMgr)
   {
      mpRefreshMgr->addMessageObserver(*(getMessageQueue()),
         SIP_REGISTER_METHOD,
         TRUE, // do want to get requests
         TRUE, // do want responses
         TRUE, // Incoming messages
         FALSE); // Don't want to see out going messages
   }
}

SipLineMgr::~SipLineMgr()
{
    dumpLines();
    waitUntilShutDown();

    // Do not delete the refresh manager as it was
    // created in another context and we do not know
    // who else may be using it
    mpRefreshMgr = NULL;
}

void SipLineMgr::dumpLines()
{
    sLineList.dumpLines();
}

void SipLineMgr::startLineMgr()
{
    if(!isStarted())
    {
        // start the thread
        start();
        mIsStarted = TRUE;
    }
}

UtlBoolean SipLineMgr::handleMessage(OsMsg &eventMessage)
{
    UtlBoolean messageProcessed = FALSE;

    int msgType = eventMessage.getMsgType();
    // int msgSubType = eventMessage.getMsgSubType();
    UtlString method;
    if(msgType == OsMsg::PHONE_APP )
    {
        const SipMessage* sipMsg =
            static_cast<const SipMessage*>(((SipMessageEvent&)eventMessage).getMessage());
        int messageType = ((SipMessageEvent&)eventMessage).getMessageStatus();

        //get line information related to this identity
        UtlString Address;
        UtlString Protocol;
        UtlString User;
        int Port;
        UtlString toUrl;
        UtlString Label;
        sipMsg->getToAddress(&Address, &Port, &Protocol, &User, &Label);
        SipMessage::buildSipUrl( &toUrl , Address , Port, Protocol, User , Label);

        SipLine* line = NULL;
          Url tempUrl ( toUrl );
        line = sLineList.getLine( tempUrl );

        if ( line)
        {   // is this a request with a timeout?
            if ( !sipMsg->isResponse() && (messageType==SipMessageEvent::TRANSPORT_ERROR) )
            {
                int CSeq;
                UtlString method;
                sipMsg->getCSeqField(&CSeq , &method);
                if (CSeq == 1) //first time registration - go directly to expired state
                {
                    line->setState(SipLine::LINE_STATE_EXPIRED);
                }
                else
                {
                    line->setState(SipLine::LINE_STATE_FAILED);
                }

                // Log the failure
                syslog(FAC_LINE_MGR, PRI_ERR, "failed to register line (cseq=%d, no response): %s",
                        CSeq, line->getLineId().data()) ;
            }
            else if( sipMsg->isResponse() )
            {
                int responseCode = sipMsg->getResponseStatusCode();
                UtlString sipResponseText;
                sipMsg->getResponseStatusText(&sipResponseText);

                if( (responseCode>= SIP_2XX_CLASS_CODE && responseCode < SIP_3XX_CLASS_CODE ))
                {
                    //set line state to correct
                    line->setState(SipLine::LINE_STATE_REGISTERED);

                    // Log the success
                    int CSeq;
                    UtlString method;
                    sipMsg->getCSeqField(&CSeq , &method);
                    syslog(FAC_LINE_MGR, PRI_DEBUG, "registered line (cseq=%d): %s",
                            CSeq, line->getLineId().data()) ;
                }
                else if(  responseCode >= SIP_3XX_CLASS_CODE )
                {
                    // get error codes
                    UtlString nonce;
                    UtlString opaque;
                    UtlString realm;
                    UtlString scheme;
                    UtlString algorithm;
                    UtlString qop;

                    //get realm and scheme
                    if ( responseCode == HTTP_UNAUTHORIZED_CODE)
                    {
                        sipMsg->getAuthenticationData(&scheme, &realm, &nonce, &opaque,
                            &algorithm, &qop, HttpMessage::SERVER);
                    }
                    else if ( responseCode == HTTP_PROXY_UNAUTHORIZED_CODE)
                    {
                        sipMsg->getAuthenticationData(&scheme, &realm, &nonce, &opaque,
                            &algorithm, &qop, HttpMessage::PROXY);
                    }

                    //SDUATODO: LINE_STATE_FAILED after timing mechanism in place
                    line->setState(SipLine::LINE_STATE_EXPIRED);

                    // Log the failure
                    int CSeq;
                    UtlString method;
                    sipMsg->getCSeqField(&CSeq , &method);
                    syslog(FAC_LINE_MGR, PRI_ERR, "failed to register line (cseq=%d, auth): %s\nnonce=%s, opaque=%s,\nrealm=%s,scheme=%s,\nalgorithm=%s, qop=%s",
                        CSeq, line->getLineId().data(),
                        nonce.data(), opaque.data(), realm.data(), scheme.data(), algorithm.data(), qop.data()) ;
                }
            }
            messageProcessed = TRUE;
        } // if line
        line = NULL;
    }
    return  messageProcessed ;
}

UtlBoolean SipLineMgr::addLine(SipLine& line)
{
   UtlBoolean added = FALSE;
   // check if it is a duplicate url
   if (!sLineList.isDuplicate(&line))
   {
      addLineToList(line);
      added = TRUE;
      syslog(FAC_LINE_MGR, PRI_INFO, "SipLineMgr::addLine added line: %s",
         line.getIdentity().toString().data());
   }

   return added;
}

UtlBoolean SipLineMgr::deleteLine(const Url& lineUri)
{
    SipLine *pLine = sLineList.getLine(lineUri);

    if (pLine)
    {
       removeFromList(*pLine);
       delete pLine;
       pLine = NULL;

       syslog(FAC_LINE_MGR, PRI_INFO, "SipLineMgr::deleteLine deleted line: %s",
          lineUri.toString().data());

       return TRUE;
    }

    return FALSE;
}

UtlBoolean SipLineMgr::registerLine(const Url& lineURI)
{
    SipLine *line = sLineList.getLine(lineURI);

    if (!line)
    {
        syslog(FAC_LINE_MGR, PRI_ERR, "unable to enable line (not found): %s",
                lineURI.toString().data()) ;
        return FALSE;
    }

    line->setState(SipLine::LINE_STATE_TRYING);
    Url canonicalUrl = line->getCanonicalUrl();
    Url preferredContact;
    line->getPreferredContactUri(preferredContact);

    if (!mpRefreshMgr->newRegisterMsg(canonicalUrl, preferredContact, -1))
    {
        //duplicate ...call reregister
        mpRefreshMgr->reRegister(lineURI);
    }
    line = NULL;

    syslog(FAC_LINE_MGR, PRI_INFO, "enabled line: %s",
            lineURI.toString().data()) ;

    return TRUE;
}

UtlBoolean SipLineMgr::unregisterLine(const Url& identity,
                                      const UtlString& lineId)
{
    SipLine *line = sLineList.getLine(identity) ;
    if (!line)
    {
        syslog(FAC_LINE_MGR, PRI_ERR, "SipLineMgr::unregisterLine unable to disable line (not found): %s",
                identity.toString().data());
        return FALSE;
    }

    // we don't implicitly un-register any more
    if (line->getState() == SipLine::LINE_STATE_REGISTERED ||
        line->getState() == SipLine::LINE_STATE_TRYING)
    {
        mpRefreshMgr->unRegisterUser(identity, lineId);
        syslog(FAC_LINE_MGR, PRI_INFO, "SipLineMgr::unregisterLine disabled line: %s",
           identity.toString().data());
        return TRUE;
    }

    return FALSE;
}

UtlBoolean SipLineMgr::getLines(size_t maxLines,
                                size_t& actualLines,
                                SipLine lines[]) const
{
    UtlBoolean linesFound = FALSE;
    linesFound = sLineList.linesInArray(maxLines, &actualLines, lines);
    return linesFound;
}

UtlBoolean SipLineMgr::getLines(size_t maxLines,
                                size_t& actualLines,
                                SipLine* lines[]) const
{
    UtlBoolean linesFound = FALSE;
    linesFound = sLineList.linesInArray(maxLines, &actualLines, lines);
    return linesFound;
}

int SipLineMgr::getNumLines() const
{
    return sLineList.getListSize();
}

SipLine* SipLineMgr::getLineforAuthentication(const SipMessage* request /*[in]*/,
                                              const SipMessage* response /*[in]*/,
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
      line = sLineList.findLine(lineId.data(), realm.data(), toFromUrl, userId.data(), mOutboundLine) ;
      if (line == NULL)
      {
         line = sLineList.findLine(lineId.data(), emptyRealm.data(), toFromUrl, userId.data(), mOutboundLine) ;
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
          line = sLineList.findLine(lineId.data(), realm.data(), toFromUrl, userId.data(), mOutboundLine) ;
          if (line == NULL)
          {
             line = sLineList.findLine(lineId.data(), emptyRealm.data(), toFromUrl, userId.data(), mOutboundLine) ;
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
    SipLine* line = NULL;
    line = getLineforAuthentication(request, NULL, TRUE);
    if(line)
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
         if(line->getCredentials(scheme, realm, userID, passToken))
         {
             credentialFound = TRUE;
         }
     }

    if( !alreadyTriedOnce && credentialFound )
    {
        if ( line->getCredentials(scheme, realm, userID, passToken))
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

void SipLineMgr::addLineToList(SipLine& line)
{
    sLineList.add(new SipLine(line));
}


void SipLineMgr::removeFromList(SipLine& line)
{
    sLineList.remove(&line);
}

UtlBoolean SipLineMgr::addCredentialForLine(const Url& lineUri,
                                            const UtlString& strRealm,
                                            const UtlString& strUserID,
                                            const UtlString& strPasswd,
                                            const UtlString& type)
{
    SipLine *line = NULL;
    if (!(line = sLineList.getLine(lineUri)))
    {
        return FALSE;
    }

    return line->addCredentials(strRealm , strUserID, strPasswd, type);
}

UtlBoolean SipLineMgr::deleteCredentialForLine(const Url& lineUri, const UtlString strRealm)
{
    SipLine *line = NULL;
    if (!(line = sLineList.getLine(lineUri)))
    {
        return FALSE;
    }

    return line->removeCredential(strRealm);
}

int SipLineMgr::getNumOfCredentialsForLine(const Url& lineUri) const
{
    SipLine *line = NULL;
    if (!(line = sLineList.getLine(lineUri)))
    {
       return 0;
    }

    return line->getNumOfCredentials();
}

UtlBoolean SipLineMgr::getCredentialListForLine(const Url& lineUri,
                                                int maxEnteries,
                                                int& actualEnteries,
                                                UtlString realmList[],
                                                UtlString userIdList[],
                                                UtlString typeList[],
                                                UtlString passTokenList[])
{
    SipLine *line = NULL;
    if (!(line = sLineList.getLine(lineUri)))
    {
        return FALSE;
    }

    return line->getAllCredentials(maxEnteries, actualEnteries, realmList, userIdList, typeList, passTokenList);
}

//can only get state
int SipLineMgr::getStateForLine( const Url& identity ) const
{
    int State = SipLine::LINE_STATE_UNKNOWN;
    SipLine *line = NULL;
    if (! (line = sLineList.getLine(identity)) )
    {
        osPrintf("ERROR::SipLineMgr::getStateForLine() - No Line for lineUri \n");
    } else
    {
      State =line->getState();
        line = NULL;
    }
    return State;
}

void SipLineMgr::setStateForLine(const Url& lineUri,
                                 int state)
{
    SipLine *line = NULL;
    if (! (line = sLineList.getLine(lineUri)) )
    {
        osPrintf("ERROR::SipLineMgr::setStateForLine() - No Line for lineUri\n");
        return;
    }
    int previousState =line->getState();
    line->setState(state);

    if ( previousState != SipLine::LINE_STATE_PROVISIONED && state == SipLine::LINE_STATE_PROVISIONED)
    {
        /* We no longer implicitly unregister */
        //unregisterLine(lineUri);
    }
    else if( previousState == SipLine::LINE_STATE_PROVISIONED && state == SipLine::LINE_STATE_REGISTERED)
    {
        registerLine(lineUri);
    }
    line = NULL;
}

//line User
UtlBoolean SipLineMgr::getUserForLine(const Url& lineUri, UtlString &User) const
{
    UtlBoolean retVal = FALSE;
    SipLine *line = NULL;
    if (! (line = sLineList.getLine(lineUri)) )
    {
        osPrintf("ERROR::SipLineMgr::getUserForLine() - No Line for lineUri \n");
    } else
    {
        User.remove(0);
        UtlString UserStr = line->getUser();
        User.append(UserStr);
        retVal = TRUE;
        line = NULL;
    }
    return retVal;
}
void SipLineMgr::setUserForLine(const Url& lineUri, const UtlString User)
{

    SipLine *line = NULL;
    if (! (line = sLineList.getLine(lineUri)) )
    {
        osPrintf("ERROR::SipLineMgr::setUserForLine() - No Line for lineUri\n");
        return;
    }
    line->setUser(User);
    line = NULL;
}

void SipLineMgr::setUserEnteredUrlForLine(const Url& lineUri, UtlString sipUrl)
{

    SipLine *line = NULL;
    if (! (line = sLineList.getLine(lineUri)) )
    {
        osPrintf("ERROR::SipLineMgr::setUserEnteredUrlForLine() - No Line for this Url\n");
        return;
    }
    line->setIdentityAndUrl(lineUri, Url(sipUrl));
    line = NULL;
}

UtlBoolean
SipLineMgr::getUserEnteredUrlForLine(
    const Url& identity,
    UtlString& rSipUrl) const
{
    UtlBoolean retVal = FALSE;
    SipLine *line = NULL;
    if (! (line = sLineList.getLine(identity)) )
    {
        osPrintf("ERROR::SipLineMgr::getUserEnteredUrlForLine() - No Line for this Url \n");
    } else
    {
        rSipUrl.remove(0);
        Url userEnteredUrl = line->getUserEnteredUrl();
        rSipUrl.append(userEnteredUrl.toString());
        retVal = TRUE;
        line = NULL;
    }
    return retVal;
}

UtlBoolean
SipLineMgr::getCanonicalUrlForLine(
    const Url& identity,
    UtlString& rSipUrl) const
{
    UtlBoolean retVal = FALSE;
    SipLine *line = NULL;
    if (! (line = sLineList.getLine(identity)) )
    {
        osPrintf("ERROR::SipLineMgr::getUserForLine() - No Line for this Url \n");
    } else
    {
        rSipUrl.remove(0);
        Url canonicalUrl = line->getCanonicalUrl();
        rSipUrl.append(canonicalUrl.toString());
        retVal = TRUE;
        line = NULL;
    }
    return retVal;
}


