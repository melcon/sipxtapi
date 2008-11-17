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

#ifndef SipXCore_h__
#define SipXCore_h__

// SYSTEM INCLUDES
// APPLICATION INCLUDES
#include "utl/UtlString.h"
#include "tapi/sipXtapi.h"
#include "utl/UtlDefs.h"
#include "utl/UtlContainable.h"
#include "os/OsRWMutex.h"
#include "os/OsReadLock.h"
#include "os/OsWriteLock.h"
#include "os/OsMutex.h"

// DEFINES

/** sipXtapi can be configured to expire after a certain date */
//add SIPXTAPI_EVAL_EXPIRATION to preprocessor definitions

#ifdef SIPXTAPI_EVAL_EXPIRATION
#  define EVAL_EXPIRE_MONTH     0       // zero based
#  define EVAL_EXPIRE_DAY       31
#  define EVAL_EXPIRE_YEAR      2008
#endif

// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
// FORWARD DECLARATIONS
class Url;
class OsNotification;
class SipSubscribeServer;
class SipSubscribeClient;
class XCpCallManager;
class SipLineMgr;
class SdpCodec;
class SdpCodecList;
class SipUserAgent;
class SipRefreshMgr;
class SipDialogMgr;
class SipRefreshManager;
class SipXKeepaliveEventListener;
class SipXEventDispatcher;
class SipPimClient;
class SipXMessageObserver;
class SipXLineEventListener;
class SipXCallEventListener;
class SipXInfoStatusEventListener;
class SipXSecurityEventListener;
class SipXMediaEventListener;
class OsSharedServerTaskMgr;

// STRUCTS
// TYPEDEFS

class AEC_SETTING
{
public:
   UtlBoolean bInitialized;/**< Is the data valid */
   SIPX_AEC_MODE mode;     /**< Is AEC enabled? */

   AEC_SETTING() : bInitialized(FALSE),
      mode(SIPX_AEC_DISABLED)
   {

   }
};

class AGC_SETTING
{
public:
   UtlBoolean bInitialized;     /**< Is the data valid */
   UtlBoolean bEnabled;         /**< Is AGC enabled? */

   AGC_SETTING() : bInitialized(FALSE),
      bEnabled(FALSE)
   {

   }
};

class NOISE_REDUCTION_SETTING
{
public:
   UtlBoolean bInitialized;           /**< Is the data valid */
   SIPX_NOISE_REDUCTION_MODE mode;    /**< Is NR enabled? */

   NOISE_REDUCTION_SETTING() : bInitialized(FALSE),
      mode(SIPX_NOISE_REDUCTION_DISABLED)
   {

   }
};


class AUDIO_CODEC_PREFERENCES
{
public:
   UtlBoolean              bInitialized; /**< Is the data valid */
   int               numCodecs;          /**< Number of codecs */
   UtlString         preferences;        /**< List of preferred codecs */
   SdpCodec**        sdpCodecArray;      /**< Pointer to an array of codecs */

   AUDIO_CODEC_PREFERENCES() : bInitialized(FALSE),
      numCodecs(0),
      preferences(NULL),
      sdpCodecArray(NULL)
   {

   }
};

class VIDEO_CODEC_PREFERENCES
{
public:
   UtlBoolean              bInitialized; /**< Is the data valid */
   int               numCodecs;          /**< Number of codecs */
   SIPX_VIDEO_FORMAT videoFormat;        /**< Selected video format */
   UtlString         preferences;        /**< List of preferred codecs */
   SdpCodec**        sdpCodecArray;      /**< Pointer to an array of codecs */

   VIDEO_CODEC_PREFERENCES() : bInitialized(FALSE),
      numCodecs(0),
      videoFormat(VIDEO_FORMAT_ANY),
      preferences(NULL),
      sdpCodecArray(NULL)
   {

   }
};

class SIPX_INSTANCE_DATA
{
public:
   SipUserAgent*    pSipUserAgent;
   SipPimClient*    pSipPimClient;
   SdpCodecList* pCodecList;
   XCpCallManager*     pCallManager;
   SipLineMgr*      pLineManager;
   SipRefreshMgr*   pRefreshManager;
   SipSubscribeServer* pSubscribeServer;
   SipSubscribeClient* pSubscribeClient;
   SipRefreshManager* pSipRefreshManager;
   SipXLineEventListener* pLineEventListener;
   SipXCallEventListener* pCallEventListener;
   SipXInfoStatusEventListener* pInfoStatusEventListener;
   SipXSecurityEventListener* pSecurityEventListener;
   SipXMediaEventListener* pMediaEventListener;
   SipXKeepaliveEventListener* pKeepaliveEventListener;
   SipDialogMgr* pDialogManager;
   OsSharedServerTaskMgr* pSharedTaskMgr;

   AUDIO_CODEC_PREFERENCES audioCodecSetting;
   VIDEO_CODEC_PREFERENCES videoCodecSetting;

   char*            inputAudioDevices[MAX_AUDIO_DEVICES];
   int              nInputAudioDevices;
   char*            outputAudioDevices[MAX_AUDIO_DEVICES];
   int              nOutputAudioDevices;
   SipXMessageObserver* pMessageObserver;
   OsNotification   *pStunNotification;   /**< Signals the initial stun success/failure
                                           when calling sipXconfigEnableStun */
   OsMutex         lock;
   int             nCalls;        /**< Counter for inprocess calls */
   int             nConferences;  /**< Counter for inprocess conferences */
   int             nLines;        /**< Counter for inprocess lines */
   void*           pVoiceEngine;  /**< Cache VoiceEngine pointer */
   UtlBoolean      bShortNames;   /**< short names in sip messages >*/
   UtlBoolean      bAllowHeader;  /**< use allow header in sip messages>*/
   UtlBoolean      bDateHeader;   /**< use Date header in sip messages>*/
   char            szAcceptLanguage[16]; /**< accept language to use in sip messages>*/
   char            szLocationHeader[256]; /**< location header */
   UtlBoolean      bRtpOverTcp;   /**< allow RTP over TCP */

   SIPX_INSTANCE_DATA() : lock(OsMutex::Q_FIFO),
      pSipUserAgent(NULL),
      pSipPimClient(NULL),
      pCodecList(NULL),
      pCallManager(NULL),
      pLineManager(NULL),
      pRefreshManager(NULL),
      pSubscribeServer(NULL),
      pSubscribeClient(NULL),
      pSipRefreshManager(NULL),
      pLineEventListener(NULL),
      pCallEventListener(NULL),
      pInfoStatusEventListener(NULL),
      pSecurityEventListener(NULL),
      pMediaEventListener(NULL),
      pDialogManager(NULL),
      pMessageObserver(NULL),
      pStunNotification(NULL),
      nCalls(0),
      nConferences(0),
      nLines(0),
      pVoiceEngine(NULL),
      bShortNames(FALSE),
      bAllowHeader(FALSE),
      bDateHeader(FALSE),
      bRtpOverTcp(FALSE),
      pKeepaliveEventListener(NULL),
      nInputAudioDevices(0),
      nOutputAudioDevices(0),
      pSharedTaskMgr(NULL)
   {
      // Clear devices
      for (int i = 0; i < MAX_AUDIO_DEVICES; i++)
      {
         inputAudioDevices[i] = NULL;
         outputAudioDevices[i] = NULL;
      }
      
      memset(szAcceptLanguage, 0, sizeof(szAcceptLanguage));
      memset(szLocationHeader, 0, sizeof(szLocationHeader));
   }
};

typedef enum SIPX_INTERNAL_CALLSTATE
{
   SIPX_INTERNAL_CALLSTATE_UNKNOWN = 0,        /** Unknown call state */
   SIPX_INTERNAL_CALLSTATE_OUTBOUND_ATTEMPT,   /** Early dialog: outbound */
   SIPX_INTERNAL_CALLSTATE_INBOUND_ATEMPT,     /** Early dialog: inbound */
   SIPX_INTERNAL_CALLSTATE_CONNECTED,          /** Active call - remote audio */
   SIPX_INTERNAL_CALLSTATE_HELD,               /** both on hold due to a local hold */
   SIPX_INTERNAL_CALLSTATE_REMOTE_HELD,        /** Remotely held call */
   SIPX_INTERNAL_CALLSTATE_BRIDGED,            /** Locally held call, bridging */
   SIPX_INTERNAL_CALLSTATE_DISCONNECTED,       /** Disconnected or failed */
   SIPX_INTERNAL_CALLSTATE_DESTROYING          /** In the process of being destroyed */
} SIPX_INTERNAL_CALLSTATE;

typedef enum CONF_HOLD_STATE
{
   CONF_STATE_UNHELD = 0,
   CONF_STATE_BRIDGING_HOLD,
   CONF_STATE_NON_BRIDGING_HOLD
} CONF_HOLD_STATE;

typedef enum SIPX_LOCK_TYPE
{
   SIPX_LOCK_NONE,
   SIPX_LOCK_READ,
   SIPX_LOCK_WRITE
} SIPX_LOCK_TYPE;

// MACROS
// GLOBAL VARIABLES
// GLOBAL FUNCTIONS

/**
* Flush handles to remove peaks between test cases -- this *WILL* leak 
* memory.
*/
SIPXTAPI_API SIPX_RESULT sipxFlushHandles();

/**
* Look for leaks in internal handles
*/
SIPXTAPI_API SIPX_RESULT sipxCheckForHandleLeaks();

const char* sipxContactTypeToString(SIPX_CONTACT_TYPE type);

//: Get the external host and port given the contact preference
void sipxSelectContact(SIPX_INSTANCE_DATA* pData, 
                       SIPX_CONTACT_TYPE& contactType, 
                       const UtlString& suggestedContactIp,
                       UtlString& contactIp,
                       int& contactPort,
                       SIPX_TRANSPORT_TYPE& transport);

void sipxIncSessionCount();
void sipxDecSessionCount();
int sipxGetSessionCount();

#endif // SipXCore_h__
