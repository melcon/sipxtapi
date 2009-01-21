//
// Copyright (C) 2007 SIPez LLC.
// Licensed to SIPfoundry under a Contributor Agreement.
//  
// Copyright (C) 2007 SIPfoundry Inc. 
// Licensed by SIPfoundry under the LGPL license. 
//  
// $$ 
////////////////////////////////////////////////////////////////////////////// 

#ifdef HAVE_ILBC // [

// WIN32: Add libilbc to linker input.
#ifdef WIN32 // [
#   ifdef _DEBUG // [
#      pragma comment(lib, "libilbcd.lib")
#   else // _DEBUG ][
#      pragma comment(lib, "libilbc.lib")
#   endif // _DEBUG ]
#endif // WIN32 ]

// SYSTEM INCLUDES
#include <limits.h>

// APPLICATION INCLUDES
#include "mp/MpdSipxILBC.h"
extern "C" {
#include <iLBC_define.h>
#include <iLBC_decode.h>
}

const MpCodecInfo MpdSipxILBC::smCodecInfo30ms(
    SdpCodec::SDP_CODEC_ILBC,   // codecType
    "iLBC",                     // codecVersion
    8000,                       // samplingRate
    16,                          // numBitsPerSample (not used)
    1,                          // numChannels
    13334,                      // bitRate
    NO_OF_BYTES_30MS * 8,       // minPacketBits
    NO_OF_BYTES_30MS * 8,       // maxPacketBits
    240);                        // numSamplesPerFrame

const MpCodecInfo MpdSipxILBC::smCodecInfo20ms(
   SdpCodec::SDP_CODEC_ILBC_20MS,   // codecType
   "iLBC",                     // codecVersion
   8000,                       // samplingRate
   16,                          // numBitsPerSample (not used)
   1,                          // numChannels
   13334,                      // bitRate
   NO_OF_BYTES_20MS * 8,       // minPacketBits
   NO_OF_BYTES_20MS * 8,       // maxPacketBits
   160);                        // numSamplesPerFrame

MpdSipxILBC::MpdSipxILBC(int payloadType, int mode)
: MpDecoderBase(payloadType, mode == 20 ? &smCodecInfo20ms : &smCodecInfo30ms)
, m_mode(mode)
, mpState(NULL)
, m_samplesPerFrame(0)
, m_packetBytes(0)
{
   m_samplesPerFrame = getInfo()->getNumSamplesPerFrame();
   m_packetBytes = getInfo()->getMaxPacketBits() / 8;
}

MpdSipxILBC::~MpdSipxILBC()
{
   freeDecode();
}

OsStatus MpdSipxILBC::initDecode()
{
   if (mpState == NULL) 
   {
      mpState = new iLBC_Dec_Inst_t();
      memset(mpState, 0, sizeof(*mpState));
      ::initDecode(mpState, m_mode, 1);
   }
   return OS_SUCCESS;
}

OsStatus MpdSipxILBC::freeDecode(void)
{
   delete mpState;
   mpState = NULL;
   return OS_SUCCESS;
}

int MpdSipxILBC::decode(const MpRtpBufPtr &pPacket,
                        unsigned decodedBufferLength,
                        MpAudioSample *samplesBuffer,
                        UtlBoolean bIsPLCFrame)
{
   // Check if available buffer size is enough for the packet.
   if (decodedBufferLength < m_samplesPerFrame)
   {
      osPrintf("MpdSipxILBC::decode: Jitter buffer overloaded. Glitch!\n");
      return 0;
   }

   // Decode incoming packet to temp buffer. If no packet - do PLC.
   float buffer[240]; // use buffer for 30ms frames, even for 20ms
   if (pPacket.isValid())
   {
      if (m_packetBytes != pPacket->getPayloadSize())
      {
         osPrintf("MpdSipxILBC::decode: Payload size: %d!\n", pPacket->getPayloadSize());
         return 0;
      }

      // Packet data available. Decode it.
      iLBC_decode(buffer, (unsigned char*)pPacket->getDataPtr(), mpState, 1);
   }
   else
   {
      // Packet data is not available. Do PLC.
      iLBC_decode(buffer, NULL, mpState, 0);
   }
   
   for (unsigned int i = 0; i < m_samplesPerFrame; ++i)
   {
      float tmp = buffer[i];
      if (tmp > SHRT_MAX)
         tmp = SHRT_MAX;
      if (tmp < SHRT_MIN)
         tmp = SHRT_MIN;

      samplesBuffer[i] = MpAudioSample(tmp + 0.5f);
   }

   return m_samplesPerFrame;
}

#endif // HAVE_ILBC ]
