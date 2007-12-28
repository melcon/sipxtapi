//  
// Copyright (C) 2007 Robert J. Andreasen, Jr.
// Licensed to SIPfoundry under a Contributor Agreement. 
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
// Copyright (C) 2007 Jaroslav Libak
// Licensed under the LGPL license.
// $$
///////////////////////////////////////////////////////////////////////////////

#ifndef CpCallLibEvents_h__
#define CpCallLibEvents_h__

// SYSTEM INCLUDES
// APPLICATION INCLUDES
// DEFINES
// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
// FORWARD DECLARATIONS
// STRUCTS
// TYPEDEFS

/**
 * Enumeration of possible media event causes inside sipxcalllib.
 *
 * @note keep synchronized with SIPX_MEDIA_CAUSE
 */
typedef enum CP_MEDIA_CAUSE
{
   CP_MEDIA_CAUSE_NORMAL = 0,             /**< Normal cause; the call was likely torn down.*/
   CP_MEDIA_CAUSE_HOLD,                   /**< Media state changed due to a local or remote
                                               hold operation */
   CP_MEDIA_CAUSE_UNHOLD,                 /**< Media state changed due to a local or remote
                                               unhold operation */
   CP_MEDIA_CAUSE_FAILED,                 /**< Media state changed due to an error condition. */
   CP_MEDIA_CAUSE_DEVICE_UNAVAILABLE,     /**< Media state changed due to an error condition,
                                               (device was removed, already in use, etc). */
   CP_MEDIA_CAUSE_INCOMPATIBLE,           /**< Incompatible destination -- We were unable
                                               to negotiate a codec */
   CP_MEDIA_CAUSE_DTMF_INBAND,		      /** Inband DTMF detected **/
   CP_MEDIA_CAUSE_DTMF_RFC2833,           /** RFC2833 DTMF detected **/
   CP_MEDIA_CAUSE_DTMF_SIPINFO	         /** SIP INFO DTMF detected **/
}CP_MEDIA_CAUSE;

/**
 * Enumeration of possible media event types.  Today, CP_MEDIA_TYPE_AUDIO and
 * CP_MEDIA_TYPE_VIDEO are supported.
 *
 * @note keep synchronized with SIPX_MEDIA_TYPE
 */
typedef enum CP_MEDIA_TYPE
{
   CP_MEDIA_TYPE_AUDIO = 0,               /**< Audio media event type */
   CP_MEDIA_TYPE_VIDEO,                   /**< Video media event type */
}CP_MEDIA_TYPE;

/**
 * Callstate cause events identify the reason for a Callstate event or 
 * provide more detail. They are passed to CpCallStateEventListener.
 *
 * @note keep synchronized with SIPX_CALLSTATE_CAUSE
 */
typedef enum CP_CALLSTATE_CAUSE
{
   CP_CALLSTATE_CAUSE_UNKNOWN,        /**< Unknown cause */
   CP_CALLSTATE_CAUSE_NORMAL,         /**< The stage changed due to normal operation */
   CP_CALLSTATE_CAUSE_TRANSFERRED,	  /**< A call is being transferred to this user 
                                           agent from another user agent.*/
   CP_CALLSTATE_CAUSE_TRANSFER,	     /**< A call on this user agent is being transferred 
                                           to another user agent. */                                     
   CP_CALLSTATE_CAUSE_CONFERENCE,     /**< A conference operation caused a stage change */
   CP_CALLSTATE_CAUSE_EARLY_MEDIA,    /**< The remote party is alerting and providing 
                                           ringback audio (early media) */
   CP_CALLSTATE_CAUSE_REQUEST_NOT_ACCEPTED, 
                                      /**< The callee rejected a request (e.g. hold) */
   CP_CALLSTATE_CAUSE_BAD_ADDRESS,    /**< The state changed due to a bad address.  This 
                                           can be caused by a malformed URL or network
                                           problems with your DNS server */
   CP_CALLSTATE_CAUSE_BUSY,           /**< The state changed because the remote party is
                                           busy */
   CP_CALLSTATE_CAUSE_RESOURCE_LIMIT, /**< Not enough resources are available to complete
                                           the desired operation */
   CP_CALLSTATE_CAUSE_NETWORK,        /**< A network error caused the desired operation to 
                                           fail */
   CP_CALLSTATE_CAUSE_REJECTED,       /**< The stage changed due to a rejection of a call. */
   CP_CALLSTATE_CAUSE_REDIRECTED,     /**< The stage changed due to a redirection of a call. */
   CP_CALLSTATE_CAUSE_NO_RESPONSE,    /**< No response was received from the remote party or 
                                           network node. */
   CP_CALLSTATE_CAUSE_AUTH,           /**< Unable to authenticate due to either bad or 
                                           missing credentials */
   CP_CALLSTATE_CAUSE_TRANSFER_INITIATED,  
                                      /**< A transfer attempt has been initiated.  This event
                                           is sent when a user agent attempts either a blind
                                           or consultative transfer. */
   CP_CALLSTATE_CAUSE_TRANSFER_ACCEPTED,  
                                      /**< A transfer attempt has been accepted by the remote
                                           transferee.  This event indicates that the 
                                           transferee supports transfers (REFER method).  The
                                           event is fired upon a 2xx class response to the SIP
                                           REFER request. */
   CP_CALLSTATE_CAUSE_TRANSFER_TRYING,
                                      /**< The transfer target is attempting the transfer.  
                                           This event is sent when transfer target (or proxy /
                                           B2BUA) receives the call invitation, but before the
                                           the tranfer target accepts is. */
   CP_CALLSTATE_CAUSE_TRANSFER_RINGING,
                                      /**< The transfer target is ringing.  This event is 
                                           generally only sent during blind transfer.  
                                           Consultative transfer should proceed directly to 
                                           TRANSFER_SUCCESS or TRANSFER_FAILURE. */
   CP_CALLSTATE_CAUSE_TRANSFER_SUCCESS,
                                      /**< The transfer was completed successfully.  The
                                           original call to transfer target will
                                           automatically disconnect.*/
   CP_CALLSTATE_CAUSE_TRANSFER_FAILURE,
                                      /**< The transfer failed.  After a transfer fails,
                                           the application layer is responsible for 
                                           recovering original call to the transferee. 
                                           That call is left on hold. */
   CP_CALLSTATE_CAUSE_REMOTE_SMIME_UNSUPPORTED,
                                      /**< Fired if the remote party's user-agent does not
                                           support S/MIME. */
   CP_CALLSTATE_CAUSE_SMIME_FAILURE,
                                      /**< Fired if a local S/MIME operation failed. 
                                           For more information, applications should 
                                           process the SECURITY event. */
   CP_CALLSTATE_CAUSE_BAD_REFER,       /**< An unusable refer was sent to this user-agent. */    
   CP_CALLSTATE_CAUSE_NO_KNOWN_INVITE, /**< This user-agent received a request or response, 
                                            but there is no known matching invite. */  
   CP_CALLSTATE_CAUSE_BYE_DURING_IDLE, /**< A BYE message was received, however, the call is in
                                            in an idle state. */       
   CP_CALLSTATE_CAUSE_UNKNOWN_STATUS_CODE, /**< A response was received with an unknown status code. */
   CP_CALLSTATE_CAUSE_BAD_REDIRECT,    /**< Receive a redirect with NO contact or a RANDOM redirect. */
   CP_CALLSTATE_CAUSE_TRANSACTION_DOES_NOT_EXIST, /**< No such transaction;  Accepting or Rejecting a call that
                                                       is part of a transfer. */
   CP_CALLSTATE_CAUSE_CANCEL,          /**< The event was fired in response to a cancel
                                            attempt from the remote party */
}CP_CALLSTATE_CAUSE;

// MACROS
// GLOBAL VARIABLES
// GLOBAL FUNCTIONS

#endif // CpCallLibEvents_h__
