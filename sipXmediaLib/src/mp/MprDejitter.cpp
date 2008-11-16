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
#include <utl/UtlSListIterator.h>
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
   memset(m_jitterBufferList, 0, sizeof(m_jitterBufferList));
}

// Destructor
MprDejitter::~MprDejitter()
{
   // free jitter buffers
   for (int i = 0; i < MAX_PAYLOADS; i++)
   {
      delete m_jitterBufferArray[i];
      m_jitterBufferArray[i] = NULL;
      m_jitterBufferList[i] = NULL;
   }
}

/* ============================ MANIPULATORS ============================== */

OsStatus MprDejitter::initJitterBuffers(const UtlSList& codecList)
{
   OsLock lock(m_rtpLock);

   // free old jitter buffers
   for (int i = 0; i < MAX_PAYLOADS; i++)
   {
      delete m_jitterBufferArray[i];
      m_jitterBufferArray[i] = NULL;
      m_jitterBufferList[i] = NULL;
   }

   int listCounter = 0;
   SdpCodec* pCodec = NULL;
   UtlSListIterator itor(codecList);
   while (itor())
   {
      pCodec = (SdpCodec*)itor.item();
      if (pCodec)
      {
         UtlString encodingName;
         pCodec->getEncodingName(encodingName);
         int codecPayloadId = pCodec->getCodecPayloadId();

         if (codecPayloadId >=0 && codecPayloadId < MAX_PAYLOADS
            && !m_jitterBufferArray[codecPayloadId])
         {
            // index is valid and there is no jitter buffer for it
            // TODO: 80 has to be replaced with ptime from SDP
            if (pCodec->getCodecType() == SdpCodec::SDP_CODEC_TONES)
            {
               // for RFC2833, disable prefetch & PLC
               m_jitterBufferArray[codecPayloadId] = new MpJitterBufferDefault(encodingName,
                  codecPayloadId,
                  MpMisc.m_audioSamplesPerFrame,
                  false);
            }
            else
            {
               m_jitterBufferArray[codecPayloadId] = new MpJitterBufferDefault(encodingName,
                  codecPayloadId,
                  MpMisc.m_audioSamplesPerFrame,
                  true, 6, true, 3);
            }
            m_jitterBufferList[listCounter++] = m_jitterBufferArray[codecPayloadId];
         }
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

void MprDejitter::frameIncrement()
{
   // notify all jitter buffers about frame increment
   for (int i = 0; m_jitterBufferList[i]; i++)
   {
      m_jitterBufferList[i]->frameIncrement();
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
