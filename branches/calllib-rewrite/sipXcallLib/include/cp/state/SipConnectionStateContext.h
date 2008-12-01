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

#ifndef SipConnectionStateContext_h__
#define SipConnectionStateContext_h__

// SYSTEM INCLUDES
// APPLICATION INCLUDES
#include <net/SipTagGenerator.h>
#include <cp/XSipConnectionContext.h>
#include <cp/Cp100RelTracker.h>
#include <cp/CpSdpNegotiation.h>
#include <cp/CpSipTransactionManager.h>

// DEFINES
// MACROS
// EXTERNAL FUNCTIONS
// CONSTANTS
// STRUCTS
// TYPEDEFS
// MACROS
// FORWARD DECLARATIONS

/**
 * SipConnectionStateContext contains public members which are visible only to state itself
 * and don't need to be locked when accessed.
 *
 * When accessing members of XSipConnectionContext, this class needs to be locked as those
 * members are public.
 */
class SipConnectionStateContext : public XSipConnectionContext
{
   /* //////////////////////////// PUBLIC //////////////////////////////////// */
public:
   /**
    * MediaSessionState tracks state of media session - if RTP is flowing or not.
    */
   typedef enum
   {
      MEDIA_SESSION_NONE = 0, ///< initial state
      MEDIA_SESSION_HOLD_REQUESTED, ///< hold operation has been initiated
      MEDIA_SESSION_HELD, ///< media session is held
      MEDIA_SESSION_ACTIVE, ///< media session is active
      MEDIA_SESSION_UNHOLD_REQUESTED ///< unhold operation has been initiated
   } MediaSessionState;

   MediaSessionState m_mediaSessionState; ///< keeps track of media session state (active, held etc)
   CpSdpNegotiation m_sdpNegotiation; ///< tracks state of SDP negotiation
   UtlString m_allowedRemote;  ///< methods supported by the other side
   UtlString m_implicitAllowedRemote; ///< methods which are allowed implicitly
   SipTagGenerator m_sipTagGenerator; ///< generator for sip tags
   Cp100RelTracker m_100RelTracker; ///< tracker for 100rel responses and PRACKs
   CpSipTransactionManager m_sipTransactionMgr; ///< sip transaction tracking & cseq generator

   /* ============================ CREATORS ================================== */

   /** Constructor */
   SipConnectionStateContext();

   /** Destructor */
   ~SipConnectionStateContext();

   /* ============================ MANIPULATORS ============================== */

   /* ============================ ACCESSORS ================================= */

   /* ============================ INQUIRY =================================== */

   /* //////////////////////////// PROTECTED ///////////////////////////////// */
protected:

   /* //////////////////////////// PRIVATE /////////////////////////////////// */
private:
   SipConnectionStateContext(const SipConnectionStateContext& rhs);

   SipConnectionStateContext& operator=(const SipConnectionStateContext& rhs);

};

#endif // SipConnectionStateContext_h__
