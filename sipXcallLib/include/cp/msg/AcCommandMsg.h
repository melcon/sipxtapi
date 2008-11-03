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

#ifndef AcCommandMsg_h__
#define AcCommandMsg_h__

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
* Abstract call command message. Instructs call to carry out some action.
*/
class AcCommandMsg : public OsMsg
{
   /* //////////////////////////// PUBLIC //////////////////////////////////// */
public:
   typedef enum
   {
      AC_GAIN_FOCUS = 0,///< gain local focus (mic, speaker)
      AC_YIELD_FOCUS, ///< loose local focus
      AC_CONNECT, ///< connects existing free call shell
      AC_ACCEPT_CONNECTION, ///< accepts inbound call
      AC_REJECT_CONNECTION, ///< rejects inbound call
      AC_REDIRECT_CONNECTION, ///< redirects inbound call
      AC_ANSWER_CONNECTION, ///< answers inbound call
      AC_DROP_CONNECTION, ///< drops inbound connection, optionally also dropping the call
      AC_TRANSFER_BLIND, ///< initiates blind call transfer
      AC_HOLD_CONNECTION, ///< holds connection
      AC_UNHOLD_CONNECTION, ///< unholds connection
      AC_LIMIT_CODEC_PREFERENCES, ///< limits codec preferences for future calls
      AC_RENEGOTIATE_CODECS, ///< renegotiates connection codecs
      AC_SEND_INFO ///< send SIP INFO message
   } SubTypesEnum;

   /* ============================ CREATORS ================================== */

   AcCommandMsg(SubTypesEnum subType);

   virtual ~AcCommandMsg();

   virtual OsMsg* createCopy(void) const;

   /* ============================ MANIPULATORS ============================== */

   /* ============================ ACCESSORS ================================= */

   /* ============================ INQUIRY =================================== */

   /* //////////////////////////// PROTECTED ///////////////////////////////// */
protected:

   /* //////////////////////////// PRIVATE /////////////////////////////////// */
private:
};

#endif // AcCommandMsg_h__
