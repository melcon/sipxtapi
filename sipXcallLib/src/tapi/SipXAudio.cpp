//
// Copyright (C) 2007 Jaroslav Libak
// Licensed to SIPfoundry under a Contributor Agreement.
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
#include "mi/CpMediaInterfaceFactoryImpl.h"
#include "mi/CpMediaInterfaceFactoryFactory.h"

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

   numDevices = waveOutGetNumDevs();
   for (int i = 0; i < numDevices && i < MAX_AUDIO_DEVICES; i++)
   {
      result = waveOutGetDevCaps(i, &outcaps, sizeof(WAVEOUTCAPS));
      assert(result == MMSYSERR_NOERROR);
      pInst.outputAudioDevices[i] = SAFE_STRDUP(outcaps.szPname) ;
   }
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
void initMicSettings(MIC_SETTING& pMicSetting)
{
   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "initMicSettings micSettings=%p",
      &pMicSetting);

   pMicSetting.bMuted = FALSE;
   pMicSetting.iGain = GAIN_DEFAULT;
   memset(&pMicSetting.device, 0, sizeof(pMicSetting.device));
}

// CHECKED
void initSpeakerSettings(SPEAKER_SETTING& pSpeakerSetting)
{
   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "initSpeakerSettings speakerSettings=%p",
      &pSpeakerSetting);

   pSpeakerSetting.iVol = VOLUME_DEFAULT;
   memset(&pSpeakerSetting.device, 0, sizeof(pSpeakerSetting.device));
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
      CpMediaInterfaceFactoryImpl* pInterface = 
         pInst->pCallManager->getMediaInterfaceFactory()->getFactoryImplementation();

      // Validate gain is within range
      assert(iLevel >= GAIN_MIN);
      assert(iLevel <= GAIN_MAX);

      if (iLevel >= GAIN_MIN && iLevel <= GAIN_MAX)
      {
         OsStatus rc = OS_SUCCESS;

         // Record Gain
         pInst->micSetting.iGain = iLevel;

         // Set Gain if not muted
         if (!pInst->micSetting.bMuted)
         {
            int iAdjustedGain = (int) ((double)((double)iLevel / (double)GAIN_MAX) * 100.0);
            rc = pInterface->setMicrophoneGain(iAdjustedGain);
            assert(rc == OS_SUCCESS);
         }

         sr = SIPX_RESULT_SUCCESS;
      }
      else
      {
         sr = SIPX_RESULT_INVALID_ARGS;
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
      CpMediaInterfaceFactoryImpl* pInterface = 
         pInst->pCallManager->getMediaInterfaceFactory()->getFactoryImplementation();

      OsStatus status = pInterface->getMicrophoneGain(*iLevel);
      assert(status == OS_SUCCESS);

      if (status == OS_SUCCESS)
      {
         sr = SIPX_RESULT_SUCCESS;
      }
   }

   return sr;
}

// CHECKED
SIPXTAPI_API SIPX_RESULT sipxAudioMute(const SIPX_INST hInst,
                                       const int bMute)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxAudioMute");
   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxAudioMute hInst=%p bMute=%d",
      hInst, bMute);

   SIPX_RESULT sr = SIPX_RESULT_FAILURE;
   SIPX_INSTANCE_DATA* pInst = (SIPX_INSTANCE_DATA*)hInst;

   if (pInst)
   {
      CpMediaInterfaceFactoryImpl* pInterface = 
         pInst->pCallManager->getMediaInterfaceFactory()->getFactoryImplementation();

      // Only process if uninitialized (first call) or the state has changed
      if (bMute != pInst->micSetting.bMuted)
      {
         if (bMute)
         {
            // get gain for storing
            int iLevel;
            if (sipxAudioGetGain(hInst, &iLevel) == SIPX_RESULT_SUCCESS)
            {
               // remember gain
               pInst->micSetting.iGain = iLevel;
            }

            // Mute gain
            OsStatus rc = pInterface->muteMicrophone(bMute);
            assert(rc == OS_SUCCESS);

            if (rc == OS_SUCCESS)
            {
               // Store setting
               pInst->micSetting.bMuted = bMute;

               sr = SIPX_RESULT_SUCCESS;
            }
         }
         else
         {
            // UnMute mic
            OsStatus rc = pInterface->muteMicrophone(bMute);
            assert(rc == OS_SUCCESS);

            if (rc == OS_SUCCESS)
            {
               // Store setting
               pInst->micSetting.bMuted = bMute;

               // Restore gain
               // convert from sipXtapi scale to 100 scale
               int iAdjustedGain = (int) (double)((((double)pInst->micSetting.iGain / (double)GAIN_MAX)) * 100.0);
               rc = pInterface->setMicrophoneGain(iAdjustedGain);
               assert(rc == OS_SUCCESS);

               if (rc == OS_SUCCESS)
               {
                  sr = SIPX_RESULT_SUCCESS;
               }
            }
         }
      }
      else
      {
         sr = SIPX_RESULT_SUCCESS;
      }
   }

   return sr;
}

// CHECKED
SIPXTAPI_API SIPX_RESULT sipxAudioIsMuted(const SIPX_INST hInst,
                                          int* bMuted)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxAudioIsMuted");
   OsSysLog::add(FAC_SIPXTAPI, PRI_DEBUG,
      "sipxAudioIsMuted hInst=%p",
      hInst);

   SIPX_RESULT sr = SIPX_RESULT_FAILURE;
   SIPX_INSTANCE_DATA* pInst = (SIPX_INSTANCE_DATA*)hInst;

   if (pInst)
   {
      *bMuted = pInst->micSetting.bMuted;

      sr = SIPX_RESULT_SUCCESS;
   }

   return sr;
}

// CHECKED
SIPXTAPI_API SIPX_RESULT sipxAudioEnableSpeaker(const SIPX_INST hInst,
                                                const SPEAKER_TYPE type)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxAudioEnableSpeaker");
   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxAudioEnableSpeaker hInst=%p type=%d",
      hInst, type);

   SIPX_RESULT sr = SIPX_RESULT_FAILURE;
   SIPX_INSTANCE_DATA* pInst = (SIPX_INSTANCE_DATA*)hInst;

   if (pInst)
   {
      CpMediaInterfaceFactoryImpl* pInterface = 
         pInst->pCallManager->getMediaInterfaceFactory()->getFactoryImplementation();

      if (pInst->enabledSpeaker != type)
      {
         OsStatus status;

         pInst->enabledSpeaker = type;

         // Lower Volume
         status = pInterface->setSpeakerVolume(0);
         assert(status == OS_SUCCESS);

         if (status == OS_SUCCESS)
         {
            // Enable Speaker
            switch (type)
            {
            case SPEAKER:
            case RINGER:
               pInterface->setSpeakerDevice(pInst->speakerSettings[type].device);
               pInterface->getSpeakerDevice(pInst->speakerSettings[type].device);
               break;
            default:
               assert(false);
               break;
            }
         }

         if (status == OS_SUCCESS)
         {
            // Reset Volume
            SIPX_RESULT rc;
            rc = sipxAudioSetVolume(hInst, type, pInst->speakerSettings[type].iVol);
            assert(rc == SIPX_RESULT_SUCCESS);

            if (rc == SIPX_RESULT_SUCCESS)
            {
               sr = SIPX_RESULT_SUCCESS;
            }
         }
      }
      else
      {
         sr = SIPX_RESULT_SUCCESS;
      }
   }

   return sr;
}

// CHECKED
SIPXTAPI_API SIPX_RESULT sipxAudioGetEnabledSpeaker(const SIPX_INST hInst,
                                                    SPEAKER_TYPE* type)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxAudioGetEnabledSpeaker");
   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxAudioGetEnabledSpeaker hInst=%p",
      hInst);

   SIPX_RESULT sr = SIPX_RESULT_FAILURE;
   SIPX_INSTANCE_DATA* pInst = (SIPX_INSTANCE_DATA*)hInst;

   if (pInst)
   {
      *type = pInst->enabledSpeaker;
      sr = SIPX_RESULT_SUCCESS;
   }

   return sr;
}

// CHECKED
SIPXTAPI_API SIPX_RESULT sipxAudioSetVolume(const SIPX_INST hInst,
                                            const SPEAKER_TYPE type,
                                            const int iLevel)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxAudioSetVolume");
   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxAudioSetVolume hInst=%p type=%d iLevel=%d",
      hInst, type, iLevel);

   SIPX_RESULT sr = SIPX_RESULT_FAILURE;
   SIPX_INSTANCE_DATA* pInst = (SIPX_INSTANCE_DATA*)hInst;

   assert(type == SPEAKER || type == RINGER);
   assert(iLevel >= VOLUME_MIN);
   assert(iLevel <= VOLUME_MAX);

   if (pInst)
   {
      CpMediaInterfaceFactoryImpl* pInterface = 
         pInst->pCallManager->getMediaInterfaceFactory()->getFactoryImplementation();

      // Validate Params
      if ((type == SPEAKER || type == RINGER) &&
         (iLevel >= VOLUME_MIN) &&
         (iLevel <= VOLUME_MAX))
      {
         // Store value
         pInst->speakerSettings[type].iVol = iLevel;
         sr = SIPX_RESULT_SUCCESS;

         // Set value if this type is enabled
         if (pInst->enabledSpeaker == type)
         {
            // the CpMediaInterfaceFactoryImpl always uses a scale of 0 - 100
            OsStatus status = pInterface->setSpeakerVolume(iLevel);
            assert(status == OS_SUCCESS);

            if (status != OS_SUCCESS)
            {
               sr = SIPX_RESULT_FAILURE;
            }
         }
      }
      else
      {
         sr = SIPX_RESULT_INVALID_ARGS;
      }
   }

   return sr;
}

// CHECKED
SIPXTAPI_API SIPX_RESULT sipxAudioGetVolume(const SIPX_INST hInst,
                                            const SPEAKER_TYPE type,
                                            int* iLevel)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxAudioGetVolume");
   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxAudioGetVolume hInst=%p type=%d",
      hInst, type);

   SIPX_RESULT sr = SIPX_RESULT_FAILURE;
   SIPX_INSTANCE_DATA* pInst = (SIPX_INSTANCE_DATA*)hInst;

   assert(type == SPEAKER || type == RINGER);

   if (pInst)
   {
      // Validate Params
      if (type == SPEAKER || type == RINGER)
      {
         if (pInst->enabledSpeaker == type)
         {
            // this speaker is enabled, return real volume from getSpeakerVolume
            CpMediaInterfaceFactoryImpl* pInterface = 
               pInst->pCallManager->getMediaInterfaceFactory()->getFactoryImplementation();

            OsStatus status = pInterface->getSpeakerVolume(*iLevel);
            assert(status == OS_SUCCESS);

            if (status == OS_SUCCESS)
            {
               pInst->speakerSettings[type].iVol = *iLevel;
               sr = SIPX_RESULT_SUCCESS;
            }
         }
         else
         {
            // this speaker is not enabled, just return stored value
            *iLevel = pInst->speakerSettings[type].iVol;
            sr = SIPX_RESULT_SUCCESS;
         }
      }
      else
      {
         sr = SIPX_RESULT_INVALID_ARGS;
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
      CpMediaInterfaceFactoryImpl* pInterface = 
         pInst->pCallManager->getMediaInterfaceFactory()->getFactoryImplementation();

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
      CpMediaInterfaceFactoryImpl* pInterface = 
         pInst->pCallManager->getMediaInterfaceFactory()->getFactoryImplementation();

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
      CpMediaInterfaceFactoryImpl* pInterface = 
         pInst->pCallManager->getMediaInterfaceFactory()->getFactoryImplementation() ;

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
      CpMediaInterfaceFactoryImpl* pInterface = 
         pInst->pCallManager->getMediaInterfaceFactory()->getFactoryImplementation();

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
      CpMediaInterfaceFactoryImpl* pInterface = 
         pInst->pCallManager->getMediaInterfaceFactory()->getFactoryImplementation();

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
      CpMediaInterfaceFactoryImpl* pInterface = 
         pInst->pCallManager->getMediaInterfaceFactory()->getFactoryImplementation();

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
                                                     size_t* numDevices)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxAudioGetNumInputDevices");
   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxAudioGetNumInputDevices hInst=%p",
      hInst);

   SIPX_RESULT rc = SIPX_RESULT_INVALID_ARGS;
   SIPX_INSTANCE_DATA* pInst = (SIPX_INSTANCE_DATA*)hInst;

   assert(pInst);
   if (pInst)
   {
      *numDevices = 0;

      while (*numDevices < MAX_AUDIO_DEVICES &&
             pInst->inputAudioDevices[*numDevices])
      {
         (*numDevices)++;
      }

      rc = SIPX_RESULT_SUCCESS;
   }

   return rc;
}

// CHECKED
SIPXTAPI_API SIPX_RESULT sipxAudioGetInputDevice(const SIPX_INST hInst,
                                                 const int index,
                                                 const char** szDevice)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxAudioGetInputDevice");
   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxAudioGetInputDevice hInst=%p index=%d",
      hInst, index);

   SIPX_RESULT rc = SIPX_RESULT_INVALID_ARGS;
   SIPX_INSTANCE_DATA* pInst = (SIPX_INSTANCE_DATA*)hInst;

   assert(pInst);
   if (pInst && (index >= 0) && (index < MAX_AUDIO_DEVICES) && szDevice)
   {
      *szDevice = pInst->inputAudioDevices[index];
      rc = SIPX_RESULT_SUCCESS;
   }

   return rc;
}

// CHECKED
SIPXTAPI_API SIPX_RESULT sipxAudioGetNumOutputDevices(const SIPX_INST hInst,
                                                      size_t* numDevices)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxAudioGetNumOutputDevices");
   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxAudioGetNumOutputDevices hInst=%p",
      hInst);

   SIPX_RESULT rc = SIPX_RESULT_INVALID_ARGS;
   SIPX_INSTANCE_DATA* pInst = (SIPX_INSTANCE_DATA*)hInst;

   assert(pInst);
   if (pInst)
   {
      *numDevices = 0;

      while (*numDevices < MAX_AUDIO_DEVICES &&
             pInst->outputAudioDevices[*numDevices])
      {
         (*numDevices)++;
      }

      rc = SIPX_RESULT_SUCCESS;
   }

   return rc;
}

// CHECKED
SIPXTAPI_API SIPX_RESULT sipxAudioGetOutputDevice(const SIPX_INST hInst,
                                                  const int index,
                                                  const char** szDevice)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxAudioGetOutputDevice");
   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxAudioGetOutputDevice hInst=%p index=%d",
      hInst, index);

   SIPX_RESULT rc = SIPX_RESULT_INVALID_ARGS;
   SIPX_INSTANCE_DATA* pInst = (SIPX_INSTANCE_DATA*)hInst;

   assert(pInst);
   if (pInst && (index >= 0) && (index < MAX_AUDIO_DEVICES) && szDevice)
   {
      *szDevice = pInst->outputAudioDevices[index];
      rc = SIPX_RESULT_SUCCESS;
   }

   return rc;
}

// CHECKED
SIPXTAPI_API SIPX_RESULT sipxAudioSetCallInputDevice(const SIPX_INST hInst,
                                                     const char* szDevice)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxAudioSetCallInputDevice");
   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxAudioSetCallInputDevice hInst=%p device=%s",
      hInst, szDevice);

   SIPX_RESULT rc = SIPX_RESULT_INVALID_ARGS;
   SIPX_INSTANCE_DATA* pInst = (SIPX_INSTANCE_DATA*)hInst;

   if (pInst)
   {
      UtlString oldDevice;

      CpMediaInterfaceFactoryImpl* pInterface = 
         pInst->pCallManager->getMediaInterfaceFactory()->getFactoryImplementation();

      // Get existing device
      OsStatus status = pInterface->getMicrophoneDevice(oldDevice);
      assert(status == OS_SUCCESS);

      if (strcasecmp(szDevice, "NONE") == 0)
      {
         // "NONE" = special device??
         pInst->micSetting.device = szDevice;
         status = pInterface->setMicrophoneDevice(pInst->micSetting.device);
         assert(status == OS_SUCCESS);

         if (status == OS_SUCCESS)
         {
            rc = SIPX_RESULT_SUCCESS;
         }
      }
      else
      {
         if (strcmp(szDevice, oldDevice) != 0)
         {
            for (int i = 0; i < MAX_AUDIO_DEVICES; i++)
            {
               if (pInst->inputAudioDevices[i])
               {
                  if (strcmp(szDevice, pInst->inputAudioDevices[i]) == 0)
                  {
                     // Match
                     pInst->micSetting.device = szDevice;
                     status = pInterface->setMicrophoneDevice(pInst->micSetting.device);

                     assert(status == OS_SUCCESS);

                     if (status == OS_SUCCESS)
                     {
                        rc = SIPX_RESULT_SUCCESS;
                     }

                     break;
                  }
               }
               else
               {
                  break;
               }
            }
         }
         else
         {
            rc = SIPX_RESULT_SUCCESS;
         }

      }
   }

   return rc;
}

// CHECKED
SIPXTAPI_API SIPX_RESULT sipxAudioSetRingerOutputDevice(const SIPX_INST hInst,
                                                        const char* szDevice)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxAudioSetRingerOutputDevice");
   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxAudioSetRingerOutputDevice hInst=%p device=%s",
      hInst, szDevice);

   SIPX_RESULT rc = SIPX_RESULT_INVALID_ARGS;
   SIPX_INSTANCE_DATA* pInst = (SIPX_INSTANCE_DATA*)hInst;

   if (pInst)
   {
      UtlString oldDevice;
      OsStatus status;

      CpMediaInterfaceFactoryImpl* pInterface = 
         pInst->pCallManager->getMediaInterfaceFactory()->getFactoryImplementation();

      // Get existing device
      oldDevice = pInst->speakerSettings[RINGER].device;

      if (strcasecmp(szDevice, "NONE") == 0)
      {
         pInst->speakerSettings[RINGER].device = szDevice;

         if (pInst->enabledSpeaker == RINGER)
         {
            status = pInterface->setSpeakerDevice(pInst->speakerSettings[RINGER].device);
            assert(status == OS_SUCCESS);

            if (status == OS_SUCCESS)
            {
               rc = SIPX_RESULT_SUCCESS;
            }
         }
         else
         {
            rc = SIPX_RESULT_SUCCESS;
         }
      }
      else
      {
         if (strcmp(szDevice, oldDevice) != 0)
         {
            // device name differs
            for (int i = 0; i < MAX_AUDIO_DEVICES; i++)
            {
               if (pInst->outputAudioDevices[i])
               {
                  if (strcmp(szDevice, pInst->outputAudioDevices[i]) == 0)
                  {
                     // Match
                     pInst->speakerSettings[RINGER].device = szDevice;

                     if (pInst->enabledSpeaker == RINGER)
                     {
                        status = pInterface->setSpeakerDevice(pInst->speakerSettings[RINGER].device);
                        assert(status == OS_SUCCESS);

                        if (status == OS_SUCCESS)
                        {
                           rc = SIPX_RESULT_SUCCESS;
                        }
                     }
                     else
                     {
                        rc = SIPX_RESULT_SUCCESS;
                     }

                     break;
                  }
               }
               else
               {
                  // the last element, break
                  break;
               }
            }
         }
         else
         {
            // device name is the same like old one
            rc = SIPX_RESULT_SUCCESS;
         }

      }
   }

   return rc;
}


// CHECKED
SIPXTAPI_API SIPX_RESULT sipxAudioSetCallOutputDevice(const SIPX_INST hInst,
                                                      const char* szDevice)
{
   OsStackTraceLogger stackLogger(FAC_SIPXTAPI, PRI_DEBUG, "sipxAudioSetCallOutputDevice");
   OsSysLog::add(FAC_SIPXTAPI, PRI_INFO,
      "sipxAudioSetCallOutputDevice hInst=%p device=%s",
      hInst, szDevice);

   SIPX_RESULT rc = SIPX_RESULT_INVALID_ARGS;
   SIPX_INSTANCE_DATA* pInst = (SIPX_INSTANCE_DATA*)hInst;

   if (pInst)
   {
      UtlString oldDevice;
      OsStatus status;

      CpMediaInterfaceFactoryImpl* pInterface = 
         pInst->pCallManager->getMediaInterfaceFactory()->getFactoryImplementation();

      // Get existing device
      oldDevice = pInst->speakerSettings[SPEAKER].device;

      if (strcasecmp(szDevice, "NONE") == 0)
      {
         pInst->speakerSettings[SPEAKER].device = szDevice;

         if (pInst->enabledSpeaker == SPEAKER)
         {
            status = pInterface->setSpeakerDevice(pInst->speakerSettings[SPEAKER].device);
            assert(status == OS_SUCCESS);

            if (status == OS_SUCCESS)
            {
               rc = SIPX_RESULT_SUCCESS;
            }
         }
         else
         {
            rc = SIPX_RESULT_SUCCESS;
         }
      }
      else
      {
         if (strcmp(szDevice, oldDevice) != 0)
         {
            for (int i = 0; i < MAX_AUDIO_DEVICES; i++)
            {
               if (pInst->outputAudioDevices[i])
               {
                  if (strcmp(szDevice, pInst->outputAudioDevices[i]) == 0)
                  {
                     // Match
                     pInst->speakerSettings[SPEAKER].device = szDevice;

                     if (pInst->enabledSpeaker == SPEAKER)
                     {
                        status = pInterface->setSpeakerDevice(pInst->speakerSettings[SPEAKER].device);
                        assert(status == OS_SUCCESS);

                        if (status == OS_SUCCESS)
                        {
                           rc = SIPX_RESULT_SUCCESS;
                        }
                     }
                     else
                     {
                        rc = SIPX_RESULT_SUCCESS;
                     }

                     break;
                  }
               }
               else
               {
                  break;
               }
            }
         }
         else
         {
            rc = SIPX_RESULT_SUCCESS;
         }

      }
   }

   return rc;
}

