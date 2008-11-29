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
#include <net/SipMessage.h>
#include <cp/CpSipTransactionManager.h>

// DEFINES
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

   }

   CpSipTransactionManager::TransactionState m_transactionState;
};

/* //////////////////////////// PUBLIC //////////////////////////////////// */

/* ============================ CREATORS ================================== */

CpSipTransactionManager::CpSipTransactionManager()
: m_iCSeq(0)
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

void CpSipTransactionManager::endTransaction(const UtlString& sipMethod, int cseq)
{
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

/* ============================ FUNCTIONS ================================= */
