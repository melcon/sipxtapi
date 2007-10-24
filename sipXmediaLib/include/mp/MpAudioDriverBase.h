//
// Copyright (C) 2007 Jaroslav Libak
//
// $$
///////////////////////////////////////////////////////////////////////////////

#ifndef MpAudioDriverBase_h__
#define MpAudioDriverBase_h__

// SYSTEM INCLUDES
// APPLICATION INCLUDES
#include <utl/UtlString.h>

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
