//
// Copyright (C) 2007 Jaroslav Libak
// Licensed under the LGPL license.
//
// Copyright (C) 2005-2007 SIPez LLC.
// Licensed to SIPfoundry under a Contributor Agreement.
// 
// Copyright (C) 2004-2007 SIPfoundry Inc.
// Licensed by SIPfoundry under the LGPL license.
//
// Copyright (C) 2004-2006 Pingtel Corp.  All rights reserved.
// Licensed to SIPfoundry under a Contributor Agreement.
//
// $$
///////////////////////////////////////////////////////////////////////////////

// SYSTEM INCLUDES
#include <assert.h>

// APPLICATION INCLUDES
#include <utl/UtlInit.h>

#include <os/OsLock.h>
#include <utl/UtlHashMapIterator.h>
#include <net/SipUserAgent.h>
#include "net/SipLineMgr.h"
#include "tapi/SipXLine.h"
#include "tapi/SipXHandleMap.h"
#include "tapi/SipXEvents.h"

// DEFINES
// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
// STATIC VARIABLE INITIALIZATIONS
// MACROS
// GLOBAL VARIABLES
SipXHandleMap gLineHandleMap(1, SIPX_LINE_NULL);  /**< Global Map of line handles */

// GLOBAL FUNCTIONS

/* ============================ FUNCTIONS ================================= */

// CHECKED
SIPX_LINE sipxLineLookupHandle(const char* szLineURI, 
                               const char* szRequestUri) 
{ 
   SIPX_LINE hLine = SIPX_LINE_NULL; 

   hLine = sipxLineLookupHandleByURI(szLineURI);
   if (!hLine)
   {
      hLine = sipxLineLookupHandleByURI(szRequestUri);
   }

   return hLine;
}

// CHECKED
static SIPX_LINE_DATA* createLineData(SIPX_INSTANCE_DATA* pInst, const Url& uri)
{
   SIPX_LINE_DATA* pData = new SIPX_LINE_DATA();
   // if there is allocation failure, std::bad_alloc exception is thrown
   // unless we use _set_new_handler to set new handler
   // we do not handle std::bad_alloc thus program will exit, which is ok
   // if we want to check for NULL, then we would have to use
   // new(std:::nothrow) instead of just new

   pData->m_lineURI = uri;
   pData->m_pInst = pInst;

   return pData;
}

/**
 * @brief Finds a line by handle and acquires a lock on it.
 */
// CHECKED
SIPX_LINE_DATA* sipxLineLookup(const SIPX_LINE hLine,
                               SIPX_LOCK_TYPE type,
                               const OsStackTraceLogger& oneBackInStack)
{
   OsStackTraceLogger logItem(FAC_SIPXTAPI, PRI_DEBUG, "sipxLineLookup", oneBackInStack);
   SIPX_LINE_DATA* pRC = NULL;
   OsStatus status = OS_FAILED; 

   gLineHandleMap.lock();
   pRC = (SIPX_LINE_DATA*)gLineHandleMap.findHandle(hLine);

   if (pRC)
   {
      if (validLineData(pRC))
      {
         switch (type)
         {
         case SIPX_LOCK_READ:
            status = pRC->m_mutex.acquire();
            assert(status == OS_SUCCESS);
            break;
         case SIPX_LOCK_WRITE:
            status = pRC->m_mutex.acquire();
            assert(status == OS_SUCCESS);
            break;
         default:
            break;
         }
      }
      else
      {
         assert(false);
         pRC = NULL;
      }
   }

   gLineHandleMap.unlock();

   return pRC;
}

// CHECKED
void sipxLineReleaseLock(SIPX_LINE_DATA* pData,
                         SIPX_LOCK_TYPE type,
                         const OsStackTraceLogger& oneBackInStack) 
{
   OsStackTraceLogger logItem(FAC_SIPXTAPI, PRI_DEBUG, "sipxLineReleaseLock", oneBackInStack);
   OsStatus status;

   if (type != SIPX_LOCK_NONE)
   {
      if (validLineData(pData))
      {
         switch (type)
         {
         case SIPX_LOCK_READ:
            status = pData->m_mutex.release();
            assert(status == OS_SUCCESS);
            break;
         case SIPX_LOCK_WRITE:
            status = pData->m_mutex.release();
            assert(status == OS_SUCCESS);
            break;
         default: 
            break;
         }
      }
      else
      {
         // something is bad if call data is not valid, fix bug
         assert(false);
      }
   }
}

/**
 * @brief Removes and deletes all lines. Lines are not unregistered.
 */
// CHECKED
void sipxLineRemoveAll(const SIPX_INST hInst) 
{
   SIPX_LINE lines[MAX_LINES];
   size_t nLines = 0 ;
   sipxLineGet(hInst, lines, MAX_LINES, &nLines);

   for (size_t i = 0; i < nLines; i++)
   {
      sipxLineRemove(lines[i]);
   }
}

/**
 * Frees line object by handle.
 */
// CHECKED
void sipxLineObjectFree(const SIPX_LINE hLine)
{
   OsStackTraceLogger logItem(FAC_SIPXTAPI, PRI_DEBUG, "sipxLineObjectFree");
   SIPX_LINE_DATA* pData = NULL;

   gLineHandleMap.lock();
   pData = sipxLineLookup(hLine, SIPX_LOCK_WRITE, logItem);
   assert(pData);

   if (pData)
   {
      pData->m_pInst->lock.acquire();
      pData->m_pInst->nLines--;
      assert(pData->m_pInst->nLines >= 0);
      pData->m_pInst->lock.release();

      const void* pRC = gLineHandleMap.removeHandle(hLine);
      assert(pRC);
      gLineHandleMap.unlock();

      UtlVoidPtr* pValue;

      while ((pValue = (UtlVoidPtr*) pData->m_lineAliases.get()))
      {
         Url* pUri = (Url*) pValue->getValue();
         delete pUri;
         delete pValue;
      }

      delete pData;
   }
   else
   {
      // pData is NULL
      gLineHandleMap.unlock();
   }
}

// CHECKED
UtlBoolean validLineData(const SIPX_LINE_DATA* pData) 
{
   UtlBoolean bValid = FALSE;

   if (pData && pData->m_pInst && 
      pData->m_pInst->pCallManager)
   {
      bValid = TRUE;
   }

   return bValid;
}

// CHECKED
SIPX_LINE sipxLineLookupHandleByURI(const char* szURI)
{
   gLineHandleMap.lock(); // global lock for line deletion

   UtlHashMapIterator iter(gLineHandleMap);
   Url urlURI(szURI);

   UtlInt* pIndex = NULL;
   UtlVoidPtr* pObj = NULL;
   SIPX_LINE hLine = SIPX_LINE_NULL;

   // First pass: strict matching
   while ((pIndex = dynamic_cast<UtlInt*>(iter())))
   {
      pObj = dynamic_cast<UtlVoidPtr*>(gLineHandleMap.findValue(pIndex));

      if (pObj)
      {
         // got access to a value, unwrap it
         SIPX_LINE_DATA* pData = (SIPX_LINE_DATA*)pObj->getValue();

         if (pData)
         {
            // got access to line object, lock it
            OsLock lock(pData->m_mutex);

            // Check main line definition
            if (urlURI.isUserHostPortEqual(pData->m_lineURI))
            {
               hLine = pIndex->getValue();
               break;
            }

            // check for line aliases
            UtlVoidPtr* pValue;
            Url* pUrl;
            UtlSListIterator iterator(pData->m_lineAliases);

            while ((pValue = (UtlVoidPtr*) iterator()))
            {
               pUrl = (Url*)pValue->getValue();

               if (urlURI.isUserHostPortEqual(*pUrl))
               {
                  hLine = pIndex->getValue();
                  break;
               }
            }
         }
      }
   }

   // Second pass: Relax port
   if (hLine == SIPX_LINE_NULL)
   {
      iter.reset();
      while ((pIndex = dynamic_cast<UtlInt*>(iter())))
      {
         pObj = dynamic_cast<UtlVoidPtr*>(gLineHandleMap.findValue(pIndex));

         if (pObj)
         {
            SIPX_LINE_DATA* pData = (SIPX_LINE_DATA*)pObj->getValue();
            if (pData)
            {
               // got access to line object, lock it
               OsLock lock(pData->m_mutex);

               // Check main line definition
               if (urlURI.isUserHostEqual(pData->m_lineURI))
               {
                  hLine = pIndex->getValue();
                  break;
               }

               // Check for line aliases
               UtlVoidPtr* pValue;
               Url* pUrl;
               UtlSListIterator iterator(pData->m_lineAliases);

               while ((pValue = (UtlVoidPtr*) iterator()))
               {
                  pUrl = (Url*) pValue->getValue();

                  if (urlURI.isUserHostEqual(*pUrl))
                  {
                     hLine = pIndex->getValue();
                     break;
                  }
               }
            }
         }
      }
   }

   // Third pass: username only
   if (hLine == SIPX_LINE_NULL)
   {
      iter.reset();
      while ((pIndex = dynamic_cast<UtlInt*>(iter())))
      {
         pObj = dynamic_cast<UtlVoidPtr*>(gLineHandleMap.findValue(pIndex));

         if (pObj)
         {
            SIPX_LINE_DATA* pData = (SIPX_LINE_DATA*) pObj->getValue();

            if (pData)
            {
               // got access to line object, lock it
               OsLock lock(pData->m_mutex);

               UtlString uriUsername;
               UtlString hostUsername;

               urlURI.getUserId(uriUsername);
               pData->m_lineURI.getUserId(hostUsername);

               if (uriUsername.compareTo(hostUsername, UtlString::ignoreCase) == 0)
               {
                  hLine = pIndex->getValue();
                  break;
               }

               // Check for line aliases
               UtlVoidPtr* pValue;
               Url* pUrl;
               UtlSListIterator iterator(pData->m_lineAliases);

               while ((pValue = (UtlVoidPtr*) iterator()))
               {
                  pUrl = (Url*) pValue->getValue();
                  UtlString aliasUsername;

                  pUrl->getUserId(aliasUsername);

                  if (uriUsername.compareTo(aliasUsername, UtlString::ignoreCase) == 0)
                  {
                     hLine = pIndex->getValue();
                     break;
                  }
               }
            }
         }
      }
   }

   gLineHandleMap.unlock();

   return hLine;
}


/*********************************************************************/
/*       Public line handling functions                              */
/*********************************************************************/

// CHECKED
SIPXTAPI_API SIPX_RESULT sipxLineRemove(SIPX_LINE hLine)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxLineRemove");
   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO, "sipxLineRemove hLine=%d", hLine);    

   SIPX_RESULT sr = SIPX_RESULT_FAILURE;

   if (hLine)
   {
      SIPX_LINE_DATA* pData = sipxLineLookup(hLine, SIPX_LOCK_READ, stackLogger);

      if (pData)
      {
         Url lineURI(pData->m_lineURI);
         SIPX_INSTANCE_DATA* pInst = pData->m_pInst;

         sipxLineReleaseLock(pData, SIPX_LOCK_READ, stackLogger);

         if (pInst)
         {
            // we can't have lock while executing lineManager methods
            pInst->pLineManager->deleteLine(lineURI); 

            sr = SIPX_RESULT_SUCCESS;
         }
         sipxLineObjectFree(hLine);
      }
   }
   else
   {
      sr = SIPX_RESULT_INVALID_ARGS;
   }

   return sr;
}

// CHECKED
SIPXTAPI_API SIPX_RESULT sipxLineRegister(const SIPX_LINE hLine, const int bRegister)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxLineRegister");
   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxLineRegister hLine=%d bRegister=%d",
      hLine, bRegister);

   SIPX_RESULT sr = SIPX_RESULT_FAILURE;
   if (hLine)
   {
      SIPX_LINE_DATA* pData = sipxLineLookup(hLine, SIPX_LOCK_WRITE, stackLogger);
      if (pData)
      {
         Url lineURI(pData->m_lineURI);
         SIPX_INSTANCE_DATA* pInst = pData->m_pInst;

         sipxLineReleaseLock(pData, SIPX_LOCK_WRITE, stackLogger);

         if (pInst)
         {
            if (bRegister)
            {
               pInst->pLineManager->enableLine(lineURI);
            }
            else
            {
               pInst->pLineManager->disableLine(lineURI,
                  false,
                  lineURI.toString());
               //  ->unRegisterUser(*pData->lineURI, false, pData->lineURI->toString());
            }
            sr = SIPX_RESULT_SUCCESS;
         }
      }// else line was not found
   }// else hLine == 0

   return sr;
}

// CHECKED
SIPXTAPI_API SIPX_RESULT sipxLineAddCredential(const SIPX_LINE hLine,                                                 
                                               const char* szUserID,
                                               const char* szPasswd,
                                               const char* szRealm)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxLineAddCredential");
   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxLineAddCredential hLine=%d userId=%s realm=%s",
      hLine, szUserID, szRealm);

   SIPX_RESULT sr = SIPX_RESULT_FAILURE;
   SIPX_LINE_DATA* pData = sipxLineLookup(hLine, SIPX_LOCK_READ, stackLogger);

   if (pData)
   {
      Url lineURI(pData->m_lineURI);
      SIPX_INSTANCE_DATA* pInst = pData->m_pInst;

      sipxLineReleaseLock(pData, SIPX_LOCK_READ, stackLogger);

      if (szUserID && szPasswd)
      {
         UtlBoolean rc = pInst->pLineManager->addCredentialForLine(lineURI,
            szRealm,
            szUserID,
            szPasswd,
            HTTP_DIGEST_AUTHENTICATION);

         if (rc)
         {
            sr = SIPX_RESULT_SUCCESS;
         }
      }
      else
      {
         sr = SIPX_RESULT_INVALID_ARGS;
      }
   }
   else
   {
      sr = SIPX_RESULT_INVALID_ARGS;
   }

   return sr;
}

// CHECKED, verify the weird Url conversion
SIPXTAPI_API SIPX_RESULT sipxLineAddAlias(const SIPX_LINE hLine, const char* szLineURL) 
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxLineAddAlias");
   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxLineAddAlias hLine=%d szLineURL=%s",
      hLine, szLineURL);

   SIPX_RESULT sr = SIPX_RESULT_FAILURE;
   if (hLine)
   {
      SIPX_LINE_DATA* pData = sipxLineLookup(hLine, SIPX_LOCK_WRITE, stackLogger);
      if (pData)
      {
         // ??????
         Url url(szLineURL);
         UtlString strURI;
         url.getUri(strURI);
         Url uri(strURI);

         pData->m_lineAliases.append(new UtlVoidPtr(new Url(uri)));

         sipxLineReleaseLock(pData, SIPX_LOCK_WRITE, stackLogger);

         sr = SIPX_RESULT_SUCCESS;
      }                
   }

   return sr;
}

// CHECKED
SIPXTAPI_API SIPX_RESULT sipxLineGetContactInfo(const SIPX_LINE  hLine,
                                                char* szContactAddress,
                                                const size_t nContactAddressSize,
                                                int* contactPort,
                                                SIPX_CONTACT_TYPE* contactType,
                                                SIPX_TRANSPORT_TYPE* transport)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxLineGetContactInfo");
   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO, 
      "sipxLineGetContactInfo hLine=%d", hLine);

   SIPX_RESULT sr = SIPX_RESULT_FAILURE;

   SIPX_LINE_DATA* pData = sipxLineLookup(hLine, SIPX_LOCK_READ, stackLogger);
   if (pData)
   {
      SAFE_STRNCPY(szContactAddress, pData->m_contactAddress.data(), nContactAddressSize);
      *contactPort = pData->m_contactPort;
      *contactType = pData->m_contactType;
      *transport = pData->m_transport;

      sipxLineReleaseLock(pData, SIPX_LOCK_READ, stackLogger);

      sr = SIPX_RESULT_SUCCESS;
   }  
   
   return sr;
}

// CHECKED
SIPXTAPI_API SIPX_RESULT sipxLineFindByURI(const SIPX_INST hInst,
                                           const char* szURI,
                                           SIPX_LINE* phLine) 
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxLineFindByURI");
   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxLineFindByURI hInst=%p szURI=%s", hInst, szURI);

   SIPX_RESULT sr = SIPX_RESULT_INVALID_ARGS;

   if (hInst && szURI)
   {
      *phLine = sipxLineLookupHandleByURI(szURI);

      if (*phLine != SIPX_LINE_NULL)
      {
         sr = SIPX_RESULT_SUCCESS;
      }
      else
      {
         sr = SIPX_RESULT_FAILURE;
      }
   }

   return sr;
}

// CHECKED
SIPXTAPI_API SIPX_RESULT sipxLineGet(const SIPX_INST hInst,
                                     SIPX_LINE lines[],
                                     const size_t max,
                                     size_t* actual)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxLineGet");
   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxLineGet hInst=%p",
      hInst);

   SIPX_RESULT sr = SIPX_RESULT_FAILURE;
   SIPX_INSTANCE_DATA* pInst = (SIPX_INSTANCE_DATA*)hInst;
   assert(pInst);

   if (pInst && lines && actual && max > 0)
   {
      *actual = 0;
      SipLine* pLines = new SipLine[max];

      pInst->pLineManager->getLines(max, *actual, pLines);    // rc is > 0 lines (useless)

      for (size_t i = 0; i < *actual; i++)
      {
         lines[i] = sipxLineLookupHandleByURI(pLines[i].getIdentity().toString());
      }

      delete [] pLines;
      sr = SIPX_RESULT_SUCCESS;
   }
   else
   {
      sr = SIPX_RESULT_INVALID_ARGS;
   }

   return sr;
}

// CHECKED
SIPXTAPI_API SIPX_RESULT sipxLineGetURI(const SIPX_LINE hLine,
                                        char* szBuffer,
                                        const size_t nBuffer,
                                        size_t* nActual)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxLineGetURI");
   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxLineGetURI hLine=%d",
      hLine);

   SIPX_RESULT sr = SIPX_RESULT_FAILURE;
   SIPX_LINE_DATA* pData = sipxLineLookup(hLine, SIPX_LOCK_READ, stackLogger);


   if (pData)
   {
      if (szBuffer)
      {
         SAFE_STRNCPY(szBuffer, pData->m_lineURI.toString(), nBuffer);

         *nActual = strlen(szBuffer) + 1;
         sr = SIPX_RESULT_SUCCESS;
      }
      else
      {
         *nActual = strlen(pData->m_lineURI.toString()) + 1;
         sr = SIPX_RESULT_SUCCESS;
      }

      sipxLineReleaseLock(pData, SIPX_LOCK_READ, stackLogger);
   }

   return sr;
}

// CHECKED
SIPXTAPI_API SIPX_RESULT sipxLineAdd(const SIPX_INST hInst,
                                     const char* szLineUrl,
                                     SIPX_LINE* phLine,
                                     SIPX_CONTACT_ID contactId)

{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxLineAdd");
   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxLineAdd hInst=%p lineUrl=%s, phLine=%p contactId=%d ",
      hInst, szLineUrl, phLine, contactId);

   SIPX_RESULT sr = SIPX_RESULT_FAILURE;
   SIPX_INSTANCE_DATA* pInst = (SIPX_INSTANCE_DATA*)hInst;

   assert(szLineUrl);
   assert(phLine);

   if (pInst)
   {
      if (szLineUrl && phLine)
      {
         Url url(szLineUrl);
         Url uri = url.getUri();
         UtlString userId = url.getUserId();

         // Set the preferred contact
         SIPX_CONTACT_ADDRESS* pContact = NULL;
         SIPX_CONTACT_TYPE contactType = CONTACT_AUTO;
         SIPX_TRANSPORT_TYPE transport = TRANSPORT_UDP;
         UtlString contactIp;
         int contactPort;

         // try to find contact by ID
         pContact = pInst->pSipUserAgent->getContactDb().find(contactId);
         if (pContact)
         {
            // contact found
            contactType = pContact->eContactType;            
         }

         UtlString lineUrl(szLineUrl);
         if (lineUrl.contains("sips:") || lineUrl.contains("transport=tls"))
         {
            // use TLS
            transport = TRANSPORT_TLS;
         }
         else if (lineUrl.contains("transport=tcp"))
         {
            // use TCP transport
            transport = TRANSPORT_TCP;
         }

         // select contact IP, port, maybe override contact type and transport. Transport is used to select port.
         sipxSelectContact(pInst, contactType, contactIp, contactPort, transport);

         SipLine line(url, uri, userId, TRUE, SipLine::LINE_STATE_UNKNOWN);
         line.setPreferredContact(contactIp, contactPort);

         UtlBoolean bRC = pInst->pLineManager->addLine(line, FALSE);
         if (bRC)
         {
            SIPX_LINE_DATA* pData = createLineData(pInst, uri);

            if (pData)
            {
               pData->m_contactAddress = contactIp;
               pData->m_contactPort = contactPort;
               pData->m_transport = transport;
               pData->m_contactType = contactType;

               UtlBoolean res = gLineHandleMap.allocHandle(*phLine, pData);
               if (res)
               {
                  pData->m_pInst->lock.acquire();
                  pData->m_pInst->nLines++;
                  pData->m_pInst->lock.release();

                  sr = SIPX_RESULT_SUCCESS;

                  pInst->pLineManager->setStateForLine(uri, SipLine::LINE_STATE_PROVISIONED);

                  sipxFireLineEvent(pInst, 
                     szLineUrl, 
                     LINESTATE_PROVISIONED,
                     LINESTATE_PROVISIONED_NORMAL);
               }
               else
               {
                  // handle allocation failure
                  OsSysLog::add(FAC_SIPXTAPI, PRI_ERR, 
                     "allocHandle failed to allocate a handle");

                  // get rid of new SIPX_LINE_DATA
                  delete pData;

                  // delete line from line manager
                  pInst->pLineManager->deleteLine(uri);
               }
            }
            else
            {
               sr = SIPX_RESULT_OUT_OF_MEMORY;
            }
         }
      }
      else
      {
         sr = SIPX_RESULT_INVALID_ARGS;
      }
   }

   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO, "sipxLineAdd hLine=%d", *phLine);

   return sr;
}
