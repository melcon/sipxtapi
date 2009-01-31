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

#ifndef ScCommandMsg_h__
#define ScCommandMsg_h__

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
* Sip connection command message. Instructs sip connection to carry out some action.
*
* This message is meant for communication between different XSipConnections through
* XCpCallManager. XCpCallManager knows how to route these messages correctly.
*/
class ScCommandMsg : public OsMsg
{
   /* //////////////////////////// PUBLIC //////////////////////////////////// */
public:
   typedef enum
   {
      CM_EMPTY = 0,
   } SubTypesEnum;

   /* ============================ CREATORS ================================== */

   ScCommandMsg(SubTypesEnum subType, const SipDialog& sipDialog);

   virtual ~ScCommandMsg();

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
   ScCommandMsg(const ScCommandMsg& rMsg);

   /** Private assignment operator */
   ScCommandMsg& operator=(const ScCommandMsg& rhs);

   SipDialog m_sipDialog; ///< sip dialog where this message should be routed
};

#endif // ScCommandMsg_h__
