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

#ifndef _SipXMediaFactoryImpl_h_
#define _SipXMediaFactoryImpl_h_

// SYSTEM INCLUDES
// APPLICATION INCLUDES
#include "mi/CpMediaInterfaceFactory.h"
#include <rtcp/RtcpConfig.h>

// DEFINES
#define SIPX_CODEC_ID_IPCMWB    "IPCMWB"
#define SIPX_CODEC_ID_ISAC      "ISAC"
#define SIPX_CODEC_ID_EG711U    "EG711U"
#define SIPX_CODEC_ID_EG711A    "EG711A"
#define SIPX_CODEC_ID_PCMA      "PCMA"
#define SIPX_CODEC_ID_PCMU      "PCMU"
#define SIPX_CODEC_ID_ILBC      "iLBC"
#define SIPX_CODEC_ID_G729A     "G729A"
#define SIPX_CODEC_ID_G723      "G723"
#define SIPX_CODEC_ID_TELEPHONE "audio/telephone-event"
#define SIPX_CODEC_ID_SPEEX     "SPEEX"
#define SIPX_CODEC_ID_SPEEX_5   "SPEEX_5"
#define SIPX_CODEC_ID_SPEEX_15  "SPEEX_15"
#define SIPX_CODEC_ID_SPEEX_24  "SPEEX_24"
#define SIPX_CODEC_ID_GSM       "GSM"

// MACROS
// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
// STRUCTS
// TYPEDEFS
// FORWARD DECLARATIONS
class MpMediaTask ;
class OsConfigDb ; 
#ifdef INCLUDE_RTCP /* [ */
struct IRTCPControl ;
#endif /* INCLUDE_RTCP ] */


/**
 *
 */
class SipXMediaFactoryImpl : public CpMediaInterfaceFactory
{
/* //////////////////////////// PUBLIC //////////////////////////////////// */
  public:

/* ============================ CREATORS ================================== */

   /**
    * Default constructor
    */
   SipXMediaFactoryImpl(OsConfigDb* pConfigDb);
     

   /**
    * Destructor
    */
   virtual ~SipXMediaFactoryImpl();

/* ============================ MANIPULATORS ============================== */
    virtual CpMediaInterface* createMediaInterface(OsMsgQ* pInterfaceNotificationQueue,
                                                   const SdpCodecList* pCodecList,
													            const char* publicIPAddress,
                                                    const char* localIPAddress,
                                                    const char* locale,
                                                    int expeditedIpTos,
                                                    const char* szStunServer,
                                                    int stunOptions,
                                                    int iStunKeepAliveSecs,
                                                    const char* szTurnServer,
                                                    int iTurnPort,
                                                    const char* szTurnUsername,
                                                    const char* szTurnPassword,
                                                    int iTurnKeepAlivePeriodSecs,
                                                    UtlBoolean bEnableICE) ;

    virtual OsStatus getAudioInputDeviceCount(int& count) const;
    virtual OsStatus getAudioOutputDeviceCount(int& count) const;
    virtual OsStatus getAudioInputDeviceInfo(int deviceIndex, CpAudioDeviceInfo& deviceInfo) const;
    virtual OsStatus getAudioOutputDeviceInfo(int deviceIndex, CpAudioDeviceInfo& deviceInfo) const;

    virtual OsStatus setAudioOutputDevice(const UtlString& device, const UtlString& driverName = "");
    virtual OsStatus setAudioInputDevice(const UtlString& device, const UtlString& driverName = "");

    virtual OsStatus muteAudioOutput(UtlBoolean bMute);
    virtual OsStatus muteAudioInput(UtlBoolean bMute);

    virtual OsStatus getAudioAECMode(MEDIA_AEC_MODE& mode) const;
    virtual OsStatus setAudioAECMode(const MEDIA_AEC_MODE mode);
    virtual OsStatus isAGCEnabled(UtlBoolean& bEnable) const;
    virtual OsStatus enableAGC(UtlBoolean bEnable) ;
    virtual OsStatus getAudioNoiseReductionMode(MEDIA_NOISE_REDUCTION_MODE& mode) const;
    virtual OsStatus setAudioNoiseReductionMode(const MEDIA_NOISE_REDUCTION_MODE mode) ;

    virtual OsStatus enableInboundDTMF(MEDIA_INBOUND_DTMF_MODE mode, UtlBoolean enable);

    virtual OsStatus buildCodecList(SdpCodecList& codecFactory, 
                                    const UtlString& sPreferences,
                                    const UtlString& sVideoPreferences);

    virtual OsStatus buildAllCodecList(SdpCodecList& codecList);

    virtual UtlString getAllSupportedAudioCodecs() const;

    virtual UtlString getAllSupportedVideoCodecs() const;

    virtual OsStatus updateVideoPreviewWindow(void* displayContext) ;

    /**
     * Set the global video preview window 
     */ 
    virtual OsStatus setVideoPreviewDisplay(void* pDisplay);


    virtual OsStatus setVideoQuality(int quality);
    virtual OsStatus setVideoParameters(int bitRate, int frameRate);
    

/* ============================ ACCESSORS ================================= */

    virtual OsStatus getCurrentAudioOutputDevice(CpAudioDeviceInfo& deviceInfo) const;
    virtual OsStatus getCurrentAudioInputDevice(CpAudioDeviceInfo& deviceInfo) const;
    virtual OsStatus getLocalAudioConnectionId(int& connectionId) const ;

    virtual OsStatus getAudioInputMixerName(UtlString& name) const;
    virtual OsStatus getAudioOutputMixerName(UtlString& name) const;
    virtual OsStatus getAudioMasterVolume(int& volume) const;
    virtual OsStatus setAudioMasterVolume(int volume);
    virtual OsStatus getAudioPCMOutputVolume(int& volume) const;
    virtual OsStatus setAudioPCMOutputVolume(int volume);
    virtual OsStatus getAudioInputVolume(int& volume) const;
    virtual OsStatus setAudioInputVolume(int volume);
    virtual OsStatus getAudioOutputBalance(int& balance) const;
    virtual OsStatus setAudioOutputBalance(int balance);
    virtual OsStatus getAudioOutputVolumeMeterReading(MEDIA_VOLUME_METER_TYPE type,
                                                      double& volume) const;
    virtual OsStatus getAudioInputVolumeMeterReading(MEDIA_VOLUME_METER_TYPE type,
                                                     double& volume) const;

    virtual OsStatus getVideoQuality(int& quality) const;
    virtual OsStatus getVideoBitRate(int& bitRate) const;
    virtual OsStatus getVideoFrameRate(int& frameRate) const;

/* ============================ INQUIRY =================================== */

    virtual OsStatus isAudioOutputMuted(UtlBoolean& bIsMuted) const;
    virtual OsStatus isAudioInputMuted(UtlBoolean& bIsMuted) const;

    virtual OsStatus isInboundDTMFEnabled(MEDIA_INBOUND_DTMF_MODE mode, UtlBoolean& enabled);

/* //////////////////////////// PROTECTED ///////////////////////////////// */
  protected:
#ifdef INCLUDE_RTCP /* [ */
    IRTCPControl*   mpiRTCPControl;   /**< Realtime Control Interface */
#endif /* INCLUDE_RTCP ] */


/* //////////////////////////// PRIVATE /////////////////////////////////// */
  private:
    static int miInstanceCount;

    UtlBoolean m_bIsAudioOutputMuted;
    UtlBoolean m_bIsAudioInputMuted;
    float m_fMutedAudioOutputVolume;
    float m_fMutedAudioInputVolume;
};

/* ============================ INLINE METHODS ============================ */

#endif  // _SipXMediaFactoryImpl_h_
