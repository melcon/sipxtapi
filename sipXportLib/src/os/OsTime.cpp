//
// Copyright (C) 2004-2006 SIPfoundry Inc.
// Licensed by SIPfoundry under the LGPL license.
//
// Copyright (C) 2004-2006 Pingtel Corp.  All rights reserved.
// Licensed to SIPfoundry under a Contributor Agreement.
//
// $$
///////////////////////////////////////////////////////////////////////////////

// SYSTEM INCLUDES
#include <assert.h>

// APPLICATION INCLUDES
#include "os/OsTime.h"

// DEFINES 
#define INFINITE_TIME 0x7FFFFFFF

// EXTERNAL FUNCTIONS

// EXTERNAL VARIABLES

// CONSTANTS

// STATIC VARIABLE INITIALIZATIONS
const long OsTime::MSECS_PER_SEC   = 1000;
const long OsTime::USECS_PER_MSEC  = 1000;
const long OsTime::USECS_PER_SEC   = 1000000;


/* //////////////////////////// PUBLIC //////////////////////////////////// */

/* ============================ CREATORS ================================== */

// Constructor (creates a zero duration interval)
OsTime::OsTime()
{
   init();
}

// Constructor
OsTime::OsTime(const long msecs)
{
   init();

   // Calculate the number of seconds and microseconds
   if (msecs < 0 || msecs >= MSECS_PER_SEC)
   {
       mSeconds =  msecs / MSECS_PER_SEC;
       mUsecs   = (msecs % MSECS_PER_SEC) * USECS_PER_MSEC;

      if (msecs < 0)
      {
         mSeconds -= 1;
         mUsecs   += USECS_PER_SEC;
      }
   }
   else     // 0 <= msecs < MSECS_PER_SEC
   {
       mUsecs = msecs * USECS_PER_MSEC;
   }
}

// Constructor
OsTime::OsTime(TimeQuantity quantity)
{
   init();

   if (quantity == OS_INFINITY)
   {
      mSeconds = INFINITE_TIME;
      mUsecs = INFINITE_TIME;
   }
   else
   {
      // NO_WAIT_TIME
      mSeconds = 0;
      mUsecs   = 0;
   }
}

// Constructor
OsTime::OsTime(const long seconds, const long usecs)
{
   init();
   mSeconds = seconds;

   // if necessary, adjust seconds and usecs so that the following holds:
   //    0 <= usecs <= (USECS_PER_SEC-1)
   if (usecs < 0 || usecs >= USECS_PER_SEC)
   {
      mSeconds += usecs / USECS_PER_SEC;
      mUsecs   =  usecs % USECS_PER_SEC;

      if (usecs < 0)
      {
         mSeconds -= 1;
         mUsecs   += USECS_PER_SEC;
      }
   }
   else     // 0 <= usecs < USECS_PER_SEC
   {
      mUsecs = usecs;
   }
}

// Copy constructor
OsTime::OsTime(const OsTime& rOsTime)
{
   mSeconds = rOsTime.mSeconds;
   mUsecs   = rOsTime.mUsecs;
}

// Destructor
OsTime::~OsTime()
{
   // no work required
}

/* ============================ MANIPULATORS ============================== */

// Assignment operator
OsTime& OsTime::operator=(TimeQuantity rhs)
{
   if (rhs == OS_INFINITY)
   {
      mSeconds = INFINITE_TIME;
      mUsecs = INFINITE_TIME;
   }
   else
   {
      // NO_WAIT_TIME
      mSeconds = 0;
      mUsecs   = 0;
   }

   return *this;
}

// Assignment operator
OsTime& 
OsTime::operator=(const OsTime& rhs)
{
   if (this == &rhs)            // handle the assignment to self case
      return *this;

   mSeconds = rhs.mSeconds;
   mUsecs   = rhs.mUsecs;

   return *this;
}

// Addition operator
OsTime OsTime::operator+(const OsTime& rhs)
{
   OsTime sum(this->mSeconds + rhs.mSeconds,
              this->mUsecs   + rhs.mUsecs);
   return sum;
}

// Subtraction operator
OsTime OsTime::operator-(const OsTime& rhs)
{
   OsTime diff(this->mSeconds - rhs.mSeconds,
               this->mUsecs   - rhs.mUsecs);
   return diff;
}

// Increment operator
OsTime OsTime::operator+=(const OsTime& rhs)
{
   *this = *this + rhs;
   return *this;
}
     
// Decrement operator
OsTime OsTime::operator-=(const OsTime& rhs)
{
   *this = *this - rhs;
   return *this;
}

// Test for equality operator
bool OsTime::operator==(const OsTime& rhs) const
{
   return (this->mSeconds == rhs.mSeconds) &&
          (this->mUsecs   == rhs.mUsecs);
}

// Test for inequality operator
bool OsTime::operator!=(const OsTime& rhs) const
{
   return (this->mSeconds != rhs.mSeconds) ||
          (this->mUsecs   != rhs.mUsecs);
}

// Test for greater than
bool OsTime::operator>(const OsTime& rhs) const
{
   if (this->mSeconds >= 0)
   {  // "this" is a positive time value
      return (this->mSeconds > rhs.mSeconds) ||
              ((this->mSeconds == rhs.mSeconds) &&
               (this->mUsecs   >  rhs.mUsecs));
   }
   else
   {  // "this" is a negative time value
      return (this->mSeconds > rhs.mSeconds) ||
              ((this->mSeconds == rhs.mSeconds) &&
               (this->mUsecs   <  rhs.mUsecs));
   }
}

// Test for greater than or equal
bool OsTime::operator>=(const OsTime& rhs) const
{
   if (this->mSeconds >= 0)
   {  // "this" is a positive time value
      return (this->mSeconds > rhs.mSeconds) ||
              ((this->mSeconds == rhs.mSeconds) &&
               (this->mUsecs   >= rhs.mUsecs));
   }
   else
   {  // "this" is a negative time value
      return (this->mSeconds > rhs.mSeconds) ||
              ((this->mSeconds == rhs.mSeconds) &&
               (this->mUsecs   <= rhs.mUsecs));
   }
}

// Test for less than
bool OsTime::operator<(const OsTime& rhs) const
{
   if (this->mSeconds >= 0)
   {  // "this" is a positive time value
      return (this->mSeconds < rhs.mSeconds) ||
              ((this->mSeconds == rhs.mSeconds) &&
               (this->mUsecs   <  rhs.mUsecs));
   }
   else
   {  // "this" is a negative time value
      return (this->mSeconds < rhs.mSeconds) ||
              ((this->mSeconds == rhs.mSeconds) &&
               (this->mUsecs   >  rhs.mUsecs));
   }
}

// Test for less than or equal
bool OsTime::operator<=(const OsTime& rhs) const
{
   if (this->mSeconds >= 0)
   {  // "this" is a positive time value
      return (this->mSeconds < rhs.mSeconds) ||
              ((this->mSeconds == rhs.mSeconds) &&
               (this->mUsecs   <= rhs.mUsecs));
   }
   else
   {  // "this" is a negative time value
      return (this->mSeconds < rhs.mSeconds) ||
              ((this->mSeconds == rhs.mSeconds) &&
               (this->mUsecs   >= rhs.mUsecs));
   }
}

/* ============================ ACCESSORS ================================= */

// Convert the time interval to milliseconds
long OsTime::cvtToMsecs(void) const
{
   return (mSeconds * MSECS_PER_SEC) + (mUsecs / USECS_PER_MSEC);
}

/* ============================ INQUIRY =================================== */

// Return TRUE if the time interval is infinite
UtlBoolean OsTime::isInfinite(void) const
{
   if (mSeconds == INFINITE_TIME && mUsecs == INFINITE_TIME)
      return TRUE;
   else
      return FALSE;
}

// Return TRUE if the time interval is zero (no wait)
UtlBoolean OsTime::isNoWait(void) const
{
   if (mSeconds == 0 && mUsecs == 0)
      return TRUE;
   else
      return FALSE;
}

/* //////////////////////////// PROTECTED ///////////////////////////////// */

/* //////////////////////////// PRIVATE /////////////////////////////////// */

// Initialize the instance variables for a newly constructed object
void OsTime::init(void)
{
   mSeconds = 0;
   mUsecs   = 0;
}

/* ============================ FUNCTIONS ================================= */


