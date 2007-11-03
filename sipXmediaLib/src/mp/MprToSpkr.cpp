//  
// Copyright (C) 2006 SIPez LLC. 
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
#include <assert.h>

// APPLICATION INCLUDES

#include <os/OsMsg.h>
#include <os/OsMsgPool.h>

#include "mp/MpBuf.h"
#include "mp/MprToSpkr.h"
#include "mp/MpBufferMsg.h"
#include "mp/dsplib.h"
#include "mp/MpMediaTask.h"

int iTrainingNoiseFlag = 0;
static int iComfortNoiseFlag = 1;

// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
// STATIC VARIABLE INITIALIZATIONS

/* //////////////////////////// PUBLIC //////////////////////////////////// */

/* ============================ CREATORS ================================== */

// Constructor
MprToSpkr::MprToSpkr(const UtlString& rName,
                     int samplesPerFrame,
                     int samplesPerSec,
                     OsMsgQ *pEchoQ)
:  MpAudioResource(rName, 1, 1, 0, 1, samplesPerFrame, samplesPerSec)
,  mpEchoQ(pEchoQ)
,  mulNoiseLevel(1000L)
{
   init_CNG();
}

// Destructor
MprToSpkr::~MprToSpkr()
{
}

/* ============================ MANIPULATORS ============================== */

/* ============================ ACCESSORS ================================= */

/* ============================ INQUIRY =================================== */

/* //////////////////////////// PROTECTED ///////////////////////////////// */

/* //////////////////////////// PRIVATE /////////////////////////////////// */

UtlBoolean MprToSpkr::doProcessFrame(MpBufPtr inBufs[],
                                     MpBufPtr outBufs[],
                                     int inBufsSize,
                                     int outBufsSize,
                                     UtlBoolean isEnabled,
                                     int samplesPerFrame,
                                     int samplesPerSecond)
{
    MpAudioBufPtr out;
    MpAudioSample* shpSamples;
    int iLength;

    // We have only one input
    if (inBufsSize != 1)
    {      
        return FALSE;
    }

    // Do processing if enabled and if data is available
    if (isEnabled && inBufs[0].isValid()) 
    {

        // Own input buffer
        out = inBufs[0];
        inBufs[0].release();

        shpSamples = out->getSamplesWritePtr();
        iLength = out->getSamplesNumber();

        /////////////////////////////////////////////////
        // samples ready for EQ processing //////////////
        /////////////////////////////////////////////////

        if(iTrainingNoiseFlag > 0) 
        {
            /* generate white noise to test the performance if AEC only.
             * This is for parameter tweaking only. The original speaker
             * signal will be dropped.
             */
            out->setSamplesNumber(samplesPerFrame);
            iLength = out->getSamplesNumber();
            white_noise_generator(shpSamples, iLength, iTrainingNoiseFlag);
        }
        else
        {
            if(out->getSpeechType() == MpAudioBuf::MP_SPEECH_COMFORT_NOISE) 
            {
                out->setSamplesNumber(samplesPerFrame);
                iLength = out->getSamplesNumber();
                if(iComfortNoiseFlag > 0) 
                {
                    comfort_noise_generator(shpSamples, iLength, mulNoiseLevel);
                }
                else 
                {
                    memset((char *)shpSamples, 0 , iLength*2);                     
                }
            }
            else 
            {
                background_noise_level_estimation(mulNoiseLevel, shpSamples, 
                        iLength);
            }
        }

        // TODO:: I don't know why we set attenuation to 0 here. BTW, it used only in the MprEchoSuppress().
        out->setAttenDb(0);

        // Push data to the output, if connected.
        if (isOutputConnected(0))
        {
            outBufs[0] = out;
        }

         // Post a copy of this message to the mpEchoQ so that it
         // can be used in AEC calculations.
         MpBufferMsg AECMsg(MpBufferMsg::ACK_EOSTREAM);

         // TODO: We should pre-allocate a bunch of messages for 
         //       this purpose (see DmaMsgPool as an example).
          
         // Buffer is moved to the message. ob pointer is invalidated.
         AECMsg.ownBuffer(out) ;         
         if (  mpEchoQ->numMsgs() >= mpEchoQ->maxMsgs()
            || mpEchoQ->send(AECMsg, OsTime::NO_WAIT_TIME) != OS_SUCCESS)
         {
         }
    }
    else 
    {
        // Push data to the output, if connected.
        if (isOutputConnected(0))
        {
            outBufs[0] = inBufs[0];
        }
    }

   return TRUE;
}

/* ============================ FUNCTIONS ================================= */
