/*
 * fcs.cxx
 *
 * T38FAX Pseudo Modem
 *
 * Copyright (c) 2003 Vyacheslav Frolov
 *
 * Open H323 Project
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.0 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
 * the License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is Open H323 Library.
 *
 * The Initial Developer of the Original Code is Vyacheslav Frolov
 *
 * Contributor(s): Equivalence Pty ltd
 *
 * $Log: fcs.cxx,v $
 * Revision 1.2  2004-02-17 13:22:50  vfrolov
 * Fixed MSVC compile errors
 *
 * Revision 1.2  2004/02/17 13:22:50  vfrolov
 * Fixed MSVC compile errors
 *
 * Revision 1.1  2003/12/04 13:29:00  vfrolov
 * Initial revision
 *
 * 
 */

#include "fcs.h"

///////////////////////////////////////////////////////////////

#define new PNEW

///////////////////////////////////////////////////////////////
void FCS::build(const void *_pBuf, PINDEX count)
{
  const BYTE *pBuf = (const BYTE *)_pBuf;

  for (PINDEX i = 0 ; i < count ; i++) {
    BYTE c = *(pBuf++);
    for (BYTE m = 0x80 ; m ; m >>= 1) {
	fcs <<= 1;
        if (c & m)
	  fcs ^= 0x10000;
	if (fcs & 0x10000)
          fcs ^= 0x11021;
    }
  }
}
///////////////////////////////////////////////////////////////

