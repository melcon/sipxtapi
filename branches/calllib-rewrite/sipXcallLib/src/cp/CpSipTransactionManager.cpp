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
   CpTransactionState(int cseqNum)
      : UtlInt(cseqNum)
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
, m_iInviteCSeq(-1)
, m_inviteTransactionState(CpSipTransactionManager::INVITE_INACTIVE)
, m_pTransactionListener(NULL)
{
   UtlRandom randomGenerator;
   m_iCSeq = (abs(randomGenerator.rand()) % 65535);
}

CpSipTransactionManager::~CpSipTransactionManager()
{
   m_pTransactionListener = NULL;
   m_transactionMap.destroyAll();
}

/* ============================ MANIPULATORS ============================== */

int CpSipTransactionManager::startTransaction(const UtlString& sipMethod)
{
   if (sipMethod.compareTo(SIP_INVITE_METHOD) != 0)
   {
      // non INVITE transaction
      UtlHashBag* pTransactionBag = dynamic_cast<UtlHashBag*>(m_transactionMap.findValue(&sipMethod));
      if (!pTransactionBag)
      {
         // create new transaction bag
         pTransactionBag = new UtlHashBag();
         m_transactionMap.insertKeyAndValue(sipMethod.clone(), pTransactionBag);
      }
      pTransactionBag->insert(new CpTransactionState(m_iCSeq));

      notifyTransactionStart(sipMethod, m_iCSeq);
      return m_iCSeq++; // post increment cseq
   }
   else
   {
      int cseq;
      UtlBoolean success = startInitialInviteTransaction(cseq);
      if (success)
      {
         return cseq;
      }
   }

   return -1; // return invalid cseqNum
}

void CpSipTransactionManager::updateActiveTransaction(const UtlString& sipMethod, int cseqNum)
{
   if (sipMethod.compareTo(SIP_INVITE_METHOD) != 0)
   {
      // non INVITE transaction

      // automatically stop old transaction
      endTransaction(sipMethod, cseqNum - 1);

      UtlHashBag* pTransactionBag = dynamic_cast<UtlHashBag*>(m_transactionMap.findValue(&sipMethod));
      if (!pTransactionBag)
      {
         // create new transaction bag
         pTransactionBag = new UtlHashBag();
         m_transactionMap.insertKeyAndValue(sipMethod.clone(), pTransactionBag);
      }

      UtlInt utlCSeq(cseqNum);
      CpTransactionState* pTransactionState = dynamic_cast<CpTransactionState*>(pTransactionBag->find(&utlCSeq));
      if (!pTransactionState)
      {
         pTransactionBag->insert(new CpTransactionState(cseqNum)); // if its not already there then add it
      }

      notifyTransactionStart(sipMethod, cseqNum);

      if (m_iCSeq <= cseqNum)
      {
         m_iCSeq = cseqNum + 1; // set next available cseqNum number
      }
   }
   else
   {
      // INVITE transaction
      updateActiveInviteTransaction(cseqNum);
   }
}

UtlBoolean CpSipTransactionManager::startInitialInviteTransaction(int& cseqNum)
{
   if (m_inviteTransactionState == CpSipTransactionManager::INVITE_INACTIVE)
   {
      m_inviteTransactionState = CpSipTransactionManager::INITIAL_INVITE_ACTIVE;
      m_iInviteCSeq = m_iCSeq;
      cseqNum = m_iCSeq++; // post increment cseqNum
      notifyTransactionStart(SIP_INVITE_METHOD, cseqNum);
      return TRUE;
   }
   else
   {
      return FALSE;
   }
}

UtlBoolean CpSipTransactionManager::startReInviteTransaction(int& cseqNum)
{
   if (m_inviteTransactionState == CpSipTransactionManager::INVITE_INACTIVE)
   {
      m_inviteTransactionState = CpSipTransactionManager::REINVITE_ACTIVE;
      m_iInviteCSeq = m_iCSeq;
      cseqNum = m_iCSeq++; // post increment cseqNum
      notifyTransactionStart(SIP_INVITE_METHOD, cseqNum);
      return TRUE;
   }
   else
   {
      return FALSE;
   }
}

UtlBoolean CpSipTransactionManager::updateActiveInviteTransaction(int cseqNum)
{
   if (m_inviteTransactionState != CpSipTransactionManager::INVITE_INACTIVE &&
      m_iInviteCSeq != cseqNum)
   {
      notifyTransactionEnd(SIP_INVITE_METHOD, cseqNum); // end old transaction
      m_iInviteCSeq = cseqNum; // update cseqNum
      notifyTransactionStart(SIP_INVITE_METHOD, cseqNum); // start new transaction
      m_iCSeq = cseqNum + 1; // set next available cseqNum number
      return TRUE;
   }

   return FALSE;
}

void CpSipTransactionManager::endTransaction(const UtlString& sipMethod, int cseqNum)
{
   cleanOldTransactions(); // maybe cleanup old terminated transactions

   if (sipMethod.compareTo(SIP_INVITE_METHOD) == 0)
   {
      // INVITE transaction
      if (m_iInviteCSeq == cseqNum)
      {
         endInviteTransaction();
      }
   }
   else
   {
      // non INVITE transaction
      UtlHashBag* pTransactionBag = dynamic_cast<UtlHashBag*>(m_transactionMap.findValue(&sipMethod));
      if (pTransactionBag)
      {
         UtlInt utlCSeq(cseqNum);
         CpTransactionState* pTransactionState = dynamic_cast<CpTransactionState*>(pTransactionBag->find(&utlCSeq));
         if (pTransactionState)
         {
            notifyTransactionEnd(sipMethod, cseqNum); // end old transaction
            pTransactionState->m_transactionState = CpSipTransactionManager::TRANSACTION_TERMINATED;
         }
      }
   }
}

void CpSipTransactionManager::endInviteTransaction()
{
   notifyTransactionEnd(SIP_INVITE_METHOD, m_iInviteCSeq); // end old transaction
   m_iInviteCSeq = -1;
   m_inviteTransactionState = CpSipTransactionManager::INVITE_INACTIVE;
}

void CpSipTransactionManager::setSipTransactionListener(CpSipTransactionListener* pTransactionListener)
{
   m_pTransactionListener = pTransactionListener;
}

/* ============================ ACCESSORS ================================= */

/* ============================ INQUIRY =================================== */

CpSipTransactionManager::TransactionState CpSipTransactionManager::getTransactionState(const UtlString& sipMethod,
                                                                                       int cseqNum) const
{
   if (sipMethod.compareTo(SIP_INVITE_METHOD) == 0)
   {
      // INVITE transaction
      if (m_iInviteCSeq != -1 && m_iInviteCSeq == cseqNum)
      {
         // we do not track terminated INVITE transactions
         return CpSipTransactionManager::TRANSACTION_ACTIVE;
      }
   }
   else
   {
      // non INVITE transaction
      UtlHashBag* pTransactionBag = dynamic_cast<UtlHashBag*>(m_transactionMap.findValue(&sipMethod));
      if (pTransactionBag)
      {
         UtlInt utlCSeq(cseqNum);
         CpTransactionState* pTransactionState = dynamic_cast<CpTransactionState*>(pTransactionBag->find(&utlCSeq));
         if (pTransactionState)
         {
            return pTransactionState->m_transactionState;
         }
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

CpSipTransactionManager::InviteTransactionState CpSipTransactionManager::getInviteTransactionState() const
{
   return m_inviteTransactionState;
}

UtlBoolean CpSipTransactionManager::isInviteTransactionActive() const
{
   return m_inviteTransactionState == CpSipTransactionManager::INITIAL_INVITE_ACTIVE;
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

void CpSipTransactionManager::notifyTransactionStart(const UtlString& sipMethod, int cseqNum)
{
   if (m_pTransactionListener)
   {
      m_pTransactionListener->onTransactionStart(sipMethod, cseqNum);
   }
}

void CpSipTransactionManager::notifyTransactionEnd(const UtlString& sipMethod, int cseqNum)
{
   if (m_pTransactionListener)
   {
      m_pTransactionListener->onTransactionEnd(sipMethod, cseqNum);
   }
}

/* ============================ FUNCTIONS ================================= */
