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

// DEFINES
// MACROS
// EXTERNAL FUNCTIONS
// CONSTANTS
// STRUCTS
// TYPEDEFS
// MACROS
// FORWARD DECLARATIONS

/**
 * CpSipTransactionManager is sipXcallLib layer transaction manager. This one uses Cseq
 * and method name to differentiate between transactions, and supports transaction
 * tracking for multiple transactions with the same method at time.
 * 
 * This transaction manager is needed because RFC3261 also requires transaction
 * tracking on UAC/UAS core layer, not only transaction layer. We cannot reuse
 * SipTransaction from transaction layer here. This class is more powerful than
 * old CSeqManager.
 * 
 * This class is NOT meant to resend INVITE, CANCEL, BYE etc. when packet loss occurs.
 * That is done by transaction layer. It is required for resending reliable provisional
 * responses (RFC3262).
 * 
 * ACK resend in INVITE transaction after 2xx is done when another 2xx is received.
 * (remote side didn't get ACK, and resent 2xx). ACK is part of INVITE transaction only
 * if final response was non 2xx. 
 * 
 * May be used to check that only 1 INVITE transaction is running at time.
 *
 * This class is NOT threadsafe.  
 */
class CpSipTransactionManager
{
   /* //////////////////////////// PUBLIC //////////////////////////////////// */
public:
   /* ============================ CREATORS ================================== */

   /* ============================ MANIPULATORS ============================== */

   /* ============================ ACCESSORS ================================= */

   /* ============================ INQUIRY =================================== */

   /* //////////////////////////// PROTECTED ///////////////////////////////// */
protected:

   /* //////////////////////////// PRIVATE /////////////////////////////////// */
private:

};

#endif // CpSipTransactionManager_h__
