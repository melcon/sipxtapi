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

#ifndef CpMediaInterfaceProvider_h__
#define CpMediaInterfaceProvider_h__

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
class CpMediaInterface;

/**
 * Abstraction for CpMediaInterface provider. Sometimes we might need to share
 * the same CpMediaInterface in multiple classes, but might want to initialize
 * it lazily. Passing pointers is then not an option. Provider is meant to
 * get the current CpMediaInterface if it exists from class managing CpMediaInterfaces.
 */
class CpMediaInterfaceProvider
{
   /* //////////////////////////// PUBLIC //////////////////////////////////// */
public:
   /* ============================ CREATORS ================================== */

   /* ============================ MANIPULATORS ============================== */

   /* ============================ ACCESSORS ================================= */

   /**
    * Gets current CpMediaInterface if it exists. If not then a new one is created.
    */
   virtual CpMediaInterface* getMediaInterface() = 0;

   /* ============================ INQUIRY =================================== */

   /* //////////////////////////// PROTECTED ///////////////////////////////// */
protected:

   /* //////////////////////////// PRIVATE /////////////////////////////////// */
private:

};

#endif // CpMediaInterfaceProvider_h__
