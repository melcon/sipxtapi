//
// Copyright (C) 2007 Jaroslav Libak
//
// $$
///////////////////////////////////////////////////////////////////////////////

// SYSTEM INCLUDES
// APPLICATION INCLUDES
#include <assert.h>
#include "mp/MpPortAudioStream.h"

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

MpPortAudioStream::MpPortAudioStream(UtlBoolean isOutput, UtlBoolean isInput)
: m_isOutput(isOutput)
, m_isInput(isInput)
{

}

MpPortAudioStream::~MpPortAudioStream(void)
{

}

/* ============================ MANIPULATORS ============================== */

int MpPortAudioStream::streamCallback(const void *input,
                                      void *output,
                                      unsigned long frameCount,
                                      const PaStreamCallbackTimeInfo* timeInfo,
                                      PaStreamCallbackFlags statusFlags,
                                      void *userData)
{
   assert(userData);
   MpPortAudioStream* instance = (MpPortAudioStream*)userData;

   return instance->instanceStreamCallback(input, output, frameCount, timeInfo, statusFlags);
}

/* ============================ ACCESSORS ================================= */

/* ============================ INQUIRY =================================== */

/* //////////////////////////// PROTECTED ///////////////////////////////// */

/* //////////////////////////// PRIVATE /////////////////////////////////// */

int MpPortAudioStream::instanceStreamCallback(const void *input,
                                              void *output,
                                              unsigned long frameCount,
                                              const PaStreamCallbackTimeInfo* timeInfo,
                                              PaStreamCallbackFlags statusFlags)
{
   // portaudio supplied us pointers to its input and output buffers

   if (m_isOutput && output)
   {
      // TODO: copy something into output buffer
   }

   if (m_isInput && input)
   {
      // TODO: copy something from input buffer
   }
   
   
   return paContinue;
}



/* ============================ FUNCTIONS ================================= */


