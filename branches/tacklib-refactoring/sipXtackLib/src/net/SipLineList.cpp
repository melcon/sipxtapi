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

// SYSTEM INCLUDES
// APPLICATION INCLUDES
#include <os/OsSysLog.h>
#include <utl/UtlHashMapIterator.h>
#include "net/SipLineList.h"

// DEFINES
// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
// STATIC VARIABLE INITIALIZATIONS
// MACROS
// GLOBAL VARIABLES
// GLOBAL FUNCTIONS

SipLineList::SipLineList()
: m_lineMap()
{

}

SipLineList::~SipLineList()
{    
   m_lineMap.destroyAll();
}

UtlBoolean SipLineList::add(SipLine *pLine)
{
   if (pLine && !lineExists(*pLine))
   {
      // if it doesn't already exist, add to bag
      m_lineMap.insertKeyAndValue(pLine->getIdentityUri().toString().clone(), pLine);
      return TRUE;
   }

   return FALSE;
}

UtlBoolean SipLineList::add(const SipLine& line)
{
   SipLine* pLine = new SipLine(line);

   if (add(pLine))
   {
      // addition successful
      return TRUE;
   }
   else
   {
      // delete copy
      delete pLine;
      pLine = NULL;
      return FALSE;
   }
}

UtlBoolean SipLineList::remove(const SipLine& line)
{
   return m_lineMap.destroy(&line.getIdentityUri().toString());
}

UtlBoolean SipLineList::remove(const Url& lineIdentityUri)
{
   return m_lineMap.destroy(&lineIdentityUri.toString());
}

UtlBoolean SipLineList::lineExists(const SipLine& line) const
{
   return m_lineMap.contains(&line.getIdentityUri().toString());
}

UtlBoolean SipLineList::lineExists(const Url& lineIdentityUri) const
{
   return m_lineMap.contains(&lineIdentityUri.toString());
}

SipLine* SipLineList::getLine(const Url& lineIdentityUri) const
{
   return dynamic_cast<SipLine*>(m_lineMap.findValue(&lineIdentityUri.toString()));
}

SipLine* SipLineList::getLine(const UtlString& lineId) const
{
   SipLine* pLine = NULL;

   if (!lineId.isNull())
   {
      UtlHashMapIterator itor(m_lineMap);
      UtlContainable* pKey = NULL;

      while ((pKey = itor()) != NULL)
      {
         pLine = dynamic_cast<SipLine*>(itor.value());
         if (pLine && !pLine->getLineId().compareTo(lineId, UtlString::ignoreCase))
         {
            return pLine;
         }
      }
   }

   return NULL;
}

SipLine* SipLineList::getLineByUserId(const UtlString& userId) const
{
   SipLine* pLine = NULL;

   if (!userId.isNull())
   {
      UtlHashMapIterator itor(m_lineMap);
      UtlContainable* pKey = NULL;

      while ((pKey = itor()) != NULL)
      {
         pLine = dynamic_cast<SipLine*>(itor.value());
         if (pLine && !pLine->getUserId().compareTo(userId, UtlString::ignoreCase))
         {
            return pLine;
         }
      }
   }

   return NULL;
}

size_t SipLineList::getLinesCount() const
{
   return m_lineMap.entries();
}

//
// Priorities:
//   1. Matches first lineID
//   2. Matches first lineUri user, host, port
//   3. Matches first userId
//
SipLine* SipLineList::findLine(const UtlString& lineId,
                               const Url& lineUri,
                               const UtlString& userId) const
{
   SipLine* pLineMatchingUri = NULL;
   SipLine* pLineMatchingUserId = NULL;
   SipLine* pLine = NULL;

   UtlHashMapIterator itor(m_lineMap);
   UtlContainable* pKey = NULL;

   while ((pKey = itor()) != NULL)
   {
      pLine = dynamic_cast<SipLine*>(itor.value());
      if (pLine)
      {
         if (!lineId.compareTo(pLine->getLineId(), UtlString::matchCase))
         {
            // lineId match
            return pLine;
         }
         else if (pLine->getIdentityUri().isUserHostPortEqual(lineUri))
         {
            // we found line with matching user, host and port
            pLineMatchingUri = pLine;
         }
         else if (!pLine->getUserId().compareTo(userId, UtlString::ignoreCase))
         {
            pLineMatchingUserId = pLine;
         }
      }
   }

   if (pLineMatchingUri)
   {
      return pLineMatchingUri;
   }
   // is NULL if nothing was found
   return pLineMatchingUserId;
}

void SipLineList::dumpLines()
{
   SipLine* pLine = NULL;

   UtlHashMapIterator itor(m_lineMap);
   UtlContainable* pKey = NULL;
   int i = 0;

   while ((pKey = itor()) != NULL)
   {
      pLine = dynamic_cast<SipLine*>(itor.value());
      if (pLine)
      {
         OsSysLog::add(FAC_LINE_MGR, PRI_DEBUG, "LineList %x [%d]: lineURI=%s, LINEID=%s, lineState=%d",
            this, i++, pLine->getIdentityUri().toString().data(), pLine->getLineId().data(), (int)pLine->getState());
      }
   }
}

void SipLineList::getLineCopies(UtlSList& lineList) const
{
   SipLine* pLine = NULL;

   UtlHashMapIterator itor(m_lineMap);
   UtlContainable* pKey = NULL;
   int i = 0;

   while ((pKey = itor()) != NULL)
   {
      pLine = dynamic_cast<SipLine*>(itor.value());
      if (pLine)
      {
         // copy line into list
         lineList.append(new SipLine(*pLine));
      }
   }
}
