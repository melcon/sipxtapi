//
// Copyright (C) 2004-2006 SIPfoundry Inc.
// Licensed by SIPfoundry under the LGPL license.
//
// Copyright (C) 2004-2006 Pingtel Corp.  All rights reserved.
// Licensed to SIPfoundry under a Contributor Agreement.
//
// Copyright (C) 2007 Pingtel Corp., certain elements licensed under a Contributor Agreement.  
// Contributors retain copyright to elements licensed under a Contributor Agreement.
// Licensed to the User under the LGPL license.
// $$
///////////////////////////////////////////////////////////////////////////////
// Author: Dan Petrie (dpetrie AT SIPez DOT com)

#ifndef _SipDialog_h_
#define _SipDialog_h_

// SYSTEM INCLUDES

// APPLICATION INCLUDES

#include <os/OsDefs.h>
#include <utl/UtlString.h>
#include <utl/UtlHashMap.h>
#include <net/Url.h>

// DEFINES
// MACROS
// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
#define DIALOG_HANDLE_SEPARATOR ','

// STRUCTS
// TYPEDEFS
// FORWARD DECLARATIONS
class SipMessage;

//! Class for containing SIP dialog state information
/*! In SIP a dialog is defined by the SIP Call-Id
*  header and the tag parameter from the SIP To
*  and From header fields.
*
*  Dialog state is split into state and substate. The reason
*  is we can construct SipDialog from call-id, tags, but have
*  no idea if this is an early or confirmed dialog according
*  to RFC2833. Dialog is established if it contains both tags
*  and is not terminated. Dialog is early after 1xx response with
*  to tag. Dialog is confirmed after 200 OK response with to tag.
*  Dialog termination of established dialog is not automatically
*  detected, and must be explicitly set by user.
*
* \par Local and Remote
*  As the To and From fields get swapped depending
*  upon which side initiates a transaction (i.e.
*  sends a request) local and remote are used in
*  SipDialog to label tags, fields and information.
*  Local and Remote are unabiquous when used in
*  an end point.  In a proxy context the SipDialog
*  can still be used.  One can visualize the
*  sides of the dialog by thinking Left and Right
*  instead of local and remote.
*
*  This class is intended to deprecate the SipSession
*  class.
*/
class SipDialog : public UtlString
{
   /* //////////////////////////// PUBLIC //////////////////////////////////// */
public:

   /**
    * State of SipDialog:
    * - initial - when SipDialog is constructed and one of tags is missing
    * - established - when SipDialog is constructed with both tags, or as result of updateDialogData
    * - terminated - after 4xx, 5xx, 6xx responses
    */
   typedef enum
   {
      DIALOG_STATE_INITIAL,
      DIALOG_STATE_ESTABLISHED,
      DIALOG_STATE_TERMINATED
   } DialogState;

   /**
    * Substate of SipDialog:
    * - unknown - the default state
    * - early - when updateDialogData can see 1xx response with remote tag
    * - confirmed - after final 200 OK response to dialog initiation, with both tags
    */
   typedef enum
   {
      DIALOG_SUBSTATE_UNKNOWN,
      DIALOG_SUBSTATE_EARLY,
      DIALOG_SUBSTATE_CONFIRMED
   } DialogSubState;

   /* ============================ CREATORS ================================== */

   //! Default Dialog constructor
   /*! Optionally construct a dialog from the given message
   *
   * \param initialMessage - message to initiate the dialog, typically this
   *        is a request.
   */
   SipDialog(const SipMessage* initialMessage = NULL);

   //! Constructor accepting the basic pieces of a session callId, toUrl, and from Url.
   /*! Optionally construct a dialog from the given message
   *
   * \param callId - sip message call-id header value
   * \param localField - sip message To or From field value representing the 
   *        local side of the dialog.
   * \param remoteField - sip message To or From field value representing the 
   *        remote side of the dialog.
   */
   SipDialog(const char* szCallId, const char* szLocalTag, const char* szRemoteTag, UtlBoolean isFromLocal = TRUE); 

   //! Constructor accepting the basic pieces of a session callId, toUrl, and from Url.
   /*! Optionally construct a dialog from the given message
   *
   * \param callId - sip message call-id header value
   * \param localField - sip message To or From field value representing the 
   *        local side of the dialog.
   * \param remoteField - sip message To or From field value representing the 
   *        remote side of the dialog.
   */
   SipDialog(const UtlString& sSipCallId, const UtlString& sLocalTag, const UtlString& sRemoteTag, UtlBoolean isFromLocal = TRUE); 

   //! Copy constructor
   SipDialog(const SipDialog& rSipDialog);

   //! Assignment operator
   SipDialog& operator=(const SipDialog& rhs);

   //! Destructor
   virtual
      ~SipDialog();


   /* ============================ MANIPULATORS ============================== */

   //! update the dialog information based upon the given message
   /*! Typically this updates things like the Contact, CSeq headers and 
   *  tag information for the dialog.
   *  \param message - SIP message which is assumed to be part of this
   *         dialog.
   */
   void updateDialogData(const SipMessage& message);

   //! Set fields in next SIP request for this dialog
   /*! Set the request URI, call-id, To, From, Route and Cseq headers
   *  fields for the given request to be sent in the given dialog.  The
   *  last local cseq of the dialog is incremented and set in the request.
   *  \param method - the sip request method for this request.
   *  \param request - the request which is to be part of this dialog
   *         and sent as orginating from the local side of the dialog.
   */
   void setRequestData(SipMessage& request, const char* method);

   /* ============================ ACCESSORS ================================= */

   //! Gets a string handle that can be used to uniquely identify this dialog
   void getHandle(UtlString& dialogHandle) const;

   /**
    * Get the initial dialog handle for this dialog. Initial dialog is dialog
    * that has not yet been established. Early and confirmed dialogs are considered
    * established dialogs.
    */
   void getInitialHandle(UtlString& initialDialogHandle) const;

   //! Gets the call-id, and tags from the dialogHandle
   static void parseHandle(const UtlString& dialogHandle,
      UtlString& callId,
      UtlString& localTag,
      UtlString& remoteTag);

   //! Reverse the order of the tags in the handle
   static void reverseTags(const UtlString& dialogHandle,
                           UtlString& reversedHandle);

   //! Get the SIP call-id header value for this dialog
   void getCallId(UtlString& callId) const;
   //! Set the SIP call-id header value for this dialog
   void setCallId(const char* callId);

   //! Get the SIP To/From header value for the local side of this dialog
   void getLocalField(Url& localUrl) const;
   //! Get the tag from the SIP To/From header value for the local side of this dialog
   void getLocalTag(UtlString& localTag) const;
   //! Set the SIP To/From header value for the local side of this dialog
   void setLocalField(const Url& localUrl);

   //! Get the SIP To/From header value for the remote side of this dialog
   void getRemoteField(Url& remoteUrl) const;
   //! Get the tag from the SIP To/From header value for the remote side of this dialog
   void getRemoteTag(UtlString& remoteTag) const;
   //! Set the SIP To/From header value for the remote side of this dialog
   void setRemoteField(const Url& remoteUrl);

   //! Get the SIP Contact header value for the remote side of this dialog
   void getRemoteContact(Url& remoteContact) const;
   //! Set the SIP Contact header value for the remote side of this dialog
   void setRemoteContact(const Url& remoteContact);

   //! Get the SIP Contact header value for the local side of this dialog
   void getLocalContact(Url& localContact) const;
   //! Get the SIP Contact header value for the remote side of this dialog
   void setLocalContact(const Url& localContact);

   //! Get the SIP method of the request that initiated this dialog
   void getInitialMethod(UtlString& method) const;
   //! Set the SIP method of the request that initiated this dialog
   void setInitialMethod(const char* method);

   //! Get the next (incremented) SIP Cseq number for the local side
   int getNextLocalCseq();
   //! Get the last used SIP Cseq number for the local side
   int getLastLocalCseq() const;
   //! Set the last used SIP Cseq number for the local side
   void setLastLocalCseq(int seqNum);

   //! Get the last used SIP Cseq number for the remote side
   int getLastRemoteCseq() const;
   //! Set the last used SIP Cseq number for the remote side
   void setLastRemoteCseq(int seqNum);

   /**
    * Get the request URI for the local side.
    *
    * This may be different than the local contact.  This is
    * what was received in the last request from the remote 
    * side.
    */
   void getLocalRequestUri(UtlString& requestUri) const;

   /** Set the request URI for the local side */
   void setLocalRequestUri(const UtlString& requestUri);

   /**
    *  Gets the requestUri we used in the initial request.
    */
   void getRemoteRequestUri(UtlString& requestUri) const;
   //! Set the request URI for the remote side
   void setRemoteRequestUri(const UtlString& requestUri);

   /** Retrieves state of SipDialog. */
   SipDialog::DialogState getDialogState() const { return m_dialogState; }
   /** Sets state of SipDialog. Avoid using it if possible. */
   void setDialogState(SipDialog::DialogState val) { m_dialogState = val; }

   /** Retrieves substate of SipDialog - unknown, early, confirmed */
   SipDialog::DialogSubState getDialogSubState() const { return m_dialogSubState; }
   void setDialogSubState(SipDialog::DialogSubState val) { m_dialogSubState = val; }

   /** Marks dialog as terminated */
   void terminateDialog();

   /** Gets the secure flag of sip dialog */
   UtlBoolean getSecure() const { return m_bSecure; }
   /** Sets the secure flag of sip dialog */
   void setSecure(UtlBoolean val) { m_bSecure = val; }

   //! Debug method to dump the contents of this SipDialog into a string
   void toString(UtlString& dialogDumpString);

   /* ============================ INQUIRY =================================== */

   //! Compare the message to see if it matches this dialog
   /*! A dialog matches if the SIP Call-Id header and
   *  the tags from the SIP message To and From field
   *  match those of this dialog.  The tags are compared in
   * both directions.
   */
   UtlBoolean isSameDialog(const SipMessage& message) const;

   //! Compare the given dialog indentifiers match those of this dialog
   /*! The tags are compared in both directions.
   */
   UtlBoolean isSameDialog(const UtlString& callId,
                           const UtlString& localTag,
                           const UtlString& remoteTag) const;

   //! Compare the given dialog handle with that of this dialog
   /*! The tags are compared in both directions.
   */
   UtlBoolean isSameDialog(const UtlString& dialogHandle);

   //! Determine if this is an early dialog
   UtlBoolean isInitialDialog() const;

   //! Determine if the given handle is for an early dialog
   /*! That is check if one of the tags is null
   */
   static UtlBoolean isInitialDialog(const char* dialogHandle);

   //! Checks if this is an early dialog for the given SIP message
   UtlBoolean isInitialDialogFor(const SipMessage& message) const;

   //! Checks if this is an early dialog for the given SIP message
   UtlBoolean isInitialDialogFor(const UtlString& callId,
      const UtlString& localTag,
      const UtlString& remoteTag) const;

   //! Checks if this was an early dialog for the given SIP message
   /*! This dialog is considered to have been an early dialog if
   *  the SIP Call-Id and one of the given tags matches one of
   *  the tags of this dialog.
   */
   UtlBoolean wasInitialDialogFor(const UtlString& callId,
      const UtlString& localTag,
      const UtlString& remoteTag) const;

   //! Query if the transaction request was sent from the local side
   /*! If the request was sent from the local side, the fromTag will
   *  match the local tag.
   */
   UtlBoolean isDialogLocallyInitiated(const UtlString& callId,
      const UtlString& fromTag,
      const UtlString& toTag) const;

   //! Query if the transaction request was sent from the remote side
   /*! If the request was sent from the local side, the fromTag will
   *  match the remote tag.
   */
   UtlBoolean isDialogRemotelyInitiated(const UtlString& callId,
      const UtlString& fromTag,
      const UtlString& toTag) const;

   //! Check if message and SIP local Cseq match
   UtlBoolean isSameLocalCseq(const SipMessage& message) const;

   //! Check if message and SIP remote Cseq match
   UtlBoolean isSameRemoteCseq(const SipMessage& message) const;

   //! Check if mesage cseq is after the last local transaction
   UtlBoolean isNextLocalCseq(const SipMessage& message) const;

   //! Check if mesage cseq is after the last remote transaction
   UtlBoolean isNextRemoteCseq(const SipMessage& message) const;
   /* //////////////////////////// PROTECTED ///////////////////////////////// */
protected:

   /* //////////////////////////// PRIVATE /////////////////////////////////// */
private:
   void updateDialogState(const SipMessage* pSipMessage = NULL);
   void updateSecureFlag(const SipMessage* pSipMessage = NULL);

   // The callId is stored in the UtlString base class data element
   Url m_localField; // To or From depending on who initiated the transaction
   Url m_remoteField; // To or From depending on who initiated the transaction
   UtlString m_sLocalTag;
   UtlString m_sRemoteTag;
   Url m_localContact;
   Url m_remoteContact; // In RFC-2833 described as "remote target"
   UtlString m_sRouteSet;
   UtlString m_sInitialMethod;
   UtlString m_sLocalRequestUri;
   UtlString m_sRemoteRequestUri;
   UtlBoolean m_sLocalInitiatedDialog;
   int m_iInitialLocalCseq;
   int m_iInitialRemoteCseq;
   int m_iLastLocalCseq;
   int m_sLastRemoteCseq;
   UtlBoolean m_bSecure;
   DialogState m_dialogState;
   DialogSubState m_dialogSubState;
};

/* ============================ INLINE METHODS ============================ */

#endif  // _SipDialog_h_
