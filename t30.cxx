/*
 * t30.cxx
 *
 * T38FAX Pseudo Modem
 *
 * Copyright (c) 2003-2008 Vyacheslav Frolov
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
 * $Log: t30.cxx,v $
 * Revision 1.5  2008-09-10 11:15:00  frolov
 * Ported to OPAL SVN trunk
 *
 * Revision 1.5  2008/09/10 11:15:00  frolov
 * Ported to OPAL SVN trunk
 *
 * Revision 1.4  2005/02/04 10:18:49  vfrolov
 * Fixed warnings for No Trace build
 *
 * Revision 1.3  2004/07/06 16:07:24  vfrolov
 * Included ptlib.h for precompiling
 *
 * Revision 1.2  2003/12/04 15:50:27  vfrolov
 * Fixed extracting ECM flag
 *
 * Revision 1.1  2003/12/04 13:38:46  vfrolov
 * Initial revision
 * 
 */

#include <ptlib.h>
#include "t30.h"

///////////////////////////////////////////////////////////////

#define new PNEW

///////////////////////////////////////////////////////////////
void T30::v21End(PBoolean myPTRACE_PARAM(sent))
{
  int size = v21frame.GetSize();
  PString msg;

  if (size < 3)
    msg = "too short";
  else
  if (v21frame[0] != 0xFF)
    msg = "w/o address field";
  else
  if ((v21frame[1] & 0xF7) != 0xC0)
    msg = "w/o control field";
  else {
    switch (v21frame[2]) {
      case 0x41:
      case 0x41 | 0x80:
        msg = "DCS";
        if (v21frame.GetSize() > 3+3 && (v21frame[3+2] & 1) && (v21frame[3+3] & 0x20)) {
          ecm = TRUE;
          msg += " with ECM";
        } else {
          ecm = FALSE;
        }

        cfr = FALSE;
        break;
      case 0x21:
      case 0x21 | 0x80:
        msg = "CFR";
        cfr = TRUE;
        break;
    }
  }
  myPTRACE(2, "T38Modem\t" << PString(sent ? "-->" : "<--") << " v21frame " << msg << PRTHEX(v21frame));
}
///////////////////////////////////////////////////////////////

