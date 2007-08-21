//
// Copyright (C) 2007 stipus@stipus.com
//
// $$
///////////////////////////////////////////////////////////////////////////////

#ifndef _MprDecodeInBandDtmf_h_
#define _MprDecodeInBandDtmf_h_

// APPLICATION INCLUDES
#include "mp/MpFlowGraphMsg.h"
#include "mp/MpAudioResource.h"

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
*  @brief The "Audio from file" media processing resource
*/
class MprDecodeInBandDtmf : public MpAudioResource
{
/* //////////////////////////// PUBLIC //////////////////////////////////// */
public:

   friend class MpRtpInputAudioConnection;

/* ============================ CREATORS ================================== */
///@name Creators
//@{

     //:Constructor
   MprDecodeInBandDtmf(const UtlString& rName, 
                       MpRtpInputAudioConnection* pConn, 
                       int samplesPerFrame,
                       int samplesPerSec);

     //:Destructor
   virtual ~MprDecodeInBandDtmf();

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

/* //////////////////////////// PRIVATE /////////////////////////////////// */
private:

   MpRtpInputAudioConnection* m_pConnection;   // Link to the parent Connection.
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

   virtual UtlBoolean doProcessFrame(MpBufPtr inBufs[],
                                     MpBufPtr outBufs[],
                                     int inBufsSize,
                                     int outBufsSize,
                                     UtlBoolean isEnabled,
                                     int samplesPerFrame = 80,
                                     int samplesPerSecond = 8000);

   /// Goertzel
   double Goertzel(const MpAudioSample *input, int numsamples, double mk);

   /// Handle messages for this resource.
   virtual UtlBoolean handleMessage(MpFlowGraphMsg& rMsg);

     /// Copy constructor (not implemented for this class)
   MprDecodeInBandDtmf(const MprDecodeInBandDtmf& rMprDecodeInBandDtmf);

     /// Assignment operator (not implemented for this class)
   MprDecodeInBandDtmf& operator=(const MprDecodeInBandDtmf& rhs);

};

/* ============================ INLINE METHODS ============================ */

#endif  // _MprDecodeInBandDtmf_h_
