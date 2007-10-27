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
   MpPortAudioStream(UtlBoolean isOutput, UtlBoolean isInput);

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

   /**
    * Stream callback of this instance to support multiple streams.
    */
   int instanceStreamCallback(const void *input,
                              void *output,
                              unsigned long frameCount,
                              const PaStreamCallbackTimeInfo* timeInfo,
                              PaStreamCallbackFlags statusFlags);

   UtlBoolean m_isOutput; ///< whether this stream is output
   UtlBoolean m_isInput; ///< whether this stream is input
};

#endif // MpPortAudioStream_h__
