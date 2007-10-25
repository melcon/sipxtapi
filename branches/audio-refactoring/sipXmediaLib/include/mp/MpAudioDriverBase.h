//
// Copyright (C) 2007 Jaroslav Libak
//
// $$
///////////////////////////////////////////////////////////////////////////////

#ifndef MpAudioDriverBase_h__
#define MpAudioDriverBase_h__

// SYSTEM INCLUDES
// APPLICATION INCLUDES
#include <os/OsStatus.h>
#include <utl/UtlString.h>
#include "mp/MpHostAudioApiInfo.h"

// DEFINES
// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
// FORWARD DECLARATIONS
class MpAudioDeviceInfo;

// STRUCTS
// TYPEDEFS
// MACROS
// GLOBAL VARIABLES
// GLOBAL FUNCTIONS

/**
* Base class for all audio drivers. Constructor and destructor are private.
* Instance is created via MpAudioDriverFactory which will create instance by
* a private static method createInstance.
*/
class MpAudioDriverBase
{
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
   virtual void pushFrame() = 0;

   /**
    * Pulls frame from given input audio stream. Signature not finished.
    */
   virtual void pullFrame() = 0;

   /**
    * Returns name of driver.
    */
   virtual const UtlString& getDriverName() const  = 0;

   /**
    * Returns driver version string.
    */
   virtual const UtlString& getDriverVersion() const = 0;

   /**
    * Returns number of host apis available for this driver.
    * Some drivers will have only 1 host API. Portaudio is an
    * exception.
    *
    * @param count Number of host apis if successful
    * @returns OS_SUCCESS if successful
    */
   virtual OsStatus getHostApiCount(int& count) const = 0;

   /**
   * Returns index of default driver host API.
   *
   * @param apiIndex Index of default host api if successful
   * @returns OS_SUCCESS if successful
   */
   virtual OsStatus getDefaultHostApi(MpHostAudioApiIndex& apiIndex) const = 0;

   /**
   * Gets information about given host API.
   *
   * @param hostApiIndex Index of host api to get information for.
   * @param apiInfo
   */
   virtual OsStatus getHostApiInfo(MpHostAudioApiIndex hostApiIndex,
                                   MpHostAudioApiInfo& apiInfo) const = 0;

   /**
   * Converts host type id to host api index. 
   *
   * @param hostApiTypeId Id of host api belonging to MpHostAudioApiTypeId enum
   * @param hostApiIndex Index of host api
   */
   virtual OsStatus hostApiTypeIdToHostApiIndex(MpHostAudioApiTypeId hostApiTypeId,
                                                MpHostAudioApiIndex& hostApiIndex) const = 0;


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
                                                    MpAudioDeviceIndex& deviceIndex) const = 0;

   /**
   * Returns the number of available devices.
   * 
   * @param deviceCount Number of available devices
   * @returns OS_SUCCESS if successful
   */
   virtual OsStatus getDeviceCount(MpAudioDeviceIndex& deviceCount) const = 0;

   /**
   * Returns index of default input device
   *
   * @param deviceIndex Index of default input device
   * @returns OS_SUCCESS if successful
   */
   virtual OsStatus getDefaultInputDevice(MpAudioDeviceIndex& deviceIndex) const = 0;

   /**
   * Returns index of default output device
   *
   * @param deviceIndex Index of default output device
   * @returns OS_SUCCESS if successful
   */
   virtual OsStatus getDefaultOutputDevice(MpAudioDeviceIndex& deviceIndex) const = 0;

   /**
   * Returns information about given audio device.
   *
   * @param deviceIndex Index of audio device.
   * @param deviceInfo Information about audio device.
   * @returns OS_SUCCESS if successful
   */
   virtual OsStatus getDeviceInfo(MpAudioDeviceIndex deviceIndex,
                                  MpAudioDeviceInfo& deviceInfo) const = 0;

   /**
    * Deletes audio driver. Use instead of deleting it directly. Implemented
    * by subclass.
    */
   virtual void release() = 0;

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

   /// Constructor.
   MpAudioDriverBase();

   /// Destructor.
   virtual ~MpAudioDriverBase(void);

   /* //////////////////////////// PRIVATE /////////////////////////////////// */
private:

   /// Copy constructor (not implemented for this class)
   MpAudioDriverBase(const MpAudioDriverBase& rMpAudioDriverBase);

   /// Assignment operator (not implemented for this class)
   MpAudioDriverBase& operator=(const MpAudioDriverBase& rhs);
};

#endif // MpAudioDriverBase_h__
