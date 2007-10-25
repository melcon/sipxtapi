//
// Copyright (C) 2007 Jaroslav Libak
//
// $$
///////////////////////////////////////////////////////////////////////////////

// SYSTEM INCLUDES
// APPLICATION INCLUDES
#include <os/OsSysLog.h>
#include <os/OsLock.h>
#include "mp/MpPortAudioDriver.h"
#include "mp/MpAudioStreamParameters.h"
#include <portaudio.h>

// DEFINES
// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
// STATIC VARIABLE INITIALIZATIONS
unsigned int MpPortAudioDriver::ms_instanceCounter = 0;
OsMutex MpPortAudioDriver::ms_driverMutex(OsMutex::Q_FIFO);
UtlString MpPortAudioDriver::ms_driverName("Portaudio");
UtlString MpPortAudioDriver::ms_driverVersion("V19");

// MACROS
// GLOBAL VARIABLES
// GLOBAL FUNCTIONS

/* //////////////////////////// PUBLIC //////////////////////////////////// */

/* ============================ CREATORS ================================== */

/* ============================ MANIPULATORS ============================== */

void MpPortAudioDriver::pushFrame()
{

}

void MpPortAudioDriver::pullFrame()
{

}

const UtlString& MpPortAudioDriver::getDriverName() const
{
   return ms_driverName;
}

const UtlString& MpPortAudioDriver::getDriverVersion() const
{
   return ms_driverVersion;
}

OsStatus MpPortAudioDriver::getHostApiCount(int& count) const
{
   OsLock lock(ms_driverMutex);
   OsStatus status = OS_FAILED;

   PaHostApiIndex paCount = Pa_GetHostApiCount();

   if (paCount >= 0)
   {
      status = OS_SUCCESS;
      count = paCount;
   }
   
   return status;
}

OsStatus MpPortAudioDriver::getDefaultHostApi(MpHostAudioApiIndex& apiIndex) const
{
   OsLock lock(ms_driverMutex);
   OsStatus status = OS_FAILED;

   PaHostApiIndex paDefault = Pa_GetDefaultHostApi();

   if (apiIndex >= 0)
   {
      status = OS_SUCCESS;
      apiIndex = paDefault;
   }

   return status;
}

OsStatus MpPortAudioDriver::getHostApiInfo(MpHostAudioApiIndex hostApiIndex,
                                           MpHostAudioApiInfo& apiInfo) const
{
   OsLock lock(ms_driverMutex);
   OsStatus status = OS_FAILED;

   const PaHostApiInfo* paApiInfo = Pa_GetHostApiInfo(hostApiIndex);

   if (paApiInfo)
   {
      status = OS_SUCCESS;
      apiInfo.setTypeId((MpHostAudioApiTypeId)paApiInfo->type);
      apiInfo.setName(paApiInfo->name);
      apiInfo.setDeviceCount(paApiInfo->deviceCount);
      apiInfo.setDefaultInputDevice(paApiInfo->defaultInputDevice);
      apiInfo.setDefaultOutputDevice(paApiInfo->defaultOutputDevice);
   }

   return status;
}

OsStatus MpPortAudioDriver::hostApiTypeIdToHostApiIndex(MpHostAudioApiTypeId hostApiTypeId,
                                                        MpHostAudioApiIndex& hostApiIndex) const
{
   OsLock lock(ms_driverMutex);
   OsStatus status = OS_FAILED;

   PaHostApiIndex paIndex = Pa_HostApiTypeIdToHostApiIndex((PaHostApiTypeId)hostApiTypeId);

   if (paIndex >= 0)
   {
      status = OS_SUCCESS;
      hostApiIndex = paIndex;
   }
   
   return status;
}

OsStatus MpPortAudioDriver::hostApiDeviceIndexToDeviceIndex(MpHostAudioApiIndex hostApiIndex,
                                                            int hostApiDeviceIndex,
                                                            MpAudioDeviceIndex& deviceIndex) const
{
   OsLock lock(ms_driverMutex);
   OsStatus status = OS_FAILED;

   MpAudioDeviceIndex paDeviceIndex = Pa_HostApiDeviceIndexToDeviceIndex(hostApiIndex, hostApiDeviceIndex);

   if (paDeviceIndex >= 0)
   {
      status = OS_SUCCESS;
      deviceIndex = paDeviceIndex;
   }
   
   return status;
}

OsStatus MpPortAudioDriver::getDeviceCount(MpAudioDeviceIndex& deviceCount) const
{
   OsLock lock(ms_driverMutex);
   OsStatus status = OS_FAILED;

   MpAudioDeviceIndex paCount = Pa_GetDeviceCount();

   if (paCount >= 0)
   {
      status = OS_SUCCESS;
      deviceCount = paCount;
   }

   return status;
}

OsStatus MpPortAudioDriver::getDefaultInputDevice(MpAudioDeviceIndex& deviceIndex) const
{
   OsLock lock(ms_driverMutex);
   OsStatus status = OS_FAILED;

   MpAudioDeviceIndex paDeviceIndex = Pa_GetDefaultInputDevice();

   if (paDeviceIndex >= 0)
   {
      status = OS_SUCCESS;
      deviceIndex = paDeviceIndex;
   }

   return status;
}

OsStatus MpPortAudioDriver::getDefaultOutputDevice(MpAudioDeviceIndex& deviceIndex) const
{
   OsLock lock(ms_driverMutex);
   OsStatus status = OS_FAILED;

   MpAudioDeviceIndex paDeviceIndex = Pa_GetDefaultOutputDevice();

   if (paDeviceIndex >= 0)
   {
      status = OS_SUCCESS;
      deviceIndex = paDeviceIndex;
   }

   return status;
}

OsStatus MpPortAudioDriver::getDeviceInfo(MpAudioDeviceIndex deviceIndex, MpAudioDeviceInfo& deviceInfo) const
{
   OsLock lock(ms_driverMutex);
   OsStatus status = OS_FAILED;

   const PaDeviceInfo* paDeviceInfo = Pa_GetDeviceInfo(deviceIndex);

   if (paDeviceInfo)
   {
      status = OS_SUCCESS;
      deviceInfo.setName(paDeviceInfo->name);
      deviceInfo.setMaxOutputChannels(paDeviceInfo->maxOutputChannels);
      deviceInfo.setMaxInputChannels(paDeviceInfo->maxInputChannels);
      deviceInfo.setHostApi(paDeviceInfo->hostApi);
      deviceInfo.setDefaultSampleRate(paDeviceInfo->defaultSampleRate);
      deviceInfo.setDefaultLowOutputLatency(paDeviceInfo->defaultLowOutputLatency);
      deviceInfo.setDefaultLowInputLatency(paDeviceInfo->defaultLowInputLatency);
      deviceInfo.setDefaultHighOutputLatency(paDeviceInfo->defaultHighOutputLatency);
      deviceInfo.setDefaultHighInputLatency(paDeviceInfo->defaultHighInputLatency);
   }

   return status;
}

OsStatus MpPortAudioDriver::isFormatSupported(const MpAudioStreamParameters* inputParameters,
                                              const MpAudioStreamParameters* outputParameters,
                                              double sampleRate) const
{
   OsLock lock(ms_driverMutex);
   OsStatus status = OS_FAILED;

   PaStreamParameters *paInputParameters = NULL;
   PaStreamParameters *paOutputParameters = NULL;

   if (inputParameters)
   {
      paInputParameters = new PaStreamParameters();
      paInputParameters->channelCount = inputParameters->getChannelCount();
      paInputParameters->device = inputParameters->getDeviceIndex();
      paInputParameters->hostApiSpecificStreamInfo = NULL;
      paInputParameters->sampleFormat = inputParameters->getSampleFormat();
      paInputParameters->suggestedLatency = inputParameters->getSuggestedLatency();
   }

   if (outputParameters)
   {
      paOutputParameters = new PaStreamParameters();
      paOutputParameters->channelCount = outputParameters->getChannelCount();
      paOutputParameters->device = outputParameters->getDeviceIndex();
      paOutputParameters->hostApiSpecificStreamInfo = NULL;
      paOutputParameters->sampleFormat = outputParameters->getSampleFormat();
      paOutputParameters->suggestedLatency = outputParameters->getSuggestedLatency();
   }
   
   PaError paError = Pa_IsFormatSupported(paInputParameters,
                                          paOutputParameters,
                                          sampleRate);

   if (paError == paFormatIsSupported)
   {
      status = OS_SUCCESS;
   }

   delete paInputParameters;
   delete paOutputParameters;
   
   return status;
}

void MpPortAudioDriver::release()
{
   delete this;
}

/* ============================ ACCESSORS ================================= */

/* ============================ INQUIRY =================================== */

/* //////////////////////////// PROTECTED ///////////////////////////////// */

/* //////////////////////////// PRIVATE /////////////////////////////////// */

MpPortAudioDriver::MpPortAudioDriver() : m_memberMutex(OsMutex::Q_FIFO)
{
}

MpPortAudioDriver::~MpPortAudioDriver(void)
{
   OsLock lock(ms_driverMutex);

   // decrease counter in thread safe manner
   ms_instanceCounter--;

   PaError err = Pa_Terminate();
   if (err != paNoError)
   {
      UtlString error(Pa_GetErrorText(err));
      OsSysLog::add(FAC_AUDIO, PRI_ERR, "Pa_Terminate failed, error: %s", error.data());
   }
}

MpPortAudioDriver* MpPortAudioDriver::createInstance()
{
   OsLock lock(ms_driverMutex);

   if (ms_instanceCounter == 0)
   {
      // we init portaudio here to avoid throwing exception in constructor
      PaError err = Pa_Initialize();

      if (err == paNoError)
      {
         ms_instanceCounter++;
         return new MpPortAudioDriver();
      }
      else
      {
         UtlString error(Pa_GetErrorText(err));
         OsSysLog::add(FAC_AUDIO, PRI_ERR, "Pa_Initialize failed, error: %s", error.data());
      }
   }

   // only 1 instance of this driver is allowed, or error occurred
   return NULL;
}


/* ============================ FUNCTIONS ================================= */


