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
      unsigned int frameSize);

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

   RtpSRC m_lastSSRC;   ///< last SSRC, if it changes we need to reset jitter buffer
   bool m_bFirstFrame;  ///< whether we have yet to receive the 1st frame
   int m_bufferLength;  ///< length of jitter buffer

   /// Buffer for incoming RTP packets
   MpRtpBufPtr m_pPackets[MAX_RTP_PACKETS];

   /// Index of the last inserted packet.
   int m_lastPushed;
   /**<
   *  As packets are added, we change this value to indicate
   *  where the buffer is wrapping.
   */

   OsMutex m_mutex;
};

#endif // MpJitterBufferDefault_h__
