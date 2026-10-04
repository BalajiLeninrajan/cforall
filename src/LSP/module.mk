######################### -*- Mode: Makefile-Gmake -*- ########################
##
## Cforall Version 1.0.0 Copyright (C) 2015 University of Waterloo
##
## The contents of this file are covered under the licence agreement in the
## file "LICENCE" distributed with Cforall.
##
## module.mk -- LSP dump mode (cfa-lsp)
##
###############################################################################

SRC_LSP = \
	LSP/Lsp.cpp \
	LSP/Lsp.hpp

SRC += $(SRC_LSP)
SRCDEMANGLE += $(SRC_LSP)

# automake puts the top build directory on every include path (for config.h), and configure writes a file named
# "version" there, which shadows the C++ <version> header that nlohmann/json includes. Lsp.cpp does not need
# config.h, so it is compiled without that directory.
LSP/Lsp.$(OBJEXT): LSP/Lsp.cpp
	$(AM_V_CXX)$(CXX) $(DEFS) -I. -I$(srcdir) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) $(AM_CXXFLAGS) $(CXXFLAGS) \
		-MT $@ -MD -MP -MF LSP/$(DEPDIR)/Lsp.Tpo -c -o $@ $< && $(am__mv) LSP/$(DEPDIR)/Lsp.Tpo LSP/$(DEPDIR)/Lsp.Po
