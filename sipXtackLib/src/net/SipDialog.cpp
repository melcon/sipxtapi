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
//////////////////////////////////////////////////////////////////////////////

// SYSTEM INCLUDES
#include <assert.h>

// APPLICATION INCLUDES
#include <net/SipDialog.h>
#include <net/SipMessage.h>
#include <utl/UtlHashMapIterator.h>
#include <os/OsSysLog.h>

// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
// STATIC VARIABLE INITIALIZATIONS

/* //////////////////////////// PUBLIC //////////////////////////////////// */

/* ============================ CREATORS ================================== */

// Constructor
SipDialog::SipDialog(const SipMessage* initialMessage)
: m_dialogState(DIALOG_STATE_INITIAL)
, m_dialogSubState(DIALOG_SUBSTATE_UNKNOWN)
{
   if(initialMessage)
   {
      UtlBoolean isFromLocal = initialMessage->isFromThisSide();
      UtlString callId;
      initialMessage->getCallIdField(&callId);
      append(callId);
      updateSecureFlag(initialMessage);

      // The transaction was initiated from this side
      if((!initialMessage->isResponse() && isFromLocal) ||
         (initialMessage->isResponse() && !isFromLocal))
      {
         // either local request or remote response (to our previous request)
         m_sLocalInitiatedDialog = TRUE;
         initialMessage->getFromUrl(m_localField);
         m_localField.getFieldParameter("tag", m_sLocalTag);
         initialMessage->getToUrl(m_remoteField);
         m_remoteField.getFieldParameter("tag", m_sRemoteTag);
         initialMessage->getCSeqField(&m_iInitialLocalCseq, &m_sInitialMethod);
         m_iLastLocalCseq = m_iInitialLocalCseq;
         m_sLastRemoteCseq = -1;
         m_iInitialRemoteCseq = -1;
      }
      // The transaction was initiated from the other side
      else
      {
         // local response or remote inbound request
         m_sLocalInitiatedDialog = FALSE;
         initialMessage->getFromUrl(m_remoteField);
         m_remoteField.getFieldParameter("tag", m_sRemoteTag);
         initialMessage->getToUrl(m_localField);
         m_localField.getFieldParameter("tag", m_sLocalTag);
         initialMessage->getCSeqField(&m_iInitialRemoteCseq, &m_sInitialMethod);
         m_sLastRemoteCseq = m_iInitialRemoteCseq;
         // Start local CSeq's at 1, because some UAs cannot handle 0.
         m_iLastLocalCseq = 0;
         m_iInitialLocalCseq = 0;
      }

      if(initialMessage->isRequest())
      {
         // this is a request
         UtlString uri;
         initialMessage->getRequestUri(&uri);
         if(isFromLocal)
         {
            m_sRemoteRequestUri = uri;
         }
         else
         {
            // Incoming initial Request, we need to set the Route set here
            if(initialMessage->isRecordRouteAccepted())
            {
               initialMessage->buildRouteField(&m_sRouteSet);
            }
            m_sLocalRequestUri = uri;
         }
      }

      UtlString contact;
      // Get the Contact, but as an addr-spec.
      initialMessage->getContactUri(0, &contact);
      if(isFromLocal)
      {
         m_localContact.fromString(contact, TRUE);
      }
      else
      {
         m_remoteContact.fromString(contact, TRUE);
      }
   }
   else
   {
      // Insert dummy values into fields that aren't automatically initialized.
      m_sLocalInitiatedDialog = TRUE;
      // Start local CSeq's at 1, because some UAs cannot handle 0.
      m_iLastLocalCseq = 0;
      m_sLastRemoteCseq = -1;
      m_iInitialLocalCseq = 0;
      m_iInitialRemoteCseq = -1;
      m_bSecure = FALSE;
   }

   updateDialogState(initialMessage);
}

// Constructor
SipDialog::SipDialog(const char* szCallId, 
                     const char* szLocalTag, 
                     const char* szRemoteTag,
                     UtlBoolean isFromLocal)
: UtlString(szCallId)
, m_dialogState(DIALOG_STATE_INITIAL)
, m_dialogSubState(DIALOG_SUBSTATE_UNKNOWN)
{
   m_sLocalTag = szLocalTag;
   m_sRemoteTag = szRemoteTag;

   m_sLocalInitiatedDialog = isFromLocal;
   m_iInitialLocalCseq = 0;
   m_iInitialRemoteCseq = -1;
   m_iLastLocalCseq = 0;
   m_sLastRemoteCseq = -1;
   m_bSecure = FALSE;

   updateDialogState();
   updateSecureFlag();
}

SipDialog::SipDialog(const UtlString& sSipCallId,
                     const UtlString& sLocalTag,
                     const UtlString& sRemoteTag,
                     UtlBoolean isFromLocal)
: UtlString(sSipCallId)
, m_dialogState(DIALOG_STATE_INITIAL)
, m_dialogSubState(DIALOG_SUBSTATE_UNKNOWN)
{
   m_sLocalTag = sLocalTag;
   m_sRemoteTag = sRemoteTag;

   m_sLocalInitiatedDialog = isFromLocal;
   m_iInitialLocalCseq = 0;
   m_iInitialRemoteCseq = -1;
   m_iLastLocalCseq = 0;
   m_sLastRemoteCseq = -1;
   m_bSecure = FALSE;

   updateDialogState();
   updateSecureFlag();
}

// Copy constructor
SipDialog::SipDialog(const SipDialog& rSipDialog)
: UtlString(rSipDialog)
{
   *this = rSipDialog;
}

// Destructor
SipDialog::~SipDialog()
{
}

/* ============================ MANIPULATORS ============================== */

// Assignment operator
SipDialog& SipDialog::operator=(const SipDialog& rhs)
{
   if (this == &rhs)            // handle the assignment to self case
      return *this;

   UtlString::operator=(rhs);  // assign fields for parent class

   m_localField = rhs.m_localField;
   m_sLocalTag = rhs.m_sLocalTag;
   m_remoteField = rhs.m_remoteField;
   m_sRemoteTag = rhs.m_sRemoteTag;
   m_localContact = rhs.m_localContact;
   m_remoteContact = rhs.m_remoteContact;
   m_sRouteSet = rhs.m_sRouteSet;
   m_sInitialMethod = rhs.m_sInitialMethod;
   m_sLocalInitiatedDialog = rhs.m_sLocalInitiatedDialog;
   m_iInitialLocalCseq = rhs.m_iInitialLocalCseq;
   m_iInitialRemoteCseq = rhs.m_iInitialRemoteCseq;
   m_iLastLocalCseq = rhs.m_iLastLocalCseq;
   m_sLastRemoteCseq = rhs.m_sLastRemoteCseq;
   m_sLocalRequestUri = rhs.m_sLocalRequestUri;
   m_sRemoteRequestUri = rhs.m_sRemoteRequestUri;
   m_bSecure = rhs.m_bSecure;
   m_dialogState = rhs.m_dialogState;
   m_dialogSubState = rhs.m_dialogSubState;

   return *this;
}

void SipDialog::updateDialogData(const SipMessage& message)
{
   // get call-id, From, To and both tags
   UtlString messageCallId;
   message.getCallIdField(messageCallId);
   Url messageFromUrl;
   message.getFromUrl(messageFromUrl);
   UtlString messageFromTag;
   messageFromUrl.getFieldParameter("tag", messageFromTag);
   Url messageToUrl;
   message.getToUrl(messageToUrl);
   UtlString messageToTag;
   messageToUrl.getFieldParameter("tag", messageToTag);

   int cSeq;
   UtlString method;
   message.getCSeqField(&cSeq, &method);
   int responseCode = message.getResponseStatusCode();

   // Figure out if the request is from the local or
   // the remote side
   if(isTransactionLocallyInitiated(messageCallId,
      messageFromTag,
      messageToTag))
   {
      // This message is part of a transaction initiated by
      // the local side of the dialog

      if(cSeq > m_iLastLocalCseq) 
      {
         // we were probably not shown some SipMessage from this dialog
         m_iLastLocalCseq = cSeq;
      }

      if(cSeq == m_iLastLocalCseq) 
      {
         // Always update the contact if it is set
         UtlString messageContact;
         // Get the Contact value, but as an addr-spec.
         if(message.getContactUri(0, &messageContact) &&
            !messageContact.isNull())
         {
            if(message.isResponse())
            {
               m_remoteContact.fromString(messageContact, TRUE);
            }
            else
            {
               m_localContact.fromString(messageContact, TRUE);
            }
         }

         // A successful response to an INVITE or SUBSCRIBE
         // make this early dialog a set up dialog
         if(m_sLocalInitiatedDialog && message.isResponse() && 
            responseCode >= SIP_1XX_CLASS_CODE && responseCode < SIP_3XX_CLASS_CODE)
         {
            if (m_sRemoteTag.isNull())// tag not set
            {
               message.getToUrl(m_remoteField);
               m_remoteField.getFieldParameter("tag", m_sRemoteTag);
            }
            if (m_sRouteSet.isNull())
            {
               // Need to get the route set as well
               // Make sure the Request Method is allowed to set Record-Routes
               if(message.isRecordRouteAccepted())
               {
                  message.buildRouteField(&m_sRouteSet);
               }
            }
         }

         updateDialogState(&message);
      }
   }
   else if(isTransactionRemotelyInitiated(messageCallId,
      messageFromTag,
      messageToTag))
   {
      int prevRemoteCseq = m_sLastRemoteCseq;

      // This message is part of a transaction initiated by
      // the callee/destination of the session
      if(cSeq > m_sLastRemoteCseq) 
      {
         // we were probably not shown some SipMessage from this dialog. Perhaps remote party sent it and it was lost.
         m_sLastRemoteCseq = cSeq;
      }

      if(cSeq == m_sLastRemoteCseq)
      {
         // Always update the contact if it is set
         UtlString messageContact;
         // Get the Contact value, but as an addr-spec.
         if(message.getContactUri(0, &messageContact) &&
            !messageContact.isNull())
         {
            if(message.isResponse())
            {
               m_localContact.fromString(messageContact, TRUE);
            }
            else
            {
               m_remoteContact.fromString(messageContact, TRUE);
            }
         }

         // A response (e.g. NOTIFY) can come before we get the
         // successful response to the initial transaction
         if(!m_sLocalInitiatedDialog && !message.isResponse() && m_sRemoteTag.isNull()) // tag not set
         {
            // Change this early dialog to a set up dialog.
            // The tag gets set in the 2xx response
            // so we need to update the URL
            message.getFromUrl(m_remoteField);
            m_remoteField.getFieldParameter("tag", m_sRemoteTag);
         }
         if(!m_sLocalInitiatedDialog && message.isResponse() && 
            responseCode >= SIP_1XX_CLASS_CODE && responseCode < SIP_3XX_CLASS_CODE && m_sLocalTag.isNull()) 
         {
            // Update the local tag
            message.getToUrl(m_localField);
            m_localField.getFieldParameter("tag", m_sLocalTag);
         }
         updateDialogState(&message);
      }
   }
}

void SipDialog::setRequestData(SipMessage& request, const char* method)
{
   UtlString methodString(method ? method : "");
   if(methodString.isNull())
   {
      request.getRequestMethod(&methodString);
   }

   // The request URI should be the remote contact
   UtlString remoteContact;
   // Use getUri() to get the contact in addr-spec format.
   // (mRemoteContact should have no field parameters, but if it has
   // URI parameters, toString would add <...>, which are not allowed
   // in URIs.)
   m_remoteContact.getUri(remoteContact);

   // If the remote contact is empty, use the remote request uri
   if (remoteContact.compareTo("sip:") == 0)
   {
      OsSysLog::add(FAC_ACD, PRI_DEBUG, "SipDialog::setRequestData - using remote request uri %s",
         m_sRemoteRequestUri.data());
      request.setSipRequestFirstHeaderLine(methodString, m_sRemoteRequestUri);
   }
   else
   {
      request.setSipRequestFirstHeaderLine(methodString, remoteContact);
   }

   // The local field is the From field
   UtlString fromField;
   m_localField.toString(fromField);
   request.setRawFromField(fromField);

   // The remote field is the To field
   UtlString toField;
   m_remoteField.toString(toField);
   request.setRawToField(toField);

   // Get the next local Cseq, the method should already be set
   getNextLocalCseq();
   request.setCSeqField(m_iLastLocalCseq, methodString);

   // Set the route header according to the route set
   if(!m_sRouteSet.isNull())
   {
      request.setRouteField(m_sRouteSet);
   }

   // Set the call-id
   request.setCallIdField(*this);
}

/* ============================ ACCESSORS ================================= */

void SipDialog::getDialogHandle(const SipMessage& sipMessage, UtlString& sDialogHandle)
{
   sDialogHandle.remove(0);
   sipMessage.getCallIdField(sDialogHandle);
   sDialogHandle.append(DIALOG_HANDLE_SEPARATOR);
   UtlString sLocalTag;
   UtlString sRemoteTag;
   getLocalRemoteTag(sipMessage, sLocalTag, sRemoteTag);
   sDialogHandle.append(sLocalTag);
   sDialogHandle.append(DIALOG_HANDLE_SEPARATOR);
   sDialogHandle.append(sRemoteTag);
}

void SipDialog::getDialogHandle(UtlString& sDialogHandle) const
{
   sDialogHandle = *this; // callId
   sDialogHandle.append(DIALOG_HANDLE_SEPARATOR);
   sDialogHandle.append(m_sLocalTag);
   sDialogHandle.append(DIALOG_HANDLE_SEPARATOR);
   sDialogHandle.append(m_sRemoteTag);
}

void SipDialog::getInitialDialogHandle(UtlString& initialDialogHandle) const
{
   // Do not add the tag for the side that did not initiate the dialog
   initialDialogHandle = *this; // callId
   initialDialogHandle.append(DIALOG_HANDLE_SEPARATOR);
   if(m_sLocalInitiatedDialog)
   {
      initialDialogHandle.append(m_sLocalTag);
   }
   initialDialogHandle.append(DIALOG_HANDLE_SEPARATOR);
   if(!m_sLocalInitiatedDialog)
   {
      initialDialogHandle.append(m_sRemoteTag);
   }
}

void SipDialog::parseHandle(const UtlString& dialogHandle,
                            UtlString& callId,
                            UtlString& localTag,
                            UtlString& remoteTag)
{
   callId="";
   localTag = "";
   remoteTag = "";

   // The call-id ends at the first comma
   const char* callIdEnd = strchr(dialogHandle, DIALOG_HANDLE_SEPARATOR);
   if(callIdEnd)
   {
      // Move past the first comma
      const char* localTagBegin = callIdEnd + 1;

      // Copy the call id
      callId.append(dialogHandle, callIdEnd - dialogHandle);

      // The local tag ends at the second comma
      const char* localTagEnd = strchr(localTagBegin, DIALOG_HANDLE_SEPARATOR);
      if(localTagEnd)
      {
         // Copy the local tag
         localTag.append(localTagBegin, localTagEnd - localTagBegin);

         // The remote tag begins beyond the comma
         const char* remoteTagBegin = localTagEnd + 1;

         // Copy the remote tag
         remoteTag.append(remoteTagBegin);
      }
   }
}

void SipDialog::reverseTags(const UtlString& dialogHandle,
                            UtlString& reversedHandle)
{
   UtlString tag1;
   UtlString tag2;
   parseHandle(dialogHandle, reversedHandle, tag1, tag2); // copies call-id into reversedHandle
   reversedHandle.capacity(dialogHandle.length() + 2);
   reversedHandle.append(DIALOG_HANDLE_SEPARATOR);
   reversedHandle.append(tag2);
   reversedHandle.append(DIALOG_HANDLE_SEPARATOR);
   reversedHandle.append(tag1);
}

void SipDialog::getCallId(UtlString& callId) const
{
   callId = *this;
}

void SipDialog::setCallId(const char* callId)
{
   remove(0);
   append(callId ? callId : "");
   updateDialogState(); // also update dialog state, maybe dialog got established now
}

void SipDialog::getLocalField(Url& localField) const
{
   localField = m_localField;
}

void SipDialog::getLocalTag(UtlString& localTag) const
{
   localTag = m_sLocalTag;
}

void SipDialog::setLocalField(const Url& localField)
{
   m_localField = localField;
   m_localField.getFieldParameter("tag", m_sLocalTag); // also set tag
   updateDialogState(); // also update dialog state, maybe dialog got established now
}

void SipDialog::getRemoteField(Url& remoteField) const
{
   remoteField = m_remoteField;
}

void SipDialog::getRemoteTag(UtlString& remoteTag) const
{
   remoteTag = m_sRemoteTag;
}

void SipDialog::setRemoteField(const Url& remoteField)
{
   m_remoteField = remoteField;
   m_remoteField.getFieldParameter("tag", m_sRemoteTag); // also set tag
   updateDialogState(); // also update dialog state, maybe dialog got established now
}

void SipDialog::getRemoteContact(Url& remoteContact) const
{
   remoteContact = m_remoteContact;
}

void SipDialog::setRemoteContact(const Url& remoteContact)
{
   m_remoteContact = remoteContact;
}

void SipDialog::getLocalContact(Url& localContact) const
{
   localContact = m_localContact;
}

void SipDialog::setLocalContact(const Url& localContact)
{
   m_localContact = localContact;
}

void SipDialog::getLocalRequestUri(UtlString& requestUri) const
{
   requestUri = m_sLocalRequestUri;
}

void SipDialog::setLocalRequestUri(const UtlString& requestUri)
{
   m_sLocalRequestUri = requestUri;
}

void SipDialog::getRemoteRequestUri(UtlString& requestUri) const
{
   requestUri = m_sRemoteRequestUri;
}

void SipDialog::setRemoteRequestUri(const UtlString& requestUri)
{
   m_sRemoteRequestUri = requestUri;
}

void SipDialog::terminateDialog()
{
   m_dialogState = DIALOG_STATE_TERMINATED;
}

void SipDialog::getInitialMethod(UtlString& method) const
{
   method = m_sInitialMethod;
}

void SipDialog::setInitialMethod(const char* method)
{
   m_sInitialMethod = method;
}

int SipDialog::getLastLocalCseq() const
{
   return(m_iLastLocalCseq);
}

void SipDialog::setLastLocalCseq(int lastLocalCseq)
{
   m_iLastLocalCseq = lastLocalCseq;
}

int SipDialog::getLastRemoteCseq() const
{
   return(m_sLastRemoteCseq);
}

void SipDialog::setLastRemoteCseq(int lastRemoteCseq)
{
   m_sLastRemoteCseq = lastRemoteCseq;
}

int SipDialog::getNextLocalCseq()
{
   m_iLastLocalCseq++;
   return(m_iLastLocalCseq);
}

/* ============================ INQUIRY =================================== */


UtlBoolean SipDialog::isSameDialog(const SipMessage& message) const
{
   UtlString messageCallId;
   UtlString localTag;
   UtlString remoteTag;

   message.getCallIdField(&messageCallId);
   getLocalRemoteTag(message, localTag, remoteTag);

   return isSameDialog(messageCallId, localTag, remoteTag);
}

UtlBoolean SipDialog::isSameDialog(const UtlString& callId,
                                   const UtlString& localTag,
                                   const UtlString& remoteTag) const
{
   // Literal/exact match of tags only
   // i.e. do not allow a null tag to match a set tag
   UtlBoolean isSameDialog = FALSE;

   if(callId.compareTo(*this, UtlString::ignoreCase) == 0)
   {
      if(localTag.compareTo(m_sLocalTag, UtlString::ignoreCase) == 0 &&
         remoteTag.compareTo(m_sRemoteTag, UtlString::ignoreCase) == 0)
      {
         isSameDialog = TRUE;
      }
   }

   return isSameDialog;
}

UtlBoolean SipDialog::isSameDialog(const UtlString& dialogHandle)
{
   UtlString callId;
   UtlString localTag;
   UtlString remoteTag;
   parseHandle(dialogHandle, callId, localTag, remoteTag);
   return isSameDialog(callId, localTag, remoteTag);
}

UtlBoolean SipDialog::isInitialDialogFor(const SipMessage& message) const
{
   UtlString messageCallId;
   UtlString localTag;
   UtlString remoteTag;

   message.getCallIdField(&messageCallId);
   getLocalRemoteTag(message, localTag, remoteTag);

   return isInitialDialogFor(messageCallId, localTag, remoteTag);
}

UtlBoolean SipDialog::isInitialDialogFor(const UtlString& callId,
                                         const UtlString& localTag,
                                         const UtlString& remoteTag) const
{
   UtlBoolean isSameInitialDialog = FALSE;

   // for initial dialog, callId and localTag must match
   if(this->compareTo(callId, UtlString::ignoreCase) == 0)
   {
      if (m_sLocalInitiatedDialog)
      {
         if (localTag.compareTo(m_sLocalTag, UtlString::ignoreCase) == 0)
         {
            isSameInitialDialog = TRUE;
         }
      }
      else
      {
         // remotely initiated dialog
         if (remoteTag.compareTo(m_sRemoteTag, UtlString::ignoreCase) == 0)
         {
            isSameInitialDialog = TRUE;
         }
      }
   }

   return isSameInitialDialog;
}

UtlBoolean SipDialog::wasInitialDialogFor(const UtlString& callId,
                                          const UtlString& localTag,
                                          const UtlString& remoteTag) const
{
   return isInitialDialogFor(callId, localTag, remoteTag);
}

UtlBoolean SipDialog::isTransactionLocallyInitiated(const UtlString& callId,
                                               const UtlString& fromTag,
                                               const UtlString& toTag) const
{
   UtlBoolean isLocalDialog = FALSE;
   if(callId.compareTo(*this, UtlString::ignoreCase) == 0)
   {
      if(fromTag.compareTo(m_sLocalTag, UtlString::ignoreCase) == 0 &&
         (toTag.compareTo(m_sRemoteTag, UtlString::ignoreCase) == 0 || toTag.isNull() || m_sRemoteTag.isNull()))
      {
         isLocalDialog = TRUE;
      }
   }

   return(isLocalDialog);
}

UtlBoolean SipDialog::isTransactionRemotelyInitiated(const UtlString& callId,
                                                const UtlString& fromTag,
                                                const UtlString& toTag) const
{
   UtlBoolean isRemoteDialog = FALSE;
   if(callId.compareTo(*this, UtlString::ignoreCase) == 0)
   {
      if(((toTag.compareTo(m_sLocalTag, UtlString::ignoreCase) == 0 || toTag.isNull() || m_sLocalTag.isNull())) &&
         (fromTag.compareTo(m_sRemoteTag, UtlString::ignoreCase) == 0))
      {
         isRemoteDialog = TRUE;
      }
   }

   return(isRemoteDialog);
}

UtlBoolean SipDialog::isInitialDialog() const
{
   // For now make the simple assumption that if one of
   // the tags is not set that this is an early dialog
   // Note: RFC 2543 clients only needed to optionally
   // set the tags.  I do not think we need to support
   // RFC 2543 in this class.
   UtlBoolean tagNotSet = FALSE;
   if(m_sLocalTag.isNull() || m_sRemoteTag.isNull())
   {
      tagNotSet = TRUE;
   }

   return(tagNotSet);
}

UtlBoolean SipDialog::isInitialDialog(const UtlString& handle)
{
   // For now make the simple assumption that if one of
   // the tags is not set that this is an early dialog
   // Note: RFC 2543 clients only needed to optionally
   // set the tags.  I do not think we need to support
   // RFC 2543 in this class.
   UtlBoolean tagNotSet = FALSE;
   if(!handle.isNull())
   {
      UtlString callId;
      UtlString localTag;
      UtlString remoteTag;
      parseHandle(handle, callId, localTag, remoteTag);
      if(localTag.isNull() || remoteTag.isNull())
      {
         tagNotSet = TRUE;
      }
   }
   return(tagNotSet);

}

UtlBoolean SipDialog::isSameLocalCseq(const SipMessage& message) const
{
   int cseq;
   message.getCSeqField(&cseq, NULL);

   return(cseq == m_iLastLocalCseq);
}

UtlBoolean SipDialog::isSameRemoteCseq(const SipMessage& message) const
{
   int cseq;
   message.getCSeqField(&cseq, NULL);

   return(cseq == m_sLastRemoteCseq);
}

UtlBoolean SipDialog::isNextLocalCseq(const SipMessage& message) const
{
   int cseq;
   message.getCSeqField(&cseq, NULL);

   return(cseq > m_iLastLocalCseq);
}

UtlBoolean SipDialog::isNextRemoteCseq(const SipMessage& message) const
{
   int cseq;
   message.getCSeqField(&cseq, NULL);

   return(cseq > m_sLastRemoteCseq);
}

void SipDialog::toString(UtlString& dialogDumpString)
{
   // Serialize all the members into the dumpString
   char numberString[20];
   dialogDumpString="SipDialog: ";
   SNPRINTF(numberString, sizeof(numberString), "%p", this);
   dialogDumpString.append(numberString);
   dialogDumpString.append("\nCall-Id:");
   // The callId is stored in the UtlString base class data element
   dialogDumpString.append(*this);
   dialogDumpString.append("\nmLocalField:");
   UtlString tmpString;
   m_localField.toString(tmpString);
   dialogDumpString.append(tmpString);
   dialogDumpString.append("\nmRemoteField:");
   m_remoteField.toString(tmpString);
   dialogDumpString.append(tmpString);
   dialogDumpString.append("\nmLocalTag:");
   dialogDumpString.append(m_sLocalTag);
   dialogDumpString.append("\nmRemoteTag:");
   dialogDumpString.append(m_sRemoteTag);
   dialogDumpString.append("\nmLocalContact:");
   m_localContact.toString(tmpString);
   dialogDumpString.append(tmpString);
   dialogDumpString.append("\nmRemoteContact:");
   m_remoteContact.toString(tmpString);
   dialogDumpString.append(tmpString);
   dialogDumpString.append("\nmRouteSet:");
   dialogDumpString.append(m_sRouteSet);
   dialogDumpString.append("\nmInitialMethod:");
   dialogDumpString.append(m_sInitialMethod);
   dialogDumpString.append("\nmsLocalRequestUri:");
   dialogDumpString.append(m_sLocalRequestUri);
   dialogDumpString.append("\nmsRemoteRequestUri:");
   dialogDumpString.append(m_sRemoteRequestUri);
   dialogDumpString.append("\nmLocalInitatedDialog:");
   dialogDumpString.append(m_sLocalInitiatedDialog);
   SNPRINTF(numberString, sizeof(numberString), "%d", m_iInitialLocalCseq);
   dialogDumpString.append("\nmInitialLocalCseq:");
   dialogDumpString.append(numberString);
   SNPRINTF(numberString, sizeof(numberString), "%d", m_iInitialRemoteCseq);
   dialogDumpString.append("\nmInitialRemoteCseq:");
   dialogDumpString.append(numberString);
   SNPRINTF(numberString, sizeof(numberString), "%d", m_iLastLocalCseq);
   dialogDumpString.append("\nmLastLocalCseq:");
   dialogDumpString.append(numberString);
   SNPRINTF(numberString, sizeof(numberString), "%d", m_sLastRemoteCseq);
   dialogDumpString.append("\nmLastRemoteCseq:");
   dialogDumpString.append(numberString);
   SNPRINTF(numberString, sizeof(numberString), "%d", (int)m_bSecure);
   dialogDumpString.append("\nmSecure:");
   dialogDumpString.append(numberString);
   SNPRINTF(numberString, sizeof(numberString), "%d", (int)m_dialogState);
   dialogDumpString.append("\nmDialogState:");
   dialogDumpString.append(numberString);
   SNPRINTF(numberString, sizeof(numberString), "%d", (int)m_dialogSubState);
   dialogDumpString.append("\nmDialogSubState:");
   dialogDumpString.append(numberString);
}

void SipDialog::getLocalRemoteTag(const SipMessage& sipMessage, UtlString& sLocalTag, UtlString& sRemoteTag)
{
   sLocalTag.remove(0);
   sRemoteTag.remove(0);
   Url messageFromUrl;
   sipMessage.getFromUrl(messageFromUrl);
   Url messageToUrl;
   sipMessage.getToUrl(messageToUrl);
   bool swapTags = isTagSwapNeeded(sipMessage);

   if (swapTags)
   {
      messageFromUrl.getFieldParameter("tag", sRemoteTag);
      messageToUrl.getFieldParameter("tag", sLocalTag);
   }
   else
   {
      messageFromUrl.getFieldParameter("tag", sLocalTag);
      messageToUrl.getFieldParameter("tag", sRemoteTag);
   }
}

bool SipDialog::isTagSwapNeeded(const SipMessage& sipMessage)
{
   return !((bool)sipMessage.isRequest()) ^ ((bool)!sipMessage.isFromThisSide()); // bool XOR, don't change to UtlBoolean!
}

/* //////////////////////////// PROTECTED ///////////////////////////////// */

/* //////////////////////////// PRIVATE /////////////////////////////////// */

void SipDialog::updateDialogState(const SipMessage* pSipMessage)
{
   // only update state if not terminated
   if (m_dialogState != DIALOG_STATE_TERMINATED)
   {
      if (!isNull())
      {
         // call-id is not null
         if (!m_sLocalTag.isNull())
         {
            if (!m_sRemoteTag.isNull())
            {
               m_dialogState = DIALOG_STATE_ESTABLISHED;
            }
            else
            {
               m_dialogState = DIALOG_STATE_INITIAL;
            }
         }
         else
         {
            m_dialogState = DIALOG_STATE_INITIAL;
         }
      }
      else
      {
         m_dialogState = DIALOG_STATE_INITIAL;
      }

      if (m_dialogState == DIALOG_STATE_ESTABLISHED)
      {
         // check if SipMessage is 200 Ok
         if (pSipMessage && pSipMessage->isResponse())
         {
            int statusCode = pSipMessage->getResponseStatusCode();
            if (statusCode >= SIP_1XX_CLASS_CODE && statusCode < SIP_2XX_CLASS_CODE)
            {
               // we have both tags and 200 response, this is a confirmed dialog
               m_dialogSubState = DIALOG_SUBSTATE_EARLY;
            }
            else if (statusCode >= SIP_2XX_CLASS_CODE && statusCode < SIP_3XX_CLASS_CODE)
            {
               // we have both tags and 200 response, this is a confirmed dialog
               m_dialogSubState = DIALOG_SUBSTATE_CONFIRMED;
            }
         }
      }
      else if (m_dialogState == DIALOG_STATE_INITIAL)
      {
         // if dialog is in initial state and we get 5xx or 6xx, we terminate it
         if (pSipMessage && pSipMessage->isResponse())
         {
            int statusCode = pSipMessage->getResponseStatusCode();
            if (statusCode >= SIP_5XX_CLASS_CODE)
            {
               m_dialogState = DIALOG_STATE_TERMINATED;
            }
         }
      }
   }
}

void SipDialog::updateSecureFlag(const SipMessage* pSipMessage /*= NULL*/)
{
   if (pSipMessage && pSipMessage->isRequest())
   {
      // init secure flag
      UtlString sRequestUri;
      pSipMessage->getRequestUri(&sRequestUri);
      Url requestUrl(sRequestUri, TRUE);
      if (requestUrl.getScheme() == Url::SipsUrlScheme)
      {
         m_bSecure = TRUE;
         return;
      }
   }

   m_bSecure = FALSE;
}

/* ============================ FUNCTIONS ================================= */

