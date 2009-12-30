//
// Copyright (C) 2007 stipus@stipus.com
//
// $$
///////////////////////////////////////////////////////////////////////////////

// SYSTEM INCLUDES
#include <assert.h>
#include <stdio.h>
#include <math.h>

#ifdef __pingtel_on_posix__
#include <stdlib.h>
#endif

// APPLICATION INCLUDES
#include "mp/MprDecodeInbandDtmf.h"
#include "mp/MpRtpInputAudioConnection.h"
#include "mp/MpResNotification.h"
#include "mp/MprRecorder.h"
#include "os/OsSysLog.h"

// DECODER DEFINES
#define PI  3.1415926
#define TwoPI 6.2831852
#define GOERTZEL_TRIGGER 21609000000.0  // square(147000.0)
                        // 

// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES

// STATIC VARIABLE INITIALIZATIONS

/* //////////////////////////// PUBLIC //////////////////////////////////// */

/* ============================ CREATORS ================================== */

// Constructor
MprDecodeInBandDtmf::MprDecodeInBandDtmf(const UtlString& rName,
                                         int samplesPerFrame,
                                         int samplesPerSec)
: MpAudioResource(rName, 0, 1, 1, 1, samplesPerFrame, samplesPerSec),
  m_dtmfLastDigit(1000), 
  m_sameDtmfDigitCount(0)
{
   Mk697 = 2 * cos(TwoPI * 697 / samplesPerSec);
   Mk770 = 2 * cos(TwoPI * 770 / samplesPerSec);
   Mk852 = 2 * cos(TwoPI * 852 / samplesPerSec);
   Mk941 = 2 * cos(TwoPI * 941 / samplesPerSec);
   Mk1209 = 2 * cos(TwoPI * 1209 / samplesPerSec);
   Mk1336 = 2 * cos(TwoPI * 1336 / samplesPerSec);
   Mk1477 = 2 * cos(TwoPI * 1477 / samplesPerSec);
   Mk1633 = 2 * cos(TwoPI * 1633 / samplesPerSec);
}

// Destructor
MprDecodeInBandDtmf::~MprDecodeInBandDtmf()
{
}

/* ============================ MANIPULATORS ============================== */

/* ============================ ACCESSORS ================================= */

/* ============================ INQUIRY =================================== */

/* //////////////////////////// PROTECTED ///////////////////////////////// */

/* //////////////////////////// PRIVATE /////////////////////////////////// */

UtlBoolean MprDecodeInBandDtmf::doProcessFrame(MpBufPtr inBufs[],
                                               MpBufPtr outBufs[],
                                               int inBufsSize,
                                               int outBufsSize,
                                               UtlBoolean isEnabled,
                                               int samplesPerFrame,
                                               int samplesPerSecond)
{
   // Don't process frame if disabled
   if (!isEnabled)
   {
      return TRUE;
   }

   // Check input buffer
   assert(inBufsSize == 1);
   if(!inBufs[0].isValid())
   {
      return TRUE;
   }

   // Get samples from buffer
   const MpAudioSample* input = ((MpAudioBufPtr)inBufs[0])->getSamplesPtr();
   int numSamples = ((MpAudioBufPtr)inBufs[0])->getSamplesNumber();

   // Initialize found digit to 'unknown'
   int dtmfDigit = 1000;

   // Calc goertzel for each searched frequency
   double d697 = Goertzel(input, numSamples, Mk697);
   double d770 = Goertzel(input, numSamples, Mk770);
   double d852 = Goertzel(input, numSamples, Mk852);
   double d941 = Goertzel(input, numSamples, Mk941);
   double d1209 = Goertzel(input, numSamples, Mk1209);
   double d1336 = Goertzel(input, numSamples, Mk1336);
   double d1477 = Goertzel(input, numSamples, Mk1477);
   double d1633 = Goertzel(input, numSamples, Mk1633);

   // CHECK WHICH DIGIT has been found
   if (d697 > GOERTZEL_TRIGGER && d1209 > GOERTZEL_TRIGGER)
      dtmfDigit = 1;
   else if (d697 > GOERTZEL_TRIGGER && d1336 > GOERTZEL_TRIGGER)
      dtmfDigit = 2;
   else if (d697 > GOERTZEL_TRIGGER && d1477 > GOERTZEL_TRIGGER)
      dtmfDigit = 3;
   else if (d697 > GOERTZEL_TRIGGER && d1633 > GOERTZEL_TRIGGER)
      dtmfDigit = 12; //'A';

   else if (d770 > GOERTZEL_TRIGGER && d1209 > GOERTZEL_TRIGGER)
      dtmfDigit = 4;
   else if (d770 > GOERTZEL_TRIGGER && d1336 > GOERTZEL_TRIGGER)
      dtmfDigit = 5;
   else if (d770 > GOERTZEL_TRIGGER && d1477 > GOERTZEL_TRIGGER)
      dtmfDigit = 6;
   else if (d770 > GOERTZEL_TRIGGER && d1633 > GOERTZEL_TRIGGER)
      dtmfDigit = 13; // 'B';

   else if (d852 > GOERTZEL_TRIGGER && d1209 > GOERTZEL_TRIGGER)
      dtmfDigit = 7;
   else if (d852 > GOERTZEL_TRIGGER && d1336 > GOERTZEL_TRIGGER)
      dtmfDigit = 8;
   else if (d852 > GOERTZEL_TRIGGER && d1477 > GOERTZEL_TRIGGER)
      dtmfDigit = 9;
   else if (d852 > GOERTZEL_TRIGGER && d1633 > GOERTZEL_TRIGGER)
      dtmfDigit = 14; // 'C';

   else if (d941 > GOERTZEL_TRIGGER && d1209 > GOERTZEL_TRIGGER)
      dtmfDigit = 10; // '*';
   else if (d941 > GOERTZEL_TRIGGER && d1336 > GOERTZEL_TRIGGER)
      dtmfDigit = 0;
   else if (d941 > GOERTZEL_TRIGGER && d1477 > GOERTZEL_TRIGGER)
      dtmfDigit = 11; // '#';
   else if (d941 > GOERTZEL_TRIGGER && d1633 > GOERTZEL_TRIGGER)
      dtmfDigit = 15; //'D';


   // TRIGGER NOTIFICATION
   // If a new digit has been found
   if (m_dtmfLastDigit == dtmfDigit)
   {
      m_sameDtmfDigitCount++;

      if (m_sameDtmfDigitCount == 4 && dtmfDigit != 1000)
      {
         notify(MP_RES_DTMF_INBAND_NOTIFICATION, dtmfDigit);
      }
   }
   else
   {
      if (dtmfDigit == 1000)
      {
         if (m_sameDtmfDigitCount >= 4)
         {
            m_sameDtmfDigitCount++;
         }
         else
         {
            m_sameDtmfDigitCount = 0;
         }

         if (m_sameDtmfDigitCount >= 30)
         {
            m_dtmfLastDigit = dtmfDigit;
            m_sameDtmfDigitCount = 0;
         }
      }
      else
      {
         m_dtmfLastDigit = dtmfDigit;
         m_sameDtmfDigitCount = 0;
      }
   }
   return TRUE;
}

double MprDecodeInBandDtmf::Goertzel(const MpAudioSample *input, int numsamples, double mk)
{
   double Qkn = 0;
   double Qkn1 = 0;
   double Qkn2;

   for(int i = 0; i < numsamples ; i++ )
   {
      Qkn2 = Qkn1;
      Qkn1 = Qkn;
      Qkn = input[i] + mk * Qkn1 - Qkn2;
   }

   // Real Goertzl should return sqrt()
   // Result is compared to square( trigger )
   return Qkn * Qkn + Qkn1 * Qkn1 - mk * Qkn * Qkn1;
}

UtlBoolean MprDecodeInBandDtmf::handleMessage(MpFlowGraphMsg& rMsg)
{
   switch (rMsg.getMsg())
   {
   case 0: // to make compiler stop complaining
   default:
      return MpAudioResource::handleMessage(rMsg);
      break;
   }
   return TRUE;
}

/* ============================ FUNCTIONS ================================= */

