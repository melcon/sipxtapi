//
// Copyright (C) 2007 Jaroslav Libak
// Licensed under the LGPL license.
// $$
///////////////////////////////////////////////////////////////////////////////

// SYSTEM INCLUDES
// APPLICATION INCLUDES
#include <os/OsLock.h>
#include <utl/UtlString.h>
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

OsStatus MpAudioDriverManager::getCurrentOutputDevice(MpAudioDeviceInfo& deviceInfo) const
{
   OsLock lock(ms_mutex);

   if (m_pAudioDriver)
   {
      OsStatus res = OS_FAILED;
      if (m_outputAudioStream)
      {
         // stream exists, then audio device is selected
         res = m_pAudioDriver->getDeviceInfo(m_outputDeviceIndex, deviceInfo);
      }
      else
      {
         // stream doesn't exist, audio device is disabled
         deviceInfo.setName("None");
         deviceInfo.setHostApiName(NULL);
         deviceInfo.setHostApi(0);
         deviceInfo.setMaxInputChannels(0);
         deviceInfo.setMaxOutputChannels(0);
         deviceInfo.setDefaultHighInputLatency(0.0);
         deviceInfo.setDefaultHighOutputLatency(0.0);
         deviceInfo.setDefaultLowInputLatency(0.0);
         deviceInfo.setDefaultLowOutputLatency(0.0);
         deviceInfo.setDefaultSampleRate(0.0);
         res = OS_SUCCESS;
      }

      return res;
   }

   return OS_FAILED;
}

OsStatus MpAudioDriverManager::getCurrentInputDevice(MpAudioDeviceInfo& deviceInfo) const
{
   OsLock lock(ms_mutex);

   if (m_pAudioDriver)
   {
      OsStatus res = OS_FAILED;
      if (m_inputAudioStream)
      {
         // stream exists, then audio device is selected
         res = m_pAudioDriver->getDeviceInfo(m_inputDeviceIndex, deviceInfo);
      }
      else
      {
         // stream doesn't exist, audio device is disabled
         deviceInfo.setName("None");
         deviceInfo.setHostApiName(NULL);
         deviceInfo.setHostApi(0);
         deviceInfo.setMaxInputChannels(0);
         deviceInfo.setMaxOutputChannels(0);
         deviceInfo.setDefaultHighInputLatency(0.0);
         deviceInfo.setDefaultHighOutputLatency(0.0);
         deviceInfo.setDefaultLowInputLatency(0.0);
         deviceInfo.setDefaultLowOutputLatency(0.0);
         deviceInfo.setDefaultSampleRate(0.0);
         res = OS_SUCCESS;
      }

      return res;
   }

   return OS_FAILED;
}

OsStatus MpAudioDriverManager::setCurrentOutputDevice(const UtlString& device,
                                                      const UtlString& driverName)
{
   OsLock lock(ms_mutex);

   if (m_pAudioDriver)
   {
      if (device.compareTo("None", UtlString::ignoreCase) == 0)
      {
         // we want to disable current output device
         return closeOutputStream();
      }
      else if (device.compareTo("Default", UtlString::ignoreCase) == 0)
      {
         MpAudioDeviceIndex defaultOutputDeviceIndex;
         // we want to set default output device
         OsStatus res = m_pAudioDriver->getDefaultOutputDevice(defaultOutputDeviceIndex);
         if (res != OS_SUCCESS)
         {
            return OS_FAILED;
         }

         if (defaultOutputDeviceIndex != m_outputDeviceIndex)
         {
            // we want to change active output device
            // first close current stream
            closeOutputStream();

            MpAudioStreamParameters outputParameters;
            outputParameters.setChannelCount(1);
            outputParameters.setSampleFormat(MP_AUDIO_FORMAT_INT16);
            outputParameters.setSuggestedLatency(0.05);
            outputParameters.setDeviceIndex(defaultOutputDeviceIndex);

            // open asynchronous output stream
            res = m_pAudioDriver->openStream(&m_outputAudioStream,
               NULL,
               &outputParameters,
               MpMisc.m_audioSampleRate,
               MpMisc.m_audioSamplesPerFrame,
               MP_AUDIO_STREAM_CLIPOFF,
               FALSE);
            if (res != OS_SUCCESS)
            {
               return OS_FAILED;
            }

            res = m_pAudioDriver->startStream(m_outputAudioStream);
            if (res != OS_SUCCESS)
            {
               return OS_FAILED;
            }
            m_outputDeviceIndex = defaultOutputDeviceIndex;
            return OS_SUCCESS;
         }
         else
         {
            // new and old device are the same, no need to change
            return OS_SUCCESS;
         }
      }
      else
      {
         // user wants to select a particular device
         OsStatus res = OS_FAILED;
         UtlBoolean bDeviceFound = FALSE;
         UtlBoolean bDriverReq = !driverName.isNull();
         MpAudioDeviceIndex deviceCount = 0;
         MpAudioDeviceIndex i = 0;
         m_pAudioDriver->getDeviceCount(deviceCount);

         // loop through all devices, and find matching one
         for (i = 0; i < deviceCount; i++)
         {
            MpAudioDeviceInfo deviceInfo;
            m_pAudioDriver->getDeviceInfo(i, deviceInfo);

            if (!bDriverReq)
            {
               // driver match is not required, match by name only
               if (deviceInfo.getName().compareTo(device.data(), UtlString::matchCase) == 0 &&
                   deviceInfo.getMaxOutputChannels() > 0)
               {
                  // we found match, we will select this device
                  bDeviceFound = TRUE;
                  break;
               }
            }
            else
            {
               // now try to match by driver name, device name
               if (deviceInfo.getName().compareTo(device.data(), UtlString::matchCase) == 0 &&
                   deviceInfo.getHostApiName().compareTo(driverName.data(), UtlString::matchCase) == 0 &&
                   deviceInfo.getMaxOutputChannels() > 0)
               {
                  // we found match, we will select this device
                  bDeviceFound = TRUE;
                  break;
               }
            }
         }
         
         if (bDeviceFound && i != m_outputDeviceIndex)
         {
            // we found a matching device
            // first close current stream
            closeOutputStream();

            MpAudioStreamParameters outputParameters;
            outputParameters.setChannelCount(1);
            outputParameters.setSampleFormat(MP_AUDIO_FORMAT_INT16);
            outputParameters.setSuggestedLatency(0.05);
            outputParameters.setDeviceIndex(i);

            // open asynchronous output stream
            res = m_pAudioDriver->openStream(&m_outputAudioStream,
               NULL,
               &outputParameters,
               MpMisc.m_audioSampleRate,
               MpMisc.m_audioSamplesPerFrame,
               MP_AUDIO_STREAM_CLIPOFF,
               FALSE);
            if (res != OS_SUCCESS)
            {
               return OS_FAILED;
            }

            res = m_pAudioDriver->startStream(m_outputAudioStream);
            if (res != OS_SUCCESS)
            {
               return OS_FAILED;
            }

            m_outputDeviceIndex = i;
            return OS_SUCCESS;
         }
      }
   }   
   
   return OS_FAILED;
}

OsStatus MpAudioDriverManager::setCurrentInputDevice(const UtlString& device,
                                                     const UtlString& driverName)
{
   OsLock lock(ms_mutex);

   if (m_pAudioDriver)
   {
      if (device.compareTo("None", UtlString::ignoreCase) == 0)
      {
         // we want to disable current input device
         return closeInputStream();
      }
      else if (device.compareTo("Default", UtlString::ignoreCase) == 0)
      {
         MpAudioDeviceIndex defaultInputDeviceIndex;
         // we want to set default input device
         OsStatus res = m_pAudioDriver->getDefaultInputDevice(defaultInputDeviceIndex);
         if (res != OS_SUCCESS)
         {
            return OS_FAILED;
         }

         if (defaultInputDeviceIndex != m_inputDeviceIndex)
         {
            // we want to change active input device
            // first close current stream
            closeInputStream();

            MpAudioStreamParameters inputParameters;
            inputParameters.setChannelCount(1);
            inputParameters.setSampleFormat(MP_AUDIO_FORMAT_INT16);
            inputParameters.setSuggestedLatency(0.05);
            inputParameters.setDeviceIndex(defaultInputDeviceIndex);

            // open asynchronous input stream
            res = m_pAudioDriver->openStream(&m_inputAudioStream,
               &inputParameters,
               NULL,
               MpMisc.m_audioSampleRate,
               MpMisc.m_audioSamplesPerFrame,
               MP_AUDIO_STREAM_CLIPOFF,
               FALSE);
            if (res != OS_SUCCESS)
            {
               return OS_FAILED;
            }

            res = m_pAudioDriver->startStream(m_inputAudioStream);
            if (res != OS_SUCCESS)
            {
               return OS_FAILED;
            }
            m_inputDeviceIndex = defaultInputDeviceIndex;
            return OS_SUCCESS;
         }
         else
         {
            // new and old device are the same, no need to change
            return OS_SUCCESS;
         }
      }
      else
      {
         // user wants to select a particular device
         OsStatus res = OS_FAILED;
         UtlBoolean bDeviceFound = FALSE;
         UtlBoolean bDriverReq = !driverName.isNull();
         MpAudioDeviceIndex deviceCount = 0;
         MpAudioDeviceIndex i = 0;
         m_pAudioDriver->getDeviceCount(deviceCount);

         // loop through all devices, and find matching one
         for (i = 0; i < deviceCount; i++)
         {
            MpAudioDeviceInfo deviceInfo;
            m_pAudioDriver->getDeviceInfo(i, deviceInfo);

            if (!bDriverReq)
            {
               // driver match is not required, match by name only
               if (deviceInfo.getName().compareTo(device.data(), UtlString::matchCase) == 0 &&
                  deviceInfo.getMaxInputChannels() > 0)
               {
                  // we found match, we will select this device
                  bDeviceFound = TRUE;
                  break;
               }
            }
            else
            {
               // now try to match by driver name, device name
               if (deviceInfo.getName().compareTo(device.data(), UtlString::matchCase) == 0 &&
                  deviceInfo.getHostApiName().compareTo(driverName.data(), UtlString::matchCase) == 0 &&
                  deviceInfo.getMaxInputChannels() > 0)
               {
                  // we found match, we will select this device
                  bDeviceFound = TRUE;
                  break;
               }
            }
         }

         if (bDeviceFound && i != m_inputDeviceIndex)
         {
            // we found a matching device
            // first close current stream
            closeInputStream();

            MpAudioStreamParameters inputParameters;
            inputParameters.setChannelCount(1);
            inputParameters.setSampleFormat(MP_AUDIO_FORMAT_INT16);
            inputParameters.setSuggestedLatency(0.05);
            inputParameters.setDeviceIndex(i);

            // open asynchronous input stream
            res = m_pAudioDriver->openStream(&m_inputAudioStream,
               &inputParameters,
               NULL,
               MpMisc.m_audioSampleRate,
               MpMisc.m_audioSamplesPerFrame,
               MP_AUDIO_STREAM_CLIPOFF,
               FALSE);
            if (res != OS_SUCCESS)
            {
               return OS_FAILED;
            }

            res = m_pAudioDriver->startStream(m_inputAudioStream);
            if (res != OS_SUCCESS)
            {
               return OS_FAILED;
            }

            m_inputDeviceIndex = i;
            return OS_SUCCESS;
         }
      }
   }   

   return OS_FAILED;
}

int MpAudioDriverManager::getInputDeviceCount() const
{
   OsLock lock(ms_mutex);

   return (int)m_inputAudioDevices.size();
}

int MpAudioDriverManager::getOutputDeviceCount() const
{
   OsLock lock(ms_mutex);

   return (int)m_outputAudioDevices.size();
}

OsStatus MpAudioDriverManager::getInputDeviceInfo(int deviceIndex, MpAudioDeviceInfo& deviceInfo)
{
   OsLock lock(ms_mutex);

   OsStatus res = OS_FAILED;

   if (deviceIndex < (int)m_inputAudioDevices.size())
   {
      deviceInfo = m_inputAudioDevices.at(deviceIndex);
      res = OS_SUCCESS;
   }
   
   return res;
}

OsStatus MpAudioDriverManager::getOutputDeviceInfo(int deviceIndex, MpAudioDeviceInfo& deviceInfo)
{
   OsLock lock(ms_mutex);

   OsStatus res = OS_FAILED;

   if (deviceIndex < (int)m_outputAudioDevices.size())
   {
      deviceInfo = m_outputAudioDevices.at(deviceIndex);
      res = OS_SUCCESS;
   }

   return res;
}

OsStatus MpAudioDriverManager::startInputStream() const
{
   OsLock lock(ms_mutex);

   if (m_inputAudioStream && m_pAudioDriver)
   {
      UtlBoolean isStopped = FALSE;
      m_pAudioDriver->isStreamStopped(m_inputAudioStream, isStopped);
      if (isStopped)
      {
         return m_pAudioDriver->startStream(m_inputAudioStream);
      }
   }

   return OS_FAILED;
}

OsStatus MpAudioDriverManager::startOutputStream() const
{
   OsLock lock(ms_mutex);

   if (m_outputAudioStream && m_pAudioDriver)
   {
      UtlBoolean isStopped = FALSE;
      m_pAudioDriver->isStreamStopped(m_outputAudioStream, isStopped);
      if (isStopped)
      {
         return m_pAudioDriver->startStream(m_outputAudioStream);
      }
   }

   return OS_FAILED;
}

OsStatus MpAudioDriverManager::abortInputStream() const
{
   OsLock lock(ms_mutex);

   if (m_inputAudioStream && m_pAudioDriver)
   {
      UtlBoolean isActive = FALSE;
      m_pAudioDriver->isStreamActive(m_inputAudioStream, isActive);
      if (isActive)
      {
         return m_pAudioDriver->abortStream(m_inputAudioStream);
      }
   }

   return OS_FAILED;
}

OsStatus MpAudioDriverManager::abortOutputStream() const
{
   OsLock lock(ms_mutex);

   if (m_outputAudioStream && m_pAudioDriver)
   {
      UtlBoolean isActive = FALSE;
      m_pAudioDriver->isStreamActive(m_outputAudioStream, isActive);
      if (isActive)
      {
         return m_pAudioDriver->abortStream(m_outputAudioStream);
      }
   }

   return OS_FAILED;
}

OsStatus MpAudioDriverManager::closeInputStream()
{
   OsLock lock(ms_mutex);

   if (m_inputAudioStream && m_pAudioDriver)
   {
      UtlBoolean isActive = FALSE;
      m_pAudioDriver->isStreamActive(m_inputAudioStream, isActive);
      if (isActive)
      {
         m_pAudioDriver->abortStream(m_inputAudioStream);
      }

      OsStatus res = m_pAudioDriver->closeStream(m_inputAudioStream);
      if (res == OS_SUCCESS)
      {
         m_inputAudioStream = 0;
         m_inputDeviceIndex = 0;
      }

      return res;
   }

   return OS_FAILED;
}

OsStatus MpAudioDriverManager::closeOutputStream()
{
   OsLock lock(ms_mutex);

   if (m_outputAudioStream && m_pAudioDriver)
   {
      UtlBoolean isActive = FALSE;
      m_pAudioDriver->isStreamActive(m_outputAudioStream, isActive);
      if (isActive)
      {
         m_pAudioDriver->abortStream(m_outputAudioStream);
      }

      OsStatus res = m_pAudioDriver->closeStream(m_outputAudioStream);
      if (res == OS_SUCCESS)
      {
         m_outputAudioStream = 0;
         m_outputDeviceIndex = 0;
      }

      return res;
   }

   return OS_FAILED;
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
, m_inputDeviceIndex(0)
, m_outputAudioStream(0)
, m_outputDeviceIndex(0)
, m_outputAudioDevices()
, m_inputAudioDevices()
{
   m_pAudioDriver = MpAudioDriverFactory::createAudioDriver(MpAudioDriverFactory::AUDIO_DRIVER_PORTAUDIO);

   MpAudioDeviceIndex deviceCount = 0;
   m_pAudioDriver->getDeviceCount(deviceCount);

   // first append "NONE" and "Default" devices
   MpAudioDeviceInfo device;
   device.setName("None");
   m_outputAudioDevices.push_back(device);
   m_inputAudioDevices.push_back(device);
   device.setName("Default");
   device.setMaxOutputChannels(2);
   m_outputAudioDevices.push_back(device);
   device.setMaxOutputChannels(0);
   device.setMaxInputChannels(2);
   m_inputAudioDevices.push_back(device);
   device.setMaxInputChannels(0);

   for (int i = 0; i < deviceCount; i++)
   {
      MpAudioDeviceInfo deviceInfo;
      m_pAudioDriver->getDeviceInfo(i, deviceInfo);

      if (deviceInfo.getMaxInputChannels() > 0)
      {
         // device is input
         m_inputAudioDevices.push_back(deviceInfo);
      }
      if (deviceInfo.getMaxOutputChannels() > 0)
      {
         // device is output
         m_outputAudioDevices.push_back(deviceInfo);
      }
   }

   MpAudioStreamParameters inputParameters;
   MpAudioStreamParameters outputParameters;

   inputParameters.setChannelCount(1);
   inputParameters.setSampleFormat(MP_AUDIO_FORMAT_INT16);
   inputParameters.setSuggestedLatency(0.05);
   outputParameters.setChannelCount(1);
   outputParameters.setSampleFormat(MP_AUDIO_FORMAT_INT16);
   outputParameters.setSuggestedLatency(0.05);

   m_pAudioDriver->getDefaultInputDevice(m_inputDeviceIndex);
   m_pAudioDriver->getDefaultOutputDevice(m_outputDeviceIndex);

   inputParameters.setDeviceIndex(m_inputDeviceIndex);
   outputParameters.setDeviceIndex(m_outputDeviceIndex);

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

   m_pAudioDriver->startStream(m_inputAudioStream);
   m_pAudioDriver->startStream(m_outputAudioStream);
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

