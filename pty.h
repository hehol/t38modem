/*
 * $Id: pty.h,v 1.1 2002-01-01 23:06:54 craigs Exp $
 *
 * T38FAX Pseudo Modem
 *
 * Original author: Vyacheslav Frolov
 *
 * $Log: pty.h,v $
 * Revision 1.1  2002-01-01 23:06:54  craigs
 * Initial version
 *
 * Revision 1.1  2002/01/01 23:06:54  craigs
 * Initial version
 *
 */

#ifndef _PTY_H
#define _PTY_H

#include "pmutils.h"

///////////////////////////////////////////////////////////////
class PseudoModemBody;
///////////////////////////////////////////////////////////////
class UniPty : public ModemThreadChild
{
    PCLASSINFO(UniPty, ModemThreadChild);
  public:
  
 
  /**@name Construction */
  //@{
    /**Create a new protocol handler.
     */
    UniPty(PseudoModemBody &_parent);
  //@}

  /**@name Operations */
  //@{
  //@}

  protected:
    PseudoModemBody &Parent() { return (PseudoModemBody &)parent; }
    int hPty;
};
///////////////////////////////////////////////////////////////
class InPty : public UniPty
{
    PCLASSINFO(InPty, UniPty);
  public:
  
 
  /**@name Construction */
  //@{
    /**Create a new protocol handler.
     */
    InPty(PseudoModemBody &_parent);
  //@}

  /**@name Operations */
  //@{
  //@}

  protected:
    virtual void Main();
};
///////////////////////////////////////////////////////////////
class OutPty : public UniPty
{
    PCLASSINFO(OutPty, UniPty);
  public:
 
  /**@name Construction */
  //@{
    /**Create a new protocol handler.
     */
    OutPty(PseudoModemBody &_parent);
  //@}

  /**@name Operations */
  //@{
  //@}

  protected:
    virtual void Main();
};
///////////////////////////////////////////////////////////////

#endif  // _PTY_H

