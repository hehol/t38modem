/*
 * t30tone.h
 *
 * T38FAX Pseudo Modem
 *
 * Copyright (c) 2002 Vyacheslav Frolov
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
 * $Log: t30tone.h,v $
 * Revision 1.1  2002-04-30 10:59:10  vfrolov
 * Initial revision
 *
 * Revision 1.1  2002/04/30 10:59:10  vfrolov
 * Initial revision
 *
 *
 */

#ifndef _T30TONE_H
#define _T30TONE_H

#include <ptlib.h>

///////////////////////////////////////////////////////////////
class T30Tone : public PObject
{ 
  PCLASSINFO(T30Tone, PObject);

  public:
  
    enum Type {
      silence,
      cng
    };

    T30Tone(Type _type);
    void Read(void * buffer, PINDEX amount);

  protected:
    
    Type type;
    PINDEX index;
};
///////////////////////////////////////////////////////////////

#endif  // _T30TONE_H

