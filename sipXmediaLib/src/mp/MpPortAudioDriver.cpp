//
// Copyright (C) 2007 Jaroslav Libak
//
// $$
///////////////////////////////////////////////////////////////////////////////

// SYSTEM INCLUDES
// APPLICATION INCLUDES
#include <os/OsSysLog.h>
#include <os/OsLock.h>
#include "mp/MpPortAudioDriver.h"
#include <portaudio.h>

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

   PaError err = Pa_Terminate();
   if (err != paNoError)
   {
      UtlString error(Pa_GetErrorText(err));
      OsSysLog::add(FAC_AUDIO, PRI_ERR, "Pa_Terminate failed, error: %s", error.data());
   }
}

MpPortAudioDriver* MpPortAudioDriver::createInstance()
{
   OsLock lock(ms_counterMutex);

   if (ms_instanceCounter == 0)
   {
      // we init portaudio here to avoid throwing exception in constructor
      PaError err = Pa_Initialize();

      if (err == paNoError)
      {
         ms_instanceCounter++;
         return new MpPortAudioDriver();
      }
      else
      {
         UtlString error(Pa_GetErrorText(err));
         OsSysLog::add(FAC_AUDIO, PRI_ERR, "Pa_Initialize failed, error: %s", error.data());
      }
   }

   // only 1 instance of this driver is allowed, or error occurred
   return NULL;
}

/* ============================ FUNCTIONS ================================= */


