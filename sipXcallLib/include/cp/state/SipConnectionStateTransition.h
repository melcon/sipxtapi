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

#ifndef SipConnectionStateTransition_h__
#define SipConnectionStateTransition_h__

// SYSTEM INCLUDES
// APPLICATION INCLUDES
// DEFINES
// MACROS
// EXTERNAL FUNCTIONS
// CONSTANTS
// STRUCTS
// TYPEDEFS
// MACROS
// FORWARD DECLARATIONS
class BaseSipConnectionState;

/**
 * Class Description
 */
class SipConnectionStateTransition
{
   /* //////////////////////////// PUBLIC //////////////////////////////////// */
public:
   /* ============================ CREATORS ================================== */

   SipConnectionStateTransition(BaseSipConnectionState* pSource,
                                BaseSipConnectionState* pDestination);

   ~SipConnectionStateTransition();

   /* ============================ MANIPULATORS ============================== */

   /* ============================ ACCESSORS ================================= */

   BaseSipConnectionState* getSource() const { return m_pSource; }
   BaseSipConnectionState* getDestination() const { return m_pDestination; }

   /* ============================ INQUIRY =================================== */

   /* //////////////////////////// PROTECTED ///////////////////////////////// */
protected:

   /* //////////////////////////// PRIVATE /////////////////////////////////// */
private:

   BaseSipConnectionState* m_pSource;
   BaseSipConnectionState* m_pDestination;
};

#endif // SipConnectionStateTransition_h__
