//
// Copyright (C) 2007 Jaroslav Libak
//
// $$
///////////////////////////////////////////////////////////////////////////////

// SYSTEM INCLUDES
// APPLICATION INCLUDES
#include <assert.h>
#include <os/OsIntTypes.h> 
#include <os/OsSysLog.h>
#include "mp/MpPortAudioStream.h"

// DEFINES
#define MIN_SAMPLE_RATE 200

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
, m_pInputBuffer(NULL)
, m_pOutputBuffer(NULL)
, m_inputBufferSize(0)
, m_outputBufferSize(0)
, m_inputSampleSize(0)
, m_outputSampleSize(0)
, m_inputWritePos(0)
, m_inputReadPos(0)
, m_outputWritePos(0)
, m_outputReadPos(0)
{   
   switch(m_inputSampleFormat & 32)
   {
   case MP_AUDIO_FORMAT_FLOAT32:
      m_inputSampleSize = sizeof(float);
      break;
   case MP_AUDIO_FORMAT_INT32:
      m_inputSampleSize = sizeof(int32_t);
      break;
   case MP_AUDIO_FORMAT_INT24:
      m_inputSampleSize = sizeof(char)*3;
      OsSysLog::add(FAC_AUDIO, PRI_WARNING, "Dangerous input sample format selected, check MpPortAudioStream.cpp\n");
      break;
   case MP_AUDIO_FORMAT_INT16:
      m_inputSampleSize = sizeof(int16_t);
      break;
   case MP_AUDIO_FORMAT_INT8:
      m_inputSampleSize = sizeof(int8_t);
      break;
   case MP_AUDIO_FORMAT_UINT8:
      m_inputSampleSize = sizeof(uint8_t);
      break;
   case MP_AUDIO_FORMAT_CUSTOMFORMAT:
   default:
      // don't create buffers, unsupported sample format
      break;
   }

   switch(m_outputSampleSize & 32)
   {
   case MP_AUDIO_FORMAT_FLOAT32:
      m_outputSampleSize = sizeof(float);
      break;
   case MP_AUDIO_FORMAT_INT32:
      m_outputSampleSize = sizeof(int32_t);
      break;
   case MP_AUDIO_FORMAT_INT24:
      m_outputSampleSize = sizeof(char)*3;
      OsSysLog::add(FAC_AUDIO, PRI_WARNING, "Dangerous output sample format selected, check MpPortAudioStream.cpp\n");
      break;
   case MP_AUDIO_FORMAT_INT16:
      m_outputSampleSize = sizeof(int16_t);
      break;
   case MP_AUDIO_FORMAT_INT8:
      m_outputSampleSize = sizeof(int8_t);
      break;
   case MP_AUDIO_FORMAT_UINT8:
      m_outputSampleSize = sizeof(uint8_t);
      break;
   case MP_AUDIO_FORMAT_CUSTOMFORMAT:
   default:
      // don't create buffers, unsupported sample format
      break;
   }

   if (m_sampleRate < MIN_SAMPLE_RATE)
   {
      // we need a minimum due to division
      m_sampleRate = MIN_SAMPLE_RATE;
   }

   int useFramesPerBuffer = m_framesPerBuffer;

   if (useFramesPerBuffer == 0)
   {
      // callback will accept frames of any size anyway...
      // use 80 for now, we need to create buffers of certain size, but will accept any number of frames
      useFramesPerBuffer = 80;
   }
   
   // allocate input buffer
   if (m_inputSampleFormat > 0 && m_inputChannelCount > 0)
   {
      m_inputBufferSize = (m_inputSampleSize * useFramesPerBuffer * m_inputChannelCount) * ((unsigned int)m_sampleRate / MIN_SAMPLE_RATE + 1);
      m_pInputBuffer = malloc(m_inputBufferSize);

      if (!m_pInputBuffer)
      {
         // allocation error, dont use buffer
         m_inputBufferSize = 0;
      }
      else
      {
         // zero buffer
         memset(m_pInputBuffer, 0, m_inputBufferSize);
         // start writing at the next sample position
         m_inputWritePos = m_inputSampleSize * useFramesPerBuffer * m_inputChannelCount;
      }      
   }
   
   // allocate output buffer
   if (m_outputSampleSize > 0 && m_outputChannelCount > 0)
   {
      m_outputBufferSize = (m_outputSampleSize * useFramesPerBuffer * m_outputChannelCount) * ((unsigned int)m_sampleRate / MIN_SAMPLE_RATE + 1);
      m_pOutputBuffer = malloc(m_outputBufferSize);

      if (!m_pOutputBuffer)
      {
         // allocation error, dont use buffer
         m_outputBufferSize = 0;
      }      
      else
      {
         // zero buffer
         memset(m_pOutputBuffer, 0, m_outputBufferSize);
         // start writing at the next sample position
         m_outputWritePos = m_outputSampleSize * useFramesPerBuffer * m_outputChannelCount;
      }
   }
}

MpPortAudioStream::~MpPortAudioStream(void)
{
   if (m_pInputBuffer)
   {
      free(m_pInputBuffer);
      m_pInputBuffer = NULL;
   }
   
   if (m_pOutputBuffer)
   {
      free(m_pOutputBuffer);
      m_pOutputBuffer = NULL;
   }
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

OsStatus MpPortAudioStream::readStreamAsync(void *buffer,
                                            unsigned long frames)
{
   OsStatus status = OS_FAILED;

   if (frames > 0)
   {
      if ((m_framesPerBuffer == 0) || (m_framesPerBuffer == frames))
      {
         // we received expected number of frames
      }      
   }

   return status;
}

OsStatus MpPortAudioStream::writeStreamAsync(const void *buffer,
                                             unsigned long frames)
{
   OsStatus status = OS_FAILED;

   if (frames > 0)
   {
      if ((m_framesPerBuffer == 0) || (m_framesPerBuffer == frames))
      {
         // we received expected number of frames
      }      
   }
   
   return status;
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


