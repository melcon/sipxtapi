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

#ifndef StateTransitionMemory_h__
#define StateTransitionMemory_h__

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

/**
 * Memory object root for transferring information in transition of a state. Do not
 * insert your fields here, subclass it.
 *
 * Subclasses of this class must provide proper copy constructor and assignment operator!
 */
class StateTransitionMemory
{
   /* //////////////////////////// PUBLIC //////////////////////////////////// */
public:
   typedef enum
   {
      TYPE_UNKNOWN
   } Type;

   /* ============================ CREATORS ================================== */
   
   /** Constructor. */
   StateTransitionMemory() { }

   StateTransitionMemory(const StateTransitionMemory& rhs) { }

   /** Virtual destructor. */
   virtual ~StateTransitionMemory() { }

   /* ============================ MANIPULATORS ============================== */

   /* ============================ ACCESSORS ================================= */

   virtual Type getType() const
   {
      return TYPE_UNKNOWN;
   }

   /* ============================ INQUIRY =================================== */

   /* //////////////////////////// PROTECTED ///////////////////////////////// */
protected:

   /* //////////////////////////// PRIVATE /////////////////////////////////// */
private:

};

#endif // StateTransitionMemory_h__
