//
// Copyright (C) 2007 Jaroslav Libak
//
// $$
///////////////////////////////////////////////////////////////////////////////

#ifndef MpHostAudioApiInfo_h__
#define MpHostAudioApiInfo_h__

// SYSTEM INCLUDES
// APPLICATION INCLUDES
#include <utl/UtlString.h>

// DEFINES
// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
// FORWARD DECLARATIONS
// STRUCTS
// TYPEDEFS

typedef int MpAudioDeviceIndex;
typedef int MpHostAudioApiIndex;

typedef enum MpHostAudioApiTypeId
{
   MP_INDEVELOPMENT = 0,
   MP_DIRECTSOUND = 1,
   MP_MME = 2,
   MP_ASIO = 3,
   MP_SOUNDMANAGER = 4,
   MP_COREAUDIO = 5,
   MP_OSS = 7,
   MP_ALSA = 8,
   MP_AL = 9,
   MP_BEOS = 10,
   MP_WDMKS = 11,
   MP_JACK = 12,
   MP_WASAPI = 13,
   MP_AUDIOSCIENCEHPI = 14
} MpHostAudioApiTypeId;

// MACROS
// GLOBAL VARIABLES
// GLOBAL FUNCTIONS

/**
* Class describing host audio API implemented by certain audio driver.
*/
class MpHostAudioApiInfo
{
   /* //////////////////////////// PUBLIC //////////////////////////////////// */
public:
   /* ============================ CREATORS ================================== */
   ///@name Creators
   //@{
   /**
   * Constructor.
   */
   MpHostAudioApiInfo();

   /**
   * Destructor.
   */
   virtual ~MpHostAudioApiInfo(void);
   //@}

   /* ============================ MANIPULATORS ============================== */
   ///@name Manipulators
   //@{
   //@}

   /* ============================ ACCESSORS ================================= */
   ///@name Accessors
   //@{

   MpHostAudioApiTypeId getTypeId() const { return m_TypeId; }
   void setTypeId(MpHostAudioApiTypeId val) { m_TypeId = val; }

   UtlString getName() const { return m_name; }
   void setName(const UtlString& val) { m_name = val; }

   int getDeviceCount() const { return m_deviceCount; }
   void setDeviceCount(int val) { m_deviceCount = val; }

   MpAudioDeviceIndex getDefaultInputDevice() const { return m_defaultInputDevice; }
   void setDefaultInputDevice(MpAudioDeviceIndex val) { m_defaultInputDevice = val; }

   MpAudioDeviceIndex getDefaultOutputDevice() const { return m_defaultOutputDevice; }
   void setDefaultOutputDevice(MpAudioDeviceIndex val) { m_defaultOutputDevice = val; }

   //@}
   /* ============================ INQUIRY =================================== */
   ///@name Inquiry
   //@{
   //@}

   /* //////////////////////////// PROTECTED ///////////////////////////////// */
protected:

   /* //////////////////////////// PRIVATE /////////////////////////////////// */
private:

   MpHostAudioApiTypeId m_TypeId; ///< unique identifier of host API
   UtlString m_name; ///< textual description of host api
   int m_deviceCount; ///< number of devices that are available to this host API
   MpAudioDeviceIndex m_defaultInputDevice; ///< index of default input device
   MpAudioDeviceIndex m_defaultOutputDevice; ///< index of default output device
};

#endif // MpHostAudioApiInfo_h__
