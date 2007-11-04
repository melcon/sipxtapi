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
    * Deletes singleton manager. Not threadsafe.
    */
   void release();

   //@}

   /* ============================ ACCESSORS ================================= */
   ///@name Accessors
   //@{

   MpAudioStreamId getInputAudioStream() const { return m_inputAudioStream; }
   void setInputAudioStream(MpAudioStreamId val) { m_inputAudioStream = val; }

   MpAudioStreamId getOutputAudioStream() const { return m_outputAudioStream; }
   void setOutputAudioStream(MpAudioStreamId val) { m_outputAudioStream = val; }

   MpAudioDeviceIndex getInputDeviceIndex() const { return m_inputDeviceIndex; }
   void setInputDeviceIndex(MpAudioDeviceIndex val) { m_inputDeviceIndex = val; }

   MpAudioDeviceIndex getOutputDeviceIndex() const { return m_outputDeviceIndex; }
   void setOutputDeviceIndex(MpAudioDeviceIndex val) { m_outputDeviceIndex = val; }

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
