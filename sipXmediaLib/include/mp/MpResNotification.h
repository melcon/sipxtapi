//
// Copyright (C) 2007 Jaroslav Libak
//
// $$
///////////////////////////////////////////////////////////////////////////////


#ifndef _MpResNotification_h_
#define _MpResNotification_h_

// SYSTEM INCLUDES
// APPLICATION INCLUDES
#include <os/OsStatus.h>
#include <os/OsIntTypes.h>
#include <os/OsNotification.h>

// DEFINES
// MACROS
// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
// STRUCTS
// TYPEDEFS
// FORWARD DECLARATIONS
class MpResource;

/**
 * Generic notification for sending synchronous notifications to resources.
 * Should be used where direct knowledge of parent object is not desired.
 */
class MpResNotification : public OsNotification
{
/* //////////////////////////// PUBLIC //////////////////////////////////// */
public:

   typedef enum
   {
      MP_RES_DTMF_2833_NOTIFICATION = 0 // used to notify decoder from MpdPtAVT
   }MpResNotificationType;

/* ============================ CREATORS ================================== */

   MpResNotification(MpResource* pResource, MpResNotificationType type);;
     //:Default constructor

   virtual ~MpResNotification();;
     //:Destructor

/* ============================ MANIPULATORS ============================== */

   virtual OsStatus signal(const intptr_t eventData);
     //:Signal the occurrence of the event

   /**
    * Disable sending notifications.
    */
   void enable();

   /**
    * Enable sending notifications. On by default.
    */
   void disable();

/* ============================ ACCESSORS ================================= */

/* ============================ INQUIRY =================================== */

/* //////////////////////////// PROTECTED ///////////////////////////////// */
protected:

/* //////////////////////////// PRIVATE /////////////////////////////////// */
private:
   MpResNotificationType m_type;
   MpResource* m_pResource;
   volatile bool m_bEnabled;

   MpResNotification(const MpResNotification& rOsNotification);
     //:Copy constructor (not implemented for this class)

   MpResNotification& operator=(const MpResNotification& rhs);
     //:Assignment operator (not implemented for this class)

};

/* ============================ INLINE METHODS ============================ */

#endif  // _MpResNotification_h_

