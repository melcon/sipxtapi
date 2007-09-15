//
// Copyright (C) 2007 Jaroslav Libak
//
// $$
///////////////////////////////////////////////////////////////////////////////

// SYSTEM INCLUDES
// APPLICATION INCLUDES
#include <os/OsLock.h>
#include <mp/MpJitterBufferDefault.h>
#include <mp/MpRtpBuf.h>

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
                                         uint8_t payloadType,
                                         unsigned int frameSize)
: MpJitterBufferBase(name, payloadType, frameSize)
, m_lastSSRC(0)
, m_bFirstFrame(true)
, m_mutex(OsMutex::Q_FIFO)
{
}

MpJitterBufferDefault::~MpJitterBufferDefault()
{
}

/* ============================ MANIPULATORS ============================== */

void MpJitterBufferDefault::reset()
{
}

void MpJitterBufferDefault::frameIncrement()
{
}

JitterBufferResult MpJitterBufferDefault::pull(MpRtpBufPtr &pOutRtp)
{
   return MP_JITTER_BUFFER_ERROR;
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

      // here store rtp frame in jitter buffer
   }
}

/* ============================ ACCESSORS ================================= */

/* ============================ INQUIRY =================================== */

/* //////////////////////////// PROTECTED ///////////////////////////////// */

/* //////////////////////////// PRIVATE /////////////////////////////////// */

/* ============================ FUNCTIONS ================================= */

