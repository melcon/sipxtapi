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
#include <cp/CpMultiStringMessage.h>

// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
// STATIC VARIABLE INITIALIZATIONS

/* //////////////////////////// PUBLIC //////////////////////////////////// */

/* ============================ CREATORS ================================== */

// Constructor
CpMultiStringMessage::CpMultiStringMessage(unsigned char messageSubtype,
                                 const char* str1, const char* str2,
                                 const char* str3, const char* str4,
                                 const char* str5,
                                 intptr_t int1, intptr_t int2, intptr_t int3, intptr_t int4,
                                 intptr_t int5, intptr_t int6, intptr_t int7, intptr_t int8) :
OsMsg(OsMsg::PHONE_APP, messageSubtype)
{
   mInt1 = int1;
   mInt2 = int2;
   mInt3 = int3;
   mInt4 = int4;
   mInt5 = int5;
   mInt6 = int6;
   mInt7 = int7;
   mInt8 = int8;

   if (str1)
                mString1Data.append(str1);
   if (str2)
                mString2Data.append(str2);
   if (str3)
                mString3Data.append(str3);
   if (str4)
                mString4Data.append(str4);
   if (str5)
            mString5Data.append(str5);
}

// Copy constructor
CpMultiStringMessage::CpMultiStringMessage(const CpMultiStringMessage& rCpMultiStringMessage):
OsMsg(OsMsg::PHONE_APP, rCpMultiStringMessage.getMsgType())
{
        mString1Data = rCpMultiStringMessage.mString1Data;
        mString2Data = rCpMultiStringMessage.mString2Data;
        mString3Data = rCpMultiStringMessage.mString3Data;
        mString4Data = rCpMultiStringMessage.mString4Data;
        mString5Data = rCpMultiStringMessage.mString5Data;
    mInt1 = rCpMultiStringMessage.mInt1;
    mInt2 = rCpMultiStringMessage.mInt2;
    mInt3 = rCpMultiStringMessage.mInt3;
    mInt4 = rCpMultiStringMessage.mInt4;
    mInt5 = rCpMultiStringMessage.mInt5;
    mInt6 = rCpMultiStringMessage.mInt6;
    mInt7 = rCpMultiStringMessage.mInt7;
    mInt8 = rCpMultiStringMessage.mInt8;
}

// Destructor
CpMultiStringMessage::~CpMultiStringMessage()
{
        mString1Data.remove(0);
        mString2Data.remove(0);
        mString3Data.remove(0);
        mString4Data.remove(0);
        mString5Data.remove(0);
}

OsMsg* CpMultiStringMessage::createCopy(void) const
{
        return (new CpMultiStringMessage(getMsgSubType(),
        mString1Data.data(), mString2Data.data(),
        mString3Data.data(), mString4Data.data(),
        mString5Data.data(),
                mInt1, mInt2,mInt3, mInt4, mInt5, mInt6, mInt7, mInt8));
}

/* ============================ MANIPULATORS ============================== */

// Assignment operator
CpMultiStringMessage&
CpMultiStringMessage::operator=(const CpMultiStringMessage& rhs)
{
   if (this == &rhs)            // handle the assignment to self case
      return *this;

   OsMsg::operator=(rhs);
        mString1Data = rhs.mString1Data;
        mString2Data = rhs.mString2Data;
        mString3Data = rhs.mString3Data;
        mString4Data = rhs.mString4Data;
    mString5Data = rhs.mString5Data;
    mInt1 = rhs.mInt1;
    mInt2 = rhs.mInt2;
    mInt3 = rhs.mInt3;
    mInt4 = rhs.mInt4;
    mInt5 = rhs.mInt5;
    mInt6 = rhs.mInt6;
    mInt7 = rhs.mInt7;
    mInt8 = rhs.mInt8;

   return *this;
}

/* ============================ ACCESSORS ================================= */

void CpMultiStringMessage::getString1Data(UtlString& str1) const
{
        str1 = mString1Data;
}

void CpMultiStringMessage::getString2Data(UtlString& str2) const
{
        str2 = mString2Data;
}

void CpMultiStringMessage::getString3Data(UtlString& str3) const
{
        str3 = mString3Data;
}

void CpMultiStringMessage::getString4Data(UtlString& str4) const
{
        str4 = mString4Data;
}

void CpMultiStringMessage::getString5Data(UtlString& str5) const
{
        str5 = mString5Data;
}

intptr_t CpMultiStringMessage::getInt1Data() const
{
        return(mInt1);
}

intptr_t CpMultiStringMessage::getInt2Data() const
{
        return(mInt2);
}

intptr_t CpMultiStringMessage::getInt3Data() const
{
        return(mInt3);
}

intptr_t CpMultiStringMessage::getInt4Data() const
{
        return(mInt4);
}

intptr_t CpMultiStringMessage::getInt5Data() const
{
        return(mInt5);
}

intptr_t CpMultiStringMessage::getInt6Data() const
{
        return(mInt6);
}

intptr_t CpMultiStringMessage::getInt7Data() const
{
        return(mInt7);
}

intptr_t CpMultiStringMessage::getInt8Data() const
{
   return(mInt8);
}

void CpMultiStringMessage::toString(UtlString& dumpString, const char* term) const
{
    const char* terminator;
    if(term == NULL) terminator = "\n";
    else terminator = term;

    if(!mString1Data.isNull())
    {
        dumpString = "String1:\"" + mString1Data + "\"";
        dumpString += terminator;
    }
    if(!mString2Data.isNull())
    {
        dumpString += "String1:\"" + mString2Data + "\"";
        dumpString += terminator;
    }
    if(!mString3Data.isNull())
    {
        dumpString += "String1:\"" + mString3Data + "\"";
        dumpString += terminator;
    }
    if(!mString4Data.isNull())
    {
        dumpString += "String1:\"" + mString4Data + "\"";
        dumpString += terminator;
    }
    if(!mString5Data.isNull())
    {
        dumpString += "String1:\"" + mString5Data + "\"";
        dumpString += terminator;
    }

    char intDataString[100];
    if(mInt1)
    {
        SNPRINTF(intDataString, sizeof(intDataString), "Int1: %d", mInt1);
        dumpString += intDataString;
        dumpString += terminator;
    }
    if(mInt2)
    {
        SNPRINTF(intDataString, sizeof(intDataString), "Int2: %d", mInt2);
        dumpString += intDataString;
        dumpString += terminator;
    }
    if(mInt3)
    {
        SNPRINTF(intDataString, sizeof(intDataString), "Int3: %d", mInt3);
        dumpString += intDataString;
        dumpString += terminator;
    }
    if(mInt4)
    {
        SNPRINTF(intDataString, sizeof(intDataString), "Int4: %d", mInt4);
        dumpString += intDataString;
        dumpString += terminator;
    }
    if(mInt5)
    {
        SNPRINTF(intDataString, sizeof(intDataString), "Int5: %d", mInt5);
        dumpString += intDataString;
        dumpString += terminator;
    }
    if(mInt6)
    {
        SNPRINTF(intDataString, sizeof(intDataString), "Int6: %d", mInt6);
        dumpString += intDataString;
        dumpString += terminator;
    }
    if(mInt7)
    {
       SNPRINTF(intDataString, sizeof(intDataString), "Int7: %d", mInt7);
       dumpString += intDataString;
       dumpString += terminator;
    }
    if(mInt8)
    {
       SNPRINTF(intDataString, sizeof(intDataString), "Int8: %d", mInt8);
       dumpString += intDataString;
       dumpString += terminator;
    }

}

/* ============================ INQUIRY =================================== */

/* //////////////////////////// PROTECTED ///////////////////////////////// */

/* //////////////////////////// PRIVATE /////////////////////////////////// */

/* ============================ FUNCTIONS ================================= */
