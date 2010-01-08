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
#include <net/SipDialog.h>
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
*
* This message is meant for communication between different XSipConnections through
* XCpCallManager. XCpCallManager knows how to route these messages correctly.
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

   ScNotificationMsg(SubTypesEnum subType, const SipDialog& sipDialog);

   virtual ~ScNotificationMsg();

   virtual OsMsg* createCopy(void) const;

   /* ============================ MANIPULATORS ============================== */

   /* ============================ ACCESSORS ================================= */

   void getSipDialog(SipDialog& val) const { val = m_sipDialog; }

   /* ============================ INQUIRY =================================== */

   /* //////////////////////////// PROTECTED ///////////////////////////////// */
protected:

   /* //////////////////////////// PRIVATE /////////////////////////////////// */
private:
   /** Private copy constructor */
   ScNotificationMsg(const ScNotificationMsg& rMsg);

   /** Private assignment operator */
   ScNotificationMsg& operator=(const ScNotificationMsg& rhs);

   SipDialog m_sipDialog; ///< sip dialog where this message should be routed
};

#endif // ScNotificationMsg_h__
