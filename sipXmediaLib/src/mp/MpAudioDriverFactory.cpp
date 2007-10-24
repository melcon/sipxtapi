//
// Copyright (C) 2007 Jaroslav Libak
//
// $$
///////////////////////////////////////////////////////////////////////////////

// SYSTEM INCLUDES
// APPLICATION INCLUDES
#include <os/OsSysLog.h>
#include <utl/UtlDefs.h>
#include "mp/MpAudioDriverFactory.h"
#include "mp/MpAudioDriverBase.h"
#include "mp/MpPortAudioDriver.h"

// DEFINES
// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
// STATIC VARIABLE INITIALIZATIONS
// MACROS
// GLOBAL VARIABLES
// GLOBAL FUNCTIONS

/* //////////////////////////// PUBLIC //////////////////////////////////// */

/* ============================ CREATORS ================================== */

/* ============================ MANIPULATORS ============================== */

MpAudioDriverBase* MpAudioDriverFactory::createAudioDriver(enum AudioDriverImplementation implementation)
{
   OsSysLog::add(FAC_AUDIO, PRI_DEBUG, "createAudioDriver implementation=%d", (int)implementation);

   switch(implementation)
   {
   case AUDIO_DRIVER_PORTAUDIO:
      return new MpPortAudioDriver();
   default:
      // illegal implementation type
      return NULL;
   }
}

UtlString MpAudioDriverFactory::getDriverNameVersion(enum AudioDriverImplementation implementation)
{
   switch(implementation)
   {
   case AUDIO_DRIVER_PORTAUDIO:
      {
         UtlString nameVersion = MpPortAudioDriver::ms_driverName + " " + MpPortAudioDriver::ms_driverVersion;
         return nameVersion;
      }
   default:
      // illegal implementation type
      return UtlString("Error, unknown audio driver name");
   }
}

/* ============================ ACCESSORS ================================= */

/* ============================ INQUIRY =================================== */

/* //////////////////////////// PROTECTED ///////////////////////////////// */

/* //////////////////////////// PRIVATE /////////////////////////////////// */


MpAudioDriverFactory::MpAudioDriverFactory()
{

}

MpAudioDriverFactory::~MpAudioDriverFactory(void)
{

}


/* ============================ FUNCTIONS ================================= */
