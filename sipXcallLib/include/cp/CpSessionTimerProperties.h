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
   typedef enum
   {
      REFRESH_DISABLED,
      REFRESHER_UAC,
      REFRESHER_UAS
   } RefresherType;

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

   /** Gets negotiated refresher type. Only valid after session timer negotiation */
   CpSessionTimerProperties::RefresherType getRefresherType() const;

   UtlString getRefresher() const { return m_sRefresher; }
   void setRefresher(UtlString val) { m_sRefresher = val; }

   int getSessionExpires() const { return m_sessionExpires; }
   void setSessionExpires(int val) { m_sessionExpires = val; }

   int getMinSessionExpires() const { return m_minSessionExpires; }
   void setMinSessionExpires(int val);

   void setInitialSessionExpires(int val) { m_initialSessionExpires = val; }
   void setInitialRefresher(UtlString val) { m_sInitialRefresher = val; }

   /* ============================ INQUIRY =================================== */

   /* //////////////////////////// PROTECTED ///////////////////////////////// */
protected:

   /* //////////////////////////// PRIVATE /////////////////////////////////// */
private:

   // initial properties, and properties after reset
   int m_initialSessionExpires; ///< initial time in seconds when session expires
   UtlString m_sInitialRefresher; ///< initial uas or uac

   UtlString m_sRefresher; ///< uas or uac
   int m_sessionExpires; ///< time in seconds when session expires
   int m_minSessionExpires; ///< minimum session expiration time that might be specified in future INVITE/UPDATE messages
};

#endif // CpSessionTimerState_h__
