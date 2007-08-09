//
// Copyright (C) 2007 Jaroslav Libak
// Licensed to SIPfoundry under a Contributor Agreement.
//
// Copyright (C) 2006 SIPfoundry Inc.
// Licensed by SIPfoundry under the LGPL license.
//
// Copyright (C) 2006 Robert J. Andreasen, Jr.
// Licensed to SIPfoundry under a Contributor Agreement.
//
// $$
//////////////////////////////////////////////////////////////////////////////

// SYSTEM INCLUDES
// APPLICATION INCLUDES
#include <utl/UtlInit.h>

#include "tapi/SipXEventDispatcher.h"
#include "tapi/sipXtapi.h"
#include "tapi/sipXEvents.h"
#include "tapi/sipXtapiEvents.h"
#include "utl/UtlVoidPtr.h"
#include "os/OsPtrMsg.h"
#include "os/OsReadLock.h"
#include "os/OsWriteLock.h"
#include "utl/UtlHashMapIterator.h"

// DEFINES
// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES

// CONSTANTS
#define SIPX_EVENT_MSG      OsMsg::USER_START + 1
// STATIC VARIABLE INITIALIZATIONS
SipXEventDispatcher* SipXEventDispatcher::m_spInstance = NULL;
int SipXEventDispatcher::m_sCounter = 0;
OsRWMutex SipXEventDispatcher::m_MemberLock(OsMutex::Q_FIFO);
OsRWMutex SipXEventDispatcher::m_InstanceLock(OsMutex::Q_FIFO);
UtlHashMap SipXEventDispatcher::m_Listeners;

// MACROS
// GLOBAL VARIABLES
// GLOBAL FUNCTIONS

class SIPX_EVENT_LISTENER_CONTEXT
{
public:
   const SIPX_INST             m_hInst;
   SIPX_EVENT_CALLBACK_PROC    m_pCallbackProc;
   void*                       m_pUserData;

   SIPX_EVENT_LISTENER_CONTEXT(const SIPX_INST inst,
                               SIPX_EVENT_CALLBACK_PROC callbackProc,
                               void* userData) :
      m_hInst(inst),
      m_pCallbackProc(callbackProc),
      m_pUserData(userData)
   {
   }
};


/* //////////////////////////// PUBLIC //////////////////////////////////// */

/* ============================ CREATORS ================================== */

// CHECKED
void SipXEventDispatcher::initDispatcher()
{
   OsWriteLock lock(m_InstanceLock);

   if (m_sCounter++ == 0)
   {
      m_spInstance = new SipXEventDispatcher();
      m_spInstance->start();
   }
}

// CHECKED
void SipXEventDispatcher::shutdownDispatcher()
{
   OsWriteLock lock(m_InstanceLock);

   if (m_sCounter > 0)
   {
      if (--m_sCounter == 0)
      {
         delete m_spInstance;
         m_spInstance = NULL;
      }      
   }
}

/* ============================ MANIPULATORS ============================== */

// CHECKED
UtlBoolean SipXEventDispatcher::handleMessage(OsMsg& rMsg)
{
    UtlBoolean bRet = false ;

    switch (rMsg.getMsgType())
    {
        case SIPX_EVENT_MSG:
            {
                SIPX_EVENT_CATEGORY category;
                void* pDataCopy;
                SIPX_RESULT rc;
                SIPX_INST hInst;

                category = (SIPX_EVENT_CATEGORY)rMsg.getMsgSubType();
                pDataCopy = ((OsPtrMsg&)rMsg).getPtr();
                hInst = ((OsPtrMsg&)rMsg).getPtr2();

                serviceListeners(hInst, category, pDataCopy);

                rc = sipxFreeDuplicatedEvent(category, pDataCopy);
                assert(rc == SIPX_RESULT_SUCCESS);
            }
            bRet = true;
            break;
        default:
            break;
    }
    return bRet;
}

// CHECKED
UtlBoolean SipXEventDispatcher::addListener(const SIPX_INST hInst,
                                            SIPX_EVENT_CALLBACK_PROC  pCallbackProc,
                                            void*                     pUserData) 
{
    OsWriteLock lock(m_MemberLock);

    assert(pCallbackProc);    

    SIPX_EVENT_LISTENER_CONTEXT* pContext = new SIPX_EVENT_LISTENER_CONTEXT(hInst, pCallbackProc, pUserData);
    m_Listeners.insert(new UtlVoidPtr(pContext));

    return TRUE;
}

// CHECKED
UtlBoolean SipXEventDispatcher::removeListener(const SIPX_INST hInst,
                                               SIPX_EVENT_CALLBACK_PROC  pCallbackProc,
                                               void*                     pUserData) 
{
    OsWriteLock lock(m_MemberLock);

    assert(pCallbackProc);    

    UtlHashMapIterator itor(m_Listeners);
    UtlVoidPtr* pValue;
    SIPX_EVENT_LISTENER_CONTEXT* pContext = NULL;
    UtlBoolean bRC = FALSE;

    while ((pValue = (UtlVoidPtr*)itor()))
    {
        pContext = (SIPX_EVENT_LISTENER_CONTEXT*)pValue->getValue();
        assert(pContext);

        if (pContext)
        {
            if (pContext->m_hInst == hInst &&
                pContext->m_pCallbackProc == pCallbackProc &&
                pContext->m_pUserData == pUserData)
            {
                m_Listeners.destroy(pValue);
                // mListeners.destroy will only delete the UtlVoidPtr object, not pContext
                delete pContext;
                bRC = TRUE;
                break;
            }
        }
    }

    return bRC;
}

// CHECKED
void SipXEventDispatcher::removeAllListeners(const SIPX_INST hInst) 
{
    OsWriteLock lock(m_MemberLock);

    UtlHashMapIterator itor(m_Listeners);
    UtlVoidPtr* pValue;
    SIPX_EVENT_LISTENER_CONTEXT* pContext = NULL;

    while ((pValue = (UtlVoidPtr*)itor()))
    {
       pContext = (SIPX_EVENT_LISTENER_CONTEXT*) pValue->getValue();
       assert(pContext);

       if (pContext && pContext->m_hInst == hInst)
       {
          m_Listeners.destroy(pValue);
          // mListeners.destroy will only delete the UtlVoidPtr object, not pContext
          delete pContext;
       }
    }
}

// CHECKED
void SipXEventDispatcher::serviceListeners(const SIPX_INST hInst,
                                           SIPX_EVENT_CATEGORY category, 
                                           void*               pInfo)
{
    OsReadLock lock(m_MemberLock);
    assert(pInfo);

    UtlHashMapIterator itor(m_Listeners);
    UtlVoidPtr* pValue;
    SIPX_EVENT_LISTENER_CONTEXT* pContext = NULL;

    while ((pValue = (UtlVoidPtr*)itor()))
    {
        pContext = (SIPX_EVENT_LISTENER_CONTEXT*)pValue->getValue();
        assert(pContext);
        assert(pContext->m_pCallbackProc);

        if (pContext && pContext->m_hInst == hInst && pContext->m_pCallbackProc)
        {
           pContext->m_pCallbackProc(category, pInfo, pContext->m_pUserData);
        }
    }
}

// CHECKED
void SipXEventDispatcher::dispatchEvent(const SIPX_INST hInst,
                                        SIPX_EVENT_CATEGORY category, 
                                        void*               pInfo)
{
   OsReadLock lock(m_InstanceLock);

   if (m_spInstance)
   {
      void* pDataCopy = NULL;
      SIPX_RESULT rc = sipxDuplicateEvent(category, pInfo, &pDataCopy);
      assert(rc == SIPX_RESULT_SUCCESS);

      if (rc == SIPX_RESULT_SUCCESS)
      {
         OsPtrMsg msg(SIPX_EVENT_MSG, (unsigned char)category, pDataCopy, (void*)hInst);

         if (m_spInstance->postMessage(msg) != OS_SUCCESS)
         {
            rc = sipxFreeDuplicatedEvent(category, pDataCopy);
            assert(rc == SIPX_RESULT_SUCCESS);
         }
      }
   }
}

/* ============================ INQUIRY =================================== */

// CHECKED
int SipXEventDispatcher::getListenerCount( const SIPX_INST hInst )
{
   OsReadLock lock(m_MemberLock);
   int counter = 0;

   UtlHashMapIterator itor(m_Listeners);
   UtlVoidPtr* pValue;
   SIPX_EVENT_LISTENER_CONTEXT* pContext = NULL;

   while ((pValue = (UtlVoidPtr*)itor()))
   {
      pContext = (SIPX_EVENT_LISTENER_CONTEXT*)pValue->getValue();
      assert(pContext);
      assert(pContext->m_pCallbackProc);

      if (pContext && pContext->m_hInst == hInst)
      {
         counter++;
      }
   }

   return counter;
}

/* //////////////////////////// PROTECTED ///////////////////////////////// */

/* //////////////////////////// PRIVATE /////////////////////////////////// */

// CHECKED
SipXEventDispatcher::SipXEventDispatcher() 
: OsServerTask("SipXEventDispatcher-%d")
{
}

// CHECKED
SipXEventDispatcher::~SipXEventDispatcher(void)
{   
   waitUntilShutDown();
}

