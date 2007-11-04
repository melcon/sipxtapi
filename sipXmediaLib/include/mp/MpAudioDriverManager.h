//
// Copyright (C) 2007 Jaroslav Libak
// Licensed under the LGPL license.
// $$
///////////////////////////////////////////////////////////////////////////////

#ifndef MpAudioDriverManager_h__
#define MpAudioDriverManager_h__

// SYSTEM INCLUDES
// APPLICATION INCLUDES
#include <os/OsMutex.h>
#include <utl/UtlBool.h>
#include "mp/MpAudioDriverDefs.h"

// DEFINES
// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
// FORWARD DECLARATIONS
class MpAudioDriverBase;
class UtlString;

// STRUCTS
// TYPEDEFS
// MACROS
// GLOBAL VARIABLES
// GLOBAL FUNCTIONS

/**
* Class responsible for creation and deletion of audio driver instance. Can be used
* to access the current active audio driver and input/output stream Ids
*/
class MpAudioDriverManager
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
    * Gets new or existing instance of MpAudioDriverManager
    *
    * @param bCreate Set to FALSE if new instance shouldn't be created if
    * it doesn't exist
    */
   static MpAudioDriverManager* getInstance(UtlBoolean bCreate = TRUE);

   /**
    * Returns name of current output device and driver name.
    */
   OsStatus getCurrentOutputDevice(UtlString& device, UtlString& driverName) const;

   /**
    * Returns name of current input device and driver name.
    */
   OsStatus getCurrentInputDevice(UtlString& device, UtlString& driverName) const;

   /**
   * Sets current output device. NONE disables it, "Default" will select
   * default one. If it fails, it will try not to disable current stream.
   */
   OsStatus setCurrentOutputDevice(const UtlString& device, const UtlString& driverName);

   /**
   * Sets current input device. NONE disables it, "Default" will select
   * default one. If it fails, it will try not to disable current stream.
   */
   OsStatus setCurrentInputDevice(const UtlString& device, const UtlString& driverName);

   /**
   * Starts input stream.
   */
   OsStatus startInputStream() const;

   /**
   * Starts output stream.
   */
   OsStatus startOutputStream() const;

   /**
    * Aborts input stream.
    */
   OsStatus abortInputStream() const;

   /**
    * Aborts output stream.
    */
   OsStatus abortOutputStream() const;

   /**
    * Closes input stream.
    */
   OsStatus closeInputStream();

   /**
    * Closes output stream.
    */
   OsStatus closeOutputStream();

   /**
    * Deletes singleton manager. Not threadsafe.
    */
   void release();

   //@}

   /* ============================ ACCESSORS ================================= */
   ///@name Accessors
   //@{

   MpAudioStreamId getInputAudioStream() const { return m_inputAudioStream; }
   MpAudioStreamId getOutputAudioStream() const { return m_outputAudioStream; }

   MpAudioDeviceIndex getInputDeviceIndex() const { return m_inputDeviceIndex; }
   MpAudioDeviceIndex getOutputDeviceIndex() const { return m_outputDeviceIndex; }

   MpAudioDriverBase* getAudioDriver() const { return m_pAudioDriver; }

   //@}
   /* ============================ INQUIRY =================================== */
   ///@name Inquiry
   //@{
   //@}

   /* //////////////////////////// PROTECTED ///////////////////////////////// */
protected:

   /* //////////////////////////// PRIVATE /////////////////////////////////// */
private:

   /// Constructor.
   MpAudioDriverManager();

   /// Destructor.
   ~MpAudioDriverManager(void);

   /// Copy constructor (not implemented for this class)
   MpAudioDriverManager(const MpAudioDriverManager& rMpAudioDriverManager);

   /// Assignment operator (not implemented for this class)
   MpAudioDriverManager& operator=(const MpAudioDriverManager& rhs);

   static OsMutex ms_mutex;
   static MpAudioDriverManager* ms_pInstance;

   MpAudioDriverBase* m_pAudioDriver; ///< pointer to audio driver
   MpAudioStreamId m_inputAudioStream; ///< ID if input audio stream
   MpAudioDeviceIndex m_inputDeviceIndex; ///< index of current input device
   MpAudioStreamId m_outputAudioStream; ///< ID of output audio stream
   MpAudioDeviceIndex m_outputDeviceIndex; ///< index of current output device
};

#endif // MpAudioDriverManager_h__
