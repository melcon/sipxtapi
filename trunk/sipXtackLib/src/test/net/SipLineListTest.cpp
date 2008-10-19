//
// Copyright (C) 2007-2008 Jaroslav Libak
// Licensed under the LGPL license.
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
// APPLICATION INCLUDES
#include <utl/UtlInt.h>
#include <utl/UtlString.h>
#include <net/SipLineList.h>
#include <sipxunit/TestUtilities.h>
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestCase.h>

// DEFINES
#define USER_ENTERED_URI_1 "\"John Doe\"<sip:user1:password1@host1:5060;urlparm=value1?headerParam=value1>;fieldParam=value1"
#define USER_ENTERED_URI_2 "\"Jane Doe\"<sip:user2:password2@host2:5061;urlparm=value2?headerParam=value2>;fieldParam=value2"
#define USER_ID_1 "user1"
#define USER_ID_2 "user2"
#define IDENTITY_URI_1 "<sip:user1:password1@host1:5060;urlparm=value1"
#define IDENTITY_URI_2 "<sip:user2:password2@host2:5061;urlparm=value2>"

// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
// STATIC VARIABLE INITIALIZATIONS
// MACROS
// GLOBAL VARIABLES
// GLOBAL FUNCTIONS

class SipLineListTest : public CppUnit::TestCase
{

   CPPUNIT_TEST_SUITE(SipLineListTest);
   CPPUNIT_TEST(testAddRemove);
   CPPUNIT_TEST(testAddTwice);
   CPPUNIT_TEST(testFindLine);
   CPPUNIT_TEST(testGetLineCount);
   CPPUNIT_TEST(testGetLine);
   CPPUNIT_TEST(testGetLineByUserId);
   CPPUNIT_TEST(testLineExists);
   CPPUNIT_TEST_SUITE_END();

private:

public:
   SipLineListTest()
   {
   }

   ~SipLineListTest()
   {
   }

   void setUp()
   {
   }

   void tearDown()
   {
   }

   void testAddRemove()
   {
      SipLine line1(USER_ENTERED_URI_1, IDENTITY_URI_1);
      SipLine line2(USER_ENTERED_URI_2, IDENTITY_URI_2);
      SipLineList lineList;

      CPPUNIT_ASSERT(lineList.add(line1));
      CPPUNIT_ASSERT(lineList.add(line2));
      // try remove
      CPPUNIT_ASSERT(lineList.remove(line1));
      CPPUNIT_ASSERT(!lineList.remove(line1)); // 2nd must fail
      CPPUNIT_ASSERT(lineList.remove(line2.getIdentityUri())); // remove by uri
   }

   void testAddTwice()
   {
      SipLine line1(USER_ENTERED_URI_1, IDENTITY_URI_1);
      SipLine line2(USER_ENTERED_URI_2, IDENTITY_URI_2);
      SipLineList lineList;

      CPPUNIT_ASSERT(lineList.add(line1));
      CPPUNIT_ASSERT(lineList.add(line2));
      // try 2nd addition
      CPPUNIT_ASSERT(!lineList.add(line1));
      CPPUNIT_ASSERT(!lineList.add(line2));
      // remove all
      lineList.removeAll();
      // try adding again
      CPPUNIT_ASSERT(lineList.add(line1));
   }

   void testFindLine()
   {
      SipLine line1(USER_ENTERED_URI_1, IDENTITY_URI_1);
      SipLineList lineList;
      lineList.add(line1);
      
      // try find by lineID
      CPPUNIT_ASSERT(lineList.findLine(line1.getLineId(), "xxx", "xxx"));
      // find by uri
      CPPUNIT_ASSERT(lineList.findLine("xxx", IDENTITY_URI_1, "xxx"));
      // find by userID
      CPPUNIT_ASSERT(lineList.findLine("xxx", "xxx", USER_ID_1));
   }

   void testGetLineCount()
   {
      SipLine line1(USER_ENTERED_URI_1, IDENTITY_URI_1);
      SipLine line2(USER_ENTERED_URI_2, IDENTITY_URI_2);
      SipLineList lineList;

      CPPUNIT_ASSERT(lineList.getLinesCount() == 0);
      lineList.add(line1);
      CPPUNIT_ASSERT(lineList.getLinesCount() == 1);
      lineList.add(line2);
      CPPUNIT_ASSERT(lineList.getLinesCount() == 2);
      lineList.remove(line2);
      CPPUNIT_ASSERT(lineList.getLinesCount() == 1);
   }

   void testGetLine()
   {
      SipLine line1(USER_ENTERED_URI_1, IDENTITY_URI_1);
      SipLineList lineList;

      lineList.add(line1);
      CPPUNIT_ASSERT(lineList.getLine(line1.getLineId()));
      CPPUNIT_ASSERT(lineList.getLine(line1.getIdentityUri()));
   }

   void testGetLineByUserId()
   {
      SipLine line1(USER_ENTERED_URI_1, IDENTITY_URI_1);
      SipLineList lineList;

      lineList.add(line1);
      CPPUNIT_ASSERT(lineList.getLineByUserId(line1.getUserId()));
      CPPUNIT_ASSERT(!lineList.getLineByUserId("xxx"));
   }

   void testLineExists()
   {
      SipLine line1(USER_ENTERED_URI_1, IDENTITY_URI_1);
      SipLineList lineList;

      lineList.add(line1);
      CPPUNIT_ASSERT(lineList.lineExists(line1.getIdentityUri()));
      CPPUNIT_ASSERT(!lineList.lineExists("xxx"));
   }
};

CPPUNIT_TEST_SUITE_REGISTRATION(SipLineListTest);
