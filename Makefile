# 
# Makefile
# 
# T38FAX Pseudo Modem
# 
# Open H323 Project
# 
# The contents of this file are subject to the Mozilla Public License
# Version 1.0 (the "License"); you may not use this file except in
# compliance with the License. You may obtain a copy of the License at
# http://www.mozilla.org/MPL/
# 
# Software distributed under the License is distributed on an "AS IS"
# basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
# the License for the specific language governing rights and limitations
# under the License.
# 
# The Original Code is Open H323 Library.
# 
# The Initial Developer of the Original Code is Vyacheslav Frolov
# 
# Contributor(s): Equivalence Pty ltd
#
# $Log: Makefile,v $
# Revision 1.3  2002-01-10 06:10:02  craigs
# Added MPL header
#
# Revision 1.3  2002/01/10 06:10:02  craigs
# Added MPL header
#
# 

PROG		= t38modem
SOURCES		:= pmutils.cxx dle.cxx pmodem.cxx pty.cxx pmodemi.cxx pmodeme.cxx t38engine.cxx main.cxx

ifndef PWLIBDIR
PWLIBDIR=$(HOME)/pwlib
endif

include $(PWLIBDIR)/make/ptlib.mak

ifndef OPENH323DIR
OPENH323DIR=$(HOME)/openh323
endif

include $(OPENH323DIR)/openh323u.mak

ifdef NOTRACE
STDCCFLAGS += -DPASN_NOPRINTON -DPASN_LEANANDMEAN
else
STDCCFLAGS += -DPTRACING
endif

# ???
STDCCFLAGS += -Wall
