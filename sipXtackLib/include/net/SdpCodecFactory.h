//
// Copyright (C) 2004-2006 SIPfoundry Inc.
// Licensed by SIPfoundry under the LGPL license.
//
// Copyright (C) 2004-2006 Pingtel Corp.  All rights reserved.
// Licensed to SIPfoundry under a Contributor Agreement.
//
// $$
///////////////////////////////////////////////////////////////////////////////

#ifndef _SdpCodecFactory_h_
#define _SdpCodecFactory_h_

// SYSTEM INCLUDES
// APPLICATION INCLUDES
#include <utl/UtlSList.h>
#include <os/OsBSem.h>
#include <os/OsRWMutex.h>
#include <sdp/SdpCodec.h>

// DEFINES
// MACROS
// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
// STRUCTS
// TYPEDEFS
// FORWARD DECLARATIONS

/**
* Factory and container for all supported codec types
*/
class SdpCodecFactory
{
   /* //////////////////////////// PUBLIC //////////////////////////////////// */
public:

   /* ============================ CREATORS ================================== */

   /** Default constructor */
   SdpCodecFactory(int numCodecs = 0, SdpCodec* codecArray[] = NULL);

   /** Constructor from list of SdpCodec instances */
   SdpCodecFactory(const UtlSList& sdpCodecList);

   /** Destructor */
   ~SdpCodecFactory();

   /* ============================ MANIPULATORS ============================== */

   /** Append a new codec type to the list of known codecs */
   void addCodec(const SdpCodec& newCodec);

   /**
   * Append a new codec types to the list of known codecs. Copies of codec items
   * are made.
   *
   * @param sdpCodecList List of SdpCodec instances.
   */
   void addCodecs(const UtlSList& sdpCodecList);

   /** Add copies of the array of codecs */
   void addCodecs(int numCodecs, SdpCodec* newCodecs[]);

   /** Assign any unset payload type ids */
   void bindPayloadIds();

   /** If there is a matching codec in this factory, set its payload type to that of the given codec */
   void copyPayloadId(const SdpCodec& codec);

   /** For all matching codecs, copy the payload type from the codecArray to the matching codec in this factory */
   void copyPayloadIds(int numCodecs, SdpCodec* codecArray[]);

   /** For all matching codecs, copy the payload type from the codecArray to the matching codec in this factory */
   void copyPayloadIds(const UtlSList& codecList);

   /** Discard all codecs */
   void clearCodecs(void);

   int buildSdpCodecFactory(const UtlString &codecList);
   //: Function just called other buildSdpCodecFactory. Here for compatibility

   int buildSdpCodecFactory(int codecCount, SdpCodec::SdpCodecTypes codecTypes[]);
   //: Add the default set of codecs specified in list; returns 0 if OK.

   /* ============================ ACCESSORS ================================= */

   /** Get a codec given an internal codec id. Returns internal pointer! */
   const SdpCodec* getCodec(SdpCodec::SdpCodecTypes internalCodecId);

   /** Get a codec given the payload type id. Returns internal pointer!*/
   const SdpCodec* getCodecByPayloadId(int payloadTypeId);

   /** Get a codec given the mime type and subtype. Returns internal pointer! */
   const SdpCodec* getCodec(const char* mimeType, 
                            const char* mimeSubType);

   /** Get the number of codecs */
   int getCodecCount();

   /** Get the number of codecs by mime type MIME_TYPE_AUDIO or MIME_TYPE_VIDEO */
   int getCodecCount(const UtlString& mimeType);

   /** Gets list of SdpCodec instances */
   void getCodecs(UtlSList& sdpCodecList);

   void getCodecs(int& numCodecs,
                  SdpCodec**& codecArray);

   void getCodecs(int& numCodecs,
                  SdpCodec**& codecArray,
                  const char* mimeType);

   void getCodecs(int& numCodecs, 
                  SdpCodec**& codecArray,
                  const char* mimeType,
                  const char* subMimeType);

   /** String representation of factory and codecs */
   void toString(UtlString& serializedFactory);

   /** Checks if audio/telephone-event is present, and if not adds it */
   static UtlString getFixedAudioCodecs(const UtlString& audioCodecs);

   /* ============================ INQUIRY =================================== */

   /* //////////////////////////// PROTECTED ///////////////////////////////// */
protected:

   /* //////////////////////////// PRIVATE /////////////////////////////////// */
private:
   /** Add a new codec type to the list of known codecs */
   void addCodecNoLock(const SdpCodec& newCodec);

   /** Copy constructor */
   SdpCodecFactory(const SdpCodecFactory& rSdpCodecFactory);

   /** Assignment operator */
   SdpCodecFactory& operator=(const SdpCodecFactory& rhs);

   UtlSList m_codecsList;
   mutable OsRWMutex mReadWriteMutex;
};

/* ============================ INLINE METHODS ============================ */

#endif  // _SdpCodecFactory_h_
