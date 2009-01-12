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
// APPLICATION INCLUDES
#include <utl/UtlSListIterator.h>
#include <os/OsWriteLock.h>
#include <os/OsReadLock.h>
#include <sdp/SdpCodecFactory.h>
#include <sdp/SdpCodecList.h>
#include <sdp/SdpCodec.h>
#include <net/NameValueTokenizer.h>

// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
// STATIC VARIABLE INITIALIZATIONS

/* //////////////////////////// PUBLIC //////////////////////////////////// */

/* ============================ CREATORS ================================== */

// Constructor
SdpCodecList::SdpCodecList(int numCodecs, SdpCodec* codecs[])
: m_memberMutex(OsRWMutex::Q_FIFO)
{
   addCodecs(numCodecs, codecs);
}

SdpCodecList::SdpCodecList(const UtlSList& sdpCodecList)
: m_memberMutex(OsRWMutex::Q_FIFO)
{
   addCodecs(sdpCodecList);
}

SdpCodecList::SdpCodecList(const SdpCodecList& rSdpCodecList)
: m_memberMutex(OsRWMutex::Q_FIFO)
{
   // populate our codec list with codecs from the other list
   rSdpCodecList.getCodecs(m_codecsList);
}

SdpCodecList& SdpCodecList::operator=(const SdpCodecList& rhs)
{
   if (this == &rhs)    // handle the assignment to self case
   {
      return *this;
   }

   UtlSList tmpCodecList; // use tmp list to avoid locking both classes at the same time (deadlock threat)
   rhs.getCodecs(tmpCodecList);

   {
      OsWriteLock lock(m_memberMutex);
      m_codecsList.destroyAll();
      SdpCodec* pCodec = NULL;
      UtlSListIterator itor(tmpCodecList);
      while (itor())
      {
         pCodec = dynamic_cast<SdpCodec*>(itor.item());
         if (pCodec)
         {
            tmpCodecList.remove(pCodec); // remove from old list
            m_codecsList.insert(pCodec); // append to new list
         }
      }
   }

   return *this;
}

// Destructor
SdpCodecList::~SdpCodecList()
{
   m_codecsList.destroyAll();
}

/* ============================ MANIPULATORS ============================== */

void SdpCodecList::addCodecs(int numCodecs, SdpCodec* codecs[])
{
   OsWriteLock lock(m_memberMutex);
   for(int index = 0; index < numCodecs; index++)
   {
      if (codecs[index])
      {
         m_codecsList.insert(new SdpCodec(*(codecs[index])));
      }
   }
}

void SdpCodecList::addCodecs(const UtlSList& sdpCodecList)
{
   OsWriteLock lock(m_memberMutex);

   SdpCodec* pCodec = NULL;
   UtlSListIterator itor(sdpCodecList);
   while (itor())
   {
      pCodec = dynamic_cast<SdpCodec*>(itor.item());
      if (pCodec)
      {
         m_codecsList.insert(new SdpCodec(*pCodec));
      }
   }
}

void SdpCodecList::addCodecs(const UtlString &codecList)
{
   OsWriteLock lock(m_memberMutex);
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

void SdpCodecList::addCodecs(const SdpCodecList& sdpCodecList)
{
   if (this == &sdpCodecList) // handle the assignment to self case
   {
      return;
   }

   UtlSList tmpCodecList; // use tmp list to avoid locking both classes at the same time (deadlock threat)
   sdpCodecList.getCodecs(tmpCodecList);

   {
      OsWriteLock lock(m_memberMutex);
      SdpCodec* pCodec = NULL;
      UtlSListIterator itor(tmpCodecList);
      while (itor())
      {
         pCodec = dynamic_cast<SdpCodec*>(itor.item());
         if (pCodec)
         {
            tmpCodecList.remove(pCodec); // remove from old list
            m_codecsList.insert(pCodec); // append to new list
         }
      }
   }
}

void SdpCodecList::addCodec(const SdpCodec& newCodec)
{
   OsWriteLock lock(m_memberMutex);
   m_codecsList.insert(new SdpCodec(newCodec));
}

void SdpCodecList::bindPayloadIds()
{
   OsWriteLock lock(m_memberMutex);
   int unusedDynamicPayloadId = SdpCodec::SDP_CODEC_MAXIMUM_STATIC_CODEC + 1;
   SdpCodec* codecWithoutPayloadId = NULL;

   // Find a codec which does not have its payload type set
   // Cheat a little and make the codec writable
   while ((codecWithoutPayloadId = (SdpCodec*) getCodecByPayloadId(-1)))
   {
      // Find an unused dynamic payload type id
      while (getCodecByPayloadId(unusedDynamicPayloadId))
      {
         unusedDynamicPayloadId++;
      }

      codecWithoutPayloadId->setCodecPayloadId(unusedDynamicPayloadId);
      unusedDynamicPayloadId++;
   }
}

void SdpCodecList::clearCodecs(void)
{
   OsWriteLock lock(m_memberMutex);
   m_codecsList.destroyAll();
}

/* ============================ ACCESSORS ================================= */

const SdpCodec* SdpCodecList::getCodec(SdpCodec::SdpCodecTypes internalCodecId) const
{
   UtlInt codecToMatch(internalCodecId);
   OsReadLock lock(m_memberMutex);
   return dynamic_cast<const SdpCodec*>(m_codecsList.find(&codecToMatch));
}

UtlBoolean SdpCodecList::getCodec(SdpCodec::SdpCodecTypes internalCodecId,
                                  SdpCodec& sdpCodec) const
{
   UtlInt codecToMatch(internalCodecId);
   OsReadLock lock(m_memberMutex);
   const SdpCodec* pCodec = dynamic_cast<const SdpCodec*>(m_codecsList.find(&codecToMatch));
   if (pCodec)
   {
      sdpCodec = *pCodec;
      return TRUE;
   }
   return FALSE;
}

UtlBoolean SdpCodecList::getCodecByIndex(const UtlString& mimeType, int index, SdpCodec& sdpCodec) const
{
   const SdpCodec* pFoundCodec = NULL;
   const SdpCodec* pTmpCodec = NULL;
   UtlString foundMimeType;

   OsReadLock lock(m_memberMutex);
   UtlSListIterator iterator(m_codecsList);
   int position = 0;

   while((pTmpCodec = (SdpCodec*) iterator()))
   {
      pTmpCodec->getMediaType(foundMimeType);
      // If the mime type matches
      if (foundMimeType.compareTo(mimeType, UtlString::ignoreCase) == 0)
      {
         if (index == position++)
         {
            pFoundCodec = pTmpCodec;
            break;
         }
      }
   }

   if (pFoundCodec)
   {
      sdpCodec = *pFoundCodec;
   }

   return pFoundCodec != NULL;
}

const SdpCodec* SdpCodecList::getCodecByPayloadId(int payloadTypeId) const
{
   SdpCodec* pCodecFound = NULL;

   OsReadLock lock(m_memberMutex);
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

const SdpCodec* SdpCodecList::getCodec(const char* mimeType, 
                                       const char* mimeSubType,
                                       unsigned sampleRate,
                                       unsigned numChannels,
                                       const UtlString& fmtp) const
{
   const SdpCodec* codecFound = NULL;
   UtlString foundMimeType;
   UtlString foundMimeSubType;
   UtlString foundFmtp;
   UtlString mimeTypeString(mimeType ? mimeType : "");
   mimeTypeString.toLower();
   UtlString mimeSubTypeString(mimeSubType ? mimeSubType : "");
   mimeSubTypeString.toLower();
   OsReadLock lock(m_memberMutex);
   UtlSListIterator iterator(m_codecsList);

   while((codecFound = (SdpCodec*) iterator()))
   {
      // If the mime type matches
      codecFound->getMediaType(foundMimeType);
      if(foundMimeType.compareTo(mimeTypeString, UtlString::ignoreCase) == 0)
      {
         // and if the mime subtype, sample rate, number of channels
         // and fmtp match.
         // TODO:: checking for fmtp match must be made intelligent, e.g. by
         //        defining isCompatible(fmtp) method for SdpCodec. Checking
         //        by string comparison leads to errors when there are two
         //        or more parameters and they're presented in random order.
         codecFound->getMimeSubType(foundMimeSubType);
         codecFound->getSdpFmtpField(foundFmtp);
         if ((foundMimeSubType.compareTo(mimeSubTypeString, UtlString::ignoreCase) == 0) &&
            (sampleRate == -1 || codecFound->getSampleRate() == sampleRate) &&
            (numChannels == -1 || codecFound->getNumChannels() == numChannels) &&
            (fmtp == foundFmtp))
         {
            // we found a match
            break;
         }
      }
   }

   return(codecFound);
}

int SdpCodecList::getCodecCount() const
{
   OsReadLock lock(m_memberMutex);
   return (int)m_codecsList.entries();
}

int SdpCodecList::getCodecCount(const UtlString& mimetype) const
{
   OsReadLock lock(m_memberMutex);
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

void SdpCodecList::getCodecs(UtlSList& sdpCodecList) const
{
   OsReadLock lock(m_memberMutex);
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

void SdpCodecList::getCodecs(int& numCodecs, 
                             SdpCodec**& codecArray) const
{
   const SdpCodec* codecFound = NULL;
   OsReadLock lock(m_memberMutex);
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

void SdpCodecList::getCodecs(int& numCodecs, 
                             SdpCodec**& codecArray,
                             const char* mimeType) const
{
   const SdpCodec* codecFound = NULL;
   OsReadLock lock(m_memberMutex);
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

void SdpCodecList::getCodecs(int& numCodecs, 
                             SdpCodec**& codecArray,
                             const char* mimeType,
                             const char* subMimeType) const
{
   const SdpCodec* codecFound = NULL;
   OsReadLock lock(m_memberMutex);
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
      codecFound->getMimeSubType(sSubMimeType);
      if (sMimeType.compareTo(mimeType, UtlString::ignoreCase) == 0 && 
         sSubMimeType.compareTo(subMimeType, UtlString::ignoreCase) == 0)
      {
         codecArray[index] = new SdpCodec(*codecFound);
         index++;
      }
   }

   numCodecs = index;
}

void SdpCodecList::toString(UtlString& str) const
{
   str.remove(0);
   const SdpCodec* codecFound = NULL;
   OsReadLock lock(m_memberMutex);
   UtlSListIterator iterator(m_codecsList);
   int index = 0;

   while((codecFound = (SdpCodec*) iterator()) != NULL)
   {
      UtlString codecString;
      char codecLabel[256];
      SNPRINTF(codecLabel, sizeof(codecLabel), "Codec[%d] cost=%d\n", index, codecFound->getCPUCost());
      str.append(codecLabel);
      codecFound->toString(codecString);
      str.append(codecString);
      str.append("\n");
      index++;
   }
}

/* ============================ INQUIRY =================================== */

/* //////////////////////////// PROTECTED ///////////////////////////////// */

/* //////////////////////////// PRIVATE /////////////////////////////////// */

/* ============================ FUNCTIONS ================================= */

