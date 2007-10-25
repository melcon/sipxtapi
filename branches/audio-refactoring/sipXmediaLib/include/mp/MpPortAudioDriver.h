//
// Copyright (C) 2007 Jaroslav Libak
//
// $$
///////////////////////////////////////////////////////////////////////////////

#ifndef MpPortAudioDriver_h__
#define MpPortAudioDriver_h__

// SYSTEM INCLUDES
// APPLICATION INCLUDES
#include <os/OsMutex.h>
#include <utl/UtlString.h>
#include "mp/MpAudioDriverBase.h"
#include "mp/MpHostAudioApiInfo.h"
#include "mp/MpAudioDeviceInfo.h"

// DEFINES
// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
// FORWARD DECLARATIONS
// STRUCTS
// TYPEDEFS
// MACROS
// GLOBAL VARIABLES
// GLOBAL FUNCTIONS

/**
* Wrapper for Portaudio V19 driver. It cannot be created directly.
* Use MpAudioDriverFactory to create it. Only 1 instance can exist
* at time.
*/
class MpPortAudioDriver : public MpAudioDriverBase
{
   /**
    * Make MpAudioDriverFactory a friend so it can create this instance
    */
   friend class MpAudioDriverFactory;

   /* //////////////////////////// PUBLIC //////////////////////////////////// */
public:
   /* ============================ CREATORS ================================== */
   ///@name Creators
   //@{
   //@}

   /* ============================ MANIPULATORS ============================== */
   ///@name Manipulators
   //@{

   /**
   * Pushes frame to given output audio stream. Signature not finished.
   */
   virtual void pushFrame();

   /**
   * Pulls frame from given input audio stream. Signature not finished.
   */
   virtual void pullFrame();

   /**
   * Returns name of driver. Threadsafe.
   */
   virtual const UtlString& getDriverName() const;

   /**
   * Returns driver version string. Threadsafe.
   */
   virtual const UtlString& getDriverVersion() const;

   /**
    * Returns number of host apis available for this driver. Portaudio
    * will usually have multiple apis for each platform.
    *
    * @param count Number of host apis if successful
    * @returns OS_SUCCESS if successful
    */
   virtual OsStatus getHostApiCount(int& count) const;

   /**
    * Returns index of default driver host api.
    *
    * @param apiIndex Index of default host api if successful
    * @returns OS_SUCCESS if successful
    */
   virtual OsStatus getDefaultHostApi(MpHostAudioApiIndex& apiIndex) const;

   /**
    * Gets information about given host API. Portaudio
    * implements several host apis.
    *
    * @param hostApiIndex Index of host api to get information for.
    * @param apiInfo
    */
   virtual OsStatus getHostApiInfo(MpHostAudioApiIndex hostApiIndex,
                                   MpHostAudioApiInfo& apiInfo) const;

   /**
    * Converts host type id to host api index. 
    *
    * @param hostApiTypeId Id of host api belonging to MpHostAudioApiTypeId enum
    * @param hostApiIndex Index of host api
    * @returns OS_SUCCESS if successful
    */
   virtual OsStatus hostApiTypeIdToHostApiIndex(MpHostAudioApiTypeId hostApiTypeId,
                                                MpHostAudioApiIndex& hostApiIndex) const;

   /**
    * Converts host api device index to driver device index. These indexes can differ.
    *
    * @param hostApiIndex Index of host api
    * @param hostApiDeviceIndex Index of device for certain host api
    * @param deviceIndex Global index of audio device
    * @returns OS_SUCCESS if successful
    */
   virtual OsStatus hostApiDeviceIndexToDeviceIndex(MpHostAudioApiIndex hostApiIndex,
                                                    int hostApiDeviceIndex,
                                                    MpAudioDeviceIndex& deviceIndex) const;

   /**
    * Returns the number of available devices.
    * 
    * @param deviceCount Number of available devices
    * @returns OS_SUCCESS if successful
    */
   virtual OsStatus getDeviceCount(MpAudioDeviceIndex& deviceCount) const;

   /**
    * Returns index of default input device
    *
    * @param deviceIndex Index of default input device
    * @returns OS_SUCCESS if successful
    */
   virtual OsStatus getDefaultInputDevice(MpAudioDeviceIndex& deviceIndex) const;

   /**
   * Returns index of default output device
   *
   * @param deviceIndex Index of default output device
   * @returns OS_SUCCESS if successful
   */
   virtual OsStatus getDefaultOutputDevice(MpAudioDeviceIndex& deviceIndex) const;

   /**
    * Returns information about given audio device.
    *
    * @param deviceIndex Index of audio device.
    * @param deviceInfo Information about audio device.
    * @returns OS_SUCCESS if successful
    */
   virtual OsStatus getDeviceInfo(MpAudioDeviceIndex deviceIndex,
                                  MpAudioDeviceInfo& deviceInfo) const;

   /**
   * Deletes audio driver. Use instead of deleting it directly.
   * Not threadsafe.
   */
   virtual void release();

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

   /// Private constructor.
   MpPortAudioDriver();

   /// Private destructor.
   virtual ~MpPortAudioDriver(void);

   /// Copy constructor (not implemented for this class)
   MpPortAudioDriver(const MpPortAudioDriver& rMpAudioDriver);

   /// Assignment operator (not implemented for this class)
   MpPortAudioDriver& operator=(const MpPortAudioDriver& rhs);

   /**
    * Creates instance of this class. To be used only by MpAudioDriverFactory.
    * Threadsafe.
    *
    * @returns instance if it can be created or NULL if no more instances are allowed.
    */
   static MpPortAudioDriver* createInstance();

   static unsigned int ms_instanceCounter; ///< counter of MpPortAudioDriver instances
   static OsMutex ms_driverMutex; ///< mutex for protection of audio driver calls
   static UtlString ms_driverName; ///< name of driver
   static UtlString ms_driverVersion; ///< version of driver

   mutable OsMutex m_memberMutex; ///< mutex for protection of members
};

#endif // MpPortAudioDriver_h__
