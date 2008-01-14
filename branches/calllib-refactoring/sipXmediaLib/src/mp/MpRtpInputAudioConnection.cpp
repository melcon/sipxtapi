//  
// Copyright (C) 2006-2007 SIPez LLC. 
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

// APPLICATION INCLUDES
#include <os/OsLock.h>
#include "os/OsIntPtrMsg.h"
#include <os/OsSysLog.h>
#include <mp/MpRtpInputAudioConnection.h>
#include <mp/MpFlowGraphBase.h>
#include <mp/MprDejitter.h>
#include <mp/MprDecode.h>
#include <mp/MpResourceMsg.h>
#include <mp/MprRtpStartReceiveMsg.h>
#include "mp/MprDecodeInbandDtmf.h"
#include "mp/MpResNotification.h"
#include <sdp/SdpCodec.h>
#ifdef RTL_ENABLED
#   include <rtl_macro.h>
#endif

// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
// STATIC VARIABLE INITIALIZATIONS

/* //////////////////////////// PUBLIC //////////////////////////////////// */

/* ============================ CREATORS ================================== */

// Constructor
MpRtpInputAudioConnection::MpRtpInputAudioConnection(const UtlString& resourceName,
                                                     MpConnectionID myID, 
                                                     OsMsgQ* pConnectionNotificationQueue,
                                                     UtlBoolean bInBandDTMFEnabled,
                                                     UtlBoolean bRFC2833DTMFEnabled,
                                                     int samplesPerFrame, 
                                                     int samplesPerSec)
: MpRtpInputConnection(resourceName,
                       myID,
                       pConnectionNotificationQueue,
#ifdef INCLUDE_RTCP // [
                       NULL // TODO: pParent->getRTCPSessionPtr()
#else // INCLUDE_RTCP ][
                       NULL
#endif // INCLUDE_RTCP ]
                       )
, mpDecode(NULL)
, mpDecodeInBandDtmf(NULL)
, m_bInBandDTMFEnabled(bInBandDTMFEnabled)
, m_bRFC2833DTMFEnabled(bRFC2833DTMFEnabled)
{
   char         name[50];
   int          i;

   SNPRINTF(name, sizeof(name), "Decode-%d", myID);
   mpDecode    = new MprDecode(name, this, samplesPerFrame, samplesPerSec);
   mpDecode->registerObserver(this);

   if (m_bInBandDTMFEnabled)
   {
      SNPRINTF(name, sizeof(name), "DecodeInBandDtmf-%d", myID);
      mpDecodeInBandDtmf = new MprDecodeInBandDtmf(name, samplesPerFrame, samplesPerSec);
      mpDecodeInBandDtmf->registerObserver(this);
   }   

 //memset((char*)mpPayloadMap, 0, (NUM_PAYLOAD_TYPES*sizeof(MpDecoderBase*)));
   for (i=0; i<NUM_PAYLOAD_TYPES; i++) {
      mpPayloadMap[i] = NULL;
   }

   // decoder does not get added to the flowgraph, this connection
   // gets added to do the decoding frameprocessing.

   //////////////////////////////////////////////////////////////////////////
   // connect Dejitter -> Decode (Non synchronous resources)
   mpDecode->setMyDejitter(mpDejitter);

   //  This got moved to the call flowgraph when the connection is
   // added to the flowgraph.  Not sure it is still needed there either
   //pParent->synchronize("new Connection, before enable(), %dx%X\n");
   //enable();
   //pParent->synchronize("new Connection, after enable(), %dx%X\n");
}

// Destructor
MpRtpInputAudioConnection::~MpRtpInputAudioConnection()
{
   delete mpDecode;
   mpDecode = NULL;
   delete mpDecodeInBandDtmf;
   mpDecodeInBandDtmf = NULL;
}

/* ============================ MANIPULATORS ============================== */

UtlBoolean MpRtpInputAudioConnection::processFrame(void)
{
    UtlBoolean result;

#ifdef RTL_ENABLED
    RTL_BLOCK((UtlString)*this);
#endif

    assert(mpDecode);
    if(mpDecode)
    {
        // call doProcessFrame to do any "real" work
        result = mpDecode->doProcessFrame(mpInBufs, 
                                          mpOutBufs,
                                          mMaxInputs, 
                                          mMaxOutputs, 
                                          mpDecode->mIsEnabled,
                                          mpDecode->getSamplesPerFrame(), 
                                          mpDecode->getSamplesPerSec());
    
		if(mpDecodeInBandDtmf)
		{
			result &= mpDecodeInBandDtmf->doProcessFrame(mpOutBufs, // OutBufs from mpDecoder = InBuf for InBand
											  NULL,					// No outBufs needed
											  mMaxOutputs,			// Again MaxOutputs from decoder = maxInput for InBand 
											  0,					// 0 output 
											  mpDecodeInBandDtmf->mIsEnabled,
											  mpDecodeInBandDtmf->getSamplesPerFrame(), 
											  mpDecodeInBandDtmf->getSamplesPerSec());
		}
	}


    // No input buffers to release
   assert(mMaxInputs == 0);

   // Push the output buffer to the next resource
   assert(mMaxOutputs == 1);
   pushBufferDownsream(0, mpOutBufs[0]);
   // release the output buffer
   mpOutBufs[0].release();


   return(result);
}

UtlBoolean MpRtpInputAudioConnection::doProcessFrame(MpBufPtr inBufs[],
                                                     MpBufPtr outBufs[],
                                                     int inBufsSize,
                                                     int outBufsSize,
                                                     UtlBoolean isEnabled,
                                                     int samplesPerFrame,
                                                     int samplesPerSecond)
{
    // Not currently used
    assert(0);

    UtlBoolean result = FALSE;
    return(result);
}

UtlBoolean MpRtpInputAudioConnection::handleMessage(MpResourceMsg& rMsg)
{
    UtlBoolean result = FALSE;
    unsigned char messageSubtype = rMsg.getMsgSubType();
    switch(messageSubtype)
    {
    case MpResourceMsg::MPRM_START_RECEIVE_RTP:
        {
            MprRtpStartReceiveMsg* startMessage = (MprRtpStartReceiveMsg*) &rMsg;
            SdpCodec** codecArray = NULL;
            int numCodecs;
            startMessage->getCodecArray(numCodecs, codecArray);
            OsSocket* rtpSocket = startMessage->getRtpSocket();
            OsSocket* rtcpSocket = startMessage->getRtcpSocket();

            handleStartReceiveRtp(codecArray, numCodecs, *rtpSocket, *rtcpSocket);
            result = TRUE;
        }
        break;

    case MpResourceMsg::MPRM_STOP_RECEIVE_RTP:
        handleStopReceiveRtp();
        result = TRUE;
        break;

    default:
        result = MpResource::handleMessage(rMsg);
        break;
    }
    return(result);
}


// Disables the input path of the connection.
// Resources on the path(s) will also be disabled by these calls.
// If the flow graph is not "started", this call takes effect
// immediately.  Otherwise, the call takes effect at the start of the
// next frame processing interval.
//!retcode: OS_SUCCESS - for now, these methods always return success

UtlBoolean MpRtpInputAudioConnection::handleDisable()
{
   mpDecode->disable();
   if (mpDecodeInBandDtmf)
   {
      mpDecodeInBandDtmf->disable();
   }
   
   return(MpResource::handleDisable());
}



// Enables the input path of the connection.
// Resources on the path(s) will also be enabled by these calls.
// Resources may allocate needed data (e.g. output path reframe buffer)
//  during this operation.
// If the flow graph is not "started", this call takes effect
// immediately.  Otherwise, the call takes effect at the start of the
// next frame processing interval.
//!retcode: OS_SUCCESS - for now, these methods always return success

UtlBoolean MpRtpInputAudioConnection::handleEnable()
{
   mpDecode->enable();
   if (mpDecodeInBandDtmf)
   {
      mpDecodeInBandDtmf->enable();
   }
   
   return(MpResource::handleEnable());
}

// Start receiving RTP and RTCP packets.

OsStatus MpRtpInputAudioConnection::startReceiveRtp(OsMsgQ& messageQueue,
                                                    const UtlString& resourceName,
                                                    SdpCodec* codecArray[], 
                                                    int numCodecs,
                                                    OsSocket& rRtpSocket,
                                                    OsSocket& rRtcpSocket)
{
    OsStatus result = OS_INVALID_ARGUMENT;
    if(numCodecs > 0 && codecArray)
    {
        // Create a message to contain the startRecieveRtp data
        MprRtpStartReceiveMsg msg(resourceName,
                                  codecArray,
                                  numCodecs,
                                  rRtpSocket,
                                  rRtcpSocket);

        // Send the message in the queue.
        result = messageQueue.send(msg);
    }
    return(result);
}

void MpRtpInputAudioConnection::handleStartReceiveRtp(SdpCodec* pCodecs[], 
                                                      int numCodecs,
                                                      OsSocket& rRtpSocket,
                                                      OsSocket& rRtcpSocket)
{
   if (numCodecs)
   {
      // if RFC2833 DTMF is disabled
      if (!m_bRFC2833DTMFEnabled)
      {
         // go through all codecs, if you find telephone event, remove it
         for (int i = 0; i < numCodecs; i++)
         {
            if (pCodecs[i]->getCodecType() == SdpCodec::SDP_CODEC_TONES)
            {
               if (i == (numCodecs - 1))
               {
                  // this is the last codec, lie that we have numCodecs-1
                  numCodecs = numCodecs - 1;
               }
               else
               {
                  SdpCodec* tmp;
                  // swap rfc2833 codec with the last one, and lie that we have one
                  // less codec than we actually have. It is safe to do, as codecs
                  // are owned by start RTP message.
                  tmp = pCodecs[i];
                  pCodecs[i] = pCodecs[numCodecs - 1];
                  pCodecs[numCodecs - 1] = tmp;

                  numCodecs = numCodecs - 1;
               }
               break;
            }
         }
      }

      // continue only if numCodecs is still greater than 0
      if (numCodecs > 0)
      {
         // initialize jitter buffers for all codecs
         mpDejitter->initJitterBuffers(pCodecs, numCodecs);
         mpDecode->selectCodecs(pCodecs, numCodecs);
      }
   }
   // No need to synchronize as the decoder is not part of the
   // flowgraph.  It is part of this connection/resource
   //mpFlowGraph->synchronize();
   prepareStartReceiveRtp(rRtpSocket, rRtcpSocket);
   // No need to synchronize as the decoder is not part of the
   // flowgraph.  It is part of this connection/resource
   //mpFlowGraph->synchronize();
   if (numCodecs)
   {
      mpDecode->enable();
      if (mpDecodeInBandDtmf)
      {
         mpDecodeInBandDtmf->enable();
      }
   }
}

OsStatus MpRtpInputAudioConnection::stopReceiveRtp(OsMsgQ& messageQueue,
                                                   const UtlString& resourceName)
{
    MpResourceMsg stopReceiveMsg(MpResourceMsg::MPRM_STOP_RECEIVE_RTP, 
                                 resourceName);

    // Send the message in the queue.
    OsStatus result = messageQueue.send(stopReceiveMsg);
    return(result);
}

// Stop receiving RTP and RTCP packets.
void MpRtpInputAudioConnection::handleStopReceiveRtp()
{
   prepareStopReceiveRtp();

   // No need to synchronize as the decoder is not part of the
   // flowgraph.  It is part of this connection/resource
   //mpFlowGraph->synchronize();


   mpDecode->deselectCodec();
   // No need to synchronize as the decoder is not part of the
   // flowgraph.  It is part of this connection/resource
   //mpFlowGraph->synchronize();

   mpDecode->disable();

   if (mpDecodeInBandDtmf)
   {
      mpDecodeInBandDtmf->disable();
   }   
}

void MpRtpInputAudioConnection::addPayloadType(int payloadType, MpDecoderBase* decoder)
{
   OsLock lock(mLock);

   // Check that payloadType is valid.
   if ((payloadType < 0) || (payloadType >= NUM_PAYLOAD_TYPES))
   {
      OsSysLog::add(FAC_MP, PRI_ERR,
                    "MpRtpInputAudioConnection::addPayloadType Attempting to add an invalid payload type %d", payloadType);
   }
   // Check to see if we already have a decoder for this payload type.
   else if (!(NULL == mpPayloadMap[payloadType]))
   {
      // This condition probably indicates that the sender of SDP specified
      // two decoders for the same payload type number.
      OsSysLog::add(FAC_MP, PRI_ERR,
                    "MpRtpInputAudioConnection::addPayloadType Attempting to add a second decoder for payload type %d",
                    payloadType);
   }
   else
   {
      mpPayloadMap[payloadType] = decoder;
   }
}

void MpRtpInputAudioConnection::deletePayloadType(int payloadType)
{
   OsLock lock(mLock);

   // Check that payloadType is valid.
   if ((payloadType < 0) || (payloadType >= NUM_PAYLOAD_TYPES))
   {
      OsSysLog::add(FAC_MP, PRI_ERR,
                    "MpRtpInputAudioConnection::deletePayloadType Attempting to delete an invalid payload type %d", payloadType);
   }
   // Check to see if this entry has already been deleted.
   else if (NULL == mpPayloadMap[payloadType])
   {
      // Either this payload type was doubly-added (and reported by
      // addPayloadType) or we've hit the race condtion in XMR-29.
      OsSysLog::add(FAC_MP, PRI_ERR,
                    "MpRtpInputAudioConnection::deletePayloadType Attempting to delete again payload type %d",
                    payloadType);
      OsSysLog::add(FAC_MP, PRI_ERR,
                    "MpRtpInputAudioConnection::deletePayloadType If there is no message from MpRtpInputAudioConnection::addPayloadType above, see XMR-29");
   }
   else
   {
      mpPayloadMap[payloadType] = NULL;
   }
}

void MpRtpInputAudioConnection::onNotify(UtlObservable* subject, int code, intptr_t userData)
{
   MpResNotificationType type = (MpResNotificationType)code;

   switch(type)
   {
   case MP_RES_DTMF_2833_NOTIFICATION:
      // DTMF notification received from MprDecode
      // tell audio connection to send connection notification
      sendConnectionNotification(MP_NOTIFICATION_DTMF_RFC2833, userData);
      break;
   case MP_RES_DTMF_INBAND_NOTIFICATION:
      // DTMF notification received from MprDecodeInBandDtmf
      sendConnectionNotification(MP_NOTIFICATION_DTMF_INBAND, userData);
      break;
   default:
      ;
   }
}

/* ============================ ACCESSORS ================================= */

MpDecoderBase* MpRtpInputAudioConnection::mapPayloadType(int payloadType)
{
   OsLock lock(mLock);

   if ((payloadType < 0) || (payloadType >= NUM_PAYLOAD_TYPES))
   {
      OsSysLog::add(FAC_MP, PRI_ERR,
                    "MpRtpInputAudioConnection::mapPayloadType Attempting to map an invalid payload type %d", payloadType);
      return NULL;
   }
   else
   {
      return mpPayloadMap[payloadType];
   }
}


/* ============================ INQUIRY =================================== */

/* //////////////////////////// PROTECTED ///////////////////////////////// */

void MpRtpInputAudioConnection::sendConnectionNotification(MpNotificationMsgType type, intptr_t data)
{
   if (m_pConnectionNotificationQueue && areNotificationsEnabled())
   {
      // create message and send it to connection notification queue
      OsIntPtrMsg connectionMsg(OsMsg::MP_CONNECTION_NOTF_MSG,
               (unsigned char)MP_NOTIFICATION_AUDIO,
               (int)type,
               mMyID,
               data);

      m_pConnectionNotificationQueue->send(connectionMsg, OsTime::NO_WAIT_TIME);
   }
}

/* //////////////////////////// PRIVATE /////////////////////////////////// */

/* ============================ FUNCTIONS ================================= */