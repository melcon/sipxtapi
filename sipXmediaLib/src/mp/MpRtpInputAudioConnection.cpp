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
#include <mp/MpMediaTask.h>
#include <mp/MpRtpInputAudioConnection.h>
#include <mp/MpFlowGraphBase.h>
#include <mp/MprFromNet.h>
#include <mp/MprDejitter.h>
#include <mp/MpJitterBuffer.h>
#include <mp/MprDecode.h>
#include <mp/MpResourceMsg.h>
#include <mp/MprRtpStartReceiveMsg.h>
#include "mp/MprDecodeInbandDtmf.h"
#include <sdp/SdpCodec.h>
#include <os/OsLock.h>
#include "os/OsIntPtrMsg.h"
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
{
   char         name[50];
   int          i;

   sprintf(name, "Decode-%d", myID);
   mpDecode    = new MprDecode(name, this, samplesPerFrame, samplesPerSec);
   sprintf(name, "DecodeInBandDtmf-%d", myID);
   mpDecodeInBandDtmf = new MprDecodeInBandDtmf(name, this, samplesPerFrame, samplesPerSec);

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
    
		assert(mpDecodeInBandDtmf);
		if( mpDecodeInBandDtmf )
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
   mpDecodeInBandDtmf->disable();
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
   mpDecodeInBandDtmf->enable();
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
       mpDecode->selectCodecs(pCodecs, numCodecs);       
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
	   mpDecodeInBandDtmf->enable();
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
   mpDecodeInBandDtmf->disable();
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
               data,
               mMyID);

      m_pConnectionNotificationQueue->send(connectionMsg, OsTime::NO_WAIT_TIME);
   }
}

/* //////////////////////////// PRIVATE /////////////////////////////////// */

/* ============================ FUNCTIONS ================================= */
