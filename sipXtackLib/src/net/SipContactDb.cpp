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
#include "net/SipContactDb.h"
#include "utl/UtlInt.h"
#include "utl/UtlVoidPtr.h"
#include "os/OsLock.h"
#include "utl/UtlHashMapIterator.h"

// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
// STATIC VARIABLE INITIALIZATIONS
// MACROS

/* //////////////////////////// PUBLIC //////////////////////////////////// */

/* ============================ CREATORS ================================== */

// Constructor
SipContactDb::SipContactDb() : 
    mNextContactId(1),
    mLock(OsMutex::Q_FIFO),
    mbTurnEnabled(FALSE)
{
    
}

// Destructor
SipContactDb::~SipContactDb()
{
    UtlHashMapIterator iterator(mContacts);
    
    UtlInt* pKey = NULL;
    while (pKey = (UtlInt*)iterator())
    {
        UtlVoidPtr* pValue = NULL;
        pValue = (UtlVoidPtr*)iterator.value();
        if (pValue)
        {
            delete pValue->getValue();
        }
    }
    mContacts.destroyAll();
}


/* ============================ MANIPULATORS ============================== */

const bool SipContactDb::addContact(SIPX_CONTACT_ADDRESS& contact)
{
    OsLock lock(mLock);
    bool bRet = false;
    
    assert (contact.id < 1);
    
    if (!isDuplicate(contact.cIpAddress, contact.iPort, contact.eContactType, contact.eTransportType))
    {
        assignContactId(contact);

        SIPX_CONTACT_ADDRESS* pContactCopy = new SIPX_CONTACT_ADDRESS(contact);
        mContacts.insertKeyAndValue(new UtlInt(pContactCopy->id), new UtlVoidPtr(pContactCopy));

        // If turn is enabled, duplicate the contact with a relay type
        if (mbTurnEnabled && contact.eContactType == CONTACT_LOCAL)
        {
            pContactCopy = new SIPX_CONTACT_ADDRESS(contact);
            pContactCopy->eContactType = CONTACT_RELAY ;
            pContactCopy->id = -1;
            addContact(*pContactCopy);
        }
        bRet = true;
    }
    else
    {
        // fill out the information in the contact,
        // to match what is already in the database
        contact = *(find(contact.cIpAddress, contact.iPort, contact.eContactType));
    }
    return bRet;
}

const UtlBoolean SipContactDb::deleteContact( const SIPX_CONTACT_ID id )
{
    OsLock lock(mLock);
    UtlInt idKey(id);
    return mContacts.destroy(&idKey);
}

SIPX_CONTACT_ADDRESS* SipContactDb::find(SIPX_CONTACT_ID id)
{
    OsLock lock(mLock);
    SIPX_CONTACT_ADDRESS* pContact = NULL;
    UtlInt idKey(id);
    
    UtlVoidPtr* pValue = (UtlVoidPtr*)mContacts.findValue(&idKey);
    if (pValue)
    {
        pContact = (SIPX_CONTACT_ADDRESS*)pValue->getValue();
    }
    
    return pContact;
}

// Find the local contact from a contact id
SIPX_CONTACT_ADDRESS* SipContactDb::getLocalContact(SIPX_CONTACT_ID id)
{
    OsLock lock(mLock);

    SIPX_CONTACT_ADDRESS* pRC = NULL ;
    SIPX_CONTACT_ADDRESS* pOriginal = find(id) ;
    if (pOriginal)
    {
        if (pOriginal->eContactType == CONTACT_LOCAL)
        {
            pRC = pOriginal ;
        }
        else
        {
            UtlHashMapIterator iterator(mContacts);
            UtlVoidPtr* pValue = NULL;
            SIPX_CONTACT_ADDRESS* pContact = NULL;
            UtlInt* pKey;
            while (pKey = (UtlInt*)iterator())
            {
                pValue = (UtlVoidPtr*)mContacts.findValue(pKey);
                assert(pValue);
                
                pContact = (SIPX_CONTACT_ADDRESS*)pValue->getValue();
                assert(pContact) ;
                if ((strcmp(pContact->cInterface, pOriginal->cInterface) == 0) && 
                    (pContact->eContactType == CONTACT_LOCAL))
                {
                    pRC = pContact ;
                    break ;
                }
            }
        }
    }

    return pRC ;
}


SIPX_CONTACT_ADDRESS* SipContactDb::find(const UtlString ipAddress, const int port, SIPX_CONTACT_TYPE type)
{
    OsLock lock(mLock);
    bool bFound = false;
    UtlHashMapIterator iterator(mContacts);

    UtlVoidPtr* pValue = NULL;
    SIPX_CONTACT_ADDRESS* pContact = NULL;
    UtlInt* pKey;
    while (pKey = (UtlInt*)iterator())
    {
        pValue = (UtlVoidPtr*)mContacts.findValue(pKey);
        assert(pValue);
        
        pContact = (SIPX_CONTACT_ADDRESS*)pValue->getValue();
        if (    (pContact->eContactType == type) &&
                (strcmp(pContact->cIpAddress, ipAddress.data()) == 0))
        {
            if (port < 0 || port == pContact->iPort)
            {
                bFound = true;
                break;
            }
        }
    }
    
    if (!bFound)
    {
        pContact = NULL;
    }
        
    return pContact;
}

void SipContactDb::getAll(SIPX_CONTACT_ADDRESS* contacts[], int& actualNum) const
{

    OsLock lock(mLock);
    UtlHashMapIterator iterator(mContacts);

    UtlVoidPtr* pValue = NULL;
    SIPX_CONTACT_ADDRESS* pContact = NULL;
    UtlInt* pKey;
    actualNum = 0; // array index
    while (pKey = (UtlInt*)iterator())
    {
        pValue = (UtlVoidPtr*)mContacts.findValue(pKey);
        assert(pValue);
        
        pContact = (SIPX_CONTACT_ADDRESS*)pValue->getValue();
        contacts[actualNum] = pContact;
        actualNum++;
    }
    return;
}
                                             
void SipContactDb::getAllForAdapter(const SIPX_CONTACT_ADDRESS* contacts[],
                                    const char* szAdapter,
                                    int& actualNum, 
                                    const SIPX_CONTACT_TYPE contactFilter) const
{

    OsLock lock(mLock);
    UtlHashMapIterator iterator(mContacts);

    UtlVoidPtr* pValue = NULL;
    SIPX_CONTACT_ADDRESS* pContact = NULL;
    UtlInt* pKey;
    actualNum = 0; // array index
    while (pKey = (UtlInt*)iterator())
    {
        pValue = (UtlVoidPtr*)mContacts.findValue(pKey);
        assert(pValue);
        
        pContact = (SIPX_CONTACT_ADDRESS*)pValue->getValue();
        
        if (0 != strcmp(pContact->cInterface, szAdapter))
        {
            continue;
        }
        if (contactFilter != CONTACT_AUTO && pContact->eContactType != contactFilter)
        {
            continue;
        }

        contacts[actualNum] = pContact;
        actualNum++;
    }
    
    return;
}


void SipContactDb::enableTurn(bool bEnable) 
{
    OsLock lock(mLock);
    UtlHashMapIterator iterator(mContacts);

    mbTurnEnabled = bEnable ;    

    UtlVoidPtr* pValue = NULL;
    SIPX_CONTACT_ADDRESS* pContact = NULL;
    UtlInt* pKey;
    while (pKey = (UtlInt*)iterator())
    {
        pValue = (UtlVoidPtr*)mContacts.findValue(pKey);
        assert(pValue);
        
        pContact = (SIPX_CONTACT_ADDRESS*)pValue->getValue();
        if (pContact)
        {
            if (mbTurnEnabled)
            {
                if ((pContact->eContactType == CONTACT_LOCAL) && (pContact->eTransportType == TRANSPORT_UDP))
                {
                    SIPX_CONTACT_ADDRESS* pContactCopy = new SIPX_CONTACT_ADDRESS(*pContact);
                    pContactCopy->eContactType = CONTACT_RELAY ;
                    assignContactId(*pContactCopy) ;
                    mContacts.insertKeyAndValue(new UtlInt(pContactCopy->id), new UtlVoidPtr(pContactCopy));
                }
            }
            else
            {
                if (pContact->eContactType == CONTACT_RELAY)
                {
                    deleteContact(pContact->id) ;
                }
            }
        }        
    }
}

/* ============================ ACCESSORS ================================= */

/* ============================ INQUIRY =================================== */



/* //////////////////////////// PROTECTED ///////////////////////////////// */


/* //////////////////////////// PRIVATE /////////////////////////////////// */

const bool SipContactDb::isDuplicate(const SIPX_CONTACT_ID id)
{
    OsLock lock(mLock);
    bool bRet = false;
    UtlInt idKey(id);
    
    UtlVoidPtr* pValue = (UtlVoidPtr*)mContacts.findValue(&idKey);
    if (pValue)
    {
        bRet = true;
    }
    return bRet;
}

const bool SipContactDb::isDuplicate(const UtlString& ipAddress, 
                                     const int port, SIPX_CONTACT_TYPE type, 
                                     SIPX_TRANSPORT_TYPE transportType)
{
    OsLock lock(mLock);
    bool bRet = false;
    UtlHashMapIterator iterator(mContacts);

    UtlVoidPtr* pValue = NULL;
    SIPX_CONTACT_ADDRESS* pContact = NULL;
    UtlInt* pKey;
    while (pKey = (UtlInt*)iterator())
    {
        pValue = (UtlVoidPtr*)mContacts.findValue(pKey);
        assert(pValue);
        
        pContact = (SIPX_CONTACT_ADDRESS*)pValue->getValue();
        if (    (pContact->eContactType == type) &&
                (strcmp(pContact->cIpAddress, ipAddress.data()) == 0) &&
                pContact->eTransportType == transportType)
        {
            if (port < 0 || port == pContact->iPort)
            {
                bRet = true;
                break;
            }
        }
    }
    return bRet;    
}

const bool SipContactDb::assignContactId(SIPX_CONTACT_ADDRESS& contact)
{
    OsLock lock(mLock);
    
    contact.id = mNextContactId;
    mNextContactId++;
    
    return true;
}

/* ============================ TESTING =================================== */

/* ============================ FUNCTIONS ================================= */

