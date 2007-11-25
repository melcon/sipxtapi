//
// Copyright (C) 2007 Jaroslav Libak
// Licensed to SIPfoundry under a Contributor Agreement.
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

#ifndef SipXMediaEventListener_h__
#define SipXMediaEventListener_h__

// SYSTEM INCLUDES
// APPLICATION INCLUDES
#include "tapi/sipXtapi.h"
#include "cp/CpMediaEventListener.h"

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


/**
* Listener for Media events
*/
class SipXMediaEventListener : public CpMediaEventListener
{
   /* //////////////////////////// PUBLIC //////////////////////////////////// */

   /* ============================ CREATORS ================================== */
public:
   SipXMediaEventListener(SIPX_INST pInst);
   virtual ~SipXMediaEventListener();

   /* ============================ MANIPULATORS ============================== */

   virtual void OnMediaLocalStart(const CpMediaEvent& event);

   virtual void OnMediaLocalStop(const CpMediaEvent& event);

   virtual void OnMediaRemoteStart(const CpMediaEvent& event);

   virtual void OnMediaRemoteStop(const CpMediaEvent& event);

   virtual void OnMediaRemoteSilent(const CpMediaEvent& event);

   virtual void OnMediaPlayfileStart(const CpMediaEvent& event);

   virtual void OnMediaPlayfileStop(const CpMediaEvent& event);

   virtual void OnMediaPlaybufferStart(const CpMediaEvent& event);

   virtual void OnMediaPlaybufferStop(const CpMediaEvent& event);

   virtual void OnMediaPlaybackPaused(const CpMediaEvent& event);

   virtual void OnMediaPlaybackResumed(const CpMediaEvent& event);

   virtual void OnMediaRemoteDTMF(const CpMediaEvent& event);

   virtual void OnMediaDeviceFailure(const CpMediaEvent& event);

   virtual void OnMediaRemoteActive(const CpMediaEvent& event);

   virtual void OnMediaRecordingStart(const CpMediaEvent& event);

   virtual void OnMediaRecordingStop(const CpMediaEvent& event);

   /* ============================ ACCESSORS ================================= */

   /* ============================ INQUIRY =================================== */

   /* //////////////////////////// PROTECTED ///////////////////////////////// */
protected:
   SIPX_INST m_pInst;

   /* //////////////////////////// PRIVATE /////////////////////////////////// */
private:

};

#endif // SipXMediaEventListener_h__
