//
// Copyright (C) 2004-2006 SIPfoundry Inc.
// Licensed by SIPfoundry under the LGPL license.
//
// Copyright (C) 2004-2006 Pingtel Corp.  All rights reserved.
// Licensed to SIPfoundry under a Contributor Agreement.
//
// $$
///////////////////////////////////////////////////////////////////////////////

#ifndef _OsSSL_h_
#define _OsSSL_h__

// SYSTEM INCLUDES

// APPLICATION INCLUDES                      
#include "os/OsBSem.h"
#include "os/OsSysLog.h"
#include "openssl/ssl.h"
#include <utl/UtlString.h>

// DEFINES
// MACROS
// EXTERNAL FUNCTIONS                                       
// EXTERNAL VARIABLES
// CONSTANTS
// STRUCTS
// TYPEDEFS
// FORWARD DECLARATIONS
class UtlString;
class UtlSList;

/**
 * Wrapper for the OpenSSL SSL_CTX context structure.
 * This class is responsible for all global policy initialization and
 * enforcement.
 *
 * If certificate or CA path is not supplied, default one is used.
 */
class OsSSL
{
/* //////////////////////////// PUBLIC //////////////////////////////////// */
  public:

/* ============================ CREATORS ================================== */

/* ============================ MANIPULATORS ============================== */
 
   static OsSSL* getInstance();

   /**
    * Sets directory containing several CA certificates. Create links using c_rehash.
    * @see http://www.openssl.org/docs/ssl/SSL_CTX_load_verify_locations.html
    */
   static void setCApath(const UtlString& path);

   /**
    * Sets PEM file (base64) containing CA certificates.
    */
   static void setCAfile(const UtlString& caFile);

   /**
    * Sets file containing PEM certificate. Certificate must not be encrypted.
    */
   static void setCertificateFile(const UtlString& file);

   /**
    * Sets file containing private key in PEM format. Key might be encrypted.
    */
   static void setPrivateKeyFile(const UtlString& file);

   /**
    * Sets password used for private key decryption.
    */
   static void setPassword(const UtlString& password);


/* ============================ ACCESSORS ================================= */

   /// Get an SSL server connection handle
   SSL* getServerConnection();

   /// Get an SSL client connection handle
   SSL* getClientConnection();

   /// Release an SSL session handle
   void releaseConnection(SSL*& connection);

   /// Get the validated names for the connection peer.
   static bool peerIdentity( SSL*       connection ///< SSL context from connection to be described
                            ,UtlSList*  altNames   /**< UtlStrings for verfied subjectAltNames
                                                    *   are added to this - caller must free them.
                                                    */
                            ,UtlString* commonName ///< the Subject name is returned here
                            );
   /**<
    * Usually, the names in the altNames will be easier to parse and use than commonName
    * Either or both of altNames or commonName may be NULL, in which case no names are returned;
    * the return value still indicates the trust relationship with the peer certificate.
    * @returns
    * - true if the connection peer is validated by a trusted authority
    * - false if not, in which case no names are returned.
    */

   /// Log SSL connection information
   static void logConnectParams(const OsSysLogFacility facility, ///< callers facility
                                const OsSysLogPriority priority, ///< log priority
                                const char* callerMsg,  ///< Identifies circumstances of connection
                                SSL*  connection  ///< SSL connection to be described
                                );
   

   /// Log an error resulting from an SSL call, with the SSL error text expanded
   static void logError(const OsSysLogFacility facility, ///< callers facility
                        const OsSysLogPriority priority, ///< how bad was it?
                        const char* callerMsg,  ///< Identifies caller and what failed
                        int errCode             ///< error returned from ssl routine
                        );
 
   /// Debug: print out list of ciphers enabled
   void dumpCipherList();

/* ============================ INQUIRY =================================== */


/* //////////////////////////// PROTECTED ///////////////////////////////// */
  protected:

     /// Construct an SSL Context from which connections are created.
     OsSSL();

     ~OsSSL();

/* //////////////////////////// PRIVATE /////////////////////////////////// */
  private:

   static bool sInitialized;
   
   SSL_CTX* mCTX;
   static OsBSem* m_spInstanceLock;
   static OsSSL* m_spInstance;
   static UtlString m_sCaPath;
   static UtlString m_sCaFile;
   static UtlString m_sCertificateFile;
   static UtlString m_sPrivateKeyFile;
   static UtlString m_sPassword;

   /// Certificate chain validation hook called by openssl
   static int verifyCallback(int valid,            ///< validity so far from openssl
                             X509_STORE_CTX* store ///< certificate information db
                             );

   /**
    * PEM password callback. Used to decrypt private keys.
    */
   static int pem_passwd_cb(char *buf, int size, int rwflag, void *userdata);

   /**<
    * @returns validity as determined by local policy
    * @note See 'man SSL_CTX_set_verify'
    */

   /// Disable copy constructor
   OsSSL(const OsSSL& rOsSSL);

   /// Disable assignment operator
   OsSSL& operator=(const OsSSL& rhs);
};

/* ============================ INLINE METHODS ============================ */

#endif  // _OsSSL_h_
