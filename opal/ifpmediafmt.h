/*
 * ifpmediafmt.h
 *
 * T38FAX Pseudo Modem
 *
 * Copyright (c) 2007 Vyacheslav Frolov
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
 * Contributor(s):
 *
 * $Log: ifpmediafmt.h,v $
 * Revision 1.1  2007-05-28 12:47:52  vfrolov
 * Initial revision
 *
 * Revision 1.1  2007/05/28 12:47:52  vfrolov
 * Initial revision
 *
 *
 */

#ifndef _IFP_MEDIA_FORMAT_H
#define _IFP_MEDIA_FORMAT_H

/////////////////////////////////////////////////////////////////////////////
extern void SetT38_IFP_PRE();
/////////////////////////////////////////////////////////////////////////////
class OpalMediaFormat;

extern const OpalMediaFormat & GetOpalT38_IFP_COR();
extern const OpalMediaFormat & GetOpalT38_IFP_PRE();

#define OpalT38_IFP_COR       GetOpalT38_IFP_COR()
#define OpalT38_IFP_PRE       GetOpalT38_IFP_PRE()
/////////////////////////////////////////////////////////////////////////////

#endif  // _IFP_MEDIA_FORMAT_H

