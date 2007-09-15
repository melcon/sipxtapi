//
// Copyright (C) 2007 Jaroslav Libak
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

   /* ============================ CREATORS ================================== */
   ///@name Creators
   //@{

   MpJitterBufferDefault(const UtlString& name,
      uint8_t payloadType,
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

   //@}

   /* //////////////////////////// PROTECTED ///////////////////////////////// */
protected:

   /* //////////////////////////// PRIVATE /////////////////////////////////// */
private:

   RtpSRC m_lastSSRC;   ///< last SSRC, if it changes we need to reset jitter buffer
   bool m_bFirstFrame;  ///< whether we have yet to receive the 1st frame

   OsMutex m_mutex;
};

#endif // MpJitterBufferDefault_h__
