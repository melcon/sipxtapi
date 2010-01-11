//
// Copyright (C) 2004-2006 SIPfoundry Inc.
// Licensed by SIPfoundry under the LGPL license.
//
// Copyright (C) 2004-2006 Pingtel Corp.  All rights reserved.
// Licensed to SIPfoundry under a Contributor Agreement.
//
// Copyright (C) 2007 Jaroslav Libak
// Licensed under the LGPL license.
// $$
///////////////////////////////////////////////////////////////////////////////

#ifndef OsNetworkAdapterInfo_h__
#define OsNetworkAdapterInfo_h__

// SYSTEM INCLUDES
// APPLICATION INCLUDES
#include <utl/UtlString.h>

// DEFINES
// MACROS
// EXTERNAL FUNCTIONS
// CONSTANTS
// STRUCTS
// TYPEDEFS
// MACROS
// FORWARD DECLARATIONS

/**
 * Contains information about network adapter like name, description, ip address.
 * Each network adapter must have a unique name.
 *
 * This class is immutable.
 */
class OsNetworkAdapterInfo
{
   /* //////////////////////////// PUBLIC //////////////////////////////////// */
public:
   /* ============================ CREATORS ================================== */

   /**
    * Constructor.
    */
   OsNetworkAdapterInfo(unsigned long index,
                        const UtlString& ipAddress,
                        const UtlString& name,
                        const UtlString& description);

   /**
    * Destructor.
    */
   ~OsNetworkAdapterInfo();

   /**
    * Copy constructor.
    */
   OsNetworkAdapterInfo(const OsNetworkAdapterInfo& rhs);

   /* ============================ MANIPULATORS ============================== */

   /* ============================ ACCESSORS ================================= */

   unsigned long getIndex() const { return m_index; }
   void getIpAddress(UtlString& ipAddress) const { ipAddress = m_ipAddress; }
   void getName(UtlString& name) const { name = m_name; }
   void getDescription(UtlString& description) const { description = m_description; }

   /* ============================ INQUIRY =================================== */

   /* //////////////////////////// PROTECTED ///////////////////////////////// */
protected:

   /* //////////////////////////// PRIVATE /////////////////////////////////// */
private:

   OsNetworkAdapterInfo& operator=(const OsNetworkAdapterInfo& rhs);

   unsigned long m_index; ///< integer index of network adapter
   UtlString m_ipAddress; ///< ip address assigned to network adapter
   UtlString m_name; ///< name of network adapter, in Linux "eth0" etc
   UtlString m_description; ///< printable name of network adapter for Windows
};

#endif // OsNetworkAdapterInfo_h__
