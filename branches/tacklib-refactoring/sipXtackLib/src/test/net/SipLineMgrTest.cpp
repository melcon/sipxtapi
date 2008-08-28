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
#include <net/SipLine.h>
#include <net/SipLineMgr.h>
#include <net/SipLineCredential.h>
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

#define CREDENTIAL_USERID "John"
#define CREDENTIAL_USERID2 "John2"
#define CREDENTIAL_PASSWORD "password"
#define CREDENTIAL_PASSWORD2 "password2"
#define CREDENTIAL_REALM "test_realm"
#define CREDENTIAL_REALM2 "test_realm2"
#define CREDENTIAL_TYPE "some_type"
#define CREDENTIAL_TYPE2 "some_type2"

// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
// STATIC VARIABLE INITIALIZATIONS
// MACROS
// GLOBAL VARIABLES
// GLOBAL FUNCTIONS

class SipLineMgrTest : public CppUnit::TestCase
{

   CPPUNIT_TEST_SUITE(SipLineMgrTest);
   CPPUNIT_TEST(testAddDeleteLine);
   CPPUNIT_TEST(testAddDeleteCredentials1);
   CPPUNIT_TEST(testAddDeleteCredentials2);
   CPPUNIT_TEST(testGetNumLines);
   CPPUNIT_TEST_SUITE_END();

private:

public:
   SipLineMgrTest()
   {
   }

   ~SipLineMgrTest()
   {
   }

   void setUp()
   {
   }

   void tearDown()
   {
   }

   void testAddDeleteLine()
   {
      SipLine line1(USER_ENTERED_URI_1, IDENTITY_URI_1);
      SipLineMgr lineMgr;

      CPPUNIT_ASSERT(lineMgr.addLine(line1));
      CPPUNIT_ASSERT(!lineMgr.addLine(line1));
      // now try deletion
      CPPUNIT_ASSERT(lineMgr.deleteLine(line1.getIdentityUri()));
      CPPUNIT_ASSERT(!lineMgr.deleteLine(line1.getIdentityUri()));
   }

   void testAddDeleteCredentials1()
   {
      SipLine line1(USER_ENTERED_URI_1, IDENTITY_URI_1);
      SipLineCredential credential(CREDENTIAL_REALM, CREDENTIAL_USERID, CREDENTIAL_PASSWORD, CREDENTIAL_TYPE);
      SipLineMgr lineMgr;

      lineMgr.addLine(line1);
      CPPUNIT_ASSERT(lineMgr.addCredentialForLine(IDENTITY_URI_1, credential));
      CPPUNIT_ASSERT(!lineMgr.addCredentialForLine(IDENTITY_URI_1, credential)); // 2nd must fail
      CPPUNIT_ASSERT(lineMgr.deleteCredentialForLine(IDENTITY_URI_1, CREDENTIAL_REALM, CREDENTIAL_TYPE));
      CPPUNIT_ASSERT(!lineMgr.deleteCredentialForLine(IDENTITY_URI_1, CREDENTIAL_REALM, CREDENTIAL_TYPE)); // 2nd must fail
   }

   void testAddDeleteCredentials2()
   {
      SipLine line1(USER_ENTERED_URI_1, IDENTITY_URI_1);
      SipLineCredential credential1(CREDENTIAL_REALM, CREDENTIAL_USERID, CREDENTIAL_PASSWORD, CREDENTIAL_TYPE);
      SipLineCredential credential2(CREDENTIAL_REALM2, CREDENTIAL_USERID2, CREDENTIAL_PASSWORD2, CREDENTIAL_TYPE2);
      SipLineMgr lineMgr;

      lineMgr.addLine(line1);
      lineMgr.addCredentialForLine(IDENTITY_URI_1, credential1);
      lineMgr.addCredentialForLine(IDENTITY_URI_1, credential2);
      CPPUNIT_ASSERT(lineMgr.deleteAllCredentialsForLine(IDENTITY_URI_1));
   }

   void testGetNumLines()
   {
      SipLine line1(USER_ENTERED_URI_1, IDENTITY_URI_1);
      SipLine line2(USER_ENTERED_URI_2, IDENTITY_URI_2);
      SipLineMgr lineMgr;

      CPPUNIT_ASSERT(lineMgr.getNumLines() == 0);
      lineMgr.addLine(line1);
      CPPUNIT_ASSERT(lineMgr.getNumLines() == 1);
      lineMgr.addLine(line2);
      CPPUNIT_ASSERT(lineMgr.getNumLines() == 2);
      lineMgr.deleteAllLines();
      CPPUNIT_ASSERT(lineMgr.getNumLines() == 0);
   }
};

CPPUNIT_TEST_SUITE_REGISTRATION(SipLineMgrTest);
