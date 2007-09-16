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

#ifndef MpJitterBufferDefault_h__
#define MpJitterBufferDefault_h__

// SYSTEM INCLUDES
// APPLICATION INCLUDES
#include <os/OsMutex.h>
#include <mp/MpJitterBufferBase.h>
#include <mp/MpRtpBuf.h>

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
* Wrapper for default jitter buffer.
*/
class MpJitterBufferDefault : public MpJitterBufferBase
{
   /* //////////////////////////// PUBLIC //////////////////////////////////// */
public:

   enum
   {
      MAX_RTP_PACKETS = 64,  ///< MUST BE A POWER OF 2, AND SHOULD BE >3
   };

   /* ============================ CREATORS ================================== */
   ///@name Creators
   //@{

   MpJitterBufferDefault(const UtlString& name,
      int payloadType,
      unsigned int frameSize,
      bool bUsePrefetch = true);

   /**
   * Speex jitter buffer destructor. Will free internal resources.
   */
   virtual ~MpJitterBufferDefault();

   //@}

   /* ============================ MANIPULATORS ============================== */
   ///@name Manipulators
   //@{

   virtual void reset();

   virtual void frameIncrement();

   /**
   * Pulls one frame from jitter buffer and stores it into supplied MpRtpBufPtr.
   *
   * @param pOutRtp MpRtpBufPtr supplied by caller where we store the frame. Only
   *        payload, payload size, payload type and timestamp are initialized.
   */
   virtual JitterBufferResult pull(MpRtpBufPtr &pOutRtp);

   /**
   * Pushes an RTP frame into jitter buffer. Jitter buffer will throw out
   * a frame with incorrect payload type.
   */
   virtual void push(const MpRtpBufPtr &pRtp);

   //@}

   /* ============================ ACCESSORS ================================= */
   ///@name Accessors
   //@{
   //@}

   /* ============================ INQUIRY =================================== */
   ///@name Inquiry
   //@{

      virtual int getBufferLength();

   //@}

   /* //////////////////////////// PROTECTED ///////////////////////////////// */
protected:

   /* //////////////////////////// PRIVATE /////////////////////////////////// */
private:

   typedef struct JitterBufferStatistics
   {
      unsigned int m_totalHits;
      unsigned int m_total2ndHits;
      unsigned int m_totalNormalUnderflows;
      unsigned int m_totalPrefetchUnderflows;
      unsigned int m_totalPulls;

      unsigned int m_totalPushCollisions;
      unsigned int m_totalPushReplacements;
      unsigned int m_totalPushInserts;
      
      JitterBufferStatistics()
      : m_totalPulls(0)
      , m_totalHits(0)
      , m_total2ndHits(0)
      , m_totalNormalUnderflows(0)
      , m_totalPrefetchUnderflows(0)
      , m_totalPushCollisions(0)
      , m_totalPushReplacements(0)
      , m_totalPushInserts(0)
      {

      }
   } JitterBufferStatistics;

   void initJitterBuffer(const MpRtpBufPtr &pOutRtp);

   RtpSRC m_lastSSRC;   ///< last SSRC, if it changes we need to reset jitter buffer
   bool m_bFirstFrame;  ///< whether we have yet to receive the 1st frame
   int m_bufferLength;  ///< length of jitter buffer
   int m_prefetchCount; ///< how many RTP frames we need to have before we allow pulling
   bool m_bPrefetchMode; ///< whether we allow pulling frames from jitter buffer, if true we don't
   bool m_bUsePrefetch; ///< whether prefetching is enabled. If disabled, we allow pulling at any time, used for DTMF

   RtpTimestamp m_expectedTimestamp;   ///< expected timestamp for pull
   RtpSeq m_lastSeqNumber; ///< sequence number of last pulled frame
   int m_lastPulled; ///< index of last pulled frame

   int m_lastPushed;   ///< Index of the last inserted packet.

   MpRtpBufPtr m_pPackets[MAX_RTP_PACKETS];   ///< Buffer for incoming RTP packets
   JitterBufferStatistics m_statistics; ///< jitter buffer statistics
   OsMutex m_mutex;
};

#endif // MpJitterBufferDefault_h__
