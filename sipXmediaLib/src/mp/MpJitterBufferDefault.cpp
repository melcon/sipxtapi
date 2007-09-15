// Copyright (C) 2007 Jaroslav Libak
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

// SYSTEM INCLUDES
// APPLICATION INCLUDES
#include <os/OsLock.h>
#include <mp/MpJitterBufferDefault.h>
#include <mp/MpRtpBuf.h>
#include <mp/MpDspUtils.h>

// DEFINES
// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
// STATIC VARIABLE INITIALIZATIONS
// MACROS
// GLOBAL VARIABLES
// GLOBAL FUNCTIONS

/* //////////////////////////// PUBLIC //////////////////////////////////// */

/* ============================ CREATORS ================================== */

MpJitterBufferDefault::MpJitterBufferDefault(const UtlString& name,
                                         int payloadType,
                                         unsigned int frameSize)
: MpJitterBufferBase(name, payloadType, frameSize)
, m_lastSSRC(0)
, m_bFirstFrame(true)
, m_mutex(OsMutex::Q_FIFO)
, m_bufferLength(0)
, m_lastPushed(0)
{
}

MpJitterBufferDefault::~MpJitterBufferDefault()
{
}

/* ============================ MANIPULATORS ============================== */

void MpJitterBufferDefault::reset()
{
   OsLock lock(m_mutex);

   for (int i = 0; i < MAX_RTP_PACKETS; i++)
   {
      m_pPackets[i] = NULL; // assign NULL to MpRtpBufPtr
   }
   m_lastPushed = 0;
   m_bufferLength = 0;
}

void MpJitterBufferDefault::frameIncrement()
{
}

JitterBufferResult MpJitterBufferDefault::pull(MpRtpBufPtr &pOutRtp)
{
   OsLock lock(m_mutex);

   if (m_bufferLength == 0)
   {
      // jitter buffer is empty
      return MP_JITTER_BUFFER_MISSING;
   }
   
   // We find a packet by starting to look in the JB just AFTER where the latest
   // push was done, and loop MAX_RTP_PACKETS times or until we find a valid frame
   int iNextPull = (m_lastPushed + 1) % MAX_RTP_PACKETS;

   for (int i = 0; i < MAX_RTP_PACKETS; i++)
   {
      // If we reach valid packet, move it out of the buffer and break search loop
      if (m_pPackets[iNextPull].isValid())
      {
         pOutRtp.swap(m_pPackets[iNextPull]);

         // Make sure we does not have copy of this buffer left in other threads.
         pOutRtp.requestWrite();

         m_bufferLength--;
         return MP_JITTER_BUFFER_OK;
      }

      // Wrap iNextPull counter if we reach end of buffer
      iNextPull = (iNextPull + 1) % MAX_RTP_PACKETS;
   }

   return MP_JITTER_BUFFER_MISSING;
}

void MpJitterBufferDefault::push(const MpRtpBufPtr &pRtp)
{
   if (pRtp.isValid() && pRtp->getRtpPayloadType() == m_payloadType)
   {
      OsLock lock(m_mutex);

      // for first frame, setup SSRC
      if (m_bFirstFrame)
      {
         m_lastSSRC = pRtp->getRtpSSRC();
         m_bFirstFrame = false;
      }
      else
      if (m_lastSSRC != pRtp->getRtpSSRC())
      {
         // SSRC changed, reset jitter buffer
         reset();
         m_lastSSRC = pRtp->getRtpSSRC();
      }

      // Find place for incoming packet
      int index = pRtp->getRtpSequenceNumber() % MAX_RTP_PACKETS;

      // Place packet to the buffer
      if (m_pPackets[index].isValid())
      {
         // Check for packets already in the buffer. Overwrite them if 
         // the just-arriving packet is newer than the existing packet
         // Don't overwrite if the just-arriving packet is older
         RtpSeq iBufSeqNo = m_pPackets[index]->getRtpSequenceNumber();
         RtpSeq iNewSeqNo = pRtp->getRtpSequenceNumber();

         if (MpDspUtils::compareSerials(iNewSeqNo, iBufSeqNo) > 0) 
         {
            // Insert the new packet over the old packet
            m_pPackets[index] = pRtp;
            m_lastPushed = index;  
            // m_bufferLength remains unchanged, since we discarded a packet, and added one
         }
         else
         {
            // Don't insert the new packet - it is a old delayed packet
            return;
         }
      }
      else
      {
         m_lastPushed = index;
         m_pPackets[index] = pRtp;
         m_bufferLength++;
      }
   }
}

/* ============================ ACCESSORS ================================= */

/* ============================ INQUIRY =================================== */

int MpJitterBufferDefault::getBufferLength()
{
   return m_bufferLength;
}

/* //////////////////////////// PROTECTED ///////////////////////////////// */

/* //////////////////////////// PRIVATE /////////////////////////////////// */

/* ============================ FUNCTIONS ================================= */

