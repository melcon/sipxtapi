## Process this file with automake to produce Makefile.in

SUBDIRS = test

INCLUDES = \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/../sipXportLib/include

lib_LTLIBRARIES = libsipXmedia.la


## Libtool version info - update only immediately before a release
# Current - increment if any interfaces are added, removed, or changed
version_Current=1
# Revision - set to 0 if Current was incremented
#            increment if any implementation has changed
version_Revision=0
# Age - set to 0 of any interfaces have been removed
#       increment if any interfaces have been added
version_Age=0

libsipXmedia_la_LDFLAGS = \
	-version-info ${version_Current}:${version_Revision}:${version_Age} \
	@GSM_LDFLAGS@

if DISABLE_LOCAL_AUDIO
LOCAL_AUDIO_CXXFLAGS = -DDISABLE_LOCAL_AUDIO
endif

libsipXmedia_la_CXXFLAGS = \
	-D_REALTIME_LINUX_AUDIO_THREADS \
	$(LOCAL_AUDIO_CXXFLAGS) \
	@GSM_CXXFLAGS@

# Disabled SPEEX_CFLAGS, broken
# SPEEX_CFLAGS

libsipXmedia_la_LIBADD = \
	@SIPXTACK_LIBS@ \
	@SIPXPORT_LIBS@ \
	@CA_LIBS@ \
	@GSM_LIBS@ \
	-lstdc++

# Disabled SPEEX_LIB, broken
# SPEEX_LIBS@

libsipXmedia_la_SOURCES =  \
    ../config/sipxmedialib-buildstamp.h \
    ../config/sipxmedialib-buildstamp.cpp \
    mp/DSPlib.cpp \
    mp/DspResampling.cpp \
    mp/FilterBank.cpp \
    mp/HandsetFilterBank.cpp \
    mp/MpArrayBuf.cpp \
    mp/MpAudioAbstract.cpp \
    mp/MpAudioBuf.cpp \
    mp/MpAudioDeviceInfo.cpp \
    mp/MpAudioDriverBase.cpp \
    mp/MpAudioDriverFactory.cpp \
    mp/MpAudioDriverManager.cpp \
    mp/MpAudioFileDecompress.cpp \
    mp/MpAudioFileOpen.cpp \
    mp/MpAudioFileUtils.cpp \
    mp/MpAudioMixerBase.cpp \
    mp/MpAudioResource.cpp \
    mp/MpAudioStreamInfo.cpp \
    mp/MpAudioStreamParameters.cpp \
    mp/MpAudioUtils.cpp \
    mp/MpAudioWaveFileRead.cpp \
    mp/MpBuf.cpp \
    mp/MpBufPool.cpp \
    mp/MpBufferMsg.cpp \
    mp/MpCallFlowGraph.cpp \
    mp/MpCodecFactory.cpp \
    mp/MpCodecInfo.cpp \
    mp/MpDataBuf.cpp \
    mp/MpDecodeBuffer.cpp \
    mp/MpDecoderBase.cpp \
    mp/MpDspUtils.cpp \
    mp/MpEncoderBase.cpp \
    mp/MpFlowGraphBase.cpp \
    mp/MpFlowGraphMsg.cpp \
    mp/MpHostAudioApiInfo.cpp \
    mp/MpJitterBufferBase.cpp \
    mp/MpJitterBufferDefault.cpp \
    mp/MpJitterBufferSpeex.cpp \
    mp/MpMMTimer.cpp \
    mp/MpMediaTask.cpp \
    mp/MpMediaTaskHelper.cpp \
    mp/MpMediaTaskMsg.cpp \
    mp/MpMisc.cpp \
    mp/MpNotificationDispatcher.cpp \
    mp/MpPlayer.cpp \
    mp/MpPlayerEvent.cpp \
    mp/MpPortAudioDriver.cpp \
    mp/MpPortAudioMixer.cpp \
    mp/MpPortAudioStream.cpp \
    mp/MpRawAudioBuffer.cpp \
    mp/MpResNotificationMsg.cpp \
    mp/MpResource.cpp \
    mp/MpResourceFactory.cpp \
    mp/MpResourceMsg.cpp \
    mp/MpResourceSortAlg.cpp \
    mp/MpResourceTopology.cpp \
    mp/MpRtpBuf.cpp \
    mp/MpRtpInputAudioConnection.cpp \
    mp/MpRtpInputConnection.cpp \
    mp/MpRtpOutputAudioConnection.cpp \
    mp/MpRtpOutputConnection.cpp \
    mp/MpSipxDecoders.cpp \
    mp/MpStreamFeeder.cpp \
    mp/MpStreamMsg.cpp \
    mp/MpStreamPlayer.cpp \
    mp/MpStreamPlaylistPlayer.cpp \
    mp/MpStreamQueuePlayer.cpp \
    mp/MpTopologyGraph.cpp \
    mp/MpUdpBuf.cpp \
    mp/MpdIPPG7231.cpp \
    mp/MpdIPPG729.cpp \
    mp/MpdPtAVT.cpp \
    mp/MpdSipxG726.cpp \
    mp/MpdSipxGSM.cpp \
    mp/MpdSipxILBC.cpp \
    mp/MpdSipxPcma.cpp \
    mp/MpdSipxPcmu.cpp \
    mp/MpdSipxSpeex.cpp \
    mp/MpeIPPG7231.cpp \
    mp/MpeIPPG729.cpp \
    mp/MpePtAVT.cpp \
    mp/MpeSipxG726.cpp \
    mp/MpeSipxGSM.cpp \
    mp/MpeSipxILBC.cpp \
    mp/MpeSipxPcma.cpp \
    mp/MpeSipxPcmu.cpp \
    mp/MpeSipxSpeex.cpp \
    mp/MprAudioFrameBuffer.cpp \
    mp/MprBridge.cpp \
    mp/MprBufferRecorder.cpp \
    mp/MprDecode.cpp \
    mp/MprSimpleDtmfDetector.cpp \
    mp/MprDejitter.cpp \
    mp/MprEchoSuppress.cpp \
    mp/MprEncode.cpp \
    mp/MprFromFile.cpp \
    mp/MprFromMic.cpp \
    mp/MprFromNet.cpp \
    mp/MprFromStream.cpp \
    mp/MprMixer.cpp \
    mp/MprNull.cpp \
    mp/MprNullAec.cpp \
    mp/MprRecorder.cpp \
    mp/MprSpeexEchoCancel.cpp \
    mp/MprSpeexPreProcess.cpp \
    mp/MprSplitter.cpp \
    mp/MprToNet.cpp \
    mp/MprToSpkr.cpp \
    mp/MprToneGen.cpp \
    mp/NetInTask.cpp \
    mp/StreamBufferDataSource.cpp \
    mp/StreamDataSource.cpp \
    mp/StreamDataSourceListener.cpp \
    mp/StreamDecoderListener.cpp \
    mp/StreamFileDataSource.cpp \
    mp/StreamFormatDecoder.cpp \
    mp/StreamHttpDataSource.cpp \
    mp/StreamQueueMsg.cpp \
    mp/StreamQueueingFormatDecoder.cpp \
    mp/StreamRAWFormatDecoder.cpp \
    mp/StreamWAVFormatDecoder.cpp \
    mp/dft.cpp \
    mp/dtmflib.cpp \
    mp/mpG711.cpp \
    mp/mpau.cpp \
    mp/resamplingby2.cpp \
    rtcp/BaseClass.cpp \
    rtcp/ByeReport.cpp \
    rtcp/Message.cpp \
    rtcp/MsgQueue.cpp \
    rtcp/NetworkChannel.cpp \
    rtcp/RTCManager.cpp \
    rtcp/RTCPConnection.cpp \
    rtcp/RTCPHeader.cpp \
    rtcp/RTCPRender.cpp \
    rtcp/RTCPSession.cpp \
    rtcp/RTCPSource.cpp \
    rtcp/RTCPTimer.cpp \
    rtcp/RTPHeader.cpp \
    rtcp/ReceiverReport.cpp \
    rtcp/SenderReport.cpp \
    rtcp/SourceDescription.cpp
