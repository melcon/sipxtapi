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
#include "mp/dmaTask.h"
#include "net/SdpCodecFactory.h"
#include "mi/CpMediaInterfaceFactoryFactory.h"
#include "mp/MpAudioDriverManager.h"
#include "mp/MpAudioDriverBase.h"
#include "mp/MpAudioDeviceInfo.h"

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


OsStatus sipXmediaFactoryImpl::setSpeakerVolume(int iVolume) 
{
   return OS_NOT_SUPPORTED;
}

OsStatus sipXmediaFactoryImpl::setSpeakerDevice(const UtlString& device, const UtlString& driverName) 
{
   return OS_NOT_SUPPORTED;
}

OsStatus sipXmediaFactoryImpl::setMicrophoneGain(int iGain) 
{
    return OS_NOT_SUPPORTED;
}

OsStatus sipXmediaFactoryImpl::setMicrophoneDevice(const UtlString& device, const UtlString& driverName) 
{
   return OS_NOT_SUPPORTED;
}

OsStatus sipXmediaFactoryImpl::muteMicrophone(UtlBoolean bMute) 
{
    return OS_NOT_SUPPORTED;
}

OsStatus sipXmediaFactoryImpl::setAudioAECMode(const MEDIA_AEC_MODE mode)
{
  if (MpCallFlowGraph::setAECMode((FLOWGRAPH_AEC_MODE)mode)) {
    return OS_SUCCESS;
  }else {
    return OS_NOT_SUPPORTED; 
  }
}

OsStatus sipXmediaFactoryImpl::enableAGC(UtlBoolean bEnable) {
  if (MpCallFlowGraph::setAGC(bEnable)) {
    return OS_SUCCESS;
  }else {
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

OsStatus sipXmediaFactoryImpl::getSpeakerVolume(int& iVolume) const
{
   return OS_NOT_SUPPORTED;
}

OsStatus sipXmediaFactoryImpl::getSpeakerDevice(UtlString& device, UtlString& driverName) const
{
#ifndef DISABLE_LOCAL_AUDIO
   MpAudioDriverManager* pAudioManager = MpAudioDriverManager::getInstance(FALSE);
   if (pAudioManager)
   {
      MpAudioDeviceIndex deviceIndex = pAudioManager->getOutputDeviceIndex();
      MpAudioStreamId streamId = pAudioManager->getOutputAudioStream();
      MpAudioDriverBase* pAudioDriver = pAudioManager->getAudioDriver();
      if (pAudioDriver)
      {
         OsStatus res = OS_FAILED;
         if (streamId)
         {
            // stream exists, then audio device is selected
            MpAudioDeviceInfo deviceInfo;
            res = pAudioDriver->getDeviceInfo(deviceIndex, deviceInfo);
            if (res == OS_SUCCESS)
            {
               device = deviceInfo.getName();
               MpHostAudioApiInfo apiInfo;
               MpHostAudioApiIndex apiIndex = deviceInfo.getHostApi();
               res = pAudioDriver->getHostApiInfo(apiIndex, apiInfo);
               if (res == OS_SUCCESS)
               {
                  driverName = apiInfo.getName();
               }
            }         
         }
         else
         {
            // stream doesn't exist, audio device is disabled
            device = "NONE";
            driverName.remove(0);
            res = OS_SUCCESS;
         }

         return res;
      }
   }

   return OS_FAILED;
#endif

   return OS_NOT_SUPPORTED;
}


OsStatus sipXmediaFactoryImpl::getMicrophoneGain(int& iGain) const
{
   return OS_NOT_SUPPORTED;
}


OsStatus sipXmediaFactoryImpl::getMicrophoneDevice(UtlString& device, UtlString& driverName) const
{
#ifndef DISABLE_LOCAL_AUDIO
   MpAudioDriverManager* pAudioManager = MpAudioDriverManager::getInstance(FALSE);
   if (pAudioManager)
   {
      MpAudioDeviceIndex deviceIndex = pAudioManager->getInputDeviceIndex();
      MpAudioStreamId streamId = pAudioManager->getInputAudioStream();
      MpAudioDriverBase* pAudioDriver = pAudioManager->getAudioDriver();
      if (pAudioDriver)
      {
         OsStatus res = OS_FAILED;
         if (streamId)
         {
            // stream exists, then audio device is selected
            MpAudioDeviceInfo deviceInfo;
            res = pAudioDriver->getDeviceInfo(deviceIndex, deviceInfo);
            if (res == OS_SUCCESS)
            {
               device = deviceInfo.getName();
               MpHostAudioApiInfo apiInfo;
               MpHostAudioApiIndex apiIndex = deviceInfo.getHostApi();
               res = pAudioDriver->getHostApiInfo(apiIndex, apiInfo);
               if (res == OS_SUCCESS)
               {
                  driverName = apiInfo.getName();
               }
            }         
         }
         else
         {
            // stream doesn't exist, audio device is disabled
            device = "NONE";
            driverName.remove(0);
            res = OS_SUCCESS;
         }

         return res;
      }
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

/* //////////////////////////// PROTECTED ///////////////////////////////// */

/* //////////////////////////// PRIVATE /////////////////////////////////// */

/* ============================ FUNCTIONS ================================= */


