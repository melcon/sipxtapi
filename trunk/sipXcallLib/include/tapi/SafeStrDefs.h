//
// Copyright (C) 2007 Jaroslav Libak
// Licensed to SIPfoundry under a Contributor Agreement.
//
// Copyright (C) 2005-2007 SIPez LLC.
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

#ifndef SafeStrDefs_h__
#define SafeStrDefs_h__

// SYSTEM INCLUDES
// APPLICATION INCLUDES
// DEFINES

#ifdef _WIN32
#define SNPRINTF _snprintf
#else
#define SNPRINTF snprintf
#endif


// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
// STATIC VARIABLE INITIALIZATIONS
// MACROS

#ifdef _WIN32
#define SAFE_STRDUP(X) (((X) == NULL) ? NULL : _strdup((X)))
#else
#define SAFE_STRDUP(X) (((X) == NULL) ? NULL : strdup((X)))
#endif

#define SAFE_STRLEN(X) (((X) == NULL) ? 0 : strlen((X)))

#define MAKESTR(X) #X

// GLOBAL VARIABLES
// GLOBAL FUNCTIONS

#endif // SafeStrDefs_h__
