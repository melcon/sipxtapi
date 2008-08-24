//
// Copyright (C) 2004-2006 SIPfoundry Inc.
// Licensed by SIPfoundry under the LGPL license.
//
// Copyright (C) 2004-2006 Pingtel Corp.  All rights reserved.
// Licensed to SIPfoundry under a Contributor Agreement.
//
// $$
///////////////////////////////////////////////////////////////////////////////



#if !defined(AFX_SIPLINECREDENTIALS_H__8B43463B_8F7F_426B_94F5_B60164A76DF3__INCLUDED_)
#define AFX_SIPLINECREDENTIALS_H__8B43463B_8F7F_426B_94F5_B60164A76DF3__INCLUDED_


#include <net/Url.h>
#include <net/HttpMessage.h>

/**
 * Class for storing SIP line credentials.
 */
class SipLineCredentials : public UtlString
{
public:

   SipLineCredentials();

   SipLineCredentials(const UtlString& realm,
                      const UtlString& userId,
                      const UtlString& passwordToken,
                      const UtlString& type);

   virtual ~SipLineCredentials();
   SipLineCredentials(const SipLineCredentials& rSipLineCredentials);
   SipLineCredentials& operator=(const SipLineCredentials& rSipLineCredentials);

   UtlString getRealm() const;
   UtlString getUserId() const;
   UtlString getPasswordToken() const;
   UtlString getPasswordMD5Digest() const; ///< gets MD5 digest of password token
   UtlString getType() const;

private:

   UtlString m_userId;
   UtlString m_passwordToken;
   UtlString m_realm;
   UtlString m_type;
};

#endif // !defined(AFX_SIPLINECREDENTIALS_H__8B43463B_8F7F_426B_94F5_B60164A76DF3__INCLUDED_)
