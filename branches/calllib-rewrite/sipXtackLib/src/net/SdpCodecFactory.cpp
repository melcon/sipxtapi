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
#include <string.h>

// APPLICATION INCLUDES
#include <utl/UtlSListIterator.h>
#include <os/OsWriteLock.h>
#include <os/OsReadLock.h>
#include <net/SdpCodecFactory.h>
#include <sdp/SdpCodec.h>
#include <net/NameValueTokenizer.h>

// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
// STATIC VARIABLE INITIALIZATIONS

/* //////////////////////////// PUBLIC //////////////////////////////////// */

/* ============================ CREATORS ================================== */

// Constructor
SdpCodecFactory::SdpCodecFactory(int numCodecs, SdpCodec* codecs[])
: mReadWriteMutex(OsRWMutex::Q_FIFO)
{
   addCodecs(numCodecs, codecs);
}

SdpCodecFactory::SdpCodecFactory(const UtlSList& sdpCodecList)
: mReadWriteMutex(OsRWMutex::Q_FIFO)
{
   addCodecs(sdpCodecList);
}

// Destructor
SdpCodecFactory::~SdpCodecFactory()
{
   m_codecsList.destroyAll();
}

/* ============================ MANIPULATORS ============================== */

void SdpCodecFactory::addCodecs(int numCodecs, SdpCodec* codecs[])
{
   OsWriteLock lock(mReadWriteMutex);
   for(int index = 0; index < numCodecs; index++)
   {
      addCodecNoLock(*(codecs[index]));
   }
}

void SdpCodecFactory::addCodecs(const UtlSList& sdpCodecList)
{
   OsWriteLock lock(mReadWriteMutex);

   SdpCodec* pCodec = NULL;
   UtlSListIterator itor(sdpCodecList);
   while (itor())
   {
      pCodec = dynamic_cast<SdpCodec*>(itor.item());
      if (pCodec)
      {
         addCodecNoLock(*pCodec);
      }
   }
}

void SdpCodecFactory::addCodec(const SdpCodec& newCodec)
{
   OsWriteLock lock(mReadWriteMutex);
   addCodecNoLock(newCodec);
}

void SdpCodecFactory::bindPayloadIds()
{
   int unusedDynamicPayloadId = SdpCodec::SDP_CODEC_MAXIMUM_STATIC_CODEC + 1;
   SdpCodec* codecWithoutPayloadId = NULL;
   UtlString prevSubmimeType = "none";
   UtlString actualSubmimeType;

   // Find a codec which does not have its payload type set
   // Cheat a little and make the codec writeable
   while((codecWithoutPayloadId = (SdpCodec*) getCodecByPayloadId(-1)))
   {
      // Find an unused dynamic payload type id
      while(getCodecByPayloadId(unusedDynamicPayloadId))
      {
         // Assuming all codecs with same submime type are stored
         // sequentially, only increment payload id for different submime types
         codecWithoutPayloadId->getEncodingName(actualSubmimeType);
         if (prevSubmimeType.compareTo(actualSubmimeType, UtlString::ignoreCase) != 0)
         {
            // Increment payload id for new submime type
            unusedDynamicPayloadId++;
            prevSubmimeType = actualSubmimeType;
         }
         else
         {
            // Just break if we have the same submime type
            break;
         }
      }

      codecWithoutPayloadId->setCodecPayloadId(unusedDynamicPayloadId);
   }
}

void SdpCodecFactory::copyPayloadId(const SdpCodec& codec)
{
   SdpCodec* pFoundCodec = NULL;
   OsWriteLock lock(mReadWriteMutex);
   UtlSListIterator itor(m_codecsList);

   while(itor())
   {
      pFoundCodec = dynamic_cast<SdpCodec*>(itor.item());
      if(pFoundCodec && pFoundCodec->isSameDefinition(codec))
      {
         pFoundCodec->setCodecPayloadId(codec.getCodecPayloadId());
      }
   }
}

void SdpCodecFactory::copyPayloadIds(int numCodecs, 
                                     SdpCodec* codecArray[])
{
   int index;
   for(index = 0; index < numCodecs; index++)
   {
      copyPayloadId(*(codecArray[index]));
   }
}

void SdpCodecFactory::copyPayloadIds(const UtlSList& codecList)
{
   UtlSListIterator itor(codecList);
   SdpCodec* pCodec = NULL;
   while (itor())
   {
      pCodec = dynamic_cast<SdpCodec*>(itor.item());
      if (pCodec)
      {
         copyPayloadId(*pCodec);
      }
   }
}

void SdpCodecFactory::clearCodecs(void)
{
   OsWriteLock lock(mReadWriteMutex);
   m_codecsList.destroyAll();
}

void SdpCodecFactory::buildSdpCodecFactory(const UtlString &codecList)
{
   OsWriteLock lock(mReadWriteMutex);
   UtlString codecName;
   int codecStringIndex = 0;
   SdpCodec::SdpCodecTypes codecType;
   NameValueTokenizer::getSubField(codecList, codecStringIndex, ", \n\r\t", &codecName);

   while(!codecName.isNull())
   {
      codecType = SdpCodec::getCodecType(codecName);
      if (codecType != SdpCodec::SDP_CODEC_UNKNOWN)
      {
         SdpCodec* pCodec = SdpCodecFactory::buildSdpCodec(codecType);
         if (pCodec)
         {
            if (!m_codecsList.find(pCodec))
            {
               // codec doesn't exist yet, add it
               m_codecsList.insert(pCodec);
            }
            else
            {
               delete pCodec;
               pCodec = NULL;
            }
         }
      }

      codecStringIndex++;
      NameValueTokenizer::getSubField(codecList, codecStringIndex, ", \n\r\t", &codecName);
   }
}

SdpCodec* SdpCodecFactory::buildSdpCodec(SdpCodec::SdpCodecTypes codecType)
{
   SdpCodec* pCodec = NULL;
   switch(codecType)
   {
   case SdpCodec::SDP_CODEC_TONES:
      pCodec = new SdpCodec(SdpCodec::SDP_CODEC_TONES,
        SdpCodec::SDP_CODEC_UNKNOWN,
        "TELEPHONE-EVENT",
        MIME_TYPE_AUDIO,
        MIME_SUBTYPE_DTMF_TONES,
        8000,
        20000,
        1,
        "",
        SdpCodec::SDP_CODEC_CPU_LOW,
        SDP_CODEC_BANDWIDTH_LOW);
      break;
#ifdef HAVE_INTEL_IPP
   case SdpCodec::SDP_CODEC_G729:
      pCodec = new SdpCodec(SdpCodec::SDP_CODEC_G729,
         SdpCodec::SDP_CODEC_UNKNOWN,
         "G729A",
         MIME_TYPE_AUDIO,
         MIME_SUBTYPE_G729,
         8000,
         20000,
         1,
         "annexb=no",
         SdpCodec::SDP_CODEC_CPU_HIGH,
         SDP_CODEC_BANDWIDTH_LOW);
      break;
   case SdpCodec::SDP_CODEC_G723:
      pCodec = new SdpCodec(SdpCodec::SDP_CODEC_G723,
         SdpCodec::SDP_CODEC_UNKNOWN,
         "G723.1",
         MIME_TYPE_AUDIO,
         MIME_SUBTYPE_G723,
         8000,
         30000,
         1,
         "",
         SdpCodec::SDP_CODEC_CPU_HIGH,
         SDP_CODEC_BANDWIDTH_LOW); 
      break;
#endif // HAVE_INTEL_IPP
   case SdpCodec::SDP_CODEC_GIPS_PCMA:
      pCodec = new SdpCodec(SdpCodec::SDP_CODEC_GIPS_PCMA,
         SdpCodec::SDP_CODEC_PCMA,
         "PCMA",
         MIME_TYPE_AUDIO,
         MIME_SUBTYPE_PCMA,
         8000,
         20000,
         1,
         "",
         SdpCodec::SDP_CODEC_CPU_LOW,
         SDP_CODEC_BANDWIDTH_NORMAL);
      break;
   case SdpCodec::SDP_CODEC_GIPS_PCMU:
      pCodec = new SdpCodec(SdpCodec::SDP_CODEC_GIPS_PCMU,
         SdpCodec::SDP_CODEC_PCMU,
         "PCMU",
         MIME_TYPE_AUDIO,
         MIME_SUBTYPE_PCMU,
         8000,
         20000,
         1,
         "",
         SdpCodec::SDP_CODEC_CPU_LOW,
         SDP_CODEC_BANDWIDTH_NORMAL);
      break;
   case SdpCodec::SDP_CODEC_ILBC:
      pCodec = new SdpCodec(SdpCodec::SDP_CODEC_ILBC,
         SdpCodec::SDP_CODEC_UNKNOWN,
         "ILBC",
         MIME_TYPE_AUDIO,
         MIME_SUBTYPE_ILBC,
         8000,
         30000,
         1,
         "",
         SdpCodec::SDP_CODEC_CPU_HIGH,
         SDP_CODEC_BANDWIDTH_LOW);
      break;
#ifdef HAVE_GSM
   case SdpCodec::SDP_CODEC_GSM:
      pCodec = new SdpCodec(SdpCodec::SDP_CODEC_GSM,
         SdpCodec::SDP_CODEC_GSM,
         "GSM",
         MIME_TYPE_AUDIO,
         MIME_SUBTYPE_GSM,
         8000,
         20000,
         1,
         "",
         SdpCodec::SDP_CODEC_CPU_HIGH,
         SDP_CODEC_BANDWIDTH_LOW);
      break;
#endif // HAVE_GSM
#ifdef HAVE_SPEEX
   case SdpCodec::SDP_CODEC_SPEEX:
      pCodec = new SdpCodec(SdpCodec::SDP_CODEC_SPEEX,
         SdpCodec::SDP_CODEC_UNKNOWN,
         "SPEEX",
         MIME_TYPE_AUDIO,
         MIME_SUBTYPE_SPEEX,
         8000,
         20000,
         1,
         "mode=3",
         SdpCodec::SDP_CODEC_CPU_LOW,
         SDP_CODEC_BANDWIDTH_LOW);
      break;
   case SdpCodec::SDP_CODEC_SPEEX_5:
      pCodec = new SdpCodec(SdpCodec::SDP_CODEC_SPEEX_5,
         SdpCodec::SDP_CODEC_UNKNOWN,
         "SPEEX_5",
         MIME_TYPE_AUDIO,
         MIME_SUBTYPE_SPEEX,
         8000,
         20000,
         1,
         "mode=2",
         SdpCodec::SDP_CODEC_CPU_LOW,
         SDP_CODEC_BANDWIDTH_LOW);
      break;
   case SdpCodec::SDP_CODEC_SPEEX_15:
      pCodec = new SdpCodec(SdpCodec::SDP_CODEC_SPEEX_15,
         SdpCodec::SDP_CODEC_UNKNOWN,
         "SPEEX_15",
         MIME_TYPE_AUDIO,
         MIME_SUBTYPE_SPEEX,
         8000,
         20000,
         1,
         "mode=5",
         SdpCodec::SDP_CODEC_CPU_LOW,
         SDP_CODEC_BANDWIDTH_NORMAL);
      break;
   case SdpCodec::SDP_CODEC_SPEEX_24:
      pCodec = new SdpCodec(SdpCodec::SDP_CODEC_SPEEX_24,
         SdpCodec::SDP_CODEC_UNKNOWN,
         "SPEEX_24",
         MIME_TYPE_AUDIO,
         MIME_SUBTYPE_SPEEX,
         8000,
         20000,
         1,
         "mode=7",
         SdpCodec::SDP_CODEC_CPU_LOW,
         SDP_CODEC_BANDWIDTH_NORMAL);
      break;
#endif // HAVE_SPEEX
#ifdef VIDEO
   case SdpCodec::SDP_CODEC_VP71_CIF:
      pCodec = new SdpCodec(SdpCodec::SDP_CODEC_VP71_CIF,
         SdpCodec::SDP_CODEC_UNKNOWN,
         "VP71-CIF",
         MIME_TYPE_VIDEO,
         MIME_SUBTYPE_VP71,
         90000,
         20000,
         1,
         "size=CIF/QCIF/SQCIF",
         SdpCodec::SDP_CODEC_CPU_LOW,
         SDP_CODEC_BANDWIDTH_NORMAL,
         SDP_VIDEO_FORMAT_CIF);
      break;
   case SdpCodec::SDP_CODEC_VP71_QCIF:
      pCodec = new SdpCodec(SdpCodec::SDP_CODEC_VP71_QCIF,
         SdpCodec::SDP_CODEC_UNKNOWN,
         "VP71-QCIF",
         MIME_TYPE_VIDEO,
         MIME_SUBTYPE_VP71,
         90000,
         20000,
         1,
         "",
         SdpCodec::SDP_CODEC_CPU_LOW,
         SDP_CODEC_BANDWIDTH_NORMAL,
         SDP_VIDEO_FORMAT_QCIF);
      break;
   case SdpCodec::SDP_CODEC_VP71_SQCIF:
      pCodec = new SdpCodec(SdpCodec::SDP_CODEC_VP71_SQCIF,
         SdpCodec::SDP_CODEC_UNKNOWN,
         "VP71-SQCIF",
         MIME_TYPE_VIDEO,
         MIME_SUBTYPE_VP71,
         90000,
         20000,
         1,
         "",
         SdpCodec::SDP_CODEC_CPU_LOW,
         SDP_CODEC_BANDWIDTH_NORMAL,
         SDP_VIDEO_FORMAT_SQCIF);
      break;
   case SdpCodec::SDP_CODEC_VP71_QVGA:
      pCodec = new SdpCodec(SdpCodec::SDP_CODEC_VP71_QVGA,
         SdpCodec::SDP_CODEC_UNKNOWN,
         "VP71-QVGA",
         MIME_TYPE_VIDEO,
         MIME_SUBTYPE_VP71,
         90000,
         20000,
         1,
         "",
         SdpCodec::SDP_CODEC_CPU_LOW,
         SDP_CODEC_BANDWIDTH_NORMAL,
         SDP_VIDEO_FORMAT_QVGA);
      break;
   case SdpCodec::SDP_CODEC_IYUV_CIF:
      pCodec = new SdpCodec(SdpCodec::SDP_CODEC_IYUV_CIF,
         SdpCodec::SDP_CODEC_UNKNOWN,
         "IYUV-CIF",
         MIME_TYPE_VIDEO,
         MIME_SUBTYPE_IYUV,
         90000,
         20000,
         1,
         "size=CIF/QCIF/SQCIF",
         SdpCodec::SDP_CODEC_CPU_LOW,
         SDP_CODEC_BANDWIDTH_HIGH,
         SDP_VIDEO_FORMAT_CIF);
      break;
   case SdpCodec::SDP_CODEC_IYUV_QCIF:
      pCodec = new SdpCodec(SdpCodec::SDP_CODEC_IYUV_QCIF,
         SdpCodec::SDP_CODEC_UNKNOWN,
         "IYUV-QCIF",
         MIME_TYPE_VIDEO,
         MIME_SUBTYPE_IYUV,
         90000,
         20000,
         1,
         "",
         SdpCodec::SDP_CODEC_CPU_LOW,
         SDP_CODEC_BANDWIDTH_HIGH,
         SDP_VIDEO_FORMAT_QCIF);
      break;
   case SdpCodec::SDP_CODEC_IYUV_SQCIF:
      pCodec = new SdpCodec(SdpCodec::SDP_CODEC_IYUV_SQCIF,
         SdpCodec::SDP_CODEC_UNKNOWN,
         "IYUV-SQCIF",
         MIME_TYPE_VIDEO,
         MIME_SUBTYPE_IYUV,
         90000,
         20000,
         1,
         "",
         SdpCodec::SDP_CODEC_CPU_LOW,
         SDP_CODEC_BANDWIDTH_HIGH,
         SDP_VIDEO_FORMAT_SQCIF);
      break;
   case SdpCodec::SDP_CODEC_IYUV_QVGA:
      pCodec = new SdpCodec(SdpCodec::SDP_CODEC_IYUV_QVGA,
         SdpCodec::SDP_CODEC_UNKNOWN,
         "IYUV-QVGA",
         MIME_TYPE_VIDEO,
         MIME_SUBTYPE_IYUV,
         90000,
         20000,
         1,
         "",
         SdpCodec::SDP_CODEC_CPU_LOW,
         SDP_CODEC_BANDWIDTH_HIGH,
         SDP_VIDEO_FORMAT_QVGA);
      break;
   case SdpCodec::SDP_CODEC_I420_CIF:
      pCodec = new SdpCodec(SdpCodec::SDP_CODEC_I420_CIF,
         SdpCodec::SDP_CODEC_UNKNOWN,
         "I420-CIF",
         MIME_TYPE_VIDEO,
         MIME_SUBTYPE_I420,
         90000,
         20000,
         1,
         "size=CIF/QCIF/SQCIF",
         SdpCodec::SDP_CODEC_CPU_LOW,
         SDP_CODEC_BANDWIDTH_HIGH,
         SDP_VIDEO_FORMAT_CIF);
      break;
   case SdpCodec::SDP_CODEC_I420_QCIF:
      pCodec = new SdpCodec(SdpCodec::SDP_CODEC_I420_QCIF,
         SdpCodec::SDP_CODEC_UNKNOWN,
         "I420-QCIF",
         MIME_TYPE_VIDEO,
         MIME_SUBTYPE_I420,
         90000,
         20000,
         1,
         "",
         SdpCodec::SDP_CODEC_CPU_LOW,
         SDP_CODEC_BANDWIDTH_HIGH,
         SDP_VIDEO_FORMAT_QCIF);
      break;
   case SdpCodec::SDP_CODEC_I420_SQCIF:
      pCodec = new SdpCodec(SdpCodec::SDP_CODEC_I420_SQCIF,
         SdpCodec::SDP_CODEC_UNKNOWN,
         "I420-SQCIF",
         MIME_TYPE_VIDEO,
         MIME_SUBTYPE_I420,
         90000,
         20000,
         1,
         "",
         SdpCodec::SDP_CODEC_CPU_LOW,
         SDP_CODEC_BANDWIDTH_HIGH,
         SDP_VIDEO_FORMAT_SQCIF);
      break;
   case SdpCodec::SDP_CODEC_I420_QVGA:
      pCodec = new SdpCodec(SdpCodec::SDP_CODEC_I420_QVGA,
         SdpCodec::SDP_CODEC_UNKNOWN,
         "I420-QVGA",
         MIME_TYPE_VIDEO,
         MIME_SUBTYPE_I420,
         90000,
         20000,
         1,
         "",
         SdpCodec::SDP_CODEC_CPU_LOW,
         SDP_CODEC_BANDWIDTH_HIGH,
         SDP_VIDEO_FORMAT_QVGA);
      break;

   case SdpCodec::SDP_CODEC_RGB24_CIF:
      pCodec = new SdpCodec(SdpCodec::SDP_CODEC_RGB24_CIF,
         SdpCodec::SDP_CODEC_UNKNOWN,
         "RGB24-CIF",
         MIME_TYPE_VIDEO,
         MIME_SUBTYPE_RGB24,
         90000,
         20000,
         1,
         "size=CIF/QCIF/SQCIF",
         SdpCodec::SDP_CODEC_CPU_LOW,
         SDP_CODEC_BANDWIDTH_HIGH,
         SDP_VIDEO_FORMAT_CIF);
      break;
   case SdpCodec::SDP_CODEC_RGB24_QCIF:
      pCodec = new SdpCodec(SdpCodec::SDP_CODEC_RGB24_QCIF,
         SdpCodec::SDP_CODEC_UNKNOWN,
         "RGB24-QCIF",
         MIME_TYPE_VIDEO,
         MIME_SUBTYPE_RGB24,
         90000,
         20000,
         1,
         "",
         SdpCodec::SDP_CODEC_CPU_LOW,
         SDP_CODEC_BANDWIDTH_HIGH,
         SDP_VIDEO_FORMAT_QCIF);
      break;
   case SdpCodec::SDP_CODEC_RGB24_SQCIF:
      pCodec = new SdpCodec(SdpCodec::SDP_CODEC_RGB24_SQCIF,
         SdpCodec::SDP_CODEC_UNKNOWN,
         "RGB24-SQCIF",
         MIME_TYPE_VIDEO,
         MIME_SUBTYPE_RGB24,
         90000,
         20000,
         1,
         "",
         SdpCodec::SDP_CODEC_CPU_LOW,
         SDP_CODEC_BANDWIDTH_HIGH,
         SDP_VIDEO_FORMAT_SQCIF);
      break;
   case SdpCodec::SDP_CODEC_RGB24_QVGA:
      pCodec = new SdpCodec(SdpCodec::SDP_CODEC_RGB24_QVGA,
         SdpCodec::SDP_CODEC_UNKNOWN,
         "RGB24-QVGA",
         MIME_TYPE_VIDEO,
         MIME_SUBTYPE_RGB24,
         90000,
         20000,
         1,
         "",
         SdpCodec::SDP_CODEC_CPU_LOW,
         SDP_CODEC_BANDWIDTH_HIGH,
         SDP_VIDEO_FORMAT_QVGA);
      break;
   case SdpCodec::SDP_CODEC_H263_CIF:
      pCodec = new SdpCodec(SdpCodec::SDP_CODEC_H263_CIF,
         SdpCodec::SDP_CODEC_H263,
         "H263-CIF",
         MIME_TYPE_VIDEO,
         MIME_SUBTYPE_H263,
         90000,
         20000,
         1,
         "",
         SdpCodec::SDP_CODEC_CPU_LOW,
         SDP_CODEC_BANDWIDTH_NORMAL,
         SDP_VIDEO_FORMAT_CIF);
      break;
   case SdpCodec::SDP_CODEC_H263_QCIF:
      SdpCodec aCodec(SdpCodec::SDP_CODEC_H263_QCIF,
         SdpCodec::SDP_CODEC_H263,
         "H263-QCIF",
         MIME_TYPE_VIDEO,
         MIME_SUBTYPE_H263,
         90000,
         20000,
         1,
         "",
         SdpCodec::SDP_CODEC_CPU_LOW,
         SDP_CODEC_BANDWIDTH_NORMAL,
         SDP_VIDEO_FORMAT_QCIF);
      break;
   case SdpCodec::SDP_CODEC_H263_SQCIF:
      pCodec = new SdpCodec(SdpCodec::SDP_CODEC_H263_SQCIF,
         SdpCodec::SDP_CODEC_H263,
         "H263-SQCIF",
         MIME_TYPE_VIDEO,
         MIME_SUBTYPE_H263,
         90000,
         20000,
         1,
         "",
         SdpCodec::SDP_CODEC_CPU_LOW,
         SDP_CODEC_BANDWIDTH_NORMAL,
         SDP_VIDEO_FORMAT_SQCIF);
      break;
   case SdpCodec::SDP_CODEC_H263_QVGA:
      pCodec = new SdpCodec(SdpCodec::SDP_CODEC_H263_QVGA,
         SdpCodec::SDP_CODEC_H263,
         "H263-QVGA",
         MIME_TYPE_VIDEO,
         MIME_SUBTYPE_H263,
         90000,
         20000,
         1,
         "",
         SdpCodec::SDP_CODEC_CPU_LOW,
         SDP_CODEC_BANDWIDTH_HIGH,
         SDP_VIDEO_FORMAT_QVGA);
      break;
#endif // VIDEO
   default:
      ;
   }

   return pCodec;
}

/* ============================ ACCESSORS ================================= */

const SdpCodec* SdpCodecFactory::getCodec(SdpCodec::SdpCodecTypes internalCodecId)
{
   UtlInt codecToMatch(internalCodecId);
   OsReadLock lock(mReadWriteMutex);
   return dynamic_cast<const SdpCodec*>(m_codecsList.find(&codecToMatch));
}

const SdpCodec* SdpCodecFactory::getCodecByPayloadId(int payloadTypeId)
{
   SdpCodec* pCodecFound = NULL;

   OsReadLock lock(mReadWriteMutex);
   UtlSListIterator itor(m_codecsList);

   while((pCodecFound = (SdpCodec*) itor()))
   {
      pCodecFound = dynamic_cast<SdpCodec*>(itor.item());
      if (pCodecFound && pCodecFound->getCodecPayloadId() == payloadTypeId)
      {
         break;
      }
   }

   return pCodecFound;
}

const SdpCodec* SdpCodecFactory::getCodec(const char* mimeType, 
                                          const char* mimeSubType)
{
   const SdpCodec* codecFound = NULL;
   UtlString foundMimeType;
   UtlString foundMimeSubType;
   UtlString mimeTypeString(mimeType ? mimeType : "");
   mimeTypeString.toLower();
   UtlString mimeSubTypeString(mimeSubType ? mimeSubType : "");
   mimeSubTypeString.toLower();
   OsReadLock lock(mReadWriteMutex);
   UtlSListIterator iterator(m_codecsList);

   while((codecFound = (SdpCodec*) iterator()))
   {
      // If the mime type matches
      codecFound->getMediaType(foundMimeType);
      if (foundMimeType.compareTo(mimeTypeString, UtlString::ignoreCase) == 0)
      {
         // and if the mime subtype matches
         codecFound->getEncodingName(foundMimeSubType);
         if (foundMimeSubType.compareTo(mimeSubTypeString, UtlString::ignoreCase) == 0)
         {
            // we found a match
            break;
         }
      }
   }

   return(codecFound);
}

int SdpCodecFactory::getCodecCount()
{
   OsReadLock lock(mReadWriteMutex);
   return (int)m_codecsList.entries();
}

int SdpCodecFactory::getCodecCount(const UtlString& mimetype)
{
   OsReadLock lock(mReadWriteMutex);
   SdpCodec* codecFound = NULL;
   UtlString foundMimeType;

   int iCount = 0;    
   UtlSListIterator itor(m_codecsList);
   while((codecFound = (SdpCodec*) itor()))
   {
      codecFound = dynamic_cast<SdpCodec*>(itor.item());
      if (codecFound)
      {
         codecFound->getMediaType(foundMimeType);
         if (foundMimeType.compareTo(mimetype, UtlString::ignoreCase) == 0)
         {
            iCount++;
         }        
      }
   }

   return iCount;
}

void SdpCodecFactory::getCodecs(UtlSList& sdpCodecList)
{
   OsReadLock lock(mReadWriteMutex);
   SdpCodec* pCodec = NULL;
   UtlSListIterator itor(m_codecsList);
   while (itor())
   {
      pCodec = dynamic_cast<SdpCodec*>(itor.item());
      if (pCodec)
      {
         sdpCodecList.append(new SdpCodec(*pCodec));
      }
   }
}

void SdpCodecFactory::getCodecs(int& numCodecs, 
                                SdpCodec**& codecArray)
{
   const SdpCodec* codecFound = NULL;
   OsReadLock lock(mReadWriteMutex);
   int arrayMaximum = m_codecsList.entries();
   codecArray = new SdpCodec*[arrayMaximum];
   UtlSListIterator iterator(m_codecsList);
   int index = 0;

   while(index < arrayMaximum &&
      (codecFound = (SdpCodec*) iterator()) != NULL)
   {
      codecArray[index] = new SdpCodec(*codecFound);
      index++;
   }

   numCodecs = index;
}

void SdpCodecFactory::getCodecs(int& numCodecs, 
                                SdpCodec**& codecArray,
                                const char* mimeType)
{
   const SdpCodec* codecFound = NULL;
   OsReadLock lock(mReadWriteMutex);
   int arrayMaximum = m_codecsList.entries();
   codecArray = new SdpCodec*[arrayMaximum];
   UtlSListIterator iterator(m_codecsList);
   UtlString sMimeType;
   int index = 0;

   while(index < arrayMaximum &&
      (codecFound = (SdpCodec*) iterator()) != NULL)
   {
      codecFound->getMediaType(sMimeType);
      if (sMimeType.compareTo(mimeType, UtlString::ignoreCase) == 0)
      {
         codecArray[index] = new SdpCodec(*codecFound);
         index++;
      }
   }

   numCodecs = index;
}

void SdpCodecFactory::getCodecs(int& numCodecs, 
                                SdpCodec**& codecArray,
                                const char* mimeType,
                                const char* subMimeType)
{
   const SdpCodec* codecFound = NULL;
   OsReadLock lock(mReadWriteMutex);
   int arrayMaximum = m_codecsList.entries();
   codecArray = new SdpCodec*[arrayMaximum];
   UtlSListIterator iterator(m_codecsList);
   UtlString sMimeType;
   UtlString sSubMimeType;
   int index = 0;

   while(index < arrayMaximum &&
      (codecFound = (SdpCodec*) iterator()) != NULL)
   {
      codecFound->getMediaType(sMimeType);
      codecFound->getEncodingName(sSubMimeType);
      if (sMimeType.compareTo(mimeType, UtlString::ignoreCase) == 0 && 
         sSubMimeType.compareTo(subMimeType, UtlString::ignoreCase) == 0)
      {
         codecArray[index] = new SdpCodec(*codecFound);
         index++;
      }
   }

   numCodecs = index;
}

void SdpCodecFactory::toString(UtlString& serializedFactory)
{
   serializedFactory.remove(0);
   const SdpCodec* codecFound = NULL;
   UtlSListIterator iterator(m_codecsList);
   int index = 0;

   while((codecFound = (SdpCodec*) iterator()) != NULL)
   {
      UtlString codecString;
      char codecLabel[256];
      SNPRINTF(codecLabel, sizeof(codecLabel), "Codec[%d] cost=%d\n", index, codecFound->getCPUCost());
      serializedFactory.append(codecLabel);
      codecFound->toString(codecString);
      serializedFactory.append(codecString);
      serializedFactory.append("\n");
      index++;
   }
}

UtlString SdpCodecFactory::getFixedAudioCodecs(const UtlString& audioCodecs)
{
   if (!audioCodecs.contains(MIME_SUBTYPE_DTMF_TONES))
   {
      // audio/telephone-event is missing, add it
      UtlString res(audioCodecs);
      if (audioCodecs.length() > 0)
      {
         res += " "MIME_SUBTYPE_DTMF_TONES;
      }
      else
      {
         res = MIME_SUBTYPE_DTMF_TONES;
      }
      return res;
   }
   else
   {
      return audioCodecs;
   }
}

/* ============================ INQUIRY =================================== */

/* //////////////////////////// PROTECTED ///////////////////////////////// */

void SdpCodecFactory::addCodecNoLock(const SdpCodec& newCodec)
{
   m_codecsList.insert(new SdpCodec(newCodec));
}

/* //////////////////////////// PRIVATE /////////////////////////////////// */

/* ============================ FUNCTIONS ================================= */

