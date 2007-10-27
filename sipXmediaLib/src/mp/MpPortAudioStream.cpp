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

MpPortAudioStream::MpPortAudioStream(int outputChannelCount,
                                     int inputChannelCount,
                                     MpAudioDriverSampleFormat outputSampleFormat,
                                     MpAudioDriverSampleFormat inputSampleFormat,
                                     double sampleRate,
                                     unsigned long framesPerBuffer)
: m_outputChannelCount(outputChannelCount)
, m_inputChannelCount(inputChannelCount)
, m_outputSampleFormat(outputSampleFormat)
, m_inputSampleFormat(inputSampleFormat)
, m_sampleRate(sampleRate)
, m_framesPerBuffer(framesPerBuffer)
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

   if (m_outputChannelCount > 0 && output)
   {
      // TODO: copy something into output buffer
   }

   if (m_inputChannelCount > 0 && input)
   {
      // TODO: copy something from input buffer
   }
   
   
   return paContinue;
}



/* ============================ FUNCTIONS ================================= */


