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
#include <utl/UtlInt.h>
#include <utl/UtlHashBag.h>
#include <utl/UtlHashMapIterator.h>
#include <utl/UtlHashBagIterator.h>
#include <net/SipMessage.h>
#include <cp/CpSipTransactionManager.h>

// DEFINES
#define CLEAN_PERIOD 100 // in seconds
#define MAX_DEAD_TRANSACTION_TIME 60

// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
// STATIC VARIABLE INITIALIZATIONS
// MACROS
// GLOBAL VARIABLES
// GLOBAL FUNCTIONS

class CpTransactionState : public UtlInt
{
public:
   CpTransactionState(int iCSeq)
      : UtlInt(iCSeq)
      , m_transactionState(CpSipTransactionManager::TRANSACTION_ACTIVE)
   {
      m_startTime = OsDateTime::getSecsSinceEpoch();
   }

   CpSipTransactionManager::TransactionState m_transactionState;
   long m_startTime; ///< start time in seconds since 1/1/1970
};

/* //////////////////////////// PUBLIC //////////////////////////////////// */

/* ============================ CREATORS ================================== */

CpSipTransactionManager::CpSipTransactionManager()
: m_iCSeq(0)
, m_lastCleanUpTime(0)
{
   UtlRandom randomGenerator;
   m_iCSeq = (abs(randomGenerator.rand()) % 65535);
}

CpSipTransactionManager::~CpSipTransactionManager()
{
   m_transactionMap.destroyAll();
}

/* ============================ MANIPULATORS ============================== */

int CpSipTransactionManager::startTransaction(const UtlString& sipMethod)
{
   UtlHashBag* pTransactionBag = dynamic_cast<UtlHashBag*>(m_transactionMap.findValue(&sipMethod));
   if (!pTransactionBag)
   {
      // create new transaction bag
      pTransactionBag = new UtlHashBag();
      m_transactionMap.insertKeyAndValue(sipMethod.clone(), pTransactionBag);
   }
   pTransactionBag->insert(new CpTransactionState(m_iCSeq));

   return m_iCSeq++; // post increment cseq
}

void CpSipTransactionManager::startTransaction(const UtlString& sipMethod, int cseq)
{
   UtlHashBag* pTransactionBag = dynamic_cast<UtlHashBag*>(m_transactionMap.findValue(&sipMethod));
   if (!pTransactionBag)
   {
      // create new transaction bag
      pTransactionBag = new UtlHashBag();
      m_transactionMap.insertKeyAndValue(sipMethod.clone(), pTransactionBag);
   }

   UtlInt utlCSeq(cseq);
   CpTransactionState* pTransactionState = dynamic_cast<CpTransactionState*>(pTransactionBag->find(&utlCSeq));
   if (!pTransactionState)
   {
      pTransactionBag->insert(new CpTransactionState(cseq)); // if its not already there then add it
   }

   if (m_iCSeq <= cseq)
   {
      m_iCSeq = cseq + 1; // set next available cseq number
   }
}

void CpSipTransactionManager::endTransaction(const UtlString& sipMethod, int cseq)
{
   cleanOldTransactions(); // maybe cleanup old terminated transactions

   UtlHashBag* pTransactionBag = dynamic_cast<UtlHashBag*>(m_transactionMap.findValue(&sipMethod));
   if (pTransactionBag)
   {
      UtlInt utlCSeq(cseq);
      CpTransactionState* pTransactionState = dynamic_cast<CpTransactionState*>(pTransactionBag->find(&utlCSeq));
      if (pTransactionState)
      {
         pTransactionState->m_transactionState = CpSipTransactionManager::TRANSACTION_TERMINATED;
      }
   }
}

/* ============================ ACCESSORS ================================= */

/* ============================ INQUIRY =================================== */

CpSipTransactionManager::TransactionState CpSipTransactionManager::getTransactionState(const UtlString& sipMethod,
                                                                                       int cseq) const
{
   UtlHashBag* pTransactionBag = dynamic_cast<UtlHashBag*>(m_transactionMap.findValue(&sipMethod));
   if (pTransactionBag)
   {
      UtlInt utlCSeq(cseq);
      CpTransactionState* pTransactionState = dynamic_cast<CpTransactionState*>(pTransactionBag->find(&utlCSeq));
      if (pTransactionState)
      {
         return pTransactionState->m_transactionState;
      }
   }

   return CpSipTransactionManager::TRANSACTION_NOT_FOUND;
}

CpSipTransactionManager::TransactionState CpSipTransactionManager::getTransactionState(const SipMessage& rSipMessage) const
{
   UtlString method;
   int cseq;
   rSipMessage.getCSeqField(cseq, method);

   return getTransactionState(method, cseq);
}

/* //////////////////////////// PROTECTED ///////////////////////////////// */

/* //////////////////////////// PRIVATE /////////////////////////////////// */

void CpSipTransactionManager::cleanOldTransactions()
{
   // get time in seconds since 1/1/1970
   long timeNow = OsDateTime::getSecsSinceEpoch();

   if (timeNow - m_lastCleanUpTime >= CLEAN_PERIOD)
   {
      m_lastCleanUpTime = timeNow;

      UtlHashMapIterator mapItor(m_transactionMap);
      while (mapItor())
      {
         UtlHashBag* pTransactionBag = dynamic_cast<UtlHashBag*>(mapItor.value());
         if (pTransactionBag)
         {
            UtlHashBagIterator bagItor(*pTransactionBag);
            while (bagItor())
            {
               CpTransactionState* pTransactionState = dynamic_cast<CpTransactionState*>(bagItor.key());
               if (pTransactionState &&
                  pTransactionState->m_transactionState == CpSipTransactionManager::TRANSACTION_TERMINATED &&
                  timeNow - pTransactionState->m_startTime >= MAX_DEAD_TRANSACTION_TIME)
               {
                  pTransactionBag->destroy(pTransactionState); // destroy transaction
               }
            }
         }
      }
   }
}

/* ============================ FUNCTIONS ================================= */
