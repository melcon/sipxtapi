//
// Copyright (C) 2007 Jaroslav Libak
//
// $$
///////////////////////////////////////////////////////////////////////////////

#ifndef MpJitterBufferBase_h__
#define MpJitterBufferBase_h__

// SYSTEM INCLUDES
// APPLICATION INCLUDES
#include <utl/UtlString.h>
#include <os/OsIntTypes.h>

// DEFINES
// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
// FORWARD DECLARATIONS
class MpRtpBufPtr;

// STRUCTS
// TYPEDEFS
// MACROS
// GLOBAL VARIABLES
// GLOBAL FUNCTIONS

/**
 * Common class for all jitter buffers.
 */
class MpJitterBufferBase
{
/* //////////////////////////// PUBLIC //////////////////////////////////// */
public:

   typedef enum JitterBufferResult
   {
      JITTER_BUFFER_OK = 0,
      JITTER_BUFFER_MISSING,
      JITTER_BUFFER_INCOMPLETE
   } JitterBufferResult;

/* ============================ CREATORS ================================== */
///@name Creators
//@{

   /**
    * Jitter buffer constructor
    *
    * @param name Name of jitter buffer i.e "GSM". Mainly for debugging.
    * @param payloadType RTP Payload type. Jitter buffer will reject
    *        RTP frames for other payload types.
    * @param frameSize Number of samples per frame used by the codec,
    *        not internal frame size in sipxmedialib. Will be probably
    *        80, 160, 240...
    */
   MpJitterBufferBase(const UtlString& name,
                      uint8_t payloadType,
                      unsigned int frameSize);

   virtual ~MpJitterBufferBase();

//@}

/* ============================ MANIPULATORS ============================== */
///@name Manipulators
//@{

   /**
    * Resets jitter buffer.
    */
   virtual void reset() = 0;

   /**
    * Notifies jitter buffer of frame incrementation. Should be
    * done every time we pull a packet.
    */
   virtual void frameIncrement() = 0;

   /**
    * Pulls one frame from jitter buffer and stores it into supplied MpRtpBufPtr.
    *
    * @param pOutRtp MpRtpBufPtr supplied by caller where we store the frame.
    */
   virtual JitterBufferResult pull(MpRtpBufPtr &pOutRtp) = 0;

   /**
    * Pushes an RTP frame into jitter buffer. Jitter buffer will throw out
    * a frame with incorrect payload type.
    */
   virtual void push(const MpRtpBufPtr &pRtp) = 0;

//@}

/* ============================ ACCESSORS ================================= */
///@name Accessors
//@{

   uint8_t getPayloadType();

//@}

/* ============================ INQUIRY =================================== */
///@name Inquiry
//@{

//@}

/* //////////////////////////// PROTECTED ///////////////////////////////// */
protected:
   UtlString m_name;
   uint8_t m_payloadType;
   unsigned int m_frameSize;

/* //////////////////////////// PRIVATE /////////////////////////////////// */
private:

};

#endif // MpJitterBufferBase_h__
