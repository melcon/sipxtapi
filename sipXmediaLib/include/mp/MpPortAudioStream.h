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
};

#endif // MpPortAudioStream_h__
