//
// Copyright (C) 2007 Jaroslav Libak
//
// $$
///////////////////////////////////////////////////////////////////////////////

#ifndef MpPortAudioStream_h__
#define MpPortAudioStream_h__

// SYSTEM INCLUDES
// APPLICATION INCLUDES
#include <utl/UtlBool.h>
#include "mp/MpAudioDriverDefs.h"
#include "portaudio.h"

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
* Representation of asynchronous portlib audio stream. For synchronous streams, it is not
* needed. Stream can be output/input/full duplex. It is only used for callback.
*/
class MpPortAudioStream
{
   /* //////////////////////////// PUBLIC //////////////////////////////////// */
public:
   /* ============================ CREATORS ================================== */
   ///@name Creators
   //@{

   /// Constructor.
   MpPortAudioStream(int outputChannelCount,
                     int inputChannelCount,
                     MpAudioDriverSampleFormat outputSampleFormat,
                     MpAudioDriverSampleFormat inputSampleFormat,
                     double sampleRate,
                     unsigned long framesPerBuffer);

   /// Destructor.
   virtual ~MpPortAudioStream(void);
   //@}

   /* ============================ MANIPULATORS ============================== */
   ///@name Manipulators
   //@{

   /**
    * audio callback according to portaudio spec. In user data it receives
    * pointer to MpPortAudioStream instance
    */
   static int streamCallback(const void *input,
                             void *output,
                             unsigned long frameCount,
                             const PaStreamCallbackTimeInfo* timeInfo,
                             PaStreamCallbackFlags statusFlags,
                             void *userData );

   /**
    * Reads given number of frames into buffer. Buffer must be allocated memory.
    *
    * @param buffer Allocated memory to read data into
    * @param frames Number of frames to read
    * @returns OS_SUCCESS if successful
    */
   virtual OsStatus readStreamAsync(void *buffer,
                                    unsigned long frames);

   /**
    * Writes given number of frames into stream from buffer.
    *
    * @param buffer Buffer with samples
    * @param frames Number of frames to write to stream
    * @returns OS_SUCCESS if successful
    */
   virtual OsStatus writeStreamAsync(const void *buffer,
                                     unsigned long frames);

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

   /// Copy constructor (not implemented for this class)
   MpPortAudioStream(const MpPortAudioStream& rMpPortAudioStream);

   /// Assignment operator (not implemented for this class)
   MpPortAudioStream& operator=(const MpPortAudioStream& rhs);

   /**
    * Stream callback of this instance to support multiple streams.
    */
   int instanceStreamCallback(const void *input,
                              void *output,
                              unsigned long frameCount,
                              const PaStreamCallbackTimeInfo* timeInfo,
                              PaStreamCallbackFlags statusFlags);

   int m_outputChannelCount; ///< number of output channels
   int m_inputChannelCount; ///< number of input channels
   MpAudioDriverSampleFormat m_outputSampleFormat; ///< sample format of output
   MpAudioDriverSampleFormat m_inputSampleFormat; ///< sample format of input
   double m_sampleRate; ///< sample rate for stream
   unsigned long m_framesPerBuffer; ///< frames per buffer for stream

   void* m_pInputBuffer; ///< buffer for storing recorded samples
   void* m_pOutputBuffer; ///< buffer for storing frames going to speaker
   unsigned int m_inputBufferSize; ///< size of input buffer
   unsigned int m_outputBufferSize; ///< size of output buffer
   unsigned int m_inputSampleSize; ///< size of input sample in bytes per channel
   unsigned int m_outputSampleSize; ///< size of output sample in bytes per channel
   volatile unsigned int m_inputWritePos; ///< position in input buffer for writing
   volatile unsigned int m_inputReadPos; ///< position in input buffer for reading
   volatile unsigned int m_outputWritePos; ///< position in output buffer for writing
   volatile unsigned int m_outputReadPos; ///< position in output buffer for reading
};

#endif // MpPortAudioStream_h__
