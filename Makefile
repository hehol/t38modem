
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
