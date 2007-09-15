//  
// Copyright (C) 2006-2007 SIPez LLC. 
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

//The averaged latency of packets in dejitter buffer is calculated in method 
//PullPacket( ) for the purpose of dejitter buffer
//backlog control (or called jitter control) by the decoder in down stream. 
//The decoder will look at the latency at certain frequency to make 
//the decision. -Brian Puh
//

// SYSTEM INCLUDES
#include <assert.h>

// APPLICATION INCLUDES
#include "os/OsDefs.h"
#include "os/OsLock.h"
#include "os/OsSysLog.h"
#include "mp/MpBuf.h"
#include "mp/MprDejitter.h"
#include "mp/MpMisc.h"
#include "mp/MpDspUtils.h"
#include <mp/MpJitterBufferBase.h>
#include <mp/MpJitterBufferDefault.h>
#include <sdp/SdpCodec.h>

// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
// STATIC VARIABLE INITIALIZATIONS

/* //////////////////////////// PUBLIC //////////////////////////////////// */

/* ============================ CREATORS ================================== */

// Constructor
MprDejitter::MprDejitter()
: m_rtpLock(OsBSem::Q_FIFO, OsBSem::FULL)
{
   memset(m_jitterBufferArray, 0, sizeof(m_jitterBufferArray));
}

// Destructor
MprDejitter::~MprDejitter()
{
   // free jitter buffers
   for (int i = 0; i < MAX_PAYLOADS; i++)
   {
      delete m_jitterBufferArray[i];
      m_jitterBufferArray[i] = NULL;
   }
}

/* ============================ MANIPULATORS ============================== */

OsStatus MprDejitter::initJitterBuffers(SdpCodec* codecs[], int numCodecs)
{
   OsLock lock(m_rtpLock);

   // free old jitter buffers
   for (int i = 0; i < MAX_PAYLOADS; i++)
   {
      delete m_jitterBufferArray[i];
      m_jitterBufferArray[i] = NULL;
   }

   // now add new jitter buffers
   for (int i = 0; i < numCodecs; i++)
   {
      UtlString encodingName;
      codecs[i]->getEncodingName(encodingName);
      int codecPayloadType = codecs[i]->getCodecPayloadFormat();

      if (codecPayloadType >=0 && codecPayloadType < MAX_PAYLOADS
          && !m_jitterBufferArray[codecPayloadType])
      {
         // index is valid and there is no jitter buffer for it
         // TODO: 80 has to be replaced with ptime from SDP
         m_jitterBufferArray[codecPayloadType] = new MpJitterBufferDefault(encodingName, codecPayloadType, 80);
      }
   }

   return OS_SUCCESS;
}

// Add a buffer containing an incoming RTP packet to the dejitter pool.
// This method places the packet to the pool depending the modulo division value.
OsStatus MprDejitter::pushPacket(const MpRtpBufPtr &pRtp)
{
   OsLock lock(m_rtpLock);
   
   int jBufferIndex = pRtp->getRtpPayloadType();
   if (jBufferIndex >=0 && jBufferIndex < MAX_PAYLOADS && m_jitterBufferArray[jBufferIndex])
   {
      // we have a jitter buffer for this rtp frame
      m_jitterBufferArray[jBufferIndex]->push(pRtp);
      return OS_SUCCESS;
   }
   else
   {
      // we don't have jitter buffer for this rtp frame
      return OS_FAILED;
   }
}

// Get a pointer to the next RTP packet, or NULL if none is available.
MpRtpBufPtr MprDejitter::pullPacket(int rtpPayloadType)
{
   OsLock locker(m_rtpLock);

   if (rtpPayloadType >=0 && rtpPayloadType < MAX_PAYLOADS && m_jitterBufferArray[rtpPayloadType])
   {
      MpRtpBufPtr found; ///< RTP packet we will return
      // we have a jitter buffer for this rtp frame
      JitterBufferResult res = m_jitterBufferArray[rtpPayloadType]->pull(found);

      // if there is not available rtp frame, jitter buffer returns empty MpRtpBufPtr
      return found;
   }
   else
   {
      // we don't have jitter buffer for this payload type
      return MpRtpBufPtr(); // return ptr without buffer (invalid one)
   }
}

/* ============================ ACCESSORS ================================= */

int MprDejitter::getBufferLength(int rtpPayloadType)
{
   if (rtpPayloadType >=0 && rtpPayloadType < MAX_PAYLOADS
      && m_jitterBufferArray[rtpPayloadType])
   {
      return m_jitterBufferArray[rtpPayloadType]->getBufferLength();
   }
   else
   {
      return 0;
   }
}

/* ============================ INQUIRY =================================== */

/* //////////////////////////// PROTECTED ///////////////////////////////// */

/* //////////////////////////// PRIVATE /////////////////////////////////// */

/* ============================ FUNCTIONS ================================= */
