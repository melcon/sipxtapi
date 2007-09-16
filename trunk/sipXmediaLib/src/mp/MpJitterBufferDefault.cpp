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
#include "os/OsIntTypes.h"
#include <mp/MpJitterBufferDefault.h>
#include <mp/MpRtpBuf.h>
#include <mp/MpDspUtils.h>

// DEFINES
#define PRINT_STATISTICS

// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
// STATIC VARIABLE INITIALIZATIONS
// MACROS

#define WRAP_NUMBER(number, range)  \
   {                                \
      if ((number) < 0)             \
      {                             \
         number = (range) - (-(number)%(range));   \
      }                             \
      else                          \
      {                             \
         number = (number)%(range); \
      }                             \
   }


// GLOBAL VARIABLES
// GLOBAL FUNCTIONS

/*
static void appendToFile(const char* fileName, const char* text)
{
   FILE* f = fopen(fileName, "a");
   fprintf(f, text);
   fclose(f);
}
*/

/* //////////////////////////// PUBLIC //////////////////////////////////// */

/* ============================ CREATORS ================================== */

MpJitterBufferDefault::MpJitterBufferDefault(const UtlString& name,
                                         int payloadType,
                                         unsigned int frameSize,
                                         bool bUsePrefetch)
: MpJitterBufferBase(name, payloadType, frameSize)
, m_bUsePrefetch(bUsePrefetch)
, m_lastSSRC(0)
, m_bFirstFrame(true)
, m_mutex(OsMutex::Q_FIFO)
, m_bufferLength(0)
, m_lastPushed(0)
, m_lastPulled(0)
, m_expectedTimestamp(0)
, m_prefetchCount(5)
, m_bPrefetchMode(true)
{
#ifdef PRINT_STATISTICS
   enableConsoleOutput(TRUE);
#endif
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
   m_lastPulled = 0;
   m_bufferLength = 0;
   m_expectedTimestamp = 0;
   m_bPrefetchMode = true;
}

void MpJitterBufferDefault::frameIncrement()
{
   if (!m_bPrefetchMode || !m_bUsePrefetch)
   {
      // wrap around is done automatically for unsigned
      m_expectedTimestamp = m_expectedTimestamp + m_frameSize;
   }
   // if in prefetch mode, we can't increment timestamp, as it could cause us to skip
   // whole prefetch buffer. If prefetch is disabled, always increment
}

JitterBufferResult MpJitterBufferDefault::pull(MpRtpBufPtr &pOutRtp)
{
   OsLock lock(m_mutex);

   if (!m_bFirstFrame)
   {
      // only increment pulls since 1st frame was received
      m_statistics.m_totalPulls++;
   }
   
#ifdef PRINT_STATISTICS
   if (m_statistics.m_totalPulls % 100 == 1)
   {
      // print them
      osPrintf("---- Jitter Buffer Statistics for %s ----\n", m_name.data());
      osPrintf("m_totalHits: %u\n", m_statistics.m_totalHits);
      osPrintf("m_total2ndHits: %u\n", m_statistics.m_total2ndHits);
      osPrintf("m_totalNormalUnderflows: %u\n", m_statistics.m_totalNormalUnderflows);
      osPrintf("m_totalPrefetchUnderflows: %u\n", m_statistics.m_totalPrefetchUnderflows);
      osPrintf("m_totalPulls: %u\n", m_statistics.m_totalPulls);
      osPrintf("m_totalPushCollisions: %u\n", m_statistics.m_totalPushCollisions);
      osPrintf("m_totalPushReplacements: %u\n", m_statistics.m_totalPushReplacements);
      osPrintf("m_totalPushInserts: %u\n", m_statistics.m_totalPushInserts);
      osPrintf("m_bufferLength: %u\n", m_bufferLength);      
      osPrintf("m_lastSeqNumber: %u\n", m_lastSeqNumber);      
      osPrintf("-------------------------------------------\n");
   }
#endif

   if (m_bUsePrefetch && m_bPrefetchMode)
   {
      m_statistics.m_totalPrefetchUnderflows++;
      // we are in prefetch mode - waiting to fill buffer
      return MP_JITTER_BUFFER_MISSING;
   }
   
   if (m_bufferLength == 0)
   {
      m_bPrefetchMode = true;
      // jitter buffer is empty
      m_statistics.m_totalNormalUnderflows++;
      return MP_JITTER_BUFFER_MISSING;
   }
   
   // we will start pulling just after the last pull
   int iNextPull = (m_lastPulled + 1) % MAX_RTP_PACKETS;
   int secondCandidate = -1;
   RtpTimestamp frameSizeHalf = m_frameSize / 2;

   for (int i = 0; i < MAX_RTP_PACKETS; i++)
   {
      if (m_pPackets[iNextPull].isValid())
      {
         // buffer is valid, check seq and timestamp
         RtpSeq iSeqNo = m_pPackets[iNextPull]->getRtpSequenceNumber();
         RtpTimestamp iTimestamp = m_pPackets[iNextPull]->getRtpTimestamp();

         if (MpDspUtils::compareSerials(iSeqNo, m_lastSeqNumber) < 0)
         {
            // we found old frame that we don't want to play, discard it
            m_pPackets[iNextPull].release();
            m_bufferLength--;
            continue;
         }

         // sequence number is ok, now check timestamp
         if (secondCandidate == -1)
         {
            // this candidate will be used if we don't find frame for matching timestamp
            secondCandidate = iNextPull;
         }

         // wrapping around max and 0 is done automatically for unsigned
         RtpTimestamp upperBound = m_expectedTimestamp + frameSizeHalf;
         RtpTimestamp lowerBound = m_expectedTimestamp - frameSizeHalf;

         if (MpDspUtils::compareSerials(iTimestamp, upperBound) < 0
            && MpDspUtils::compareSerials(iTimestamp, lowerBound) > 0)
         {
            // we found the expected frame, use it
            pOutRtp.swap(m_pPackets[iNextPull]);
            // Make sure we does not have copy of this buffer left in other threads.
            pOutRtp.requestWrite();
            assert(!m_pPackets[iNextPull].isValid());

            m_lastSeqNumber = iSeqNo;
            m_lastPulled = iNextPull;
            m_expectedTimestamp = iTimestamp; // next time it will be incremented in frameIncrement
            m_statistics.m_totalHits++;

            m_bufferLength--;
            return MP_JITTER_BUFFER_OK;
         }
      }

      // Wrap iNextPull counter if we reach end of buffer
      iNextPull = (iNextPull + 1) % MAX_RTP_PACKETS;
   }

   // we didn't find frame with expected timestamp, use the 2nd candidate if available
   if (secondCandidate != -1)
   {
      assert(m_pPackets[secondCandidate].isValid());

      m_lastSeqNumber = m_pPackets[secondCandidate]->getRtpSequenceNumber();
      m_lastPulled = secondCandidate;
      m_expectedTimestamp = m_pPackets[secondCandidate]->getRtpTimestamp(); // next time it will be incremented in frameIncrement
      m_statistics.m_total2ndHits++;

      pOutRtp.swap(m_pPackets[secondCandidate]);
      // Make sure we does not have copy of this buffer left in other threads.
      pOutRtp.requestWrite();

      m_bufferLength--;
      return MP_JITTER_BUFFER_OK;
   }
   
   // we didn't find any suitable frame :(
   m_bPrefetchMode = true;
   m_statistics.m_totalNormalUnderflows++;
   return MP_JITTER_BUFFER_MISSING;
}

void MpJitterBufferDefault::push(const MpRtpBufPtr &pRtp)
{
   if (pRtp.isValid() && pRtp->getRtpPayloadType() == m_payloadType)
   {
      OsLock lock(m_mutex);

      RtpSeq inRtpSeq = pRtp->getRtpSequenceNumber();
      // for first frame, setup SSRC
      if (m_bFirstFrame)
      {
         if (!m_bUsePrefetch)
         {
            // init jitter buffer variables
            // so that it can catch up at the correct index during pull
            initJitterBuffer(pRtp);
         }
         m_bFirstFrame = false;
      }
      else
      if (m_lastSSRC != pRtp->getRtpSSRC())
      {
         // SSRC changed, reset jitter buffer
         reset();
         if (!m_bUsePrefetch)
         {
            // init jitter buffer variables
            // so that it can catch up at the correct index during pull
            initJitterBuffer(pRtp);
         }
      }
/*      else
      if (MpDspUtils::compareSerials(inRtpSeq, m_lastSeqNumber) < 0)
      {
         // this is for safety
         // if we are getting totally wrong sequence numbers for some reason

      }*/

      // Find place for incoming packet
      int index = (inRtpSeq % MAX_RTP_PACKETS);

      // Place packet to the buffer
      if (m_pPackets[index].isValid())
      {
         m_statistics.m_totalPushCollisions++;
         // Check for packets already in the buffer. Overwrite them if 
         // the just-arriving packet is newer than the existing packet
         // Don't overwrite if the just-arriving packet is older
         RtpSeq iBufSeqNo = m_pPackets[index]->getRtpSequenceNumber();

         if (MpDspUtils::compareSerials(inRtpSeq, iBufSeqNo) > 0) 
         {
            m_statistics.m_totalPushReplacements++;
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
         m_statistics.m_totalPushInserts++;
         m_lastPushed = index;
         m_pPackets[index] = pRtp;
         m_bufferLength++;

         if (m_bUsePrefetch && m_bPrefetchMode)
         {
            if (m_bufferLength == 1)
            {
               // first frame in prefetch mode was received, init jitter buffer variables
               // so that it can catch up at the correct index during pull
               initJitterBuffer(pRtp);
            }
            else
            if (m_bufferLength >= m_prefetchCount)
            {
               m_bPrefetchMode = false; // turn off prefetch mode, so that we can start pulling
            }
         }         
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

void MpJitterBufferDefault::initJitterBuffer(const MpRtpBufPtr &pRtp)
{
   int index = pRtp->getRtpSequenceNumber() % MAX_RTP_PACKETS;

   m_lastSSRC = pRtp->getRtpSSRC();

   // now init data for pulling correct frames
   m_lastPulled = index - 1;
   WRAP_NUMBER(m_lastPulled, MAX_RTP_PACKETS);
   // these 2 wrap around correctly automatically, since they are unsigned
   m_lastSeqNumber = pRtp->getRtpSequenceNumber() - 1;
   m_expectedTimestamp = pRtp->getRtpTimestamp() - m_frameSize;
}

/* ============================ FUNCTIONS ================================= */

