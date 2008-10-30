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

#ifndef ScNotificationMsg_h__
#define ScNotificationMsg_h__

// SYSTEM INCLUDES
// APPLICATION INCLUDES
#include <os/OsDefs.h>
#include <os/OsMsg.h>
#include <cp/CpMessageTypes.h>

// DEFINES
// MACROS
// EXTERNAL FUNCTIONS
// CONSTANTS
// STRUCTS
// TYPEDEFS
// MACROS
// FORWARD DECLARATIONS

/**
* Sip connection notification message. Informs sip connection about some event.
*/
class ScNotificationMsg : public OsMsg
{
   /* //////////////////////////// PUBLIC //////////////////////////////////// */
public:
   typedef enum
   {
      CM_EMPTY = 0,
   } SubTypesEnum;

   /* ============================ CREATORS ================================== */

   ScNotificationMsg(SubTypesEnum subType);

   virtual ~ScNotificationMsg();

   virtual OsMsg* createCopy(void) const;

   /* ============================ MANIPULATORS ============================== */

   /* ============================ ACCESSORS ================================= */

   /* ============================ INQUIRY =================================== */

   /* //////////////////////////// PROTECTED ///////////////////////////////// */
protected:

   /* //////////////////////////// PRIVATE /////////////////////////////////// */
private:
};

#endif // ScNotificationMsg_h__
