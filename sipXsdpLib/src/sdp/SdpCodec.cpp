//
// Copyright (C) 2004-2007 SIPfoundry Inc.
// Licensed by SIPfoundry under the LGPL license.
//
// Copyright (C) 2004-2007 Pingtel Corp.  All rights reserved.
// Licensed to SIPfoundry under a Contributor Agreement.
//
// $$
///////////////////////////////////////////////////////////////////////////////

// SYSTEM INCLUDES

// APPLICATION INCLUDES
#include <sdp/SdpCodec.h>

// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
// STATIC VARIABLE INITIALIZATIONS

/* //////////////////////////// PUBLIC //////////////////////////////////// */

/* ============================ CREATORS ================================== */

// Constructor
SdpCodec::SdpCodec(enum SdpCodecTypes sdpCodecType,
                   int payloadId,
                   const char* mimeType,
                   const char* mimeSubtype,
                   int sampleRate,
                   int preferredPacketLength,
                   int numChannels,
                   const char* formatSpecificData,
                   const int CPUCost,
                   const int BWCost,
                   const int videoFormat,
                   const int videoFmtp) :
   mCodecPayloadId(payloadId),
   mMimeType(mimeType),
   mMimeSubtype(mimeSubtype),
   mSampleRate(sampleRate),
   mPacketLength(preferredPacketLength),
   mNumChannels(numChannels),
   mFormatSpecificData(formatSpecificData),
   mCPUCost(CPUCost),
   mBWCost(BWCost),
   mVideoFormat(videoFormat),
   mVideoFmtp(videoFmtp)
{
   mMimeSubtype.toLower();
   mMimeType.toLower();
   setValue(sdpCodecType);
}

SdpCodec::SdpCodec(int payloadId,
                   const char* mimeType,
                   const char* mimeSubtype,
                   int sampleRate, // samples per second
                   int preferredPacketLength, // micro seconds
                   int numChannels,
                   const char* formatSpecificData) :
   mCodecPayloadId(payloadId),
   mMimeType(mimeType),
   mMimeSubtype(mimeSubtype),
   mSampleRate(sampleRate),
   mPacketLength(preferredPacketLength),
   mNumChannels(numChannels),
   mFormatSpecificData(formatSpecificData),
   mCPUCost(SDP_CODEC_CPU_LOW),
   mBWCost(SDP_CODEC_BANDWIDTH_NORMAL),
   mVideoFormat(SDP_VIDEO_FORMAT_QCIF),
   mVideoFmtp(0)
{
   mMimeSubtype.toLower();
   mMimeType.toLower();

   // !slg! Note:  CPU and BW Costs copied from SdpCodecFactory
   if(mMimeType.compareTo("audio") == 0)
   {
      if(mMimeSubtype.compareTo("pcmu") == 0)
      {
         //setValue(SDP_CODEC_PCMU);
         setValue(SDP_CODEC_GIPS_PCMU); // ?slg? Use GIPS?
      }
      else if(mMimeSubtype.compareTo("pcma") == 0)
      {
         //setValue(SDP_CODEC_PCMA);
         setValue(SDP_CODEC_GIPS_PCMA); // ?slg? Use GIPS?
      }
      else if(mMimeSubtype.compareTo("gsm") == 0)
      {
         setValue(SDP_CODEC_GSM);
         mCPUCost = SDP_CODEC_CPU_HIGH;
         mBWCost = SDP_CODEC_BANDWIDTH_LOW;
      }
      else if(mMimeSubtype.compareTo("g723") == 0)
      {
         setValue(SDP_CODEC_G723);
         mCPUCost = SDP_CODEC_CPU_HIGH;
         mBWCost = SDP_CODEC_BANDWIDTH_LOW;
      }
      else if(mMimeSubtype.compareTo("l16") == 0)
      {
         if(mNumChannels == 2)
         {
            setValue(SDP_CODEC_L16_STEREO);
         }
         else
         {
            setValue(SDP_CODEC_L16_MONO);
         }
      }
      else if(mMimeSubtype.compareTo("pcm") == 0)
      {
         setValue(SDP_CODEC_L16_8K);
      }
      else if(mMimeSubtype.compareTo("g729") == 0)
      {
         setValue(SDP_CODEC_G729);
         mCPUCost = SDP_CODEC_CPU_HIGH;
         mBWCost = SDP_CODEC_BANDWIDTH_LOW;
      }
      else if(mMimeSubtype.compareTo("telephone-event") == 0)
      {
         setValue(SDP_CODEC_TONES);
         mBWCost = SDP_CODEC_BANDWIDTH_LOW;
      }
      else if(mMimeSubtype.compareTo("ilbc") == 0)
      {
         setValue(SDP_CODEC_ILBC);
         mCPUCost = SDP_CODEC_CPU_HIGH;
         mBWCost = SDP_CODEC_BANDWIDTH_LOW;
      }
      else if(mMimeSubtype.compareTo("speex") == 0)
      {
         if(mFormatSpecificData.compareTo("mode=2", UtlString::ignoreCase) == 0)
         {
            setValue(SDP_CODEC_SPEEX_5);  // Speex Profile 1
            mBWCost = SDP_CODEC_BANDWIDTH_LOW;
         }
         else if(mFormatSpecificData.compareTo("mode=5", UtlString::ignoreCase) == 0)
         {
            setValue(SDP_CODEC_SPEEX_15);  // Speex Profile 2
            mBWCost = SDP_CODEC_BANDWIDTH_NORMAL;
         }
         else if(mFormatSpecificData.compareTo("mode=7", UtlString::ignoreCase) == 0)
         {
            setValue(SDP_CODEC_SPEEX_24);  // Speex Profile 3
            mBWCost = SDP_CODEC_BANDWIDTH_NORMAL;
         }
         else
         {
            // mode = 3
            setValue(SDP_CODEC_SPEEX);
            mBWCost = SDP_CODEC_BANDWIDTH_LOW;
         }
      }
      else
      {
         setValue(SDP_CODEC_UNKNOWN);
      }
   }
   else if(mMimeType.compareTo("video") == 0)
   {
       unsigned int index = 0;
       unsigned int start = 0;
       UtlString temp;
       const char* formatSpecificData = mFormatSpecificData.data();
       // !slg! logic taken from SdpBody.cpp - but logic in SdpBody doesn't look complete
       for(index = 0; index <= mFormatSpecificData.length(); index++)
       {
          if(formatSpecificData[index] == ' ' ||
             formatSpecificData[index] == '\t' ||
             formatSpecificData[index] == '/' ||
             formatSpecificData[index] == ':' ||
             formatSpecificData[index] == '=' ||
             formatSpecificData[index] == '\0')
          {
             if(start != index)
             {
                temp.resize(0);
                temp.append(&formatSpecificData[start], index-start);
                if (temp.compareTo("CIF", UtlString::ignoreCase) == 0)
                {
                   if(mVideoFmtp == 0)
                   {
                      mVideoFormat = SDP_VIDEO_FORMAT_CIF;
                   }
                   mVideoFmtp |= SDP_VIDEO_FORMAT_CIF;
                }
                else if (temp.compareTo("QCIF", UtlString::ignoreCase) == 0)
                {
                   if(mVideoFmtp == 0)
                   {
                      mVideoFormat = SDP_VIDEO_FORMAT_QCIF;
                   }
                   mVideoFmtp |= SDP_VIDEO_FORMAT_QCIF;
                }
                else if (temp.compareTo("SQCIF", UtlString::ignoreCase) == 0)
                {
                   if(mVideoFmtp == 0)
                   {
                      mVideoFormat = SDP_VIDEO_FORMAT_SQCIF;
                   }
                   mVideoFmtp |= SDP_VIDEO_FORMAT_SQCIF;
                }
             }
             start = index+1;
          }
       }  

      if(mMimeSubtype.compareTo("vp71") == 0)
      {
         switch(mVideoFormat)
         {
         case SDP_VIDEO_FORMAT_CIF:
            setValue(SDP_CODEC_VP71_CIF);
            break;
         case SDP_VIDEO_FORMAT_QCIF:
            setValue(SDP_CODEC_VP71_QCIF);
            break;
         case SDP_VIDEO_FORMAT_SQCIF:
            setValue(SDP_CODEC_VP71_SQCIF);
            break;
         case SDP_VIDEO_FORMAT_QVGA:
            setValue(SDP_CODEC_VP71_QVGA);
            break;
         default:
            setValue(SDP_CODEC_VP71_CIF);
            break;
         }
         mBWCost = SDP_CODEC_BANDWIDTH_NORMAL;
      }
      else if(mMimeSubtype.compareTo("iyuv") == 0)
      {
         switch(mVideoFormat)
         {
         case SDP_VIDEO_FORMAT_CIF:
            setValue(SDP_CODEC_IYUV_CIF);
            break;
         case SDP_VIDEO_FORMAT_QCIF:
            setValue(SDP_CODEC_IYUV_QCIF);
            break;
         case SDP_VIDEO_FORMAT_SQCIF:
            setValue(SDP_CODEC_IYUV_SQCIF);
            break;
         case SDP_VIDEO_FORMAT_QVGA:
            setValue(SDP_CODEC_IYUV_QVGA);
            break;
         default:
            setValue(SDP_CODEC_IYUV_CIF);
            break;
         }
         mBWCost = SDP_CODEC_BANDWIDTH_HIGH;
      }
      else if(mMimeSubtype.compareTo("i420") == 0)
      {
         switch(mVideoFormat)
         {
         case SDP_VIDEO_FORMAT_CIF:
            setValue(SDP_CODEC_I420_CIF);
            break;
         case SDP_VIDEO_FORMAT_QCIF:
            setValue(SDP_CODEC_I420_QCIF);
            break;
         case SDP_VIDEO_FORMAT_SQCIF:
            setValue(SDP_CODEC_I420_SQCIF);
            break;
         case SDP_VIDEO_FORMAT_QVGA:
            setValue(SDP_CODEC_I420_QVGA);
            break;
         default:
            setValue(SDP_CODEC_I420_CIF);
            break;
         }
         mBWCost = SDP_CODEC_BANDWIDTH_HIGH;
      }
      else if(mMimeSubtype.compareTo("rgb24") == 0)
      {
         switch(mVideoFormat)
         {
         case SDP_VIDEO_FORMAT_CIF:
            setValue(SDP_CODEC_RGB24_CIF);
            break;
         case SDP_VIDEO_FORMAT_QCIF:
            setValue(SDP_CODEC_RGB24_QCIF);
            break;
         case SDP_VIDEO_FORMAT_SQCIF:
            setValue(SDP_CODEC_RGB24_SQCIF);
            break;
         case SDP_VIDEO_FORMAT_QVGA:
            setValue(SDP_CODEC_RGB24_QVGA);
            break;
         default:
            setValue(SDP_CODEC_RGB24_CIF);
            break;
         }
         mBWCost = SDP_CODEC_BANDWIDTH_HIGH;
      }
      else if(mMimeSubtype.compareTo("h263") == 0)
      {
         switch(mVideoFormat)
         {
         case SDP_VIDEO_FORMAT_CIF:
            setValue(SDP_CODEC_H263_CIF);
            break;
         case SDP_VIDEO_FORMAT_QCIF:
            setValue(SDP_CODEC_H263_QCIF);
            break;
         case SDP_VIDEO_FORMAT_SQCIF:
            setValue(SDP_CODEC_H263_SQCIF);
            break;
         case SDP_VIDEO_FORMAT_QVGA:
            setValue(SDP_CODEC_H263_QVGA);
            break;
         default:
            setValue(SDP_CODEC_H263_CIF);
            break;
         }
         mBWCost = SDP_CODEC_BANDWIDTH_NORMAL;
      }
      else
      {
         setValue(SDP_CODEC_UNKNOWN);
      }
   }
   else
   {
      setValue(SDP_CODEC_UNKNOWN);
   }
}

// Copy constructor
SdpCodec::SdpCodec(const SdpCodec& rSdpCodec)
{
    setValue(rSdpCodec.getValue());
    mCodecPayloadId = rSdpCodec.mCodecPayloadId;
    mSampleRate = rSdpCodec.mSampleRate;
    mPacketLength = rSdpCodec.mPacketLength;
    mNumChannels = rSdpCodec.mNumChannels;
    mMimeType = rSdpCodec.mMimeType;
    mMimeSubtype = rSdpCodec.mMimeSubtype;
    mFormatSpecificData = rSdpCodec.mFormatSpecificData;
    mCPUCost  = rSdpCodec.mCPUCost;
    mBWCost  = rSdpCodec.mBWCost;
    mVideoFormat = rSdpCodec.mVideoFormat;
    mVideoFmtp = rSdpCodec.mVideoFmtp;
    mVideoFmtpString = rSdpCodec.mVideoFmtpString;
}

// Destructor
SdpCodec::~SdpCodec()
{
}

/* ============================ MANIPULATORS ============================== */

// Assignment operator
SdpCodec&
SdpCodec::operator=(const SdpCodec& rhs)
{

   if (this == &rhs)            // handle the assignment to self case
      return *this;
    setValue(rhs.getValue());
    mCodecPayloadId = rhs.mCodecPayloadId;
    mSampleRate = rhs.mSampleRate;
    mPacketLength = rhs.mPacketLength;
    mNumChannels = rhs.mNumChannels;
    mMimeType = rhs.mMimeType;
    mMimeSubtype = rhs.mMimeSubtype;
    mFormatSpecificData = rhs.mFormatSpecificData;
    mCPUCost  = rhs.mCPUCost;
    mBWCost = rhs.mBWCost;
    mVideoFormat = rhs.mVideoFormat;
    mVideoFmtp = rhs.mVideoFmtp;
    mVideoFmtpString = rhs.mVideoFmtpString;

   return *this;
}

/* ============================ ACCESSORS ================================= */

SdpCodec::SdpCodecTypes SdpCodec::getCodecType() const
{
   return((enum SdpCodecTypes) getValue());
}

int SdpCodec::getCodecPayloadId() const
{
    return(mCodecPayloadId);
}

void SdpCodec::setCodecPayloadId(int formatId)
{
    mCodecPayloadId = formatId;
}

void SdpCodec::getSdpFmtpField(UtlString& formatSpecificData) const
{
    formatSpecificData = mFormatSpecificData;
}

void SdpCodec::getMediaType(UtlString& mimeType) const
{
    mimeType = mMimeType;
}

void SdpCodec::getEncodingName(UtlString& mimeSubtype) const
{
    mimeSubtype = mMimeSubtype;
}

int SdpCodec::getSampleRate() const // samples per second
{
    return(mSampleRate);
}

int SdpCodec::getVideoFormat() const // samples per second
{
    return(mVideoFormat);
}

int SdpCodec::getPacketLength() const //micro seconds
{
    return(mPacketLength);
}

int SdpCodec::getNumChannels() const
{
    return(mNumChannels);
}

void SdpCodec::setVideoFmtp(const int videoFmtp)
{
    mVideoFmtp = videoFmtp;
}

int SdpCodec::getVideoFmtp() const
{
    return mVideoFmtp;
}

void SdpCodec::setPacketSize(const int packetSize)
{
    mPacketLength = packetSize;
}

void SdpCodec::getVideoFmtpString(UtlString& fmtpString) const
{
    fmtpString = mVideoFmtpString;
}

void SdpCodec::setVideoFmtpString(int videoFmtp)
{
    UtlString tempFmtp(NULL);

    switch (videoFmtp)
    {
    case SDP_VIDEO_FORMAT_SQCIF:
        mVideoFmtpString.append("SQCIF/");
        break;
    case SDP_VIDEO_FORMAT_QCIF:
        mVideoFmtpString.append("QCIF/");
        break;
    case SDP_VIDEO_FORMAT_CIF:
        mVideoFmtpString.append("CIF/");
        break;
    case SDP_VIDEO_FORMAT_QVGA:
        mVideoFmtpString.append("QVGA/");
        break;
    default:
        break;
    }
}

void SdpCodec::clearVideoFmtpString()
{
    mVideoFmtpString = "";
}

void SdpCodec::toString(UtlString& sdpCodecContents) const
{
    char stringBuffer[256];
    SNPRINTF(stringBuffer, sizeof(stringBuffer), "SdpCodec: codecId=%d, payloadId=%d, mime=\'%s/%s\', rate=%d, pktLen=%d, numCh=%d, fmtData=\'%s\'\n",
            getValue(), mCodecPayloadId,
            mMimeType.data(), mMimeSubtype.data(),
            mSampleRate, mPacketLength, mNumChannels,
            mFormatSpecificData.data());
    sdpCodecContents = stringBuffer;
}

// Get the CPU cost for this codec.
int SdpCodec::getCPUCost() const
{
   return mCPUCost;
}

// Get the bandwidth cost for this codec.
int SdpCodec::getBWCost() const
{
   return mBWCost;
}

/* ============================ INQUIRY =================================== */

UtlBoolean SdpCodec::isSameDefinition(const SdpCodec& codec) const
{
    return(mSampleRate == codec.mSampleRate &&
           mNumChannels == codec.mNumChannels &&
           mMimeType.compareTo(codec.mMimeType, UtlString::ignoreCase) == 0 &&
           mMimeSubtype.compareTo(codec.mMimeSubtype, UtlString::ignoreCase) == 0);
}

SdpCodec::SdpCodecTypes SdpCodec::getCodecType(const UtlString& shortCodecName)
{
    SdpCodec::SdpCodecTypes retType = SdpCodec::SDP_CODEC_UNKNOWN;
    UtlString compareString(shortCodecName);
    compareString.toUpper();

    if (strcmp(compareString,"TELEPHONE-EVENT") == 0 ||
       strcmp(compareString,"AUDIO/TELEPHONE-EVENT") == 0 || 
       strcmp(compareString,"128") == 0 ||
       strcmp(compareString,"AVT-TONES") == 0 ||
       strcmp(compareString,"AVT") == 0)
        retType = SdpCodec::SDP_CODEC_TONES;
    else
    if (strcmp(compareString,"PCMU") == 0 ||
       strcmp(compareString,"G711U") == 0 || 
       strcmp(compareString,"0") == 0 ||
       strcmp(compareString,"258") == 0)
        retType = SdpCodec::SDP_CODEC_GIPS_PCMU;
    else
    if (strcmp(compareString,"PCMA") == 0 ||
       strcmp(compareString,"G711A") == 0 || 
       strcmp(compareString,"8") == 0 ||
       strcmp(compareString,"257") == 0)
        retType = SdpCodec::SDP_CODEC_GIPS_PCMA;
    else
    if (strcmp(compareString,"G729") == 0 ||
       strcmp(compareString,"G729A") == 0)
        retType = SdpCodec::SDP_CODEC_G729;
    else
    if (strcmp(compareString,"G723") == 0 ||
        strcmp(compareString,"G723.1") == 0)
        retType = SdpCodec::SDP_CODEC_G723;
    else
    if (strcmp(compareString,"ILBC") == 0)
        retType = SdpCodec::SDP_CODEC_ILBC;
    else
    if (strcmp(compareString,"GSM") == 0)
        retType = SdpCodec::SDP_CODEC_GSM;
   else
      if (strcmp(compareString,"SPEEX") == 0)
         retType = SdpCodec::SDP_CODEC_SPEEX;
   else 
      if (strcmp(compareString,"SPEEX_5") == 0)
         retType = SdpCodec::SDP_CODEC_SPEEX_5;
   else 
      if (strcmp(compareString,"SPEEX_15") == 0)
         retType = SdpCodec::SDP_CODEC_SPEEX_15;
   else 
      if (strcmp(compareString,"SPEEX_24") == 0)
         retType = SdpCodec::SDP_CODEC_SPEEX_24;
   else 
      if (strcmp(compareString,"GSM") == 0)
         retType = SdpCodec::SDP_CODEC_GSM;
   else
    if (strcmp(compareString,"VP71-CIF") == 0)
        retType = SdpCodec::SDP_CODEC_VP71_CIF;
   else
    if (strcmp(compareString,"VP71-QCIF") == 0)
        retType = SdpCodec::SDP_CODEC_VP71_QCIF;
   else
    if (strcmp(compareString,"VP71-SQCIF") == 0)
        retType = SdpCodec::SDP_CODEC_VP71_SQCIF;
   else
    if (strcmp(compareString,"VP71-QVGA") == 0)
        retType = SdpCodec::SDP_CODEC_VP71_QVGA;
   else
    if (strcmp(compareString,"IYUV-CIF") == 0)
        retType = SdpCodec::SDP_CODEC_IYUV_CIF;
   else
    if (strcmp(compareString,"IYUV-QCIF") == 0)
        retType = SdpCodec::SDP_CODEC_IYUV_QCIF;
   else
    if (strcmp(compareString,"IYUV-SQCIF") == 0)
        retType = SdpCodec::SDP_CODEC_IYUV_SQCIF;
   else
    if (strcmp(compareString,"IYUV-QVGA") == 0)
        retType = SdpCodec::SDP_CODEC_IYUV_QVGA;
   else
    if (strcmp(compareString,"I420-CIF") == 0)
        retType = SdpCodec::SDP_CODEC_I420_CIF;
   else
    if (strcmp(compareString,"I420-QCIF") == 0)
        retType = SdpCodec::SDP_CODEC_I420_QCIF;
   else
    if (strcmp(compareString,"I420-SQCIF") == 0)
        retType = SdpCodec::SDP_CODEC_I420_SQCIF;
   else
    if (strcmp(compareString,"I420-QVGA") == 0)
        retType = SdpCodec::SDP_CODEC_I420_QVGA;
   else
    if (strcmp(compareString,"H263-CIF") == 0)
        retType = SdpCodec::SDP_CODEC_H263_CIF;
   else
    if (strcmp(compareString,"H263-QCIF") == 0)
        retType = SdpCodec::SDP_CODEC_H263_QCIF;
   else
    if (strcmp(compareString,"H263-SQCIF") == 0)
        retType = SdpCodec::SDP_CODEC_H263_SQCIF;
   else
    if (strcmp(compareString,"H263-QVGA") == 0)
        retType = SdpCodec::SDP_CODEC_H263_QVGA;
   else
    if (strcmp(compareString,"RGB24-CIF") == 0)
        retType = SdpCodec::SDP_CODEC_RGB24_CIF;
   else
    if (strcmp(compareString,"RGB24-QCIF") == 0)
        retType = SdpCodec::SDP_CODEC_RGB24_QCIF;
   else
    if (strcmp(compareString,"RGB24-SQCIF") == 0)
        retType = SdpCodec::SDP_CODEC_RGB24_SQCIF;
   else
    if (strcmp(compareString,"RGB24-QVGA") == 0)
        retType = SdpCodec::SDP_CODEC_RGB24_QVGA;
    else
       retType = SdpCodec::SDP_CODEC_UNKNOWN;
    return retType;
}

/* //////////////////////////// PROTECTED ///////////////////////////////// */

/* //////////////////////////// PRIVATE /////////////////////////////////// */

/* ============================ FUNCTIONS ================================= */
