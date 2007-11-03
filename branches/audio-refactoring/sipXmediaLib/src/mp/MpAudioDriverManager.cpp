//
// Copyright (C) 2007 Jaroslav Libak
// Licensed under the LGPL license.
// $$
///////////////////////////////////////////////////////////////////////////////

// SYSTEM INCLUDES
// APPLICATION INCLUDES
#include <os/OsLock.h>
#include "mp/MpAudioDriverManager.h"
#include "mp/MpAudioDriverFactory.h"
#include "mp/MpAudioDriverBase.h"
#include "mp/MpAudioStreamParameters.h"
#include "mp/MpMisc.h"

// DEFINES
// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
// STATIC VARIABLE INITIALIZATIONS

OsMutex MpAudioDriverManager::ms_mutex(OsMutex::Q_FIFO);
MpAudioDriverManager* MpAudioDriverManager::ms_pInstance = NULL;

// MACROS
// GLOBAL VARIABLES
// GLOBAL FUNCTIONS

/* //////////////////////////// PUBLIC //////////////////////////////////// */

/* ============================ CREATORS ================================== */

/* ============================ MANIPULATORS ============================== */

MpAudioDriverManager* MpAudioDriverManager::getInstance(UtlBoolean bCreate /*= TRUE*/)
{
   OsLock lock(ms_mutex);

   if (ms_pInstance || !bCreate)
   {
      return ms_pInstance;
   }
   else
   {
      ms_pInstance = new MpAudioDriverManager();
      return ms_pInstance;
   }
}


void MpAudioDriverManager::release()
{
   delete this;
}

/* ============================ ACCESSORS ================================= */

/* ============================ INQUIRY =================================== */

/* //////////////////////////// PROTECTED ///////////////////////////////// */

/* //////////////////////////// PRIVATE /////////////////////////////////// */

MpAudioDriverManager::MpAudioDriverManager()
: m_pAudioDriver(NULL)
, m_inputAudioStream(0)
, m_outputAudioStream(0)
{
   m_pAudioDriver = MpAudioDriverFactory::createAudioDriver(MpAudioDriverFactory::AUDIO_DRIVER_PORTAUDIO);

   MpAudioStreamParameters inputParameters;
   MpAudioStreamParameters outputParameters;
   MpAudioDeviceIndex inputDeviceIndex = 0;
   MpAudioDeviceIndex outputDeviceIndex = 0;

   inputParameters.setChannelCount(1);
   inputParameters.setSampleFormat(MP_AUDIO_FORMAT_INT16);
   inputParameters.setSuggestedLatency(0.05);
   outputParameters.setChannelCount(1);
   outputParameters.setSampleFormat(MP_AUDIO_FORMAT_INT16);
   outputParameters.setSuggestedLatency(0.05);

   m_pAudioDriver->getDefaultInputDevice(inputDeviceIndex);
   m_pAudioDriver->getDefaultOutputDevice(outputDeviceIndex);

   inputParameters.setDeviceIndex(inputDeviceIndex);
   outputParameters.setDeviceIndex(outputDeviceIndex);

   // open asynchronous input stream
   m_pAudioDriver->openStream(&m_inputAudioStream,
      &inputParameters,
      NULL,
      MpMisc.m_audioSampleRate,
      MpMisc.m_audioSamplesPerFrame,
      MP_AUDIO_STREAM_CLIPOFF,
      FALSE);

   // open asynchronous output stream
   m_pAudioDriver->openStream(&m_outputAudioStream,
      NULL,
      &outputParameters,
      MpMisc.m_audioSampleRate,
      MpMisc.m_audioSamplesPerFrame,
      MP_AUDIO_STREAM_CLIPOFF,
      FALSE);

   // do not start streams yet
}

MpAudioDriverManager::~MpAudioDriverManager(void)
{
   OsLock lock(ms_mutex);

   ms_pInstance = NULL;

   if (m_pAudioDriver)
   {
      if (m_inputAudioStream)
      {
         UtlBoolean isActive = FALSE;
         m_pAudioDriver->isStreamActive(m_inputAudioStream, isActive);
         if (isActive)
         {
            m_pAudioDriver->stopStream(m_inputAudioStream);
         }         
         m_pAudioDriver->closeStream(m_inputAudioStream);
         m_inputAudioStream = 0;
      }

      if (m_outputAudioStream)
      {
         UtlBoolean isActive = FALSE;
         m_pAudioDriver->isStreamActive(m_inputAudioStream, isActive);
         if (isActive)
         {
            m_pAudioDriver->stopStream(m_outputAudioStream);
         }
         m_pAudioDriver->closeStream(m_outputAudioStream);
         m_outputAudioStream = 0;
      }

      m_pAudioDriver->release();
      m_pAudioDriver = NULL;
   }
}

/* ============================ FUNCTIONS ================================= */


