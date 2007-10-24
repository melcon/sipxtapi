//
// Copyright (C) 2007 Jaroslav Libak
//
// $$
///////////////////////////////////////////////////////////////////////////////

// SYSTEM INCLUDES
// APPLICATION INCLUDES
#include <os/OsLock.h>
#include "mp/MpPortAudioDriver.h"

// DEFINES
// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
// STATIC VARIABLE INITIALIZATIONS
unsigned int MpPortAudioDriver::ms_instanceCounter = 0;
OsMutex MpPortAudioDriver::ms_counterMutex(OsMutex::Q_FIFO);
UtlString MpPortAudioDriver::ms_driverName("Portaudio");
UtlString MpPortAudioDriver::ms_driverVersion("V19");

// MACROS
// GLOBAL VARIABLES
// GLOBAL FUNCTIONS

/* //////////////////////////// PUBLIC //////////////////////////////////// */

/* ============================ CREATORS ================================== */

/* ============================ MANIPULATORS ============================== */

void MpPortAudioDriver::pushFrame()
{

}

void MpPortAudioDriver::pullFrame()
{

}

const UtlString& MpPortAudioDriver::getDriverName() const
{
   return ms_driverName;
}

const UtlString& MpPortAudioDriver::getDriverVersion() const
{
   return ms_driverVersion;
}

void MpPortAudioDriver::release()
{
   delete this;
}

/* ============================ ACCESSORS ================================= */

/* ============================ INQUIRY =================================== */

/* //////////////////////////// PROTECTED ///////////////////////////////// */

/* //////////////////////////// PRIVATE /////////////////////////////////// */

MpPortAudioDriver::MpPortAudioDriver() : m_memberMutex(OsMutex::Q_FIFO)
{
}

MpPortAudioDriver::~MpPortAudioDriver(void)
{
   OsLock lock(ms_counterMutex);

   // decrease counter in thread safe manner
   ms_instanceCounter--;
}

MpPortAudioDriver* MpPortAudioDriver::createInstance()
{
   OsLock lock(ms_counterMutex);

   if (ms_instanceCounter == 0)
   {
      ms_instanceCounter++;
      return new MpPortAudioDriver();
   }
   else
   {
      // only 1 instance of this driver is allowed
      return NULL;
   }   
}

/* ============================ FUNCTIONS ================================= */


