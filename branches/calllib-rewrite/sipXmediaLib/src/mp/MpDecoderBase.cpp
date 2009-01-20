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

#include <assert.h>
#include <mp/MpDefs.h>
#include "mp/MpDecoderBase.h"

// Constructor
// Returns a new decoder object.
// param: payloadType - (in) RTP payload type associated with this decoder
MpDecoderBase::MpDecoderBase(int payloadType, const MpCodecInfo* pInfo)
: mpCodecInfo(pInfo)
, mPayloadType(payloadType)
, m_pNoiseState(NULL)
{
   m_pNoiseState = noise_init_dbm0(NULL, 6513, NOISE_LEVEL, NOISE_CLASS_HOTH, 5);
 // initializers do it all!
}

//Destructor
MpDecoderBase::~MpDecoderBase()
{
   if (m_pNoiseState)
   {
      free((void*)m_pNoiseState);
      m_pNoiseState = NULL;
   }
}

/* ============================ MANIPULATORS ============================== */

/* ============================ ACCESSORS ================================= */

// Get static information about the decoder
// Returns a pointer to an <i>MpCodecInfo</i> object that provides
// static information about the decoder.
const MpCodecInfo* MpDecoderBase::getInfo(void) const
{
   return(mpCodecInfo);
}


// Returns the RTP payload type associated with this decoder.
int MpDecoderBase::getPayloadType(void)
{
   return(mPayloadType);
}

/* ============================ INQUIRY =================================== */

/* //////////////////////////// PROTECTED ///////////////////////////////// */

void MpDecoderBase::generateComfortNoise(MpAudioSample *samplesBuffer, unsigned sampleCount)
{
   UtlBoolean bGenerated = FALSE;
#ifdef HAVE_SPAN_DSP
   if (m_pNoiseState && samplesBuffer)
   {
      for (int i = 0; i < sampleCount; i++)
      {
         samplesBuffer[i] = noise(m_pNoiseState); // generate 1 sample of noise
      }
      bGenerated = TRUE;
   }
#endif

   if (!bGenerated)
   {
      // set all 0s
      memset(samplesBuffer, 0, sampleCount * sizeof(MpAudioSample));
   }
}
