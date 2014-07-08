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

#ifndef __BT_UTILS__
#define __BT_UTILS__

#include "GenericTypeDefs.h"

void* BT_malloc(size_t uSize);
void BT_free(void* pData);

#define BT_MALLOC(X)    BT_malloc(X);
#define BT_FREE(X)       BT_free(X);

/*Read (little-endian 16bits*/
WORD BT_readLE16(const BYTE *pData, UINT uOffset);

/*Store (little-endian) 16bits*/
BOOL BT_storeLE16(WORD value, BYTE *pData, UINT uOffset);

/*Store (little-endian) 32bits*/
BOOL BT_storeLE32(DWORD value, BYTE *pData, UINT uOffset);

/*Read (big-endian 16bits)*/
WORD BT_readBE16(const BYTE *pData, UINT uOffset);

/*Store (big-endian) 16bits*/
BOOL BT_storeBE16(WORD value, BYTE *pData, UINT uOffset);

/*Read (big-endian) 32bits*/
DWORD BT_readBE32(const BYTE *pData, UINT uOffset);

/*Store (big-endian) 32bits*/
BOOL BT_storeBE32(DWORD value, BYTE *pData, UINT uOffset);

/*Store string*/
BOOL store_STR(const char *str, int str_size, BYTE *pData, UINT uOffset);

BOOL BT_isEqualBD_ADDR(const BYTE *pBD_ADDR1, const BYTE *pBD_ADDR2);

#endif /*BT_UTILS*/
