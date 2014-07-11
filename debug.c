/*
   Copyright 2012 Guillem Vinals Gangolells <guillem@guillem.co.uk>

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
 */

#include "debug.h"
#include "HardwareProfile.h"
#include <stdarg.h>

inline void DBG_assert(BOOL bCondition, CHAR *pszFile, INT iLine)
{
    if (!bCondition)
    {
        DBG_ERROR("ERROR: Assertion failed! (%s:%d)\n", pszFile, iLine);
        exit(0);
    }
}

void DBG_trace(UINT uClass, CHAR *pszFile, INT iLine)
{
    if (uClass & DBG_MASK)
        SIOPrintString( "0x%04X Trace (%s:%d)\n", uClass, pszFile, iLine);
}

/*
 * NOTICE: Microchip's implementation of SIOPrintString does not support
 * an va_list as an argument. This limitation makes almost useless the entire
 * following implementation. Please use it just as a reference.
 */

#if 0
void DBG_info(UINT uClass, CHAR *pszString, ...)
{
    va_list pArg;
    va_start(pArg, pszString);
    if ((uClass & DBG_MASK) && (DBG_INFO >= DBG_LEVEL))
    {
        SIOPrintString(pszString, pArg);
    }
    va_end(pArg);
}

void DBG_exInfo(UINT uClass, CHAR *pszString, ...)
{
    va_list pArg;
    va_start(pArg, pszString);
    if ((uClass & DBG_MASK) && (DBG_EXINFO >= DBG_LEVEL))
    {
        SIOPrintString(pszString, pArg);
    }
    va_end(pArg);
}

void DBG_warn(UINT uClass, CHAR *pszString, ...)
{
    va_list pArg;
    va_start(pArg, pszString);
    if ((uClass & DBG_MASK) && (DBG_WARN >= DBG_LEVEL))
    {
        SIOPrintString(pszString, pArg);
    }
    va_end(pArg);
}

void DBG_error(UINT uClass, CHAR *pszString, ...)
{
    va_list pArg;
    va_start(pArg, pszString);
    if ((uClass & DBG_MASK) && (DBG_ERR >= DBG_LEVEL))
    {
        SIOPrintString(pszString, pArg);
    }
    va_end(pArg);
}
#endif

void DBG_dump(UINT uClass, BYTE *pData, UINT uLen)
{
    if ((uClass & DBG_MASK) && (DBG_INFO >= DBG_LEVEL))
    {
        INT i;
        for(i = 0; i<uLen; ++i)
        {
//            SIOPrintString("%02X ", pData[i]);
            xprintf("%02X ", pData[i]);
        }
        xprintf("\r\n");
    }
}
