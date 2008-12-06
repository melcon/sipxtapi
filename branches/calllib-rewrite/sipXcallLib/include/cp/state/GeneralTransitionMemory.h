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

#ifndef GeneralTransitionMemory_h__
#define GeneralTransitionMemory_h__

// SYSTEM INCLUDES
// APPLICATION INCLUDES
#include <utl/UtlString.h>
#include <cp/CpDefs.h>
#include <cp/state/StateTransitionMemory.h>

// DEFINES
// MACROS
// EXTERNAL FUNCTIONS
// CONSTANTS
// STRUCTS
// TYPEDEFS
// MACROS
// FORWARD DECLARATIONS

/**
 * GeneralTransitionMemory keeps call state cause, SIP response code and text.
 */
class GeneralTransitionMemory : public StateTransitionMemory
{
   /* //////////////////////////// PUBLIC //////////////////////////////////// */
public:
   /* ============================ CREATORS ================================== */

   /** Constructor. */
   GeneralTransitionMemory(CP_CALLSTATE_CAUSE cause,
                           int sipResponseCode = 0,
                           const UtlString& sipResponseText = NULL);

   /** Destructor. */
   virtual ~GeneralTransitionMemory();

   /* ============================ MANIPULATORS ============================== */

   /** Creates copy of transition memory. */
   virtual StateTransitionMemory* clone() const;

   /* ============================ ACCESSORS ================================= */

   int getSipResponseCode() const { return m_sipResponseCode; }
   UtlString getSipResponseText() const { return m_sipResponseText; }
   CP_CALLSTATE_CAUSE getCause() const { return m_cause; }

   virtual StateTransitionMemory::Type getType() const
   {
      return SIP_RESPONSE_MEMORY;
   }

   /* ============================ INQUIRY =================================== */

   /* //////////////////////////// PROTECTED ///////////////////////////////// */
protected:

   /* //////////////////////////// PRIVATE /////////////////////////////////// */
private:

   CP_CALLSTATE_CAUSE m_cause; ///< cause of event
   int m_sipResponseCode; ///< stores sip response code
   UtlString m_sipResponseText; ///< stores response text
};

#endif // GeneralTransitionMemory_h__
