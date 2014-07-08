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

#ifndef __RFCOMM_FCS_H__
#define __RFCOMM_FCS_H__

#define RFCOMM_INITIAL_CRC 0xFF
#define RFCOMM_VALID_CRC 0xCF

BYTE RFCOMM_FCS_CheckCRC(const BYTE *pData, UINT uLen, BYTE bCheckSum);
BYTE RFCOMM_FCS_CalcCRC(const BYTE *pData, UINT uLen);

#endif /* __RFCOMM_FCS_H__ */
