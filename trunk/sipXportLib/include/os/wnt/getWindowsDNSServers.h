//
// Copyright (C) 2006-2007 SIPez LLC.
// Licensed to SIPfoundry under a Contributor Agreement.
//
// Copyright (C) 2004-2007 SIPfoundry Inc.
// Licensed by SIPfoundry under the LGPL license.
//
// Copyright (C) 2004-2006 Pingtel Corp.  All rights reserved.
// Licensed to SIPfoundry under a Contributor Agreement.
//
// $$
///////////////////////////////////////////////////////////////////////////////

#ifndef getWindowsDNSServers_h_
#define getWindowsDNSServers_h_

#ifdef _WIN32

// SYSTEM INCLUDES
#include <winsock2.h>
#include <iptypes.h>
#include <time.h>

// APPLICATION INCLUDES
// DEFINES

#define MAXIPLEN 40 

// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
// FORWARD DECLARATIONS
#ifdef __cplusplus

class UtlString;

// STRUCTS
// TYPEDEFS
// MACROS
// GLOBAL VARIABLES
// GLOBAL FUNCTIONS

/// Get this host's IP addresses.
extern "C" bool getAllLocalHostIps(const class HostAdapterAddress* localHostAddresses[], int &numAddresses);
/**<
*  @param localHostAddresses Preallocated array for determined IP addresses.
*  @param numAddresses Input: Size of the preallocated array.
*                      Output: Number of IPs found by the system.
*/
extern "C" bool getAdapterName(UtlString &adapterName, const UtlString &ipAddress);
//: Returns a generated adapter name associated with the IP address

extern "C" int getWindowsDNSServers(char DNSServers[][MAXIPLEN], int max, const char* szLocalIp);

#else

int getWindowsDNSServers(char DNSServers[][MAXIPLEN], int max, const char* szLocalIp);

#endif

#endif //WIN32

#endif // getWindowsDNSServers_h_
