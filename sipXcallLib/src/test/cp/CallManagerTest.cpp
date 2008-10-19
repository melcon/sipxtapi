//
// Copyright (C) 2006 SIPez LLC.
// Licensed to SIPfoundry under a Contributor Agreement.
//
// Copyright (C) 2004-2006 SIPfoundry Inc.
// Licensed by SIPfoundry under the LGPL license.
//
// Copyright (C) 2004-2006 Pingtel Corp.  All rights reserved.
// Licensed to SIPfoundry under a Contributor Agreement.
//
// $$
///////////////////////////////////////////////////////////////////////////////
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestCase.h>
#include <sipxunit/TestUtilities.h>

#include <cp/CallManager.h>
#include <net/SipUserAgent.h>
#include <cp/CpTestSupport.h>
#include <net/SipMessage.h>
#include <net/SipLineMgr.h>
#include <net/SipRefreshMgr.h>
#include <mi/CpMediaInterfaceFactoryFactory.h>

#if defined _WIN32 && !defined WINCE
  #define _CRTDBG_MAP_ALLOC
  #include <crtdbg.h>

_CrtMemState MemStateBegin;
_CrtMemState MemStateEnd;
_CrtMemState MemStateDiff;
#endif 

#define BROKEN_INITTEST

#define NUM_OF_RUNS 10
/**
 * Unittest for CallManager
 */
class CallManangerTest : public CppUnit::TestCase
{
    CPPUNIT_TEST_SUITE(CallManangerTest);

    CPPUNIT_TEST(testSimpleTeardown);
  //CPPUNIT_TEST(testUATeardown);
  //CPPUNIT_TEST(testLineMgrUATeardown);
  //CPPUNIT_TEST(testRefreshMgrUATeardown);
    CPPUNIT_TEST_SUITE_END();

public:


    void testSimpleTeardown()
    {
#if defined _WIN32 && !defined WINCE
        _CrtMemCheckpoint(&MemStateBegin);
#endif
        int i;
        for (i=0; i<NUM_OF_RUNS; ++i)
        {
            CallManager *pCallManager =
               new CallManager(FALSE,
                               NULL, //LineMgr
                               TRUE, // early media in 180 ringing
                               NULL, // CodecFactory
                               9000, // rtp start
                               9002, // rtp end
                               "sip:153@pingtel.com",
                               "sip:153@pingtel.com",
                               NULL, //SipUserAgent
                               0, // sipSessionReinviteTimer
                               NULL, // pCallEventListener
                               NULL, // pInfoStatusEventListener
                               NULL, // pSecurityEventListener
                               NULL, // pMediaEventListener
                               NULL, // mgcpStackTask
                               NULL, // defaultCallExtension
                               Connection::RING, // availableBehavior
                               NULL, // unconditionalForwardUrl
                               -1, // forwardOnNoAnswerSeconds
                               NULL, // forwardOnNoAnswerUrl
                               Connection::BUSY, // busyBehavior
                               NULL, // sipForwardOnBusyUrl
                               NULL, // speedNums
                               CallManager::SIP_CALL, // phonesetOutgoingCallProtocol
                               4, // numDialPlanDigits
                               5000, // offeringDelay
                               "", // pLocal
                               CP_MAXIMUM_RINGING_EXPIRE_SECONDS, //inviteExpireSeconds
                               QOS_LAYER3_LOW_DELAY_IP_TOS, // expeditedIpTos
                               10, //maxCalls
                               sipXmediaFactoryFactory(NULL)); //pMediaFactory
#if 0
            printf("Starting CallManager\n");
#endif
            pCallManager->start();
            
            pCallManager->requestShutdown();

#if 0
            printf("Deleting CallManager\n");
#endif
            delete pCallManager;
        }
        
        for (i=0; i<NUM_OF_RUNS; ++i)
        {
            sipxDestroyMediaFactoryFactory() ;
        }
            
#if defined _WIN32 && !defined WINCE
        _CrtMemCheckpoint(&MemStateEnd);
        if (_CrtMemDifference(&MemStateDiff, &MemStateBegin, &MemStateEnd))
        {
            _CrtMemDumpStatistics(&MemStateDiff);
        }
#endif
    }

    void testUATeardown()
    {
       int i;
        for (i=0; i<NUM_OF_RUNS; ++i)
        {
            SipUserAgent* sipUA = new SipUserAgent( 5090
                                                    ,5090
                                                    ,5091
                                                    ,NULL     // default publicAddress
                                                    ,NULL     // default defaultUser
                                                    ,"127.0.0.1" // default defaultSipAddress
                                                    ,NULL     // default sipProxyServers
                                                    ,NULL     // default sipDirectoryServers
                                                    ,NULL     // default sipRegistryServers
                                                    ,NULL     // default authenticationScheme
                                                    ,NULL     // default authenicateRealm
                                                    ,NULL     // default authenticateDb
                                                    ,NULL     // default authorizeUserIds
                                                    ,NULL     // default authorizePasswords
                                                    ,NULL //lineMgr
                                                   );

            sipUA->start();

            CallManager *pCallManager =
               new CallManager(FALSE,
                               NULL, //LineMgr
                               TRUE, // early media in 180 ringing
                               NULL, // CodecFactory
                               9000, // rtp start
                               9002, // rtp end
                               "sip:153@pingtel.com",
                               "sip:153@pingtel.com",
                               sipUA, //SipUserAgent
                               0, // sipSessionReinviteTimer
                               NULL, // pCallEventListener
                               NULL, // pInfoStatusEventListener
                               NULL, // pSecurityEventListener
                               NULL, // pMediaEventListener
                               NULL, // mgcpStackTask
                               NULL, // defaultCallExtension
                               Connection::RING, // availableBehavior
                               NULL, // unconditionalForwardUrl
                               -1, // forwardOnNoAnswerSeconds
                               NULL, // forwardOnNoAnswerUrl
                               Connection::BUSY, // busyBehavior
                               NULL, // sipForwardOnBusyUrl
                               NULL, // speedNums
                               CallManager::SIP_CALL, // phonesetOutgoingCallProtocol
                               4, // numDialPlanDigits
                               5000, // offeringDelay
                               "", // pLocal
                               CP_MAXIMUM_RINGING_EXPIRE_SECONDS, //inviteExpireSeconds
                               QOS_LAYER3_LOW_DELAY_IP_TOS, // expeditedIpTos
                               10, //maxCalls
                               sipXmediaFactoryFactory(NULL)); //pMediaFactory
#if 0
            printf("Starting CallManager\n");
#endif
            pCallManager->start();

            sipUA->shutdown(TRUE);
            pCallManager->requestShutdown();

#if 0
            printf("Deleting CallManager\n");
#endif
            delete pCallManager;

        }
        
        for (i=0; i<NUM_OF_RUNS; ++i)
        {
            sipxDestroyMediaFactoryFactory() ;
        }
    }

    void testLineMgrUATeardown()
    {
        int i;
        for (i=0; i<NUM_OF_RUNS; ++i)
        {
            SipLineMgr*    lineMgr = new SipLineMgr();
            lineMgr->startLineMgr();
            SipUserAgent* sipUA = new SipUserAgent( 5090
                                                    ,5090
                                                    ,5091
                                                    ,NULL     // default publicAddress
                                                    ,NULL     // default defaultUser
                                                    ,"127.0.0.1" // default defaultSipAddress
                                                    ,NULL     // default sipProxyServers
                                                    ,NULL     // default sipDirectoryServers
                                                    ,NULL     // default sipRegistryServers
                                                    ,NULL     // default authenticationScheme
                                                    ,NULL     // default authenicateRealm
                                                    ,NULL     // default authenticateDb
                                                    ,NULL     // default authorizeUserIds
                                                    ,NULL     // default authorizePasswords
                                                    ,lineMgr
                                                   );

            sipUA->start();
            CallManager *pCallManager =
               new CallManager(FALSE,
                               NULL, //LineMgr
                               TRUE, // early media in 180 ringing
                               NULL, // CodecFactory
                               9000, // rtp start
                               9002, // rtp end
                               "sip:153@pingtel.com",
                               "sip:153@pingtel.com",
                               sipUA, //SipUserAgent
                               0, // sipSessionReinviteTimer
                               NULL, // pCallEventListener
                               NULL, // pInfoStatusEventListener
                               NULL, // pSecurityEventListener
                               NULL, // pMediaEventListener
                               NULL, // mgcpStackTask
                               NULL, // defaultCallExtension
                               Connection::RING, // availableBehavior
                               NULL, // unconditionalForwardUrl
                               -1, // forwardOnNoAnswerSeconds
                               NULL, // forwardOnNoAnswerUrl
                               Connection::BUSY, // busyBehavior
                               NULL, // sipForwardOnBusyUrl
                               NULL, // speedNums
                               CallManager::SIP_CALL, // phonesetOutgoingCallProtocol
                               4, // numDialPlanDigits
                               5000, // offeringDelay
                               "", // pLocal
                               CP_MAXIMUM_RINGING_EXPIRE_SECONDS, //inviteExpireSeconds
                               QOS_LAYER3_LOW_DELAY_IP_TOS, // expeditedIpTos
                               10, //maxCalls
                               sipXmediaFactoryFactory(NULL)); //pMediaFactory
#if 0
            printf("Starting CallManager\n");
#endif
            pCallManager->start();

            lineMgr->requestShutdown();
            sipUA->shutdown(TRUE);
            pCallManager->requestShutdown();

#if 0
            printf("Deleting CallManager\n");
#endif

            // Delete lineMgr *after* CallManager - this seems to fix the problem
            // that SipClient->run() encounters a NULL socket. 
            delete pCallManager;
            delete lineMgr;
        }
        
        for (i=0; i<NUM_OF_RUNS; ++i)
        {
            sipxDestroyMediaFactoryFactory() ;
        }
    }

    void testRefreshMgrUATeardown()
    {
        int i;
        for (i=0; i<NUM_OF_RUNS; ++i)
        {
            SipRefreshMgr* refreshMgr = new SipRefreshMgr();
            SipLineMgr* lineMgr = new SipLineMgr(refreshMgr);
            lineMgr->startLineMgr();

            SipUserAgent* sipUA = new SipUserAgent( 5090
                                                    ,5090
                                                    ,5091
                                                    ,NULL     // default publicAddress
                                                    ,NULL     // default defaultUser
                                                    ,"127.0.0.1" // default defaultSipAddress
                                                    ,NULL     // default sipProxyServers
                                                    ,NULL     // default sipDirectoryServers
                                                    ,NULL     // default sipRegistryServers
                                                    ,NULL     // default authenticationScheme
                                                    ,NULL     // default authenicateRealm
                                                    ,NULL     // default authenticateDb
                                                    ,NULL     // default authorizeUserIds
                                                    ,NULL     // default authorizePasswords
                                                    ,lineMgr
                                                   );

            sipUA->start();
            refreshMgr->setSipUserAgent(sipUA);


            CallManager *pCallManager =
               new CallManager(FALSE,
                               NULL, //LineMgr
                               TRUE, // early media in 180 ringing
                               NULL, // CodecFactory
                               9000, // rtp start
                               9002, // rtp end
                               "sip:153@pingtel.com",
                               "sip:153@pingtel.com",
                               sipUA, //SipUserAgent
                               0, // sipSessionReinviteTimer
                               NULL, // pCallEventListener
                               NULL, // pInfoStatusEventListener
                               NULL, // pSecurityEventListener
                               NULL, // pMediaEventListener
                               NULL, // mgcpStackTask
                               NULL, // defaultCallExtension
                               Connection::RING, // availableBehavior
                               NULL, // unconditionalForwardUrl
                               -1, // forwardOnNoAnswerSeconds
                               NULL, // forwardOnNoAnswerUrl
                               Connection::BUSY, // busyBehavior
                               NULL, // sipForwardOnBusyUrl
                               NULL, // speedNums
                               CallManager::SIP_CALL, // phonesetOutgoingCallProtocol
                               4, // numDialPlanDigits
                               5000, // offeringDelay
                               "", // pLocal
                               CP_MAXIMUM_RINGING_EXPIRE_SECONDS, //inviteExpireSeconds
                               QOS_LAYER3_LOW_DELAY_IP_TOS, // expeditedIpTos
                               10, //maxCalls
                               sipXmediaFactoryFactory(NULL)); //pMediaFactory
#if 0
            printf("Starting CallManager\n");
#endif
            pCallManager->start();

            lineMgr->requestShutdown();
            refreshMgr->requestShutdown();
            sipUA->shutdown(TRUE);
            pCallManager->requestShutdown();

#if 0
            printf("Deleting CallManager\n");
#endif

            delete pCallManager;
            delete refreshMgr;
            delete lineMgr;
        }
        
        for (i=0; i<NUM_OF_RUNS; ++i)
        {
            sipxDestroyMediaFactoryFactory() ;
        }
    }


};

CPPUNIT_TEST_SUITE_REGISTRATION(CallManangerTest);
