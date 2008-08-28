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

#ifndef SipLineList_h__
#define SipLineList_h__

// SYSTEM INCLUDES
// APPLICATION INCLUDES
#include <utl/UtlHashMap.h>
#include <net/SipLine.h>

// DEFINES
// MACROS
// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
// STRUCTS
// TYPEDEFS
// FORWARD DECLARATIONS

/**
 * List of SipLine instances. Only one SipLine with given lineURI may be present
 * at time.
 * Not thread safe.
 */
class SipLineList
{
   /* //////////////////////////// PUBLIC //////////////////////////////////// */
public:
   /* ============================ CREATORS ================================== */

   SipLineList();
   virtual ~SipLineList();

   /* ============================ MANIPULATORS ============================== */

   /** Adds a new line to the list. Returns FALSE if it was not added. */
   UtlBoolean add(SipLine* pLine);

   /** Adds a copy of line to the list. Returns FALSE if it was not added. */
   UtlBoolean add(const SipLine& line);

   /** Removes line with the same lineIdentityUri from line list and deletes it */
   UtlBoolean remove(const SipLine& line);

   /** Removes line with the same lineIdentityUri from line list and deletes it */
   UtlBoolean remove(const Url& lineIdentityUri);

   /** Removes all lines from list and deletes them */
   void removeAll();

   /** Prints line list into log file */
   void dumpLines();

   /**
    * Tries to find line according to given parameters. First try lookup by
    * lineId if its supplied. If lineId is not supplied, lookup by identityUri. If
    * not found by identityUri, try by userId.
    */
   SipLine* findLine(const UtlString& lineId,
                     const Url& lineUri,
                     const UtlString& userId) const;

   /* ============================ ACCESSORS ================================= */

   /** Copies line clones into supplied list */
   void getLineCopies(UtlSList& lineList) const;

   /** Gets number of lines in list */
   size_t getLinesCount() const;

   /** Gets line from the list by identityUri */
   SipLine* getLine(const Url& identityUri) const;

   /** Gets line from the list by lineId */
   SipLine* getLine(const UtlString& lineId) const;

   /**
    * Gets line from the list by userID. Multiple lines might match, but
    * only the first match is returned.
    */
   SipLine* getLineByUserId(const UtlString& userId) const;

   /* ============================ INQUIRY =================================== */

   /** Checks whether this line is present in line list */
   UtlBoolean lineExists(const SipLine& line) const;

   /** Checks whether this line is present in line list */
   UtlBoolean lineExists(const Url& lineIdentityUri) const;

   /* //////////////////////////// PROTECTED ///////////////////////////////// */
protected:

   /* //////////////////////////// PRIVATE /////////////////////////////////// */
private:
   mutable UtlHashMap m_lineMap; ///< map of SipLine instances

};

#endif // SipLineList_h__
