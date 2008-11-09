## Process this file with automake to produce Makefile.in


EXTRA_DIST = tapi

SUBDIRS = .

INCLUDES = -I$(top_srcdir)/include -I$(top_srcdir)/../sipXmediaAdapterLib/interface/include

lib_LTLIBRARIES = libsipXcall.la

## Libtool version info - update only immediately before a release
# Current - increment if any interfaces are added, removed, or changed
version_Current=1
# Revision - set to 0 if Current was incremented
#            increment if any implementation has changed
version_Revision=0
# Age - set to 0 of any interfaces have been removed
#       increment if any interfaces have been added
version_Age=0

libsipXcall_la_LDFLAGS = -version-info ${version_Current}:${version_Revision}:${version_Age}

libsipXcall_la_LIBADD = \
	@SIPXMEDIA_MP_LIBS@ -lstdc++

libsipXcall_la_CXXFLAGS = \
	-DSIPX_CONFDIR=\"@SIPX_CONFDIR@\" \
    @VIDEO_DEFINE@

libsipXcall_la_SOURCES = \
    ../config/sipxcalllib-buildstamp.h \
    ../config/sipxcalllib-buildstamp.cpp \
    cp/CSeqManager.cpp \
    cp/CallManager.cpp \
    cp/Connection.cpp \
    cp/CpCall.cpp \
    cp/CpCallManager.cpp \
    cp/CpGhostConnection.cpp \
    cp/CpIntMessage.cpp \
    cp/CpMultiStringMessage.cpp \
    cp/CpPeerCall.cpp \
    cp/SipConnection.cpp \
    cp/XCpAbstractCall.cpp \
    cp/XCpCall.cpp \
    cp/XCpCallManager.cpp \
    cp/XCpConference.cpp \
    cp/XSipConnection.cpp \
    cp/XSipConnectionContext.cpp \
    cp/msg/AcAcceptConnectionMsg.cpp \
    cp/msg/AcAnswerConnectionMsg.cpp \
    cp/msg/AcAudioBufferPlayMsg.cpp \
    cp/msg/AcAudioFilePlayMsg.cpp \
    cp/msg/AcAudioPausePlaybackMsg.cpp \
    cp/msg/AcAudioRecordStartMsg.cpp \
    cp/msg/AcAudioRecordStopMsg.cpp \
    cp/msg/AcAudioResumePlaybackMsg.cpp \
    cp/msg/AcAudioStopPlaybackMsg.cpp \
    cp/msg/AcAudioToneStartMsg.cpp \
    cp/msg/AcAudioToneStopMsg.cpp \
    cp/msg/AcCommandMsg.cpp \
    cp/msg/AcConnectMsg.cpp \
    cp/msg/AcDropAllConnectionsMsg.cpp \
    cp/msg/AcDropConnectionMsg.cpp \
    cp/msg/AcGainFocusMsg.cpp \
    cp/msg/AcHoldAllConnectionsMsg.cpp \
    cp/msg/AcHoldConnectionMsg.cpp \
    cp/msg/AcLimitCodecPreferencesMsg.cpp \
    cp/msg/AcMuteInputConnectionMsg.cpp \
    cp/msg/AcNotificationMsg.cpp \
    cp/msg/AcRedirectConnectionMsg.cpp \
    cp/msg/AcRejectConnectionMsg.cpp \
    cp/msg/AcRenegotiateCodecsAllMsg.cpp \
    cp/msg/AcRenegotiateCodecsMsg.cpp \
    cp/msg/AcSendInfoMsg.cpp \
    cp/msg/AcTransferBlindMsg.cpp \
    cp/msg/AcUnholdAllConnectionsMsg.cpp \
    cp/msg/AcUnholdConnectionMsg.cpp \
    cp/msg/AcUnmuteInputConnectionMsg.cpp \
    cp/msg/AcYieldFocusMsg.cpp \
    cp/msg/CmCommandMsg.cpp \
    cp/msg/CmGainFocusMsg.cpp \
    cp/msg/CmNotificationMsg.cpp \
    cp/msg/CmYieldFocusMsg.cpp \
    cp/msg/CpTimerMsg.cpp \
    cp/msg/ScCommandMsg.cpp \
    cp/msg/ScNotificationMsg.cpp \
    cp/state/AlertingSipConnectionState.cpp \
    cp/state/BaseSipConnectionState.cpp \
    cp/state/DialingSipConnectionState.cpp \
    cp/state/DisconnectedSipConnectionState.cpp \
    cp/state/EstablishedSipConnectionState.cpp \
    cp/state/FailedSipConnectionState.cpp \
    cp/state/IdleSipConnectionState.cpp \
    cp/state/NewCallSipConnectionState.cpp \
    cp/state/OfferingSipConnectionState.cpp \
    cp/state/QueuedSipConnectionState.cpp \
    cp/state/RemoteAlertingSipConnectionState.cpp \
    cp/state/RemoteOfferingSipConnectionState.cpp \
    cp/state/RemoteQueuedSipConnectionState.cpp \
    cp/state/SipConnectionStateMachine.cpp \
    cp/state/UnknownSipConnectionState.cpp \
    tapi/CallStateEventMsg.cpp \
    tapi/InfoStatusEventMsg.cpp \
    tapi/KeepaliveEventMsg.cpp \
    tapi/LineStateEventMsg.cpp \
    tapi/MediaEventMsg.cpp \
    tapi/SecurityEventMsg.cpp \
    tapi/SecurityHelper.cpp \
    tapi/SipXAudio.cpp \
    tapi/SipXCall.cpp \
    tapi/SipXCallEventListener.cpp \
    tapi/SipXConference.cpp \
    tapi/SipXConfig.cpp \
    tapi/SipXCore.cpp \
    tapi/SipXEventDispatcher.cpp \
    tapi/SipXEvents.cpp \
    tapi/SipXHandleMap.cpp \
    tapi/SipXInfo.cpp \
    tapi/SipXInfoStatusEventListener.cpp \
    tapi/SipXKeepaliveEventListener.cpp \
    tapi/SipXLine.cpp \
    tapi/SipXLineEventListener.cpp \
    tapi/SipXMediaEventListener.cpp \
    tapi/SipXMessageObserver.cpp \
    tapi/SipXPim.cpp \
    tapi/SipXPublish.cpp \
    tapi/SipXSecurityEventListener.cpp \
    tapi/SipXSubscribe.cpp \
    tapi/sipXtapi.cpp \
    tapi/SipXTransport.cpp \
    tapi/SipXUtil.cpp

DISTCLEANFILES = Makefile.in
