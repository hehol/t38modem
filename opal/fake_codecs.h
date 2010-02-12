/*
 * fake_codecs.h
 *
 * T38FAX Pseudo Modem
 *
 * Copyright (c) 2010 Vyacheslav Frolov
 *
 * t38modem Project
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
 * The Original Code is t38modem.
 *
 * The Initial Developer of the Original Code is Vyacheslav Frolov
 *
 * Contributor(s):
 *
 * $Log: fake_codecs.h,v $
 * Revision 1.1  2010-02-12 08:20:10  vfrolov
 * Initial revision
 *
 * Revision 1.1  2010/02/12 08:20:10  vfrolov
 * Initial revision
 *
 */

#ifndef _FAKE_CODECS_H
#define _FAKE_CODECS_H

/////////////////////////////////////////////////////////////////////////////
namespace FakeCodecs {
/////////////////////////////////////////////////////////////////////////////
PStringArray GetAvailableAudioFormatsDescription(const char *name, const char *protocol);
void RegisterFakeAudioFormats(const PStringArray &wildcards);
/////////////////////////////////////////////////////////////////////////////
} // namespace FakeCodecs
/////////////////////////////////////////////////////////////////////////////

#endif  // _FAKE_CODECS_H

