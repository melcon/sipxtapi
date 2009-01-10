//
// Copyright (C) 2007 stipus@stipus.com
//
// $$
///////////////////////////////////////////////////////////////////////////////

#ifndef _MprDecodeInBandDtmf_h_
#define _MprDecodeInBandDtmf_h_

// APPLICATION INCLUDES
#include <mp/MprDtmfDetectorBase.h>

// DEFINES
// MACROS
// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
// STRUCTS
// TYPEDEFS
// FORWARD DECLARATIONS

class MpRtpInputAudioConnection;

/**
*  Simple inband DTMF detector using Goertzel algorithm from wikipedia.
*/
class MprSimpleDtmfDetector : public MprDtmfDetectorBase
{
/* //////////////////////////// PUBLIC //////////////////////////////////// */
public:

   friend class MpRtpInputAudioConnection;

/* ============================ CREATORS ================================== */
///@name Creators
//@{

     //:Constructor
   MprSimpleDtmfDetector(const UtlString& rName, 
                       int samplesPerFrame,
                       int samplesPerSec);

     //:Destructor
   virtual ~MprSimpleDtmfDetector();

//@}

/* ============================ MANIPULATORS ============================== */
///@name Manipulators
//@{

//@}

/* ============================ ACCESSORS ================================= */
///@name Accessors
//@{

//@}

/* ============================ INQUIRY =================================== */
///@name Inquiry
//@{

//@}

/* //////////////////////////// PROTECTED ///////////////////////////////// */
protected:

   virtual UtlBoolean doProcessFrame(MpBufPtr inBufs[],
                                     MpBufPtr outBufs[],
                                     int inBufsSize,
                                     int outBufsSize,
                                     UtlBoolean isEnabled,
                                     int samplesPerFrame = 80,
                                     int samplesPerSecond = 8000);

/* //////////////////////////// PRIVATE /////////////////////////////////// */
private:

   int m_dtmfLastDigit;
   int m_sameDtmfDigitCount;
   double Mk697;
   double Mk770;
   double Mk852;
   double Mk941;
   double Mk1209;
   double Mk1336;
   double Mk1477;
   double Mk1633;

   /// Goertzel
   double Goertzel(const MpAudioSample *input, int numsamples, double mk);

   /// Handle messages for this resource.
   virtual UtlBoolean handleMessage(MpFlowGraphMsg& rMsg);

     /// Copy constructor (not implemented for this class)
   MprSimpleDtmfDetector(const MprSimpleDtmfDetector& rMprDecodeInBandDtmf);

     /// Assignment operator (not implemented for this class)
   MprSimpleDtmfDetector& operator=(const MprSimpleDtmfDetector& rhs);

};

/* ============================ INLINE METHODS ============================ */

#endif  // _MprDecodeInBandDtmf_h_
