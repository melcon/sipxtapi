// 
// Copyright (C) 2005-2006 SIPez LLC.
// Licensed to SIPfoundry under a Contributor Agreement.
// 
// Copyright (C) 2004-2006 SIPfoundry Inc.
// Licensed by SIPfoundry under the LGPL license.
//
// Copyright (C) 2004-2006 Pingtel Corp.  All rights reserved.
// Licensed to SIPfoundry under a Contributor Agreement.
//
// $$
//////////////////////////////////////////////////////////////////////////////

// Author: Dan Petrie (dpetrie AT SIPez DOT com)

#ifndef _CpPhoneMediaInterface_h_
#define _CpPhoneMediaInterface_h_

// SYSTEM INCLUDES
//#include <>

// APPLICATION INCLUDES
#include <os/OsStatus.h>
#include <os/OsDefs.h>
#include <net/QoS.h>
#include <sdp/SdpCodecList.h>
#include "mi/CpMediaInterface.h"

// DEFINES
// MACROS
// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
// STRUCTS
// TYPEDEFS
// FORWARD DECLARATIONS
class MpCallFlowGraph;
class SdpCodec;
class OsDatagramSocket;
class CpPhoneMediaConnection;
class ISocketEvent;

//:Class short description which may consist of multiple lines (note the ':')
// Class detailed description which may extend to multiple lines
class CpPhoneMediaInterface : public CpMediaInterface
{
/* //////////////////////////// PUBLIC //////////////////////////////////// */
public:

   enum OutputAudioDevice
   {
      UNKNOWN = 0x0,
      HANDSET = 0x1,
      SPEAKER = 0x2,
      HEADSET = 0x4
   };

/* ============================ CREATORS ================================== */

   CpPhoneMediaInterface(CpMediaInterfaceFactory* pFactoryImpl,
						       OsMsgQ* pInterfaceNotificationQueue,
                         const SdpCodecList* pCodecList = NULL,
                         const char* publicAddress = NULL, 
                         const char* localAddress = NULL,
                         const char* pLocale = "",
                         int expeditedIpTos = QOS_LAYER3_LOW_DELAY_IP_TOS,
                         const char* szStunServer = NULL,
                         int iStunPort = PORT_NONE,
                         int iStunKeepAlivePeriodSecs = 28,
                         const char* szTurnServer = NULL,
                         int iTurnPort = PORT_NONE,
                         const char* szTurnUsername = NULL,
                         const char* szTurnPassword = NULL,
                         int iTurnKeepAlivePeriodSecs = 28,
                         UtlBoolean bEnableICE = FALSE);
     //:Default constructor

  protected:
   virtual
   ~CpPhoneMediaInterface();
     //:Destructor
  public:

   /**
    * public interface for destroying this media interface
    */ 
   void release();

/* ============================ MANIPULATORS ============================== */


   virtual OsStatus createConnection(int& connectionId,
                                     const char* szLocalAddress,
                                     int localPort = 0,
                                     void* videoWindowHandle = NULL,
                                     void* const pSecurityAttributes = NULL,
                                     OsMsgQ* pConnectionNotificationQueue = NULL,
                                     const RtpTransportOptions rtpTransportOptions=RTP_TRANSPORT_UDP);
   
   virtual void setInterfaceNotificationQueue(OsMsgQ* pInterfaceNotificationQueue);

   virtual OsStatus getCapabilities(int connectionId, 
                                    UtlString& rtpHostAddress, 
                                    int& rtpAudioPort,
                                    int& rtcpAudioPort,
                                    int& rtpVideoPort,
                                    int& rtcpVideoPort,
                                    SdpCodecList& supportedCodecs,
                                    SdpSrtpParameters& srtpParams,
                                    int bandWidth,
                                    int& videoBandwidth,
                                    int& videoFramerate) ;

   virtual OsStatus getCapabilitiesEx(int connectionId, 
                                      int nMaxAddresses,
                                      UtlString rtpHostAddresses[], 
                                      int rtpAudioPorts[],
                                      int rtcpAudioPorts[],
                                      int rtpVideoPorts[],
                                      int rtcpVideoPorts[],
                                      RTP_TRANSPORT transportTypes[],
                                      int& nActualAddresses,
                                      SdpCodecList& supportedCodecs,
                                      SdpSrtpParameters& srtpParameters,
                                      int bandWidth,
                                      int& videoBandwidth,
                                      int& videoFramerate);

   virtual OsStatus setMediaNotificationsEnabled(bool enabled, 
                                                 const UtlString& resourceName);

   virtual OsStatus setConnectionDestination(int connectionId,
                                             const char* rtpHostAddress, 
                                             int rtpAudioPort,
                                             int rtcpAudioPort,
                                             int rtpVideoPort,
                                             int rtcpVideoPort);

   virtual OsStatus startRtpSend(int connectionId, 
                                 const SdpCodecList& codecList);

   virtual OsStatus startRtpReceive(int connectionId,
                                    const SdpCodecList& codecList);

   virtual OsStatus stopRtpSend(int connectionId);
   virtual OsStatus stopRtpReceive(int connectionId);

   virtual OsStatus deleteConnection(int connectionId);

   virtual OsStatus startTone(int toneId, UtlBoolean local, UtlBoolean remote);
   virtual OsStatus stopTone();

   virtual OsStatus startChannelTone(int connectionId, int toneId, UtlBoolean local, UtlBoolean remote) ;
   virtual OsStatus stopChannelTone(int connectionId) ;

   virtual OsStatus muteInput(int connectionId, UtlBoolean bMute);

   virtual OsStatus playAudio(const char* url, 
                              UtlBoolean repeat,
                              UtlBoolean local, 
                              UtlBoolean remote,
                              UtlBoolean mixWithMic = false,
                              int downScaling = 100,
                              void* pCookie = NULL);


    virtual OsStatus playBuffer(void* buf, 
                               size_t bufSize,
                               int type, 
                              UtlBoolean repeat,
                              UtlBoolean local, 
                              UtlBoolean remote,
                              UtlBoolean mixWithMic = false,
                              int downScaling = 100,
                              void* pCookie = NULL);

    virtual OsStatus stopAudio();

    virtual OsStatus pausePlayback();

    virtual OsStatus resumePlayback();

    virtual OsStatus playChannelAudio(int connectionId,
                                     const char* url,
                                     UtlBoolean repeat,
                                     UtlBoolean local,
                                     UtlBoolean remote,
                                     UtlBoolean mixWithMic = false,
                                     int downScaling = 100,
                                     void* pCookie = NULL) ;


   virtual OsStatus stopChannelAudio(int connectionId) ;


   virtual OsStatus recordChannelAudio(int connectionId,
                                       const char* szFile) ;

   virtual OsStatus stopRecordChannelAudio(int connectionId) ;

   virtual OsStatus giveFocus();
   virtual OsStatus defocus();

   virtual OsStatus stopRecording();
   virtual OsStatus ezRecord(int ms, 
           int silenceLength, 
           const char* fileName, 
           double& duration, 
           int& dtmfterm,
           OsProtectedEvent* ev = NULL);

   /**
   * Starts recording all audio channels into given file.
   */
   virtual OsStatus recordAudio(const char* szFile);

     /// @copydoc CpMediaInterface::recordMic(UtlString*)
   virtual OsStatus recordMic(UtlString* pAudioBuffer);

     /// @copydoc CpMediaInterface::recordMic(int, int, const char*)
   virtual OsStatus recordMic(int ms,
                              int silenceLength,
                              const char* fileName) ; 

    virtual void setContactType(int connectionId, SIPX_CONTACT_TYPE eType, SIPX_CONTACT_ID contactId) ;
     //: Set the contact type for this Phone media interface.  
     //  It is important to set the contact type BEFORE creating the 
     //  connection -- setting after the connection has been created
     //  is essentially a NOP.

    //! Rebuild the codec factory on the fly
    virtual OsStatus setAudioCodecBandwidth(int connectionId, int bandWidth) ;

    /**
     * Rebuilds SdpCodecList using supplied list of SdpCodec instances
     * @param sdpCodecList List with instances of SdpCodec
     */
    virtual OsStatus rebuildCodecList(const UtlSList& sdpCodecList);

    //! Set conneection bitrate on the fly
    virtual OsStatus setConnectionBitrate(int connectionId, int bitrate) ;

    //! Set connection framerate on the fly
    virtual OsStatus setConnectionFramerate(int connectionId, int framerate) ;

    virtual OsStatus setSecurityAttributes(const void* security) ;

    virtual OsStatus addAudioRtpConnectionDestination(int         connectionId,
                                                      int         iPriority,
                                                      const char* candidateIp, 
                                                      int         candidatePort) ;

    virtual OsStatus addAudioRtcpConnectionDestination(int         connectionId,
                                                       int         iPriority,
                                                       const char* candidateIp, 
                                                       int         candidatePort) ;

    virtual OsStatus addVideoRtpConnectionDestination(int         connectionId,
                                                      int         iPriority,
                                                      const char* candidateIp, 
                                                      int         candidatePort) ;

    virtual OsStatus addVideoRtcpConnectionDestination(int         connectionId,
                                                       int         iPriority,
                                                       const char* candidateIp, 
                                                       int         candidatePort) ;
    
    virtual void setConnectionTcpRole(const int connectionId, const RtpTcpRoles role)
    {
        // NOT IMPLEMENTED
    }

	virtual OsStatus generateVoiceQualityReport(int         connectiond,
                                                const char* callId,
                                                UtlString&  report) ;


/* ============================ ACCESSORS ================================= */

   virtual int getCodecCPUCost();
      //:Calculate the current cost for our sending/receiving codecs

   virtual int getCodecCPULimit();
      //:Calculate the worst cost for our sending/receiving codecs

   virtual OsMsgQ* getMsgQ();
     //:Returns the flowgraph's message queue

     /// @copydoc CpMediaInterface::getMediaNotificationDispatcher()
   virtual OsMsgDispatcher* getMediaNotificationDispatcher();

   virtual OsStatus getVideoQuality(int& quality);
   virtual OsStatus getVideoBitRate(int& bitRate);
   virtual OsStatus getVideoFrameRate(int& frameRate);

     //:Returns primary codec for the connection
   virtual OsStatus setVideoWindowDisplay(const void* hWnd);
   virtual const void* getVideoWindowDisplay();

   //! Set a media property on the media interface
   virtual OsStatus setMediaProperty(const UtlString& propertyName,
                                     const UtlString& propertyValue);

   //! Get a media property on the media interface
   virtual OsStatus getMediaProperty(const UtlString& propertyName,
                                     UtlString& propertyValue);

   //! Set a media property associated with a connection
   virtual OsStatus setMediaProperty(int connectionId,
                                     const UtlString& propertyName,
                                     const UtlString& propertyValue);

   //! Get a media property associated with a connection
   virtual OsStatus getMediaProperty(int connectionId,
                                     const UtlString& propertyName,
                                     UtlString& propertyValue);



   virtual OsStatus setVideoQuality(int quality);
   virtual OsStatus setVideoParameters(int bitRate, int frameRate);

   // Returns the primary codec for the connection
   virtual OsStatus getPrimaryCodec(int connectionId, 
                                    UtlString& audioCodec,
                                    UtlString& videoCodec,
                                    int* audiopPayloadType,
                                    int* videoPayloadType,
                                    bool& isEncrypted) ;

   virtual UtlString getType() { return "CpPhoneMediaInterface"; };

/* ============================ INQUIRY =================================== */

   virtual UtlBoolean isSendingRtpAudio(int connectionId);
   virtual UtlBoolean isReceivingRtpAudio(int connectionId);
   virtual UtlBoolean isSendingRtpVideo(int connectionId);
   virtual UtlBoolean isReceivingRtpVideo(int connectionId);
   virtual UtlBoolean isDestinationSet(int connectionId);   
   virtual UtlBoolean canAddParty() ;
   virtual UtlBoolean isVideoInitialized(int connectionId) ;
   virtual UtlBoolean isAudioInitialized(int connectionId) ;
   virtual UtlBoolean isAudioAvailable() ;
   virtual UtlBoolean isVideoConferencing() { return false; } ;


/* //////////////////////////// PROTECTED ///////////////////////////////// */
protected:

    UtlBoolean getLocalAddresses( int connectionId,
                                  UtlString& hostIp,
                                  int& rtpAudioPort,
                                  int& rtcpAudioPort,
                                  int& rtpVideoPort,
                                  int& rtcpVideoPort) ;

    UtlBoolean getNatedAddresses( int connectionId,
                                  UtlString& hostIp,
                                  int& rtpAudioPort,
                                  int& rtcpAudioPort,
                                  int& rtpVideoPort,
                                  int& rtcpVideoPort) ;


    UtlBoolean getRelayAddresses( int connectionId,
                                  UtlString& hostIp,
                                  int& rtpAudioPort,
                                  int& rtcpAudioPort,
                                  int& rtpVideoPort,
                                  int& rtcpVideoPort) ;


    OsStatus addLocalContacts(  int connectionId, 
                                int nMaxAddresses,
                                UtlString rtpHostAddresses[], 
                                int rtpAudioPorts[],
                                int rtcpAudioPorts[],
                                int rtpVideoPorts[],
                                int rtcpVideoPorts[],
                                int& nActualAddresses) ;

    OsStatus addNatedContacts(  int connectionId, 
                                int nMaxAddresses,
                                UtlString rtpHostAddresses[], 
                                int rtpAudioPorts[],
                                int rtcpAudioPorts[],
                                int rtpVideoPorts[],
                                int rtcpVideoPorts[],
                                int& nActualAddresses) ;

    OsStatus addRelayContacts(  int connectionId, 
                                int nMaxAddresses,
                                UtlString rtpHostAddresses[], 
                                int rtpAudioPorts[],
                                int rtcpAudioPorts[],
                                int rtpVideoPorts[],
                                int rtcpVideoPorts[],
                                int& nActualAddresses) ;

    void applyAlternateDestinations(int connectionId) ;

      /// Create socket pair for RTP/RTCP streams.
    OsStatus createRtpSocketPair(UtlString localAddress,
                                 int localPort,
                                 SIPX_CONTACT_TYPE contactType,
                                 OsDatagramSocket* &rtpSocket,
                                 OsDatagramSocket* &rtcpSocket);
      /**<
      *  For RTP/RTCP port pair will be set next free port pair.
      *  
      *  @param[in] localAddress - Address to bind to (for multihomed hosts).
      *  @param[in] localPort - Local port to bind to (0 for auto select).
      *  @param[in] contactType - Contact type (see SIPX_CONTACT_TYPE).
      *  @param[out] rtpSocket - Created socket for RTP stream.
      *  @param[out] rtcpSocket - Created socket for RTCP stream.
      */


/* //////////////////////////// PRIVATE /////////////////////////////////// */
private:
    CpPhoneMediaConnection* getMediaConnection(int connectionId);
    CpPhoneMediaConnection* removeMediaConnection(int connectionId);
    OsStatus doDeleteConnection(CpPhoneMediaConnection* mediaConnection);

   UtlString mRtpReceiveHostAddress; ///< Advertised as place to send RTP/RTCP
   UtlString mLocalAddress;          ///< Address on which ports are bound
   MpCallFlowGraph* mpFlowGraph;     ///< Flowgraph for audio part of call
   UtlBoolean mRingToneFromFile;
   SdpCodecList mSdpCodecList;
   UtlDList mMediaConnections;
   int mExpeditedIpTos;
   UtlString mStunServer ;
   int mStunPort;
   int mStunRefreshPeriodSecs ;
   UtlString mTurnServer ;
   int mTurnPort ;
   int mTurnRefreshPeriodSecs ;
   UtlString mTurnUsername ;
   UtlString mTurnPassword ;
   UtlBoolean mEnableIce ;
   UtlHashMap mInterfaceProperties;
   OsMsgQ* m_pInterfaceNotificationQueue;

   // Disabled copy constructor
   CpPhoneMediaInterface(const CpPhoneMediaInterface& rCpPhoneMediaInterface);

   // Disabled equals operator
   CpPhoneMediaInterface& operator=(const CpPhoneMediaInterface& rhs);
     

};

/* ============================ INLINE METHODS ============================ */
#endif  // _CpPhoneMediaInterface_h_
