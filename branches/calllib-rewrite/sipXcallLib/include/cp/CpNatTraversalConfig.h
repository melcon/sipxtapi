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

#ifndef CpNatTraversalConfig_h__
#define CpNatTraversalConfig_h__

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
 * Container for holding NAT traversal configuration.
 */
class CpNatTraversalConfig
{
   /* //////////////////////////// PUBLIC //////////////////////////////////// */
public:
   // not thread safe
   UtlString m_sStunServer; ///< address or ip of stun server
   int m_iStunPort; ///< port for stun server
   int m_iStunKeepAlivePeriodSecs; ///< stun refresh period

   UtlString m_sTurnServer; ///< turn server address or ip
   int m_iTurnPort; ///< turn server port
   UtlString m_sTurnUsername; ///< turn username
   UtlString m_sTurnPassword; ///< turn password
   int m_iTurnKeepAlivePeriodSecs; ///< turn refresh period

   // thread safe atomic
   UtlBoolean m_bEnableICE; 

   /* ============================ CREATORS ================================== */

   /** Default constructor */
   CpNatTraversalConfig()
      : m_iStunPort(0)
      , m_iStunKeepAlivePeriodSecs(0)
      , m_iTurnPort(0)
      , m_iTurnKeepAlivePeriodSecs(0)
      , m_bEnableICE(FALSE)
   {

   }
   
   /** Copy constructor */
   CpNatTraversalConfig(const CpNatTraversalConfig& rhs)
      : m_sStunServer(rhs.m_sStunServer)
      , m_iStunPort(rhs.m_iStunPort)
      , m_iStunKeepAlivePeriodSecs(rhs.m_iStunKeepAlivePeriodSecs)
      , m_sTurnServer(rhs.m_sTurnServer)
      , m_iTurnPort(rhs.m_iTurnPort)
      , m_sTurnUsername(rhs.m_sTurnUsername)
      , m_sTurnPassword(rhs.m_sTurnPassword)
      , m_iTurnKeepAlivePeriodSecs(rhs.m_iTurnKeepAlivePeriodSecs)
      , m_bEnableICE(rhs.m_bEnableICE)
   {

   }

   /** Assignment operator */
   CpNatTraversalConfig& operator=(const CpNatTraversalConfig& rhs)
   {
      if (&rhs == this)
      {
         return *this; // handle self assignment
      }

      m_sStunServer = rhs.m_sStunServer;
      m_iStunPort = rhs.m_iStunPort;
      m_iStunKeepAlivePeriodSecs = rhs.m_iStunKeepAlivePeriodSecs;
      m_sTurnServer = rhs.m_sTurnServer;
      m_iTurnPort = rhs.m_iTurnPort;
      m_sTurnUsername = rhs.m_sTurnUsername;
      m_sTurnPassword = rhs.m_sTurnPassword;
      m_iTurnKeepAlivePeriodSecs = rhs.m_iTurnKeepAlivePeriodSecs;
      m_bEnableICE = rhs.m_bEnableICE;

      return *this;
   }

   /* ============================ MANIPULATORS ============================== */

   /* ============================ ACCESSORS ================================= */

   /* ============================ INQUIRY =================================== */

   /* //////////////////////////// PROTECTED ///////////////////////////////// */
protected:

   /* //////////////////////////// PRIVATE /////////////////////////////////// */
private:

};

#endif // CpNatTraversalConfig_h__
