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

#ifndef AcRenegotiateCodecsMsg_h__
#define AcRenegotiateCodecsMsg_h__

// SYSTEM INCLUDES
// APPLICATION INCLUDES
#include <os/OsDefs.h>
#include <os/OsMsg.h>
#include <utl/UtlString.h>
#include <net/SipDialog.h>
#include <cp/CpMessageTypes.h>
#include <cp/CpAudioCodecInfo.h>
#include <cp/CpVideoCodecInfo.h>
#include <cp/msg/AcCommandMsg.h>

// DEFINES
// MACROS
// EXTERNAL FUNCTIONS
// CONSTANTS
// STRUCTS
// TYPEDEFS
// MACROS
// FORWARD DECLARATIONS

/**
* Abstract call command message. Instructs call connection to renegotiate codecs.
* If sipDialog has NULL callid, then command is issued for all connections.
*/
class AcRenegotiateCodecsMsg : public AcCommandMsg
{
   /* //////////////////////////// PUBLIC //////////////////////////////////// */
public:
   /* ============================ CREATORS ================================== */

   AcRenegotiateCodecsMsg(const SipDialog& sipDialog,
                          CP_AUDIO_BANDWIDTH_ID audioBandwidthId,
                          const UtlString& sAudioCodecs,
                          CP_VIDEO_BANDWIDTH_ID videoBandwidthId,
                          const UtlString& sVideoCodecs);

   virtual ~AcRenegotiateCodecsMsg();

   virtual OsMsg* createCopy(void) const;

   /* ============================ MANIPULATORS ============================== */

   /* ============================ ACCESSORS ================================= */

   void setSipDialog(SipDialog& val) const { val = m_sipDialog; }
   CP_AUDIO_BANDWIDTH_ID getAudioBandwidthId() const { return m_audioBandwidthId; }
   UtlString getAudioCodecs() const { return m_sAudioCodecs; }
   CP_VIDEO_BANDWIDTH_ID getVideoBandwidthId() const { return m_videoBandwidthId; }
   UtlString getVideoCodecs() const { return m_sVideoCodecs; }

   /* ============================ INQUIRY =================================== */

   /* //////////////////////////// PROTECTED ///////////////////////////////// */
protected:

   /* //////////////////////////// PRIVATE /////////////////////////////////// */
private:
   SipDialog m_sipDialog;
   CP_AUDIO_BANDWIDTH_ID m_audioBandwidthId;
   UtlString m_sAudioCodecs;
   CP_VIDEO_BANDWIDTH_ID m_videoBandwidthId;
   UtlString m_sVideoCodecs;
};

#endif // AcRenegotiateCodecsMsg_h__
