//
// Copyright (C) 2007 Jaroslav Libak
// Licensed under the LGPL license.
// $$
///////////////////////////////////////////////////////////////////////////////

// SYSTEM INCLUDES
// APPLICATION INCLUDES
#include <os/OsSysLog.h>
#include <os/OsLock.h>
#include <utl/UtlPtr.h>
#include <utl/UtlTypedValue.h>
#include <utl/UtlHashMapIterator.h>
#include "mp/MpPortAudioDriver.h"
#include "mp/MpPortAudioStream.h"
#include "mp/MpAudioStreamInfo.h"
#include "mp/MpAudioStreamParameters.h"
#include "mp/MpPortAudioMixer.h"
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

   // one api in list is always "skeleton implementation" and it is the last api
   PaHostApiIndex paCount = Pa_GetHostApiCount() - 1;

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
      deviceInfo.setName(paDeviceInfo->name);
      deviceInfo.setMaxOutputChannels(paDeviceInfo->maxOutputChannels);
      deviceInfo.setMaxInputChannels(paDeviceInfo->maxInputChannels);
      deviceInfo.setHostApi(paDeviceInfo->hostApi);
      deviceInfo.setDefaultSampleRate(paDeviceInfo->defaultSampleRate);
      deviceInfo.setDefaultLowOutputLatency(paDeviceInfo->defaultLowOutputLatency);
      deviceInfo.setDefaultLowInputLatency(paDeviceInfo->defaultLowInputLatency);
      deviceInfo.setDefaultHighOutputLatency(paDeviceInfo->defaultHighOutputLatency);
      deviceInfo.setDefaultHighInputLatency(paDeviceInfo->defaultHighInputLatency);

      const PaHostApiInfo* paApiInfo = Pa_GetHostApiInfo(paDeviceInfo->hostApi);

      if (paApiInfo)
      {
         deviceInfo.setHostApiName(paApiInfo->name);
         status = OS_SUCCESS;
      }

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

MpAudioMixerBase* MpPortAudioDriver::getMixerForStream(MpAudioStreamId stream,
                                                       int mixerIndex) const
{
   OsLock lock(ms_driverMutex);

   if (stream && mixerIndex >= 0)
   {
      return MpPortAudioMixer::createMixer(stream, mixerIndex);
   }
   
   return NULL;
}

OsStatus MpPortAudioDriver::openStream(MpAudioStreamId* stream,
                                       const MpAudioStreamParameters *inputParameters,
                                       const MpAudioStreamParameters *outputParameters,
                                       double sampleRate,
                                       unsigned long framesPerBuffer,
                                       MpAudioStreamFlags streamFlags,
                                       UtlBoolean synchronous)
{
   OsLock lock(ms_driverMutex);
   OsStatus status = OS_FAILED;

   PaStreamParameters *paInputParameters = NULL;
   PaStreamParameters *paOutputParameters = NULL;
   int numInputChannels = 0;
   int numOutputChannels = 0;
   MpAudioDriverSampleFormat outputSampleFormat = 0;
   MpAudioDriverSampleFormat inputSampleFormat = 0;

   if (inputParameters)
   {
      paInputParameters = new PaStreamParameters();
      numInputChannels = paInputParameters->channelCount = inputParameters->getChannelCount();
      paInputParameters->device = inputParameters->getDeviceIndex();
      paInputParameters->hostApiSpecificStreamInfo = NULL;
      inputSampleFormat = paInputParameters->sampleFormat = inputParameters->getSampleFormat();
      paInputParameters->suggestedLatency = inputParameters->getSuggestedLatency();
   }

   if (outputParameters)
   {
      paOutputParameters = new PaStreamParameters();
      numOutputChannels = paOutputParameters->channelCount = outputParameters->getChannelCount();
      paOutputParameters->device = outputParameters->getDeviceIndex();
      paOutputParameters->hostApiSpecificStreamInfo = NULL;
      outputSampleFormat = paOutputParameters->sampleFormat = outputParameters->getSampleFormat();
      paOutputParameters->suggestedLatency = outputParameters->getSuggestedLatency();
   }

   if (numInputChannels > 0 || numOutputChannels > 0)
   {
      MpPortAudioStream* strm = NULL;

      if (!synchronous)
      {
         strm = new MpPortAudioStream(numOutputChannels,
                                      numInputChannels,
                                      outputSampleFormat,
                                      inputSampleFormat,
                                      sampleRate,
                                      framesPerBuffer);
      }
      
      PaError paError = Pa_OpenStream(stream,
            paInputParameters,
            paOutputParameters,
            sampleRate,
            framesPerBuffer,
            streamFlags,
            synchronous ? NULL : MpPortAudioStream::streamCallback,
            strm);

      if (paError == paNoError)
      {
         if (!synchronous)
         {
            m_audioStreamMap.insertKeyAndValue(new UtlTypedValue<MpAudioStreamId>(*stream),
                                               new UtlPtr<MpPortAudioStream>(strm, TRUE));
         }

         status = OS_SUCCESS;
      }
      else
      {
         // delete doesnt mind NULL
         delete strm;
      }
   }
   
   delete paInputParameters;
   delete paOutputParameters;

   return status;
}

OsStatus MpPortAudioDriver::openDefaultStream(MpAudioStreamId* stream,
                                              int numInputChannels,
                                              int numOutputChannels,
                                              MpAudioDriverSampleFormat sampleFormat,
                                              double sampleRate,
                                              unsigned long framesPerBuffer,
                                              UtlBoolean synchronous)
{
   OsLock lock(ms_driverMutex);
   OsStatus status = OS_FAILED;

   if ((numInputChannels > 0) || (numOutputChannels > 0))
   {
      MpPortAudioStream* strm = NULL; 

      if (!synchronous)
      {
         // we only create representation for asynchronous streams
         strm = new MpPortAudioStream(numOutputChannels,
                                      numInputChannels,
                                      sampleFormat,
                                      sampleFormat,
                                      sampleRate,
                                      framesPerBuffer);
      }
      
      PaError paError = Pa_OpenDefaultStream(stream,
         numInputChannels,
         numOutputChannels,
         sampleFormat,
         sampleRate,
         framesPerBuffer,
         synchronous ? NULL : MpPortAudioStream::streamCallback,
         strm);

      if (paError == paNoError)
      {
         if (!synchronous)
         {
            m_audioStreamMap.insertKeyAndValue(new UtlTypedValue<MpAudioStreamId>(*stream),
                                               new UtlPtr<MpPortAudioStream>(strm, TRUE));
         }

         status = OS_SUCCESS;
      }
      else
      {
         // delete doesnt mind NULL
         delete strm;
      }
   }
      
   return status;
}

OsStatus MpPortAudioDriver::closeStream(MpAudioStreamId stream)
{
   OsLock lock(ms_driverMutex);
   OsStatus status = OS_FAILED;

   PaError paError = Pa_CloseStream(stream);

   if (paError == paNoError)
   {
      UtlTypedValue<MpAudioStreamId> key(stream);
      UtlBoolean res = m_audioStreamMap.destroy(&key);

      status = OS_SUCCESS;
   }
   
   return status;
}

OsStatus MpPortAudioDriver::startStream(MpAudioStreamId stream)
{
   OsLock lock(ms_driverMutex);
   OsStatus status = OS_FAILED;

   PaError paError = Pa_StartStream(stream);

   if (paError == paNoError)
   {
      status = OS_SUCCESS;
   }

   return status;
}

OsStatus MpPortAudioDriver::stopStream(MpAudioStreamId stream)
{
   OsLock lock(ms_driverMutex);
   OsStatus status = OS_FAILED;

   PaError paError = Pa_StopStream(stream);

   if (paError == paNoError)
   {
      resetAsyncStream(stream);
      status = OS_SUCCESS;
   }

   return status;
}

OsStatus MpPortAudioDriver::abortStream(MpAudioStreamId stream)
{
   OsLock lock(ms_driverMutex);
   OsStatus status = OS_FAILED;

   PaError paError = Pa_AbortStream(stream);

   if (paError == paNoError)
   {
      resetAsyncStream(stream);
      status = OS_SUCCESS;
   }

   return status;
}

OsStatus MpPortAudioDriver::isStreamStopped(MpAudioStreamId stream,
                                            UtlBoolean& isStopped) const
{
   OsLock lock(ms_driverMutex);
   OsStatus status = OS_FAILED;

   PaError paError = Pa_IsStreamStopped(stream);

   if (paError >= 0)
   {
      if (paError == 0)
      {
         isStopped = FALSE;
      }
      else
      {
         isStopped = TRUE;
      }
      
      status = OS_SUCCESS;
   }

   return status;
}

OsStatus MpPortAudioDriver::isStreamActive(MpAudioStreamId stream,
                                           UtlBoolean& isActive) const
{
   OsLock lock(ms_driverMutex);
   OsStatus status = OS_FAILED;

   PaError paError = Pa_IsStreamActive(stream);

   if (paError >= 0)
   {
      if (paError == 0)
      {
         isActive = FALSE;
      }
      else
      {
         isActive = TRUE;
      }

      status = OS_SUCCESS;
   }

   return status;
}

OsStatus MpPortAudioDriver::getStreamInfo(MpAudioStreamId stream,
                                          MpAudioStreamInfo& streamInfo) const
{
   OsLock lock(ms_driverMutex);
   OsStatus status = OS_FAILED;

   const PaStreamInfo* paStreamInfo = Pa_GetStreamInfo(stream);

   if (paStreamInfo)
   {
      streamInfo.setSampleRate(paStreamInfo->sampleRate);
      streamInfo.setOutputLatency(paStreamInfo->outputLatency);
      streamInfo.setInputLatency(paStreamInfo->inputLatency);
      status = OS_SUCCESS;
   }

   return status;
}

OsStatus MpPortAudioDriver::getStreamTime(MpAudioStreamId stream,
                                          double& streamTime) const
{
   OsLock lock(ms_driverMutex);
   OsStatus status = OS_FAILED;

   double paTime = Pa_GetStreamTime(stream);

   if (paTime > 0)
   {
      streamTime = paTime;
      status = OS_SUCCESS;
   }

   return status;
}

OsStatus MpPortAudioDriver::getStreamCpuLoad(MpAudioStreamId stream,
                                             double& cpuLoad) const
{
   OsLock lock(ms_driverMutex);
   OsStatus status = OS_SUCCESS;

   cpuLoad = Pa_GetStreamCpuLoad(stream);

   return status;
}

OsStatus MpPortAudioDriver::readStreamSync(MpAudioStreamId stream,
                                           void *buffer,
                                           unsigned long frames)
{
   OsLock lock(ms_driverMutex);
   OsStatus status = OS_FAILED;

   // first verify that stream is synchronous
   UtlTypedValue<MpAudioStreamId> strm(stream);
   UtlContainable* res = m_audioStreamMap.find(&strm);

   if (!res)
   {
      // if not found, stream is synchronous...
      PaError paError = Pa_ReadStream(stream, buffer, frames);

      if (paError == paNoError)
      {
         status = OS_SUCCESS;
      }
      else if (paError == paInputOverflowed)
      {
         status = OS_OVERFLOW;
      }
   }

   return status;
}

OsStatus MpPortAudioDriver::writeStreamSync(MpAudioStreamId stream,
                                            const void *buffer,
                                            unsigned long frames)
{
   OsLock lock(ms_driverMutex);
   OsStatus status = OS_FAILED;

   // first verify that stream is synchronous
   UtlTypedValue<MpAudioStreamId> strm(stream);
   UtlContainable* res = m_audioStreamMap.find(&strm);

   if (!res)
   {
      // if not found, stream is synchronous...
      PaError paError = Pa_WriteStream(stream, buffer, frames);

      if (paError == paNoError)
      {
         status = OS_SUCCESS;
      }
      else if (paError == paOutputUnderflowed)
      {
         status = OS_OVERFLOW;
      }
   }

   return status;
}

OsStatus MpPortAudioDriver::readStreamAsync(MpAudioStreamId stream,
                                            void *buffer,
                                            unsigned long frames)
{
   OsLock lock(ms_driverMutex);
   OsStatus status = OS_FAILED;

   if (frames > 0)
   {
      // first verify that stream is asynchronous
      UtlTypedValue<MpAudioStreamId> strm(stream);
      UtlContainable* res = m_audioStreamMap.findValue(&strm);

      if (res)
      {
         // cast to UtlPtr
         UtlPtr<MpPortAudioStream>* pStrmPtr = (UtlPtr<MpPortAudioStream>*)res;
         // get pointer to MpPortAudioStream
         MpPortAudioStream* pStrm = pStrmPtr->getValue();
         return pStrm->readStreamAsync(buffer, frames);
      }
   }   
   
   return status;
}

OsStatus MpPortAudioDriver::writeStreamAsync(MpAudioStreamId stream,
                                             const void *buffer,
                                             unsigned long frames)
{
   OsLock lock(ms_driverMutex);
   OsStatus status = OS_FAILED;

   if (frames > 0)
   {
      // first verify that stream is asynchronous
      UtlTypedValue<MpAudioStreamId> strm(stream);
      UtlContainable* res = m_audioStreamMap.findValue(&strm);

      if (res)
      {
         // cast to UtlPtr
         UtlPtr<MpPortAudioStream>* pStrmPtr = (UtlPtr<MpPortAudioStream>*)res;
         // get pointer to MpPortAudioStream
         MpPortAudioStream* pStrm = pStrmPtr->getValue();
         return pStrm->writeStreamAsync(buffer, frames);
      }
   }   

   return status;
}

OsStatus MpPortAudioDriver::getStreamReadAvailable(MpAudioStreamId stream,
                                                   long& framesAvailable) const
{
   OsLock lock(ms_driverMutex);
   OsStatus status = OS_FAILED;

   // first verify that stream is synchronous
   UtlTypedValue<MpAudioStreamId> strm(stream);
   UtlContainable* res = m_audioStreamMap.find(&strm);

   if (!res)
   {
      signed long paFramesAvailable = Pa_GetStreamReadAvailable(stream);

      if (paFramesAvailable >= 0)
      {
         framesAvailable = paFramesAvailable;
         status = OS_SUCCESS;
      }
   }

   return status;
}

OsStatus MpPortAudioDriver::getStreamWriteAvailable(MpAudioStreamId stream,
                                                    long& framesAvailable) const
{
   OsLock lock(ms_driverMutex);
   OsStatus status = OS_FAILED;

   // first verify that stream is synchronous
   UtlTypedValue<MpAudioStreamId> strm(stream);
   UtlContainable* res = m_audioStreamMap.find(&strm);

   if (!res)
   {
      signed long paFramesAvailable = Pa_GetStreamWriteAvailable(stream);

      if (paFramesAvailable >= 0)
      {
         framesAvailable = paFramesAvailable;
         status = OS_SUCCESS;
      }
   }

   return status;
}

OsStatus MpPortAudioDriver::getSampleSize(MpAudioDriverSampleFormat format,
                                          int& sampleSize) const
{
   OsLock lock(ms_driverMutex);
   OsStatus status = OS_FAILED;

   PaError paSampleSize = Pa_GetSampleSize(format);

   if (paSampleSize != paSampleFormatNotSupported)
   {
      sampleSize = paSampleSize;
      status = OS_SUCCESS;
   }

   return status;
}

OsStatus MpPortAudioDriver::getInputVolumeMeterReading(MpAudioStreamId stream,
                                                       unsigned int& volume,
                                                       MP_VOLUME_METER_TYPE type) const
{
   OsLock lock(ms_driverMutex);

   // first verify that stream is asynchronous
   UtlTypedValue<MpAudioStreamId> strm(stream);
   UtlContainable* res = m_audioStreamMap.findValue(&strm);

   if (res)
   {
      // cast to UtlPtr
      UtlPtr<MpPortAudioStream>* pStrmPtr = (UtlPtr<MpPortAudioStream>*)res;
      // get pointer to MpPortAudioStream
      MpPortAudioStream* pStrm = pStrmPtr->getValue();
      volume = pStrm->getInputStreamVolume(type);
      return OS_SUCCESS;
   }

   return OS_NOT_SUPPORTED;
}

OsStatus MpPortAudioDriver::getOutputVolumeMeterReading(MpAudioStreamId stream,
                                                        unsigned int& volume,
                                                        MP_VOLUME_METER_TYPE type) const
{
   OsLock lock(ms_driverMutex);

   // first verify that stream is asynchronous
   UtlTypedValue<MpAudioStreamId> strm(stream);
   UtlContainable* res = m_audioStreamMap.findValue(&strm);

   if (res)
   {
      // cast to UtlPtr
      UtlPtr<MpPortAudioStream>* pStrmPtr = (UtlPtr<MpPortAudioStream>*)res;
      // get pointer to MpPortAudioStream
      MpPortAudioStream* pStrm = pStrmPtr->getValue();
      volume = pStrm->getOutputStreamVolume(type);
      return OS_SUCCESS;
   }

   return OS_NOT_SUPPORTED;
}

void MpPortAudioDriver::release()
{
   delete this;
}

/* ============================ ACCESSORS ================================= */

/* ============================ INQUIRY =================================== */

/* //////////////////////////// PROTECTED ///////////////////////////////// */

/* //////////////////////////// PRIVATE /////////////////////////////////// */

MpPortAudioDriver::MpPortAudioDriver()
{
}

MpPortAudioDriver::~MpPortAudioDriver(void)
{
   OsLock lock(ms_driverMutex);

   // decrease counter in thread safe manner
   ms_instanceCounter--;

   if (m_audioStreamMap.entries() > 0)
   {
      OsSysLog::add(FAC_AUDIO, PRI_ERR, "There are some unclosed streams!!\n");

      // MpPortAudioStream will be deleted as well as it uses special UtlPtr<MpPortAudioStream*>
      // with delete content flag TRUE
      m_audioStreamMap.destroyAll();
   }

   PaError err = Pa_Terminate();
   if (err != paNoError)
   {
      UtlString error(Pa_GetErrorText(err));
      OsSysLog::add(FAC_AUDIO, PRI_ERR, "Pa_Terminate failed, error: %s\n", error.data());
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
         OsSysLog::add(FAC_AUDIO, PRI_ERR, "Pa_Initialize failed, error: %s\n", error.data());
      }
   }

   // only 1 instance of this driver is allowed, or error occurred
   return NULL;
}

void MpPortAudioDriver::resetAsyncStream(MpAudioStreamId stream)
{
   // external lock is assumed

   // first verify that stream is asynchronous
   UtlTypedValue<MpAudioStreamId> strm(stream);
   UtlContainable* res = m_audioStreamMap.findValue(&strm);

   if (res)
   {
      // cast to UtlPtr
      UtlPtr<MpPortAudioStream>* pStrmPtr = (UtlPtr<MpPortAudioStream>*)res;
      // get pointer to MpPortAudioStream
      MpPortAudioStream* pStrm = pStrmPtr->getValue();
      // asynchronous stream needs to be reset after abort
      return pStrm->resetStream();
   }
}


/* ============================ FUNCTIONS ================================= */

