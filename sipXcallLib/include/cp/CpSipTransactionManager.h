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
 * startTransaction must be called with cseq, and endTransaction with cseq-1
 * when authentication retry message is detected.
 * SipUserAgent automatically just increments cseq for authentication retry.
 *
 * Terminated transactions are automatically cleaned up after some time.
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

   /* ============================ CREATORS ================================== */

   /** Constructor. */
   CpSipTransactionManager();

   /** Destructor. */
   ~CpSipTransactionManager();

   /* ============================ MANIPULATORS ============================== */

   /**
    * Starts new transaction for given method returning next cseq.
    */
   int startTransaction(const UtlString& sipMethod);

   /**
    * Starts new transaction for given method and cseq. This method should only
    * be called as a result of 401 or 407 authentication retry, when cseq is incremented
    * automatically by SipUserAgent without paying attention to the correct next cseq.
    *
    * If supplied cseq is greater or equal to our current cseq, internal next available
    * cseq is set to cseq + 1.
    */
   void startTransaction(const UtlString& sipMethod, int cseq);

   /**
    * Marks given transaction as terminated.
    */
   void endTransaction(const UtlString& sipMethod, int cseq);

   /* ============================ ACCESSORS ================================= */

   /* ============================ INQUIRY =================================== */

   /**
   * Finds transaction state for given sip method.
   *
   * @param szSipMethod Can be INVITE, INFO, NOTIFY, REFER, OPTIONS.
   * @param cseq CSeq number from Sip message.
   */
   CpSipTransactionManager::TransactionState getTransactionState(const UtlString& sipMethod, int cseq) const;

   /**
    * Finds transaction state for given sip message.
    */
   CpSipTransactionManager::TransactionState getTransactionState(const SipMessage& rSipMessage) const;

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

   UtlHashMap m_transactionMap; ///< hashmap for storing transactions by method name
   int m_iCSeq; ///< current available CSeq number
   long m_lastCleanUpTime;
};

#endif // CpSipTransactionManager_h__
