//
// Copyright (C) 2004-2006 SIPfoundry Inc.
// Licensed by SIPfoundry under the LGPL license.
//
// Copyright (C) 2004-2006 Pingtel Corp.  All rights reserved.
// Licensed to SIPfoundry under a Contributor Agreement.
//
// $$
///////////////////////////////////////////////////////////////////////////////


#ifndef _CpCall_h_
#define _CpCall_h_

// SYSTEM INCLUDES
// APPLICATION INCLUDES
#include <os/OsServerTask.h>
#include <os/OsMutex.h>
#include <ptapi/PtEvent.h>

// DEFINES
// MACROS
// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
// STRUCTS
// TYPEDEFS
// FORWARD DECLARATIONS
class CpCallManager;
class CpMediaInterface;
class OsEventMsg;
class Connection;

//:Class short description which may consist of multiple lines (note the ':')
// Class detailed description which may extend to multiple lines
class CpCall : public OsServerTask
{
   /* //////////////////////////// PUBLIC //////////////////////////////////// */
public:

   enum eventType
   {
      CONNECTION_STATE = 0,
      TERMINAL_CONNECTION_STATE,
      CALL_STATE
   };

   enum callTypes
   {
      CP_NORMAL_CALL,

      // There are three types of parties involved with a transfer:
      // Transfer Controller -
      // Transferee -
      // Transfer Target -
      // There are two calls involved with a transfer:
      // Original call - between the transfer controller and transferee(s)
      // Target Call - between all parties, the transfer controller and
      //     transfer target connect on this call first.  If this is
      //     consultative the transfer controller does not drop out
      //     immediately
      CP_TRANSFER_CONTROLLER_ORIGINAL_CALL,
      CP_TRANSFER_CONTROLLER_TARGET_CALL,
      CP_TRANSFEREE_ORIGINAL_CALL,
      CP_TRANSFEREE_TARGET_CALL,
      CP_TRANSFER_TARGET_TARGET_CALL
   };

   // The following enumeration defines the degree of willingness
   // that a call has for handling a message.  It is not always
   // a binary decision, so this allows a weighting as to the amount
   // of willingness.
   enum handleWillingness
   {
      // Does not match by any means
      CP_WILL_NOT_HANDLE = 0,

      // Will handle if a better match is not found
      // (i.e. check all the remaining calls first).
      CP_MAY_HANDLE,

      // Is a definite match no need to search any further
      CP_DEFINITELY_WILL_HANDLE
   };
   /* ============================ CREATORS ================================== */

   CpCall(CpCallManager* manager = NULL,
      CpMediaInterface* callMediaInterface = NULL,
      int callIndex = -1,
      const char* callId = NULL);
   //:Default constructor

   virtual ~CpCall();
   //:Destructor

   /* ============================ MANIPULATORS ============================== */

   void setDropState(UtlBoolean state);

   void postTaoListenerMessage(int responseCode,
      UtlString responseText,
      int eventId,
      int type,
      int cause = PtEvent::CAUSE_NORMAL,
      int remoteIsCallee = 1,
      UtlString remoteAddress = "",
      int isRemote = 0,
      UtlString targetCallId = NULL);

   void setCallState(int responseCode, UtlString responseText, int state, int cause = PtEvent::CAUSE_NORMAL);

   virtual void inFocus(int talking = 1);
   virtual void outOfFocus();

   virtual void localHold();
   virtual void hangUp(UtlString callId, int metaEventId);

   virtual void getCallId(UtlString& callId);
   //: Gets the main call Id for this call
   // Note: a call may have many callIds (i.e. one for each connection)
   virtual void setCallId(const char* callId);
   //: Sets the main call Id for this call

   virtual OsStatus getConnectionCallIds(UtlSList& pCallIdList) = 0;

   void setLocalConnectionState(int newState);
   //: Sets the local connection state for this call

   /* ============================ ACCESSORS ================================= */

   int getCallIndex();
   int getCallState();

   // Meta Event Utilities
   // For the meta events, the first callId (index=0) is the new
   // call, the subsequent callIds (index = 1 through numCalls)
   // are the old calls
   virtual void startMetaEvent(int metaEventId, int metaEventType,
      int numCalls, const char* metaEventCallIds[], int remoteIsCallee = -1); // remoteIsCallee = -1 means not set

   virtual void setMetaEvent(int metaEventId, int metaEventType,
      int numCalls, const char* metaEventCallIds[]);

   void getMetaEvent(int& metaEventId, int& metaEventType,
      int& numCalls, const UtlString* metaEventCallIds[]) const;

   virtual void stopMetaEvent(int remoteIsCallee = -1); // remoteIsCallee = -1 means not set

   void setCallType(int callType);
   int getCallType() const;

   void setTargetCallId(const char* targetCallId);
   void getTargetCallId(UtlString& targetCallId) const;
   void setOriginalCallId(const char* targetCallId);
   void getOriginalCallId(UtlString& targetCallId) const;

   int getLocalConnectionStateFromPt(int state);

   UtlString getBindIPAddress() const;
   void setBindIPAddress(const UtlString& val);

   /* ============================ INQUIRY =================================== */

   virtual UtlBoolean hasCallId(const char* callId) = 0;

   virtual enum handleWillingness willHandleMessage(const OsMsg& eventMessage) = 0;

   virtual UtlBoolean isCallIdSet();

   virtual UtlBoolean canDisconnectConnection(Connection* pConnection) = 0;

   virtual UtlBoolean isInFocus() const { return mCallInFocus;};

   /* //////////////////////////// PROTECTED ///////////////////////////////// */
protected:
   virtual UtlBoolean handleMessage(OsMsg& eventMessage);
   virtual UtlBoolean handleCallMessage(OsMsg& eventMessage) = 0;
   virtual UtlBoolean handleConnectionNotfMessage(OsMsg& eventMessage) = 0;
   virtual UtlBoolean handleInterfaceNotfMessage(OsMsg& eventMessage) = 0;
   virtual void onHook() = 0;

   CpCallManager* mpManager;
   UtlString mCallId;
   UtlString m_bindIPAddress;
   volatile UtlBoolean mCallInFocus;
   mutable OsMutex m_memberMutex;
   CpMediaInterface* mpMediaInterface;
   int mCallIndex;
   int mCallState;
   int mLocalConnectionState;
   int mLocalTermConnectionState;
   UtlBoolean mLocalHeld;

   UtlBoolean mDropping;
   int mMetaEventId;
   int mMetaEventType;
   int mNumMetaEventCalls;
   UtlString* mpMetaEventCallIds;

   /* //////////////////////////// PRIVATE /////////////////////////////////// */
private:
   int mCallType;
   UtlString mOriginalCallId;
   UtlString mTargetCallId;

   CpCall& operator=(const CpCall& rhs);
   //:Assignment operator (disabled)
   CpCall(const CpCall& rCpCall);
   //:Copy constructor (disabled)

   int tcStateFromEventId(int eventId);

};

/* ============================ INLINE METHODS ============================ */

#endif  // _CpCall_h_
