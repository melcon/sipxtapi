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

#ifndef CpSipTransactionManager_h__
#define CpSipTransactionManager_h__

// SYSTEM INCLUDES
// APPLICATION INCLUDES
#include <utl/UtlDefs.h>
#include <utl/UtlRandom.h>
#include <utl/UtlHashMap.h>
#include <cp/CpSipTransactionListener.h>

// DEFINES
// MACROS
// EXTERNAL FUNCTIONS
// CONSTANTS
// STRUCTS
// TYPEDEFS
// MACROS
// FORWARD DECLARATIONS
class SipMessage;

/**
 * CpSipTransactionManager is sipXcallLib layer transaction manager. This one uses Cseq number
 * and method name to differentiate between transactions, and supports transaction
 * tracking for multiple transactions with the same method at time. It is only meant
 * t be used for outbound transactions. Inbound transactions don't need to be tracked
 * in UAC/UAS core layer.
 * 
 * This transaction manager is needed because RFC3261 also requires transaction
 * tracking on UAC/UAS core layer, not only transaction layer. We cannot reuse
 * SipTransaction from transaction layer here. This class is more powerful than
 * old CSeqManager. It tracks correctly individual transactions, not only the last one.
 * 
 * This class is NOT meant to resend INVITE, CANCEL, BYE etc. when packet loss occurs.
 * That is done by transaction layer.
 *
 * ACK resend in INVITE transaction after 2xx is done when another 2xx is received.
 * (remote side didn't get ACK, and resent 2xx). ACK is part of INVITE transaction only
 * if final response was non 2xx.
 * 
 * To track transaction state correctly for 401 and 407 authentication retry,
 * updateActiveTransaction must be called with cseqNum, and endTransaction with cseqNum-1
 * when authentication retry message is detected.
 * SipUserAgent automatically just increments cseqNum for authentication retry.
 *
 * Terminated transactions are automatically cleaned up after some time.
 *
 * INVITE transactions are tracked differently. Only 1 INVITE transaction can be ocurring
 * at time. This class therefore supports tracking only 1 INVITE transaction.
 *
 * This class is NOT threadsafe.  
 */
class CpSipTransactionManager
{
   /* //////////////////////////// PUBLIC //////////////////////////////////// */
public:
   typedef enum
   {
      TRANSACTION_NOT_FOUND = 0,
      TRANSACTION_ACTIVE,
      TRANSACTION_TERMINATED
   } TransactionState;

   typedef enum
   {
      INVITE_INACTIVE = 0, ///< no INVITE transaction is active
      INITIAL_INVITE_ACTIVE, ///< initial INVITE transaction is active
      REINVITE_NORMAL_ACTIVE, ///< a normal re-INVITE transaction is active
      REINVITE_SESSION_REFRESH_ACTIVE ///< media session re-INVITE transaction is active, cannot fail
   } InviteTransactionState;

   /* ============================ CREATORS ================================== */

   /** Constructor. */
   CpSipTransactionManager();

   /** Destructor. */
   ~CpSipTransactionManager();

   /* ============================ MANIPULATORS ============================== */

   /**
    * Starts new transaction for given method returning next cseqNum.
    * For INVITE, this method starts normal INVITE transaction. For starting
    * re-INVITE transaction, use separate method. We treat re-INVITE transaction
    * in a different way - failure is not necessarily fatal.
    *
    * Only 1 INVITE/re-INVITE transaction may be active at time.
    *
    * @return New cseqNum
    */
   int startTransaction(const UtlString& sipMethod);

   /**
    * Starts new transaction for given method and cseqNum. This method should only
    * be called as a result of 401 or 407 authentication retry, when cseqNum is incremented
    * automatically by SipUserAgent without paying attention to the correct next cseqNum.
    *
    * If supplied cseqNum is greater or equal to our current cseqNum, internal next available
    * cseqNum is set to cseqNum + 1.
    *
    * Old transaction with cseqNum - 1 and same method is stopped automatically.
    */
   void updateActiveTransaction(const UtlString& sipMethod, int cseqNum);

   /**
    * Starts initial INVITE transaction. We allow only 1 INVITE transaction at time.
    * New cseqNum will be returned.
    */
   UtlBoolean startInitialInviteTransaction(int& cseqNum);

   /**
   * Starts re-INVITE transaction.
   *
   * @param bIsSessionRefresh TRUE for session timer initiated refresh. This must succeed,
   * otherwise call must be dropped. Normal re-INVITE failure can be ignored.
   */
   UtlBoolean startReInviteTransaction(int& cseqNum, UtlBoolean bIsSessionRefresh = FALSE);

   /**
    * Updates an active INVITE/re-INVITE transaction with new cseqNum. We need to supply
    * new cseqNum after 401/407 authentication retry, when cseqNum is normally incremented.
    * Old transaction will be terminated automatically.
    */
   UtlBoolean updateActiveInviteTransaction(int cseqNum);

   /**
    * Marks given transaction as terminated.
    *
    * This method can be used for INVITE transaction.
    */
   void endTransaction(const UtlString& sipMethod, int cseqNum);

   /**
    * Stops INVITE transaction regardless of cseqNum.
    */
   void endInviteTransaction();

   /** Sets listener that should be notified about transaction start/end .*/
   void setSipTransactionListener(CpSipTransactionListener* pTransactionListener);

   /* ============================ ACCESSORS ================================= */

   /* ============================ INQUIRY =================================== */

   /**
   * Finds transaction state for given sip method.
   *
   * @param szSipMethod Can be INVITE, INFO, NOTIFY, REFER, OPTIONS.
   * @param cseqNum CSeq number from Sip message.
   */
   CpSipTransactionManager::TransactionState getTransactionState(const UtlString& sipMethod, int cseqNum) const;

   /**
    * Finds transaction state for given sip message.
    */
   CpSipTransactionManager::TransactionState getTransactionState(const SipMessage& rSipMessage) const;

   /**
    * Gets state of invite transaction. This is more accurate than getTransactionState.
    */
   CpSipTransactionManager::InviteTransactionState getInviteTransactionState() const;

   /** Returns TRUE if an INVITE transaction is active */
   UtlBoolean isInviteTransactionActive() const;

   /* //////////////////////////// PROTECTED ///////////////////////////////// */
protected:

   /* //////////////////////////// PRIVATE /////////////////////////////////// */
private:
   /** Disabled copy constructor */ 
   CpSipTransactionManager(const CpSipTransactionManager& rhs);     

   /** Disabled assignment operator */
   CpSipTransactionManager& operator=(const CpSipTransactionManager& rhs);

   /** Cleans terminated transactions older than 60s */
   void cleanOldTransactions();

   /** Called to notify listeners that a transaction has been started */
   void notifyTransactionStart(const UtlString& sipMethod, int cseqNum);

   /** Called to notify listeners that a transaction has been stopped */
   void notifyTransactionEnd(const UtlString& sipMethod, int cseqNum);

   // members for generic transactions
   UtlHashMap m_transactionMap; ///< hashmap for storing transactions by method name
   int m_iCSeq; ///< current available CSeq number
   long m_lastCleanUpTime;
   CpSipTransactionListener* m_pTransactionListener; ///< listener that gets notifications about transaction start/end

   // INVITE transaction specific
   int m_iInviteCSeq; ///< CSeq of INVITE transaction
   InviteTransactionState m_inviteTransactionState; ///< keeps invite transaction state
};

#endif // CpSipTransactionManager_h__
