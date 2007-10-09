//
// Copyright (C) 2007 Jaroslav Libak
// Licensed under the LGPL license.
//
// Copyright (C) 2005-2007 SIPez LLC.
// Licensed to SIPfoundry under a Contributor Agreement.
// 
// Copyright (C) 2004-2007 SIPfoundry Inc.
// Licensed by SIPfoundry under the LGPL license.
//
// Copyright (C) 2004-2006 Pingtel Corp.  All rights reserved.
// Licensed to SIPfoundry under a Contributor Agreement.
//
// $$
///////////////////////////////////////////////////////////////////////////////

#ifndef CpMediaEventListener_h__
#define CpMediaEventListener_h__

// SYSTEM INCLUDES
// APPLICATION INCLUDES
#include <utl/UtlString.h>
#include "tapi/sipXtapiEvents.h"

// DEFINES
// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
// FORWARD DECLARATIONS
// STRUCTS
// TYPEDEFS
// MACROS
// GLOBAL VARIABLES
// GLOBAL FUNCTIONS

class CpMediaEvent
{
public:
   UtlString m_sCallId;
   UtlString m_sSessionCallId;
   UtlString m_sRemoteAddress;
   void* m_pEventData;
   SIPX_MEDIA_CAUSE m_Cause;
   SIPX_MEDIA_TYPE m_MediaType;
};


/**
* Listener for Media events
*/
class CpMediaEventListener
{
   /* //////////////////////////// PUBLIC //////////////////////////////////// */

   /* ============================ CREATORS ================================== */
public:
   CpMediaEventListener() {}
   virtual ~CpMediaEventListener() {}

   /* ============================ MANIPULATORS ============================== */

   virtual void OnMediaLocalStart(const CpMediaEvent& event) = 0;

   virtual void OnMediaLocalStop(const CpMediaEvent& event) = 0;

   virtual void OnMediaRemoteStart(const CpMediaEvent& event) = 0;

   virtual void OnMediaRemoteStop(const CpMediaEvent& event) = 0;

   virtual void OnMediaRemoteSilent(const CpMediaEvent& event) = 0;

   virtual void OnMediaPlayfileStart(const CpMediaEvent& event) = 0;

   virtual void OnMediaPlayfileStop(const CpMediaEvent& event) = 0;

   virtual void OnMediaPlaybufferStart(const CpMediaEvent& event) = 0;

   virtual void OnMediaPlaybufferStop(const CpMediaEvent& event) = 0;

   virtual void OnMediaPlaybackPaused(const CpMediaEvent& event) = 0;

   virtual void OnMediaPlaybackResumed(const CpMediaEvent& event) = 0;

   virtual void OnMediaRemoteDTMF(const CpMediaEvent& event) = 0;

   virtual void OnMediaDeviceFailure(const CpMediaEvent& event) = 0;

   virtual void OnMediaRemoteActive(const CpMediaEvent& event) = 0;

   virtual void OnMediaRecordingStart(const CpMediaEvent& event) = 0;

   virtual void OnMediaRecordingStop(const CpMediaEvent& event) = 0;

   /* ============================ ACCESSORS ================================= */

   /* ============================ INQUIRY =================================== */

   /* //////////////////////////// PROTECTED ///////////////////////////////// */
protected:

   /* //////////////////////////// PRIVATE /////////////////////////////////// */
private:

};

#endif // CpMediaEventListener_h__



