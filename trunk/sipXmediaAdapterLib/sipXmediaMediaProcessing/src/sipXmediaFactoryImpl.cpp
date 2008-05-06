//
// Copyright (C) 2005-2007 SIPez LLC.
// Licensed to SIPfoundry under a Contributor Agreement.
// 
// Copyright (C) 2004-2006 SIPfoundry Inc.
// Licensed by SIPfoundry under the LGPL license.
//
// Copyright (C) 2004-2006 Pingtel Corp.  All rights reserved.
// Licensed to SIPfoundry under a Contributor Agreement.
//
// $$
///////////////////////////////////////////////////////////////////////////////


// SYSTEM INCLUDES
#include <stdlib.h>

// APPLICATION INCLUDES
#include "os/OsConfigDb.h"
#include "include/sipXmediaFactoryImpl.h"
#include "include/CpPhoneMediaInterface.h"
#include "sdp/SdpCodec.h"
#include "mp/MpMediaTask.h"
#include "mp/MpMisc.h"
#include "mp/MpCodec.h"
#include "mp/MpCallFlowGraph.h"
#include "net/SdpCodecFactory.h"
#include "mi/CpMediaInterfaceFactoryFactory.h"
#include "mp/MpAudioDriverDefs.h"
#include "mp/MpAudioDriverManager.h"
#include "mi/CpAudioDeviceInfo.h"

#ifdef INCLUDE_RTCP /* [ */
#include "rtcp/RTCManager.h"
#endif /* INCLUDE_RTCP ] */

#ifdef ENABLE_TOPOLOGY_FLOWGRAPH_INTERFACE_FACTORY
#include "mp/NetInTask.h"
#endif


// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
// GLOBAL FUNCTION

#define CONFIG_PHONESET_SEND_INBAND_DTMF  "PHONESET_SEND_INBAND_DTMF"
// initial pool size for audio buffers, if we run out of them,
// we allocate a new pool. They are deleted only during shutdown.
// Audio codecs number calculation:

#define GENERIC_AUDIO_CODECS_NUM 3

#define SPEEX_AUDIO_CODECS_BEGIN  GENERIC_AUDIO_CODECS_NUM
#ifdef HAVE_SPEEX /* [ */
#define SPEEX_AUDIO_CODECS_NUM 4
#else /* HAVE_SPEEX ] [ */
#define SPEEX_AUDIO_CODECS_NUM 0
#endif /* HAVE_SPEEX ] */

#define GSM_AUDIO_CODECS_BEGIN  (SPEEX_AUDIO_CODECS_BEGIN+SPEEX_AUDIO_CODECS_NUM)
#ifdef HAVE_GSM /* [ */
#define GSM_AUDIO_CODECS_NUM 1
#else /* HAVE_GSM ] [ */
#define GSM_AUDIO_CODECS_NUM 0
#endif /* HAVE_GSM ] */

#define ILBC_AUDIO_CODECS_BEGIN  (GSM_AUDIO_CODECS_BEGIN+GSM_AUDIO_CODECS_NUM)
#ifdef HAVE_ILBC /* [ */
#define ILBC_AUDIO_CODECS_NUM 1
#else /* HAVE_ILBC ] [ */
#define ILBC_AUDIO_CODECS_NUM 0
#endif /* HAVE_ILBC ] */
 
#define IPP_AUDIO_CODECS_BEGIN (ILBC_AUDIO_CODECS_BEGIN + ILBC_AUDIO_CODECS_NUM)
#ifdef HAVE_INTEL_IPP /* [ */
#define IPP_AUDIO_CODECS_NUM 2
#else /* HAVE_INTEL_IPP ] [ */
#define IPP_AUDIO_CODECS_NUM 0
#endif /* HAVE_INTEL_IPP ] */

#define TOTAL_AUDIO_CODECS_NUM (IPP_AUDIO_CODECS_BEGIN + IPP_AUDIO_CODECS_NUM)

// Video codecs  number calculation:


#define TOTAL_VIDEO_CODECS_NUM 0

// STATIC VARIABLE INITIALIZATIONS
int sipXmediaFactoryImpl::miInstanceCount = 0;

CpMediaInterfaceFactory* spFactory = NULL;
int siInstanceCount = 0;

extern "C" CpMediaInterfaceFactory* cpDefaultMediaFactoryFactory(OsConfigDb* pConfigDb)
{
    if (!spFactory)
    {
        spFactory = new sipXmediaFactoryImpl(pConfigDb);
    }
    siInstanceCount++;
    
    return spFactory;
}

#ifndef DISABLE_DEFAULT_PHONE_MEDIA_INTERFACE_FACTORY
extern "C" CpMediaInterfaceFactory* sipXmediaFactoryFactory(OsConfigDb* pConfigDb)
{
    return(cpDefaultMediaFactoryFactory(pConfigDb));
}
#endif

extern "C" void sipxDestroyMediaFactoryFactory()
{
    if (siInstanceCount > 0)
    {
        siInstanceCount--;
        if (siInstanceCount == 0 && spFactory)
        {
            delete spFactory;
            spFactory = NULL;
        }
    }
}


/* //////////////////////////// PUBLIC //////////////////////////////////// */

/* ============================ CREATORS ================================== */

// Constructor
sipXmediaFactoryImpl::sipXmediaFactoryImpl(OsConfigDb* pConfigDb)
: m_bIsAudioOutputMuted(FALSE)
, m_bIsAudioInputMuted(FALSE)
, m_fMutedAudioOutputVolume(0.0)
, m_fMutedAudioInputVolume(0.0)
{    
    UtlString strInBandDTMF;
    
    if (pConfigDb)
    {
        pConfigDb->get(CONFIG_PHONESET_SEND_INBAND_DTMF, strInBandDTMF);
        strInBandDTMF.toUpper();
    }

    // Start audio subsystem if still not started.
    if (miInstanceCount == 0)
    {
        mpStartUp(8000, 80);
    }

    // Should we send inband DTMF by default?    
    if (strInBandDTMF.compareTo("DISABLE") == 0)
    {
        MpCallFlowGraph::setInbandDTMF(false) ;
    }
    else
    {
        MpCallFlowGraph::setInbandDTMF(true) ;
    }

#ifdef INCLUDE_RTCP /* [ */
    mpiRTCPControl = CRTCManager::getRTCPControl();
#endif /* INCLUDE_RTCP ] */

    miGain = 7 ;
    ++miInstanceCount;

    // We are missing synchronization -- give the tasks time to start
    OsTask::delay(100) ;
}


// Destructor
sipXmediaFactoryImpl::~sipXmediaFactoryImpl()
{
    // TODO: Shutdown
    --miInstanceCount;
    if (miInstanceCount == 0)
    {
        mpShutdown();
    }
}

/* ============================ MANIPULATORS ============================== */

CpMediaInterface* sipXmediaFactoryImpl::createMediaInterface(OsMsgQ* pInterfaceNotificationQueue,
															 const char* publicAddress,
                                                             const char* localAddress,
                                                             int numCodecs,
                                                             SdpCodec* sdpCodecArray[],
                                                             const char* locale,
                                                             int expeditedIpTos,
                                                             const char* szStunServer,
                                                             int iStunPort,
                                                             int iStunKeepAliveSecs,
                                                             const char* szTurnServer,
                                                             int iTurnPort,
                                                             const char* szTurnUsername,
                                                             const char* szTurnPassword,
                                                             int iTurnKeepAlivePeriodSecs,
                                                             UtlBoolean bEnableICE) 
{
    return new CpPhoneMediaInterface(this, pInterfaceNotificationQueue, publicAddress, localAddress, 
            numCodecs, sdpCodecArray, locale, expeditedIpTos, szStunServer,
            iStunPort, iStunKeepAliveSecs, szTurnServer, iTurnPort, 
            szTurnUsername, szTurnPassword, iTurnKeepAlivePeriodSecs, 
            bEnableICE) ;
}

OsStatus sipXmediaFactoryImpl::getAudioInputDeviceCount(int& count) const
{
#ifndef DISABLE_LOCAL_AUDIO
   MpAudioDriverManager* pAudioManager = MpAudioDriverManager::getInstance();
   if (pAudioManager)
   {
      count = pAudioManager->getInputDeviceCount();
      return OS_SUCCESS;
   }

   return OS_FAILED;
#endif

   return OS_NOT_SUPPORTED;
}

OsStatus sipXmediaFactoryImpl::getAudioOutputDeviceCount(int& count) const
{
#ifndef DISABLE_LOCAL_AUDIO
   MpAudioDriverManager* pAudioManager = MpAudioDriverManager::getInstance();
   if (pAudioManager)
   {
      count = pAudioManager->getOutputDeviceCount();
      return OS_SUCCESS;
   }

   return OS_FAILED;
#endif

   return OS_NOT_SUPPORTED;
}

OsStatus sipXmediaFactoryImpl::getAudioInputDeviceInfo(int deviceIndex, CpAudioDeviceInfo& deviceInfo) const
{
#ifndef DISABLE_LOCAL_AUDIO
   MpAudioDriverManager* pAudioManager = MpAudioDriverManager::getInstance();
   if (pAudioManager)
   {
      MpAudioDeviceInfo mDeviceInfo;
      OsStatus res = pAudioManager->getInputDeviceInfo(deviceIndex, mDeviceInfo);

      if (res == OS_SUCCESS)
      {
         deviceInfo.m_bIsInput = TRUE;
         deviceInfo.m_defaultSampleRate = mDeviceInfo.getDefaultSampleRate();
         deviceInfo.m_deviceName = mDeviceInfo.getName();
         deviceInfo.m_driverName = mDeviceInfo.getHostApiName();
         deviceInfo.m_maxChannels = mDeviceInfo.getMaxInputChannels();
      }
      
      return res;
   }

   return OS_FAILED;
#endif

   return OS_NOT_SUPPORTED;
}

OsStatus sipXmediaFactoryImpl::getAudioOutputDeviceInfo(int deviceIndex, CpAudioDeviceInfo& deviceInfo) const
{
#ifndef DISABLE_LOCAL_AUDIO
   MpAudioDriverManager* pAudioManager = MpAudioDriverManager::getInstance();
   if (pAudioManager)
   {
      MpAudioDeviceInfo mDeviceInfo;
      OsStatus res = pAudioManager->getOutputDeviceInfo(deviceIndex, mDeviceInfo);

      if (res == OS_SUCCESS)
      {
         deviceInfo.m_bIsInput = FALSE;
         deviceInfo.m_defaultSampleRate = mDeviceInfo.getDefaultSampleRate();
         deviceInfo.m_deviceName = mDeviceInfo.getName();
         deviceInfo.m_driverName = mDeviceInfo.getHostApiName();
         deviceInfo.m_maxChannels = mDeviceInfo.getMaxOutputChannels();
      }

      return res;
   }

   return OS_FAILED;
#endif

   return OS_NOT_SUPPORTED;
}

OsStatus sipXmediaFactoryImpl::setAudioOutputDevice(const UtlString& device, const UtlString& driverName) 
{
#ifndef DISABLE_LOCAL_AUDIO
   MpAudioDriverManager* pAudioManager = MpAudioDriverManager::getInstance();
   if (pAudioManager)
   {
      return pAudioManager->setCurrentOutputDevice(device, driverName);
   }

   return OS_FAILED;
#endif

   return OS_NOT_SUPPORTED;
}

OsStatus sipXmediaFactoryImpl::setAudioInputDevice(const UtlString& device, const UtlString& driverName) 
{
#ifndef DISABLE_LOCAL_AUDIO
   MpAudioDriverManager* pAudioManager = MpAudioDriverManager::getInstance();
   if (pAudioManager)
   {
      return pAudioManager->setCurrentInputDevice(device, driverName);
   }

   return OS_FAILED;
#endif

   return OS_NOT_SUPPORTED;
}

OsStatus sipXmediaFactoryImpl::muteAudioOutput(UtlBoolean bMute)
{
#ifndef DISABLE_LOCAL_AUDIO
   MpAudioDriverManager* pAudioManager = MpAudioDriverManager::getInstance();
   if (pAudioManager)
   {
      if (bMute)
      {
         // if we want to mute
         if (!m_bIsAudioOutputMuted)
         {
            // if not muted, save volume
            m_fMutedAudioOutputVolume = pAudioManager->getPCMOutputVolume();
            m_bIsAudioOutputMuted = TRUE;
         }

         pAudioManager->setPCMOutputVolume(0.0);
         // mute after mute will succeed
         return OS_SUCCESS;
      }
      else
      {
         // if we want to unmute
         if (m_bIsAudioOutputMuted)
         {
            // if muted, restore volume
            pAudioManager->setPCMOutputVolume(m_fMutedAudioOutputVolume);
            m_bIsAudioOutputMuted = FALSE;
         }

         // unmute after unmute will succeed
         return OS_SUCCESS;
      }
   }

   return OS_FAILED;
#endif

   return OS_NOT_SUPPORTED;
}

OsStatus sipXmediaFactoryImpl::muteAudioInput(UtlBoolean bMute) 
{
#ifndef DISABLE_LOCAL_AUDIO
   MpAudioDriverManager* pAudioManager = MpAudioDriverManager::getInstance();
   if (pAudioManager)
   {
      if (bMute)
      {
         // if we want to mute
         if (!m_bIsAudioInputMuted)
         {
            // if not muted, save volume
            m_fMutedAudioInputVolume = pAudioManager->getInputVolume();
            // if input volume cannot be discovered, unmute won't work
            if (m_fMutedAudioInputVolume < 0) m_fMutedAudioInputVolume = 0.0f;
            m_bIsAudioInputMuted = TRUE;
         }

         pAudioManager->setInputVolume(0.0);
         // mute after mute will succeed
         return OS_SUCCESS;
      }
      else
      {
         // if we want to unmute
         if (m_bIsAudioInputMuted)
         {
            // if muted, restore volume
            pAudioManager->setInputVolume(m_fMutedAudioInputVolume);
            m_bIsAudioInputMuted = FALSE;
         }

         // unmute after unmute will succeed
         return OS_SUCCESS;
      }
   }

   return OS_FAILED;
#endif

   return OS_NOT_SUPPORTED;
}

OsStatus sipXmediaFactoryImpl::getAudioAECMode(MEDIA_AEC_MODE& mode) const
{
   if (MpCallFlowGraph::getAECMode((FLOWGRAPH_AEC_MODE&)mode)) {
      return OS_SUCCESS;
   } else {
      return OS_NOT_SUPPORTED; 
   }
}

OsStatus sipXmediaFactoryImpl::setAudioAECMode(const MEDIA_AEC_MODE mode)
{
  if (MpCallFlowGraph::setAECMode((FLOWGRAPH_AEC_MODE)mode)) {
    return OS_SUCCESS;
  } else {
    return OS_NOT_SUPPORTED; 
  }
}

OsStatus sipXmediaFactoryImpl::isAGCEnabled(UtlBoolean& bEnable) const {
   if (MpCallFlowGraph::getAGC(bEnable)) {
      return OS_SUCCESS;
   } else {
      return OS_NOT_SUPPORTED; 
   }
}

OsStatus sipXmediaFactoryImpl::enableAGC(UtlBoolean bEnable) {
  if (MpCallFlowGraph::setAGC(bEnable)) {
    return OS_SUCCESS;
  } else {
    return OS_NOT_SUPPORTED; 
  }
}

OsStatus sipXmediaFactoryImpl::getAudioNoiseReductionMode(MEDIA_NOISE_REDUCTION_MODE& mode) const {
   UtlBoolean bMode = FALSE;
   if (MpCallFlowGraph::getAudioNoiseReduction(bMode)) {
      if (bMode) {
         mode = MEDIA_NOISE_REDUCTION_HIGH;
      } else {
         mode = MEDIA_NOISE_REDUCTION_DISABLED;
      }
      return OS_SUCCESS;
   } else {
      return OS_NOT_SUPPORTED;
   }
}

OsStatus sipXmediaFactoryImpl::setAudioNoiseReductionMode(const MEDIA_NOISE_REDUCTION_MODE mode) {
  if (mode == MEDIA_NOISE_REDUCTION_DISABLED) {
    if (MpCallFlowGraph::setAudioNoiseReduction(FALSE)) {
      return OS_SUCCESS;
    }
  } else {
    if (MpCallFlowGraph::setAudioNoiseReduction(TRUE)) {
      return OS_SUCCESS;
    }
  }
  return OS_NOT_SUPPORTED;
}

OsStatus sipXmediaFactoryImpl::enableInboundDTMF(MEDIA_INBOUND_DTMF_MODE mode, UtlBoolean enable)
{
   switch(mode)
   {
   case MEDIA_INBOUND_DTMF_INBAND:
      MpCallFlowGraph::enableInboundInBandDTMF(enable);
      break;
   case MEDIA_INBOUND_DTMF_RFC2833:
      MpCallFlowGraph::enableInboundRFC2833DTMF(enable);
      break;
   default:
      return OS_FAILED;
   }

   return OS_SUCCESS;
}

OsStatus sipXmediaFactoryImpl::buildCodecFactory(SdpCodecFactory *pFactory, 
                                                 const UtlString& sAudioPreferences,
                                                 const UtlString& sVideoPreferences,
                                                 int videoFormat,
                                                 int* iRejected)
{
    OsStatus rc = OS_FAILED;

    UtlString codecName;
    UtlString codecList;

    SdpCodec::SdpCodecTypes codecs[TOTAL_AUDIO_CODECS_NUM+TOTAL_VIDEO_CODECS_NUM];

    *iRejected = 0;

    int numAudioCodecs = TOTAL_AUDIO_CODECS_NUM;
    SdpCodec::SdpCodecTypes *audioCodecs = codecs;
    int numVideoCodecs = TOTAL_VIDEO_CODECS_NUM;
    SdpCodec::SdpCodecTypes *videoCodecs = codecs+TOTAL_AUDIO_CODECS_NUM;

    codecs[0] = SdpCodec::SDP_CODEC_GIPS_PCMU;
    codecs[1] = SdpCodec::SDP_CODEC_GIPS_PCMA;
    codecs[2] = SdpCodec::SDP_CODEC_TONES;

#ifdef HAVE_SPEEX /* [ */
    codecs[SPEEX_AUDIO_CODECS_BEGIN+0] = SdpCodec::SDP_CODEC_SPEEX;
    codecs[SPEEX_AUDIO_CODECS_BEGIN+1] = SdpCodec::SDP_CODEC_SPEEX_5;
    codecs[SPEEX_AUDIO_CODECS_BEGIN+2] = SdpCodec::SDP_CODEC_SPEEX_15;
    codecs[SPEEX_AUDIO_CODECS_BEGIN+3] = SdpCodec::SDP_CODEC_SPEEX_24;
#endif /* HAVE_SPEEX ] */

#ifdef HAVE_GSM /* [ */
    codecs[GSM_AUDIO_CODECS_BEGIN+0] = SdpCodec::SDP_CODEC_GSM;
#endif /* HAVE_GSM ] */

#ifdef HAVE_ILBC /* [ */
    codecs[ILBC_AUDIO_CODECS_BEGIN+0] = SdpCodec::SDP_CODEC_ILBC;
#endif /* HAVE_ILBC ] */

#ifdef HAVE_INTEL_IPP /* [ */
    codecs[IPP_AUDIO_CODECS_BEGIN+0] = SdpCodec::SDP_CODEC_G729A;
    codecs[IPP_AUDIO_CODECS_BEGIN+1] = SdpCodec::SDP_CODEC_G7231;
#endif /* HAVE_INTEL_IPP ] */

    if (pFactory)
    {
        pFactory->clearCodecs();

        // add preferred audio codecs first
        if (sAudioPreferences.length() > 0)
        {
            UtlString references = sAudioPreferences;
            *iRejected = pFactory->buildSdpCodecFactory(references);
            OsSysLog::add(FAC_MP, PRI_DEBUG, 
                          "sipXmediaFactoryImpl::buildCodecFactory: sReferences = %s with NumReject %d",
                           references.data(), *iRejected);
                           
            // Now pick preferences out of all available codecs
            SdpCodec** codecsArray = NULL;
            pFactory->getCodecs(numAudioCodecs, codecsArray);
            
            UtlString preferences;
            int i;
            for (i = 0; i < numAudioCodecs; i++)
            {
                if (getCodecNameByType(codecsArray[i]->getCodecType(), codecName) == OS_SUCCESS)
                {
                    preferences = preferences + " " + codecName;
                }
            }
            
            pFactory->clearCodecs();
            *iRejected = pFactory->buildSdpCodecFactory(preferences);
            OsSysLog::add(FAC_MP, PRI_DEBUG, 
                          "sipXmediaFactoryImpl::buildCodecFactory: supported codecs = %s with NumReject %d",
                          preferences.data(), *iRejected);
                          
            // Free up the codecs and the array
            for (i = 0; i < numAudioCodecs; i++)
            {
                delete codecsArray[i];
                codecsArray[i] = NULL;
            }
            delete[] codecsArray;
            codecsArray = NULL;
                          
            rc = OS_SUCCESS;
        }
        else
        {
            // Build up the supported codecs
            *iRejected = pFactory->buildSdpCodecFactory(numAudioCodecs, audioCodecs);
            rc = OS_SUCCESS;
        }


        // add preferred video codecs first
        if (sVideoPreferences.length() > 0)
        {
            UtlString references = sVideoPreferences;
            *iRejected = pFactory->buildSdpCodecFactory(references);
            OsSysLog::add(FAC_MP, PRI_DEBUG, 
                          "sipXmediaFactoryImpl::buildCodecFactory: sReferences = %s with NumReject %d",
                           references.data(), *iRejected);
                           
            // Now pick preferences out of all available codecs
            SdpCodec** codecsArray = NULL;
            pFactory->getCodecs(numVideoCodecs, codecsArray);
            
            UtlString preferences;
            int i;
            for (i = 0; i < numVideoCodecs; i++)
            {
                if (getCodecNameByType(codecsArray[i]->getCodecType(), codecName) == OS_SUCCESS)
                {
                    preferences = preferences + " " + codecName;
                }
            }
            
            pFactory->clearCodecs();
            *iRejected = pFactory->buildSdpCodecFactory(preferences);
            OsSysLog::add(FAC_MP, PRI_DEBUG, 
                          "sipXmediaFactoryImpl::buildCodecFactory: supported codecs = %s with NumReject %d",
                          preferences.data(), *iRejected);
                          
            // Free up the codecs and the array
            for (i = 0; i < numVideoCodecs; i++)
            {
                delete codecsArray[i];
                codecsArray[i] = NULL;
            }
            delete[] codecsArray;
            codecsArray = NULL;
                          
            rc = OS_SUCCESS;
        }
        else
        {
            // Build up the supported codecs
            *iRejected = pFactory->buildSdpCodecFactory(numVideoCodecs, videoCodecs);
            rc = OS_SUCCESS;
        }

    }            

    return rc;
}


OsStatus sipXmediaFactoryImpl::updateVideoPreviewWindow(void* displayContext) 
{
    return OS_NOT_SUPPORTED ;
}


/* ============================ ACCESSORS ================================= */

OsStatus sipXmediaFactoryImpl::getCurrentAudioOutputDevice(CpAudioDeviceInfo& deviceInfo) const
{
#ifndef DISABLE_LOCAL_AUDIO
   MpAudioDriverManager* pAudioManager = MpAudioDriverManager::getInstance();
   if (pAudioManager)
   {
      MpAudioDeviceInfo mDeviceInfo;
      OsStatus res = pAudioManager->getCurrentOutputDevice(mDeviceInfo);
      if (res == OS_SUCCESS)
      {
         deviceInfo.m_bIsInput = FALSE;
         deviceInfo.m_defaultSampleRate = mDeviceInfo.getDefaultSampleRate();
         deviceInfo.m_deviceName = mDeviceInfo.getName();
         deviceInfo.m_driverName = mDeviceInfo.getHostApiName();
         deviceInfo.m_maxChannels = mDeviceInfo.getMaxOutputChannels();
      }
      
      return res;
   }

   return OS_FAILED;
#endif

   return OS_NOT_SUPPORTED;
}


OsStatus sipXmediaFactoryImpl::getCurrentAudioInputDevice(CpAudioDeviceInfo& deviceInfo) const
{
#ifndef DISABLE_LOCAL_AUDIO
   MpAudioDriverManager* pAudioManager = MpAudioDriverManager::getInstance();
   if (pAudioManager)
   {
      MpAudioDeviceInfo mDeviceInfo;
      OsStatus res = pAudioManager->getCurrentInputDevice(mDeviceInfo);
      if (res == OS_SUCCESS)
      {
         deviceInfo.m_bIsInput = TRUE;
         deviceInfo.m_defaultSampleRate = mDeviceInfo.getDefaultSampleRate();
         deviceInfo.m_deviceName = mDeviceInfo.getName();
         deviceInfo.m_driverName = mDeviceInfo.getHostApiName();
         deviceInfo.m_maxChannels = mDeviceInfo.getMaxInputChannels();
      }

      return res;
   }

   return OS_FAILED;
#endif

   return OS_NOT_SUPPORTED;
}


OsStatus sipXmediaFactoryImpl::getNumOfCodecs(int& iCodecs) const
{
    iCodecs = TOTAL_AUDIO_CODECS_NUM;
    return OS_SUCCESS;
}


OsStatus sipXmediaFactoryImpl::getCodec(int iCodec, UtlString& codec, int &bandWidth) const
{
    OsStatus rc = OS_SUCCESS;

    switch (iCodec)
    {
    case 0: codec = (const char*) SdpCodec::SDP_CODEC_GIPS_PCMU;
        break;
    case 1: codec = (const char*) SdpCodec::SDP_CODEC_GIPS_PCMA;
        break;
    case 2: codec = (const char*) SdpCodec::SDP_CODEC_TONES;
        break;

#ifdef HAVE_SPEEX /* [ */
    case SPEEX_AUDIO_CODECS_BEGIN+0: codec = (const char*) SdpCodec::SDP_CODEC_SPEEX;
        break;
    case SPEEX_AUDIO_CODECS_BEGIN+1: codec = (const char*) SdpCodec::SDP_CODEC_SPEEX_5;
        break;
    case SPEEX_AUDIO_CODECS_BEGIN+2: codec = (const char*) SdpCodec::SDP_CODEC_SPEEX_15;
        break;
    case SPEEX_AUDIO_CODECS_BEGIN+3: codec = (const char*) SdpCodec::SDP_CODEC_SPEEX_24;
        break;
#endif /* HAVE_SPEEX ] */

#ifdef HAVE_GSM /* [ */
    case GSM_AUDIO_CODECS_BEGIN+0: codec = (const char*) SdpCodec::SDP_CODEC_GSM;
        break;
#endif /* HAVE_GSM ] */

#ifdef HAVE_ILBC /* [ */
    case ILBC_AUDIO_CODECS_BEGIN+0: codec = (const char*) SdpCodec::SDP_CODEC_ILBC;
        break;
#endif /* HAVE_ILBC ] */

#ifdef HAVE_INTEL_IPP /* [ */
    case IPP_AUDIO_CODECS_BEGIN+0: codec = (const char*) SdpCodec::SDP_CODEC_G729A;
       break;
    case IPP_AUDIO_CODECS_BEGIN+1: codec = (const char*) SdpCodec::SDP_CODEC_G7231;
       break;
#endif /* HAVE_INTEL_IPP ] */

    default: rc = OS_FAILED;
    }

    return rc;
}

OsStatus sipXmediaFactoryImpl::setVideoPreviewDisplay(void* pDisplay)
{
    return OS_NOT_YET_IMPLEMENTED;
}

OsStatus sipXmediaFactoryImpl::setVideoQuality(int quality)
{
    return OS_NOT_YET_IMPLEMENTED;
}

OsStatus sipXmediaFactoryImpl::setVideoParameters(int bitRate, int frameRate)
{
    return OS_NOT_YET_IMPLEMENTED;
}

OsStatus sipXmediaFactoryImpl::getVideoQuality(int& quality) const
{
    return OS_NOT_YET_IMPLEMENTED;
}

OsStatus sipXmediaFactoryImpl::getVideoBitRate(int& bitRate) const
{
    return OS_NOT_YET_IMPLEMENTED;
}

OsStatus sipXmediaFactoryImpl::getVideoFrameRate(int& frameRate) const
{
    return OS_NOT_YET_IMPLEMENTED;
}

OsStatus sipXmediaFactoryImpl::getCodecNameByType(SdpCodec::SdpCodecTypes type, UtlString& codecName) const
{
    OsStatus rc = OS_FAILED;

    codecName = "";

    switch (type)
    {
    case SdpCodec::SDP_CODEC_TONES:
        codecName = SIPX_CODEC_ID_TELEPHONE;
        break;
    case SdpCodec::SDP_CODEC_G729:
    case SdpCodec::SDP_CODEC_G729A:
        codecName = SIPX_CODEC_ID_G729A;
        break;
    case SdpCodec::SDP_CODEC_G7231:
        codecName = SIPX_CODEC_ID_G7231;
        break;
    case SdpCodec::SDP_CODEC_GIPS_PCMA:
        codecName = SIPX_CODEC_ID_PCMA;
        break;
    case SdpCodec::SDP_CODEC_GIPS_PCMU:
        codecName = SIPX_CODEC_ID_PCMU;
        break;
    case SdpCodec::SDP_CODEC_GIPS_IPCMA:
        codecName = SIPX_CODEC_ID_EG711A;
        break;
    case SdpCodec::SDP_CODEC_GIPS_IPCMU:
        codecName = SIPX_CODEC_ID_EG711U;
        break;
    case SdpCodec::SDP_CODEC_GIPS_IPCMWB:
        codecName = SIPX_CODEC_ID_IPCMWB;
        break;
    case SdpCodec::SDP_CODEC_ILBC:
        codecName = SIPX_CODEC_ID_ILBC;
        break;
    case SdpCodec::SDP_CODEC_GIPS_ISAC:
        codecName = SIPX_CODEC_ID_ISAC;
        break;
    case SdpCodec::SDP_CODEC_SPEEX:
        codecName = SIPX_CODEC_ID_SPEEX;
        break;
    case SdpCodec::SDP_CODEC_SPEEX_5:
        codecName = SIPX_CODEC_ID_SPEEX_5;
        break;
    case SdpCodec::SDP_CODEC_SPEEX_15:
        codecName = SIPX_CODEC_ID_SPEEX_15;
        break;
    case SdpCodec::SDP_CODEC_SPEEX_24:
        codecName = SIPX_CODEC_ID_SPEEX_24;
        break;
    case SdpCodec::SDP_CODEC_GSM:
        codecName = SIPX_CODEC_ID_GSM;
        break;
    default:
        OsSysLog::add(FAC_MP, PRI_WARNING,
                      "sipXmediaFactoryImpl::getCodecNameByType unsupported type %d.",
                      type);
    
    }

    if (codecName != "")
    {
        rc = OS_SUCCESS;
    }

    return rc;
}

OsStatus sipXmediaFactoryImpl::getLocalAudioConnectionId(int& connectionId) const 
{
    connectionId = -1 ;

    return OS_NOT_SUPPORTED ;

}

OsStatus sipXmediaFactoryImpl::getAudioInputMixerName(UtlString& name) const
{
#ifndef DISABLE_LOCAL_AUDIO
   MpAudioDriverManager* pAudioManager = MpAudioDriverManager::getInstance();
   if (pAudioManager)
   {
      pAudioManager->getInputMixerName(name);
      return OS_SUCCESS;
   }

   return OS_FAILED;
#endif

   return OS_NOT_SUPPORTED;
}

OsStatus sipXmediaFactoryImpl::getAudioOutputMixerName(UtlString& name) const
{
#ifndef DISABLE_LOCAL_AUDIO
   MpAudioDriverManager* pAudioManager = MpAudioDriverManager::getInstance();
   if (pAudioManager)
   {
      pAudioManager->getOutputMixerName(name);
      return OS_SUCCESS;
   }

   return OS_FAILED;
#endif

   return OS_NOT_SUPPORTED;
}

OsStatus sipXmediaFactoryImpl::getAudioMasterVolume(int& volume) const
{
#ifndef DISABLE_LOCAL_AUDIO
   MpAudioDriverManager* pAudioManager = MpAudioDriverManager::getInstance();
   if (pAudioManager)
   {
      float fVolume = pAudioManager->getMasterVolume();
      volume = (int)(fVolume * 100 + 0.5f);
      return OS_SUCCESS;
   }

   return OS_FAILED;
#endif

   return OS_NOT_SUPPORTED;
}

OsStatus sipXmediaFactoryImpl::setAudioMasterVolume(int volume)
{
#ifndef DISABLE_LOCAL_AUDIO
   MpAudioDriverManager* pAudioManager = MpAudioDriverManager::getInstance();
   if (pAudioManager)
   {
      pAudioManager->setMasterVolume((float)volume / 100);
      return OS_SUCCESS;
   }

   return OS_FAILED;
#endif

   return OS_NOT_SUPPORTED;
}

OsStatus sipXmediaFactoryImpl::getAudioPCMOutputVolume(int& volume) const
{
#ifndef DISABLE_LOCAL_AUDIO
   MpAudioDriverManager* pAudioManager = MpAudioDriverManager::getInstance();
   if (pAudioManager)
   {
      float fVolume = pAudioManager->getPCMOutputVolume();
      volume = (int)(fVolume * 100 + 0.5f);
      return OS_SUCCESS;
   }

   return OS_FAILED;
#endif

   return OS_NOT_SUPPORTED;
}

OsStatus sipXmediaFactoryImpl::setAudioPCMOutputVolume(int volume)
{
#ifndef DISABLE_LOCAL_AUDIO
   MpAudioDriverManager* pAudioManager = MpAudioDriverManager::getInstance();
   if (pAudioManager)
   {
      if (!m_bIsAudioOutputMuted)
      {
         // output is not muted, set volume
         pAudioManager->setPCMOutputVolume((float)volume / 100);
      }
      else
      {
         // output is muted, just update internal variable
         m_fMutedAudioOutputVolume = (float)volume / 100;
      }
      
      return OS_SUCCESS;
   }

   return OS_FAILED;
#endif

   return OS_NOT_SUPPORTED;
}

OsStatus sipXmediaFactoryImpl::getAudioInputVolume(int& volume) const
{
#ifndef DISABLE_LOCAL_AUDIO
   MpAudioDriverManager* pAudioManager = MpAudioDriverManager::getInstance();
   if (pAudioManager)
   {
      float fVolume = pAudioManager->getInputVolume();
      if (fVolume >= 0.0f) {
         volume = (int)(fVolume * 100 + 0.5f);
         return OS_SUCCESS;
      }
   }

   return OS_FAILED;
#endif

   return OS_NOT_SUPPORTED;
}

OsStatus sipXmediaFactoryImpl::setAudioInputVolume(int volume)
{
#ifndef DISABLE_LOCAL_AUDIO
   MpAudioDriverManager* pAudioManager = MpAudioDriverManager::getInstance();
   if (pAudioManager)
   {
      if (!m_bIsAudioInputMuted)
      {
         // input is not muted, set volume
         pAudioManager->setInputVolume((float)volume / 100);
      }
      else
      {
         // input is muted, just update internal variable
         m_fMutedAudioInputVolume = (float)volume / 100;
      }
      
      return OS_SUCCESS;
   }

   return OS_FAILED;
#endif

   return OS_NOT_SUPPORTED;
}

OsStatus sipXmediaFactoryImpl::getAudioOutputBalance(int& balance) const
{
#ifndef DISABLE_LOCAL_AUDIO
   MpAudioDriverManager* pAudioManager = MpAudioDriverManager::getInstance();
   if (pAudioManager)
   {
      float fBalance = pAudioManager->getOutputBalance();
      balance = (int)(fBalance * 100);
      return OS_SUCCESS;
   }

   return OS_FAILED;
#endif

   return OS_NOT_SUPPORTED;
}

OsStatus sipXmediaFactoryImpl::setAudioOutputBalance(int balance)
{
#ifndef DISABLE_LOCAL_AUDIO
   MpAudioDriverManager* pAudioManager = MpAudioDriverManager::getInstance();
   if (pAudioManager)
   {
      pAudioManager->setOutputBalance((float)balance / 100);
      return OS_SUCCESS;
   }

   return OS_FAILED;
#endif

   return OS_NOT_SUPPORTED;
}

OsStatus sipXmediaFactoryImpl::getAudioOutputVolumeMeterReading(MEDIA_VOLUME_METER_TYPE type,
                                                                double& volume) const
{
#ifndef DISABLE_LOCAL_AUDIO
   MpAudioDriverManager* pAudioManager = MpAudioDriverManager::getInstance();
   if (pAudioManager)
   {
      return pAudioManager->getOutputVolumeMeterReading((MP_VOLUME_METER_TYPE)type, volume);
   }

   return OS_FAILED;
#endif

   return OS_NOT_SUPPORTED;
}

OsStatus sipXmediaFactoryImpl::getAudioInputVolumeMeterReading(MEDIA_VOLUME_METER_TYPE type,
                                                               double& volume) const
{
#ifndef DISABLE_LOCAL_AUDIO
   MpAudioDriverManager* pAudioManager = MpAudioDriverManager::getInstance();
   if (pAudioManager)
   {
      return pAudioManager->getInputVolumeMeterReading((MP_VOLUME_METER_TYPE)type, volume);
   }

   return OS_FAILED;
#endif

   return OS_NOT_SUPPORTED;
}

/* ============================ INQUIRY =================================== */

OsStatus sipXmediaFactoryImpl::isInboundDTMFEnabled(MEDIA_INBOUND_DTMF_MODE mode, UtlBoolean& enabled)
{
   switch(mode)
   {
   case MEDIA_INBOUND_DTMF_INBAND:
      enabled = MpCallFlowGraph::isInboundInBandDTMFEnabled();
      break;
   case MEDIA_INBOUND_DTMF_RFC2833:
      enabled = MpCallFlowGraph::isInboundRFC2833DTMFEnabled();
      break;
   default:
      return OS_FAILED;
   }

   return OS_SUCCESS;  
}

OsStatus sipXmediaFactoryImpl::isAudioOutputMuted(UtlBoolean& bIsMuted) const
{
   bIsMuted = m_bIsAudioOutputMuted;
   return OS_SUCCESS;
}

OsStatus sipXmediaFactoryImpl::isAudioInputMuted(UtlBoolean& bIsMuted) const
{
   bIsMuted = m_bIsAudioInputMuted;
   return OS_SUCCESS;
}


/* //////////////////////////// PROTECTED ///////////////////////////////// */

/* //////////////////////////// PRIVATE /////////////////////////////////// */

/* ============================ FUNCTIONS ================================= */


