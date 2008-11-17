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
#include "sdp/SdpCodecFactory.h"
#include "mi/CpMediaInterfaceFactoryFactory.h"
#include "mp/MpAudioDriverDefs.h"
#include "mp/MpAudioDriverManager.h"
#include "mi/CpAudioDeviceInfo.h"

#ifdef INCLUDE_RTCP /* [ */
#include "rtcp/RTCManager.h"
#endif /* INCLUDE_RTCP ] */

// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
// GLOBAL FUNCTION
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
    // Start audio subsystem if still not started.
    if (miInstanceCount == 0)
    {
        mpStartUp(8000, 80);
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
                                                             const UtlSList* pCodecList,
															                const char* publicAddress,
                                                             const char* localAddress,
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
    return new CpPhoneMediaInterface(this, pInterfaceNotificationQueue, pCodecList, publicAddress, localAddress, 
            locale, expeditedIpTos, szStunServer,
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
            // if output volume cannot be discovered, unmute won't work
            if (m_fMutedAudioOutputVolume < 0) m_fMutedAudioOutputVolume = 0.0f;
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

UtlString sipXmediaFactoryImpl::getAllSupportedAudioCodecs() const
{
   UtlString supportedCodecs = 
#ifdef HAVE_SPEEX // [
      "SPEEX SPEEX_5 SPEEX_15 SPEEX_24 "
#endif // HAVE_SPEEX ]
#ifdef HAVE_GSM // [
      "GSM "
#endif // HAVE_GSM ]
#ifdef HAVE_ILBC // [
      "ILBC "
#endif // HAVE_ILBC ]
#ifdef HAVE_INTEL_IPP // [
      "G729A G723.1 "
#endif // HAVE_INTEL_IPP ]
      "PCMU PCMA TELEPHONE-EVENT";
   return supportedCodecs;
}

UtlString sipXmediaFactoryImpl::getAllSupportedVideoCodecs() const
{
   return "";
}

OsStatus sipXmediaFactoryImpl::buildAllCodecFactory(SdpCodecFactory& codecFactory)
{
   codecFactory.clearCodecs();
   codecFactory.buildSdpCodecFactory(getAllSupportedAudioCodecs());
   codecFactory.buildSdpCodecFactory(getAllSupportedVideoCodecs());
   codecFactory.bindPayloadIds();
   return OS_SUCCESS;
}

OsStatus sipXmediaFactoryImpl::buildCodecFactory(SdpCodecFactory& codecFactory, 
                                                 const UtlString& sAudioPreferences,
                                                 const UtlString& sVideoPreferences)
{
   OsStatus rc = OS_SUCCESS;
   codecFactory.clearCodecs();

   // add preferred audio codecs first
   if (sAudioPreferences.length() > 0)
   {
      UtlString audioCodecs(SdpCodecFactory::getFixedAudioCodecs(sAudioPreferences));
      codecFactory.buildSdpCodecFactory(audioCodecs);
   }
   else
   {
      // Build up all supported audio codecs
      codecFactory.buildSdpCodecFactory(getAllSupportedAudioCodecs());
   }

   // add preferred video codecs first
   if (sVideoPreferences.length() > 0)
   {
      codecFactory.buildSdpCodecFactory(sVideoPreferences);
   }
   else
   {
      // Build up all supported video codecs
      codecFactory.buildSdpCodecFactory(getAllSupportedVideoCodecs());
   }

   codecFactory.bindPayloadIds();

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
      if (fVolume >= 0.0f) {
         volume = (int)(fVolume * 100 + 0.5f);
         return OS_SUCCESS;
      }
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
      if (fVolume >= 0.0f) {
         volume = (int)(fVolume * 100 + 0.5f);
         return OS_SUCCESS;
      }
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


