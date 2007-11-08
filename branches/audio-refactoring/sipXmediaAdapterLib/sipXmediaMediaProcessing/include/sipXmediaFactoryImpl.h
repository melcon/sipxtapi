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

#ifndef _sipXmediaFactoryImpl_h_
#define _sipXmediaFactoryImpl_h_

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
#define SIPX_CODEC_ID_G729      "G729"
#define SIPX_CODEC_ID_G729A     "G729A"
#define SIPX_CODEC_ID_G723      "G723"
#define SIPX_CODEC_ID_G7231     "G723.1"
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
class sipXmediaFactoryImpl : public CpMediaInterfaceFactory
{
/* //////////////////////////// PUBLIC //////////////////////////////////// */
  public:

/* ============================ CREATORS ================================== */

   /**
    * Default constructor
    */
   sipXmediaFactoryImpl(OsConfigDb* pConfigDb);
     

   /**
    * Destructor
    */
   virtual ~sipXmediaFactoryImpl();

/* ============================ MANIPULATORS ============================== */
    virtual CpMediaInterface* createMediaInterface(OsMsgQ* pInterfaceNotificationQueue,
													const char* publicAddress,
                                                    const char* localAddress,
                                                    int numCodecs,
                                                    SdpCodec* sdpCodecArray[],
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

    virtual OsStatus setSpeakerDevice(const UtlString& device, const UtlString& driverName = "");
    virtual OsStatus setMicrophoneDevice(const UtlString& device, const UtlString& driverName = "");

    virtual OsStatus muteSpeaker(UtlBoolean bMute);
    virtual OsStatus muteMic(UtlBoolean bMute);

    virtual OsStatus setAudioAECMode(const MEDIA_AEC_MODE mode);
    virtual OsStatus enableAGC(UtlBoolean bEnable) ;
    virtual OsStatus setAudioNoiseReductionMode(const MEDIA_NOISE_REDUCTION_MODE mode) ;

    virtual OsStatus enableInboundDTMF(MEDIA_INBOUND_DTMF_MODE mode, UtlBoolean enable);

    virtual OsStatus buildCodecFactory(SdpCodecFactory *pFactory, 
                                       const UtlString& sPreferences,
                                       const UtlString& sVideoPreferences,
                                       int videoFormat,
                                       int* iRejected);

    virtual OsStatus updateVideoPreviewWindow(void* displayContext) ;

    /**
     * Set the global video preview window 
     */ 
    virtual OsStatus setVideoPreviewDisplay(void* pDisplay);


    virtual OsStatus setVideoQuality(int quality);
    virtual OsStatus setVideoParameters(int bitRate, int frameRate);
    

/* ============================ ACCESSORS ================================= */

    virtual OsStatus getSpeakerDevice(CpAudioDeviceInfo& deviceInfo) const;
    virtual OsStatus getMicrophoneDevice(CpAudioDeviceInfo& deviceInfo) const;

    virtual OsStatus getNumOfCodecs(int& iCodecs) const;
    virtual OsStatus getCodec(int iCodec, UtlString& codec, int& bandWidth) const;

    virtual OsStatus getCodecNameByType(SdpCodec::SdpCodecTypes codecType, UtlString& codecName) const;

    virtual OsStatus getLocalAudioConnectionId(int& connectionId) const ;

    virtual OsStatus getInputMixerName(UtlString& name) const;
    virtual OsStatus getOutputMixerName(UtlString& name) const;
    virtual OsStatus getMasterVolume(int& volume) const;
    virtual OsStatus setMasterVolume(int volume);
    virtual OsStatus getPCMOutputVolume(int& volume) const;
    virtual OsStatus setPCMOutputVolume(int volume);
    virtual OsStatus getInputVolume(int& volume) const;
    virtual OsStatus setInputVolume(int volume);
    virtual OsStatus getOutputBalance(int& balance) const;
    virtual OsStatus setOutputBalance(int balance);

    virtual OsStatus getVideoQuality(int& quality) const;
    virtual OsStatus getVideoBitRate(int& bitRate) const;
    virtual OsStatus getVideoFrameRate(int& frameRate) const;

/* ============================ INQUIRY =================================== */

    virtual OsStatus isSpeakerMuted(UtlBoolean& bIsMuted) const;
    virtual OsStatus isMicMuted(UtlBoolean& bIsMuted) const;

    virtual OsStatus isInboundDTMFEnabled(MEDIA_INBOUND_DTMF_MODE mode, UtlBoolean& enabled);

/* //////////////////////////// PROTECTED ///////////////////////////////// */
  protected:
    MpMediaTask*    mpMediaTask ;     /**< Media task instance */
#ifdef INCLUDE_RTCP /* [ */
    IRTCPControl*   mpiRTCPControl;   /**< Realtime Control Interface */
#endif /* INCLUDE_RTCP ] */


/* //////////////////////////// PRIVATE /////////////////////////////////// */
  private:
    static int miInstanceCount;

    UtlBoolean m_bIsSpeakerMuted;
    UtlBoolean m_bIsMicMuted;
    float m_bMutedSpeakerVolume;
    float m_bMutedMicVolume;
};

/* ============================ INLINE METHODS ============================ */

#endif  // _sipXmediaFactoryImpl_h_
