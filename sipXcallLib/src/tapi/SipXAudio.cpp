//
// Copyright (C) 2007 Jaroslav Libak
// Licensed under the LGPL license.
//
// Copyright (C) 2005-2007 SIPez LLC.
// Licensed to SIPfoundry under a Contributor Agreement.
// 
// Copyright (C) 2004-2007 SIPfoundry Inc.
// Licensed by SIPfoundry under the LGPL license.
//
// Copyright (C) 2004-2006 Pingtel Corp.  All rights reserved.
// Licensed to SIPfoundry under a Contributor Agreement.
//
// $$
///////////////////////////////////////////////////////////////////////////////

// SYSTEM INCLUDES
#include <assert.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>
#endif /* _WIN32 */

// APPLICATION INCLUDES
#include "tapi/SipXAudio.h"
#include "cp/CallManager.h"
#include "mi/CpMediaInterfaceFactory.h"
#include "mi/CpMediaInterfaceFactoryFactory.h"
#include <mi/CpAudioDeviceInfo.h>

// DEFINES
// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
// STATIC VARIABLE INITIALIZATIONS
// MACROS
// GLOBAL VARIABLES
// GLOBAL FUNCTIONS

// CHECKED
void initAudioDevices(SIPX_INSTANCE_DATA& pInst)
{
#if defined(_WIN32)
   WAVEOUTCAPS outcaps;
   WAVEINCAPS  incaps;
   int numDevices;
   MMRESULT result;

   numDevices = waveInGetNumDevs();
   for (int i = 0; i < numDevices && i < MAX_AUDIO_DEVICES; i++)
   {
      result = waveInGetDevCaps(i, &incaps, sizeof(WAVEINCAPS));
      assert(result == MMSYSERR_NOERROR);
      pInst.inputAudioDevices[i] = SAFE_STRDUP(incaps.szPname);
   }
   pInst.nInputAudioDevices = numDevices;

   numDevices = waveOutGetNumDevs();
   for (int i = 0; i < numDevices && i < MAX_AUDIO_DEVICES; i++)
   {
      result = waveOutGetDevCaps(i, &outcaps, sizeof(WAVEOUTCAPS));
      assert(result == MMSYSERR_NOERROR);
      pInst.outputAudioDevices[i] = SAFE_STRDUP(outcaps.szPname) ;
   }
   pInst.nOutputAudioDevices = numDevices;

#else
   pInst.inputAudioDevices[0] = SAFE_STRDUP("Default");
   pInst.outputAudioDevices[0] = SAFE_STRDUP("Default");
#endif
}

// CHECKED
void freeAudioDevices(SIPX_INSTANCE_DATA& pInst)
{
   for (int i = 0; i < MAX_AUDIO_DEVICES; i++)
   {
      if (pInst.inputAudioDevices[i])
      {
         free(pInst.inputAudioDevices[i]);
         pInst.inputAudioDevices[i] = NULL;
      }

      if (pInst.outputAudioDevices[i])
      {
         free(pInst.outputAudioDevices[i]);
         pInst.outputAudioDevices[i] = NULL;
      }
   }
}

// CHECKED
SIPXTAPI_API SIPX_RESULT sipxAudioSetGain(const SIPX_INST hInst,
                                          const int iLevel)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxAudioSetGain");
   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxAudioSetGain hInst=%p iLevel=%d",
      hInst, iLevel);

   SIPX_RESULT sr = SIPX_RESULT_FAILURE;
   SIPX_INSTANCE_DATA* pInst = (SIPX_INSTANCE_DATA*)hInst;

   if (pInst)
   {
      CpMediaInterfaceFactory* pInterface = pInst->pCallManager->getMediaInterfaceFactory();

      if (pInterface)
      {
         // Validate gain is within range
         if (iLevel >= GAIN_MIN && iLevel <= GAIN_MAX)
         {
            OsStatus rc = OS_FAILED;
            rc = pInterface->setMicrophoneGain(iLevel);

            sr = SIPX_RESULT_SUCCESS;
         }
         else
         {
            sr = SIPX_RESULT_INVALID_ARGS;
         }
      }      
   }

   return sr;
}

// CHECKED
SIPXTAPI_API SIPX_RESULT sipxAudioGetGain(const SIPX_INST hInst,
                                          int* iLevel)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxAudioGetGain");
   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxAudioGetGain hInst=%p",
      hInst);

   SIPX_RESULT sr = SIPX_RESULT_FAILURE;
   SIPX_INSTANCE_DATA* pInst = (SIPX_INSTANCE_DATA*)hInst;

   if (pInst)
   {
      CpMediaInterfaceFactory* pInterface = 
         pInst->pCallManager->getMediaInterfaceFactory();

      OsStatus status = pInterface->getMicrophoneGain(*iLevel);

      if (status == OS_SUCCESS)
      {
         sr = SIPX_RESULT_SUCCESS;
      }
   }

   return sr;
}

// CHECKED
SIPXTAPI_API SIPX_RESULT sipxAudioMuteMic(const SIPX_INST hInst,
                                          const int bMute)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxAudioMuteMic");
   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxAudioMuteMic hInst=%p bMute=%d",
      hInst, bMute);

   SIPX_RESULT sr = SIPX_RESULT_FAILURE;
   SIPX_INSTANCE_DATA* pInst = (SIPX_INSTANCE_DATA*)hInst;

   if (pInst)
   {
      CpMediaInterfaceFactory* pInterface = pInst->pCallManager->getMediaInterfaceFactory();

      if (pInterface)
      {
         // Mute or unmute gain
         OsStatus rc = pInterface->muteMicrophone(bMute);

         if (rc == OS_SUCCESS)
         {
            sr = SIPX_RESULT_SUCCESS;
         }
      }     
   }

   return sr;
}

// CHECKED
SIPXTAPI_API SIPX_RESULT sipxAudioMuteSpeaker(const SIPX_INST hInst,
                                              const int bMute)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxAudioMuteSpeaker");
   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxAudioMuteSpeaker hInst=%p bMute=%d",
      hInst, bMute);

   SIPX_RESULT sr = SIPX_RESULT_FAILURE;
   SIPX_INSTANCE_DATA* pInst = (SIPX_INSTANCE_DATA*)hInst;

   if (pInst)
   {
      CpMediaInterfaceFactory* pInterface = pInst->pCallManager->getMediaInterfaceFactory();

      if (pInterface)
      {
         // Mute or unmute speaker
         OsStatus rc = pInterface->muteSpeaker(bMute);

         if (rc == OS_SUCCESS)
         {
            sr = SIPX_RESULT_SUCCESS;
         }
      }     
   }

   return sr;
}

// CHECKED
SIPXTAPI_API SIPX_RESULT sipxAudioIsMicMuted(const SIPX_INST hInst,
                                             int* bMuted)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxAudioIsMicMuted");
   OsSysLog::add(FAC_SIPXTAPI, PRI_DEBUG,
      "sipxAudioIsMicMuted hInst=%p",
      hInst);

   SIPX_RESULT sr = SIPX_RESULT_FAILURE;
   SIPX_INSTANCE_DATA* pInst = (SIPX_INSTANCE_DATA*)hInst;

   if (pInst)
   {
      CpMediaInterfaceFactory* pInterface = pInst->pCallManager->getMediaInterfaceFactory();
      if (pInterface)
      {
         UtlBoolean bIsMuted;
         OsStatus res = pInterface->isMicrophoneMuted(bIsMuted);
         if (res == OS_SUCCESS)
         {
            *bMuted = bIsMuted;
            sr = SIPX_RESULT_SUCCESS;
         }         
      }
   }

   return sr;
}

// CHECKED
SIPXTAPI_API SIPX_RESULT sipxAudioIsSpeakerMuted(const SIPX_INST hInst,
                                                 int* bMuted)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxAudioIsSpeakerMuted");
   OsSysLog::add(FAC_SIPXTAPI, PRI_DEBUG, "sipxAudioIsSpeakerMuted hInst=%p", hInst);

   SIPX_RESULT sr = SIPX_RESULT_FAILURE;
   SIPX_INSTANCE_DATA* pInst = (SIPX_INSTANCE_DATA*)hInst;

   if (pInst)
   {
      CpMediaInterfaceFactory* pInterface = pInst->pCallManager->getMediaInterfaceFactory();
      if (pInterface)
      {
         UtlBoolean bIsMuted;
         OsStatus res = pInterface->isSpeakerMuted(bIsMuted);
         if (res == OS_SUCCESS)
         {
            *bMuted = bIsMuted;
            sr = SIPX_RESULT_SUCCESS;
         }         
      }
   }

   return sr;
}

// CHECKED
SIPXTAPI_API SIPX_RESULT sipxAudioSetVolume(const SIPX_INST hInst,
                                            const int iLevel)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxAudioSetVolume");
   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxAudioSetVolume hInst=%p iLevel=%d",
      hInst, iLevel);

   SIPX_RESULT sr = SIPX_RESULT_FAILURE;
   SIPX_INSTANCE_DATA* pInst = (SIPX_INSTANCE_DATA*)hInst;

   if (pInst)
   {
      CpMediaInterfaceFactory* pInterface = pInst->pCallManager->getMediaInterfaceFactory();

      if (pInterface)
      {
         // Validate Params
         if (iLevel >= VOLUME_MIN && iLevel <= VOLUME_MAX)
         {
            // the CpMediaInterfaceFactoryImpl always uses a scale of 0 - 100
            OsStatus status = pInterface->setSpeakerVolume(iLevel);
            if (status == OS_SUCCESS)
            {
               sr = SIPX_RESULT_SUCCESS;
            }
         }
      }      
   }

   return sr;
}

// CHECKED
SIPXTAPI_API SIPX_RESULT sipxAudioGetVolume(const SIPX_INST hInst,
                                            int* iLevel)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxAudioGetVolume");
   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO, "sipxAudioGetVolume hInst=%p", hInst);

   SIPX_RESULT sr = SIPX_RESULT_FAILURE;
   SIPX_INSTANCE_DATA* pInst = (SIPX_INSTANCE_DATA*)hInst;

   if (pInst)
   {
      CpMediaInterfaceFactory* pInterface = pInst->pCallManager->getMediaInterfaceFactory();

      if (pInterface)
      {
         OsStatus status = pInterface->getSpeakerVolume(*iLevel);
         if (status == OS_SUCCESS)
         {
            sr = SIPX_RESULT_SUCCESS;
         }
      }
   }

   return sr;
}

// CHECKED
SIPXTAPI_API SIPX_RESULT sipxAudioSetAECMode(const SIPX_INST hInst,
                                             const SIPX_AEC_MODE mode) 
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxAudioSetAECMode");

   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxAudioSetAECMode hInst=%p mode=%d",
      hInst, mode);

   SIPX_RESULT sr = SIPX_RESULT_FAILURE;
   SIPX_INSTANCE_DATA* pInst = (SIPX_INSTANCE_DATA*)hInst;

   if (pInst)
   {
      CpMediaInterfaceFactory* pInterface = 
         pInst->pCallManager->getMediaInterfaceFactory();

      if (pInterface)
      {
         if (pInterface->setAudioAECMode((MEDIA_AEC_MODE)mode) == OS_SUCCESS)
         {
            if (!pInst->aecSetting.bInitialized)
            {
               pInst->aecSetting.bInitialized = true;
            }
            pInst->aecSetting.mode = mode;
            sr = SIPX_RESULT_SUCCESS;
         }
      }
   }

   return sr;
}

// CHECKED
SIPXTAPI_API SIPX_RESULT sipxAudioGetAECMode(const SIPX_INST hInst,
                                             SIPX_AEC_MODE* mode)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxAudioGetAECMode");

   OsSysLog::add(FAC_SIPXTAPI, PRI_DEBUG,
      "sipxAudioGetAECMode hInst=%p",
      hInst);

   SIPX_RESULT sr = SIPX_RESULT_FAILURE;
   SIPX_INSTANCE_DATA* pInst = (SIPX_INSTANCE_DATA*)hInst;

   if (pInst)
   {
      CpMediaInterfaceFactory* pInterface = 
         pInst->pCallManager->getMediaInterfaceFactory();

      if (pInterface)
      {
         if (!pInst->aecSetting.bInitialized)
         {
            MEDIA_AEC_MODE aceMode;
            if (pInterface->getAudioAECMode(aceMode) == OS_SUCCESS)
            {
               pInst->aecSetting.bInitialized = true;
               pInst->aecSetting.mode = (SIPX_AEC_MODE)aceMode;
               *mode = (SIPX_AEC_MODE)aceMode;

               sr = SIPX_RESULT_SUCCESS;
            }
         }
         else
         {
            *mode = pInst->aecSetting.mode;
            sr = SIPX_RESULT_SUCCESS;
         }
      }
   }

   return sr;
}

// CHECKED
SIPXTAPI_API SIPX_RESULT sipxAudioSetAGCMode(const SIPX_INST hInst,
                                             const int bEnable) 
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxAudioSetAGCMode");

   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxAudioSetAGCMode hInst=%p enable=%d",
      hInst, bEnable);

   SIPX_RESULT sr = SIPX_RESULT_FAILURE;
   SIPX_INSTANCE_DATA* pInst = (SIPX_INSTANCE_DATA*)hInst;

   if (pInst)
   {
      CpMediaInterfaceFactory* pInterface = 
         pInst->pCallManager->getMediaInterfaceFactory();

      if (pInterface)
      {
         if (pInterface->enableAGC(bEnable) == OS_SUCCESS)
         {
            if (!pInst->agcSetting.bInitialized)
            {
               pInst->agcSetting.bInitialized = true;
            }
            pInst->agcSetting.bEnabled = bEnable;
            sr = SIPX_RESULT_SUCCESS;
         }
      }
   }

   return sr;
}

// CHECKED
SIPXTAPI_API SIPX_RESULT sipxAudioGetAGCMode(const SIPX_INST hInst,
                                             int* bEnabled) 
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxAudioGetAGCMode");

   OsSysLog::add(FAC_SIPXTAPI, PRI_DEBUG,
      "sipxAudioGetAGCMode hInst=%p",
      hInst);

   SIPX_RESULT sr = SIPX_RESULT_FAILURE;
   SIPX_INSTANCE_DATA* pInst = (SIPX_INSTANCE_DATA*)hInst;

   if (pInst)
   {
      CpMediaInterfaceFactory* pInterface = 
         pInst->pCallManager->getMediaInterfaceFactory();

      if (pInterface)
      {
         if (!pInst->agcSetting.bInitialized)
         {            
            UtlBoolean bCheck;
            if (pInterface->isAGCEnabled(bCheck) == OS_SUCCESS)
            {
               pInst->agcSetting.bInitialized = true;
               pInst->agcSetting.bEnabled = (bCheck == TRUE);
               *bEnabled = bCheck;

               sr = SIPX_RESULT_SUCCESS;
            }
         }
         else
         {
            *bEnabled = pInst->agcSetting.bEnabled;
            sr = SIPX_RESULT_SUCCESS;
         }
      }      
   }

   return sr;
}

// CHECKED
SIPXTAPI_API SIPX_RESULT sipxAudioSetNoiseReductionMode(const SIPX_INST hInst,
                                                        const SIPX_NOISE_REDUCTION_MODE mode) 
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxAudioSetNoiseReductionMode");

   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxAudioSetNoiseReductionMode hInst=%p mode=%d",
      hInst, mode);

   SIPX_RESULT sr = SIPX_RESULT_FAILURE;
   SIPX_INSTANCE_DATA* pInst = (SIPX_INSTANCE_DATA*)hInst;

   if (pInst)
   {
      CpMediaInterfaceFactory* pInterface = 
         pInst->pCallManager->getMediaInterfaceFactory();

      if (pInterface)
      {
         if (pInterface->setAudioNoiseReductionMode((MEDIA_NOISE_REDUCTION_MODE)mode) == OS_SUCCESS)
         {
            if (!pInst->nrSetting.bInitialized)
            {
               pInst->nrSetting.bInitialized = true;
            }
            pInst->nrSetting.mode = mode;
            sr = SIPX_RESULT_SUCCESS;
         }
      }
   }

   return sr;
}

// CHECKED
SIPXTAPI_API SIPX_RESULT sipxAudioGetNoiseReductionMode(const SIPX_INST hInst,
                                                        SIPX_NOISE_REDUCTION_MODE* mode) 
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxAudioGetNoiseReductionMode");

   OsSysLog::add(FAC_SIPXTAPI, PRI_DEBUG,
      "sipxAudioGetNoiseReductionMode hInst=%p",
      hInst);

   SIPX_RESULT sr = SIPX_RESULT_FAILURE;
   SIPX_INSTANCE_DATA* pInst = (SIPX_INSTANCE_DATA*)hInst;

   if (pInst)
   {
      CpMediaInterfaceFactory* pInterface = 
         pInst->pCallManager->getMediaInterfaceFactory();

      if (pInterface)
      {
         if (!pInst->nrSetting.bInitialized)
         {
            MEDIA_NOISE_REDUCTION_MODE nrMode;
            if (pInterface->getAudioNoiseReductionMode(nrMode) == OS_SUCCESS)
            {
               pInst->nrSetting.bInitialized = true;
               pInst->nrSetting.mode = (SIPX_NOISE_REDUCTION_MODE)nrMode;
               *mode = (SIPX_NOISE_REDUCTION_MODE)nrMode;

               sr = SIPX_RESULT_SUCCESS;
            }
         }
         else
         {
            *mode = pInst->nrSetting.mode;
            sr = SIPX_RESULT_SUCCESS;
         }
      }
   }

   return sr;
}

// CHECKED
SIPXTAPI_API SIPX_RESULT sipxAudioGetNumInputDevices(const SIPX_INST hInst,
                                                     int* numDevices)
{
   SIPX_RESULT rc = SIPX_RESULT_INVALID_ARGS;
   SIPX_INSTANCE_DATA* pInst = (SIPX_INSTANCE_DATA*)hInst;

   if (pInst)
   {
      CpMediaInterfaceFactory* pInterface = pInst->pCallManager->getMediaInterfaceFactory();

      if (pInterface)
      {
         OsStatus res = pInterface->getAudioInputDeviceCount(*numDevices);
         if (res == OS_SUCCESS)
         {
            rc = SIPX_RESULT_SUCCESS;
         }
      }
   }

   return rc;
}

// CHECKED
SIPXTAPI_API SIPX_RESULT sipxAudioGetInputDeviceInfo(const SIPX_INST hInst,
                                                     const int index,
                                                     SIPX_AUDIO_DEVICE* deviceInfo)
{
   SIPX_RESULT rc = SIPX_RESULT_INVALID_ARGS;
   SIPX_INSTANCE_DATA* pInst = (SIPX_INSTANCE_DATA*)hInst;

   if (pInst && deviceInfo && index >= 0)
   {
      CpMediaInterfaceFactory* pInterface = pInst->pCallManager->getMediaInterfaceFactory();

      if (pInterface)
      {
         CpAudioDeviceInfo cpDeviceInfo;
         OsStatus res = pInterface->getAudioInputDeviceInfo(index, cpDeviceInfo);
         if (res == OS_SUCCESS)
         {
            SAFE_STRNCPY(deviceInfo->deviceName, cpDeviceInfo.m_deviceName, sizeof(deviceInfo->deviceName));
            SAFE_STRNCPY(deviceInfo->driverName, cpDeviceInfo.m_driverName, sizeof(deviceInfo->deviceName));
            deviceInfo->bIsInput = cpDeviceInfo.m_bIsInput;
            deviceInfo->defaultSampleRate = cpDeviceInfo.m_defaultSampleRate;
            deviceInfo->maxChannels = cpDeviceInfo.m_maxChannels;
            rc = SIPX_RESULT_SUCCESS;
         }
      }
   }

   return rc;
}

// CHECKED
SIPXTAPI_API SIPX_RESULT sipxAudioGetNumOutputDevices(const SIPX_INST hInst,
                                                      int* numDevices)
{
   SIPX_RESULT rc = SIPX_RESULT_INVALID_ARGS;
   SIPX_INSTANCE_DATA* pInst = (SIPX_INSTANCE_DATA*)hInst;

   if (pInst)
   {
      CpMediaInterfaceFactory* pInterface = pInst->pCallManager->getMediaInterfaceFactory();

      if (pInterface)
      {
         OsStatus res = pInterface->getAudioOutputDeviceCount(*numDevices);
         if (res == OS_SUCCESS)
         {
            rc = SIPX_RESULT_SUCCESS;
         }
      }
   }

   return rc;
}

// CHECKED
SIPXTAPI_API SIPX_RESULT sipxAudioGetOutputDeviceInfo(const SIPX_INST hInst,
                                                      const int index,
                                                      SIPX_AUDIO_DEVICE* deviceInfo)
{
   SIPX_RESULT rc = SIPX_RESULT_INVALID_ARGS;
   SIPX_INSTANCE_DATA* pInst = (SIPX_INSTANCE_DATA*)hInst;

   if (pInst && deviceInfo && index >= 0)
   {
      CpMediaInterfaceFactory* pInterface = pInst->pCallManager->getMediaInterfaceFactory();

      if (pInterface)
      {
         CpAudioDeviceInfo cpDeviceInfo;
         OsStatus res = pInterface->getAudioOutputDeviceInfo(index, cpDeviceInfo);
         if (res == OS_SUCCESS)
         {
            SAFE_STRNCPY(deviceInfo->deviceName, cpDeviceInfo.m_deviceName, sizeof(deviceInfo->deviceName));
            SAFE_STRNCPY(deviceInfo->driverName, cpDeviceInfo.m_driverName, sizeof(deviceInfo->deviceName));
            deviceInfo->bIsInput = cpDeviceInfo.m_bIsInput;
            deviceInfo->defaultSampleRate = cpDeviceInfo.m_defaultSampleRate;
            deviceInfo->maxChannels = cpDeviceInfo.m_maxChannels;
            rc = SIPX_RESULT_SUCCESS;
         }
      }
   }

   return rc;
}

// CHECKED
SIPXTAPI_API SIPX_RESULT sipxAudioSetInputDevice(const SIPX_INST hInst,
                                                 const char* szDevice,
                                                 const char* szDriver)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxAudioSetInputDevice");
   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxAudioSetInputDevice hInst=%p device=%s",
      hInst, szDevice ? szDevice : "null");

   SIPX_RESULT rc = SIPX_RESULT_INVALID_ARGS;
   SIPX_INSTANCE_DATA* pInst = (SIPX_INSTANCE_DATA*)hInst;

   if (pInst)
   {
      CpMediaInterfaceFactory* pInterface = pInst->pCallManager->getMediaInterfaceFactory();

      if (pInterface)
      {
         OsStatus res = pInterface->setMicrophoneDevice(szDevice, szDriver);
         if (res == OS_SUCCESS)
         {
            rc = SIPX_RESULT_SUCCESS;
         }
      }
   }

   return rc;
}

// CHECKED
SIPXTAPI_API SIPX_RESULT sipxAudioGetInputDevice(const SIPX_INST hInst,
                                                 SIPX_AUDIO_DEVICE* deviceInfo)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxAudioGetInputDevice");
   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxAudioGetInputDevice hInst=%p", hInst);

   SIPX_RESULT rc = SIPX_RESULT_INVALID_ARGS;
   SIPX_INSTANCE_DATA* pInst = (SIPX_INSTANCE_DATA*)hInst;

   if (pInst && deviceInfo)
   {
      CpMediaInterfaceFactory* pInterface = pInst->pCallManager->getMediaInterfaceFactory();

      if (pInterface)
      {
         CpAudioDeviceInfo cpDeviceInfo;
         OsStatus res = pInterface->getMicrophoneDevice(cpDeviceInfo);
         if (res == OS_SUCCESS)
         {
            SAFE_STRNCPY(deviceInfo->deviceName, cpDeviceInfo.m_deviceName, sizeof(deviceInfo->deviceName));
            SAFE_STRNCPY(deviceInfo->driverName, cpDeviceInfo.m_driverName, sizeof(deviceInfo->deviceName));
            deviceInfo->bIsInput = cpDeviceInfo.m_bIsInput;
            deviceInfo->defaultSampleRate = cpDeviceInfo.m_defaultSampleRate;
            deviceInfo->maxChannels = cpDeviceInfo.m_maxChannels;
            rc = SIPX_RESULT_SUCCESS;
         }
      }
   }

   return rc;
}

// CHECKED
SIPXTAPI_API SIPX_RESULT sipxAudioSetOutputDevice(const SIPX_INST hInst,
                                                  const char* szDevice,
                                                  const char* szDriver)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxAudioSetOutputDevice");
   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxAudioSetOutputDevice hInst=%p device=%s",
      hInst, szDevice ? szDevice : "null");

   SIPX_RESULT rc = SIPX_RESULT_INVALID_ARGS;
   SIPX_INSTANCE_DATA* pInst = (SIPX_INSTANCE_DATA*)hInst;

   if (pInst)
   {
      CpMediaInterfaceFactory* pInterface = pInst->pCallManager->getMediaInterfaceFactory();

      if (pInterface)
      {
         OsStatus res = pInterface->setSpeakerDevice(szDevice, szDriver);
         if (res == OS_SUCCESS)
         {
            rc = SIPX_RESULT_SUCCESS;
         }
      }
   }

   return rc;
}

// CHECKED
SIPXTAPI_API SIPX_RESULT sipxAudioGetOutputDevice(const SIPX_INST hInst,
                                                  SIPX_AUDIO_DEVICE* deviceInfo)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxAudioGetOutputDevice");
   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxAudioGetOutputDevice hInst=%p", hInst);

   SIPX_RESULT rc = SIPX_RESULT_INVALID_ARGS;
   SIPX_INSTANCE_DATA* pInst = (SIPX_INSTANCE_DATA*)hInst;

   if (pInst && deviceInfo)
   {
      CpMediaInterfaceFactory* pInterface = pInst->pCallManager->getMediaInterfaceFactory();

      if (pInterface)
      {
         CpAudioDeviceInfo cpDeviceInfo;
         OsStatus res = pInterface->getSpeakerDevice(cpDeviceInfo);
         if (res == OS_SUCCESS)
         {
            SAFE_STRNCPY(deviceInfo->deviceName, cpDeviceInfo.m_deviceName, sizeof(deviceInfo->deviceName));
            SAFE_STRNCPY(deviceInfo->driverName, cpDeviceInfo.m_driverName, sizeof(deviceInfo->deviceName));
            deviceInfo->bIsInput = cpDeviceInfo.m_bIsInput;
            deviceInfo->defaultSampleRate = cpDeviceInfo.m_defaultSampleRate;
            deviceInfo->maxChannels = cpDeviceInfo.m_maxChannels;
            rc = SIPX_RESULT_SUCCESS;
         }
      }
   }

   return rc;
}
