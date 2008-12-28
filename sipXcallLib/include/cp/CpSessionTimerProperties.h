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

#ifndef CpSessionTimerState_h__
#define CpSessionTimerState_h__

// SYSTEM INCLUDES
// APPLICATION INCLUDES
#include <utl/UtlString.h>
#include <cp/CpDefs.h>

// DEFINES
// MACROS
// EXTERNAL FUNCTIONS
// CONSTANTS
// STRUCTS
// TYPEDEFS
// MACROS
// FORWARD DECLARATIONS

/**
 * This class keeps various negotiated properties of session timer according to RFC4028.
 */
class CpSessionTimerProperties
{
   /* //////////////////////////// PUBLIC //////////////////////////////////// */
public:
   /* ============================ CREATORS ================================== */

   /** Constructor */
   CpSessionTimerProperties();

   /** Destructor */
   ~CpSessionTimerProperties();

   /* ============================ MANIPULATORS ============================== */

   /**
    * Resets session timer properties to initial configuration. If full reset,
    * then also session expiration is reset. Otherwise only refresher is reset.
    */
   void reset(UtlBoolean bFullReset = FALSE);

   /* ============================ ACCESSORS ================================= */

   /** 
    * Gets refresher value for given transaction direction. Refresher value depends on direction
    * of transaction.
    */
   UtlString getRefresher(UtlBoolean bIsOutboundTransaction) const;

   /** Gets negotiated refresher in transaction direction independent way */
   CP_SESSION_TIMER_REFRESH getRefresher() const { return m_sRefresher; }

   /** Reconfigures current refresher, and stores the value independent of INVITE/UPDATE transaction direction */
   void setRefresher(UtlString refresher, UtlBoolean bIsOutboundTransaction);

   int getSessionExpires() const { return m_sessionExpires; }
   void setSessionExpires(int val) { m_sessionExpires = val; }

   int getMinSessionExpires() const { return m_minSessionExpires; }
   void setMinSessionExpires(int val);

   void setInitialSessionExpires(int val) { m_initialSessionExpires = val; }
   void setInitialRefresher(CP_SESSION_TIMER_REFRESH sessionTimerRefresh) { m_sInitialRefresher = sessionTimerRefresh; }

   /* ============================ INQUIRY =================================== */

   /* //////////////////////////// PROTECTED ///////////////////////////////// */
protected:

   /* //////////////////////////// PRIVATE /////////////////////////////////// */
private:

   // initial properties, and properties after reset
   int m_initialSessionExpires; ///< initial time in seconds when session expires
   CP_SESSION_TIMER_REFRESH m_sInitialRefresher; ///< configuration of initial refresher, independent of call direction
   UtlBoolean m_bIsLocallyInitiatedCall; ///< TRUE if call is locally initiated

   CP_SESSION_TIMER_REFRESH m_sRefresher; ///< current refresher
   int m_sessionExpires; ///< time in seconds when session expires
   int m_minSessionExpires; ///< minimum session expiration time that might be specified in future INVITE/UPDATE messages
};

#endif // CpSessionTimerState_h__
