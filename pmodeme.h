/*
 * $Id: pmodeme.h,v 1.1 2002-01-01 23:06:54 craigs Exp $
 *
 * T38FAX Pseudo Modem
 *
 * Original author: Vyacheslav Frolov
 *
 * $Log: pmodeme.h,v $
 * Revision 1.1  2002-01-01 23:06:54  craigs
 * Initial version
 *
 * Revision 1.1  2002/01/01 23:06:54  craigs
 * Initial version
 *
 */

#ifndef _PMODEME_H
#define _PMODEME_H

#include "pmodemi.h"

///////////////////////////////////////////////////////////////
class ModemEngineBody;
class PseudoModemBody;
class T38Engine;

class ModemEngine : public ModemThreadChild
{
    PCLASSINFO(ModemEngine, ModemThreadChild);
  public:
 
  /**@name Construction */
  //@{
    ModemEngine(PseudoModemBody &_parent);
    ~ModemEngine();
  //@}

  /**@name Operations */
  //@{
    BOOL IsReady() const;
    BOOL Request(PStringToString &request) const;
    BOOL Attach(T38Engine *t38engine) const;
    void Detach(T38Engine *t38engine) const;
    const PString &modemToken() const { return Parent().modemToken(); }
  //@}

  protected:
    PseudoModemBody &Parent() const { return (PseudoModemBody &)parent; }
    virtual void Main();
    const PString &ptyName() const { return Parent().ptyName(); }
    void ToPtyQ(const void *buf, PINDEX count) const { Parent().ToPtyQ(buf, count, TRUE); }
    
    ModemEngineBody *body;
};
///////////////////////////////////////////////////////////////

#endif  // _PMODEME_H

