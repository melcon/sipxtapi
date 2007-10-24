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
   static OsMutex ms_counterMutex; ///< mutex for protection of ms_instanceCounter
   static UtlString ms_driverName; ///< name of driver
   static UtlString ms_driverVersion; ///< version of driver

   OsMutex m_memberMutex; ///< mutex for protection of members
};

#endif // MpPortAudioDriver_h__
