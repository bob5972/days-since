# -*- mode: Makefile;-*-
#
# This file is part of days-since.
# Copyright (c) 2015 Michael Banack <bob5972@banack.net>
#
# days-since is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# days-since is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with days-since.  If not, see <http://www.gnu.org/licenses/>.
#
#
# Makefile -- days-since

include config.mk
-include local.mk

SRCROOT=.
INCLUDE_FLAGS=-I MBLib/public

SRCDIR=$(SRCROOT)
BUILDDIR=$(BUILDROOT)
DEPDIR=$(DEPROOT)

TARGET=days-since

#The BUILDROOT folder is included for config.h
CFLAGS = ${DEFAULT_CFLAGS} -I $(BUILDROOT) $(INCLUDE_FLAGS)
CPPFLAGS = ${CFLAGS}

LIBFLAGS = -lboost_date_time

CC=gcc
CXX=g++

$(BUILDDIR)/%.o: $(SRCDIR)/%.c
	${CC} -c ${CFLAGS} -o $(BUILDDIR)/$*.o $<;
$(BUILDDIR)/%.opp: $(SRCDIR)/%.cpp
	${CXX} -c ${CPPFLAGS} -o $(BUILDDIR)/$*.opp $<;

#Autogenerate dependencies information
#The generated makefiles get sourced into this file later
$(DEPDIR)/%.d: $(SRCDIR)/%.c Makefile
	$(CC) -M -MM -MT "$(BUILDDIR)/$*.o" -MT "$(DEPDIR)/$*.d" -MF $@ ${CFLAGS} $<;
$(DEPROOT)/%.dpp: $(SRCDIR)/%.cpp Makefile
	$(CXX) -M -MM -MT "$(BUILDDIR)/$*.opp" \
	    -MT "$(DEPDIR)/$*.dpp" \
	    -MF $@ ${CPPFLAGS} $<;

#No paths. VPATH is assumed
C_SOURCES = 
CPP_SOURCES = main.cpp

#For reasons I cannot fathom, MBLIB_OBJ has to be last
# or things don't link right...

MBLIB_OBJ = $(MBLIB_BUILDDIR)/MBLib.a

C_OBJ=$(addprefix $(BUILDDIR)/, $(subst .c,.o, $(C_SOURCES)))
CPP_OBJ=$(addprefix $(BUILDDIR)/, $(subst .cpp,.opp, $(CPP_SOURCES)))
TARGET_OBJ = $(C_OBJ) $(CPP_OBJ) $(MBLIB_OBJ)
		     
.PHONY: all clean distclean dist MBLib

#The config check is to test if we've been configured
all: config.mk $(BUILDROOT)/config.h $(TARGET)

$(MBLIB_OBJ): MBLib

MBLib:
	$(MAKE) -f $(MBLIB_SRCDIR)/Makefile all

$(TARGET): $(BUILDROOT)/$(TARGET)

$(BUILDROOT)/$(TARGET): $(TARGET_OBJ)
	${CXX} ${CFLAGS} ${LIBFLAGS} $(TARGET_OBJ) -o $(BUILDROOT)/$(TARGET)

# XXX: I don't yet have a way to auto create the build dirs before
#      building... 
# clean leaves the dep files
clean:
	$(MAKE) -f $(MBLIB_SRCDIR)/Makefile clean
	rm -f $(TMPDIR)/*
	rm -f $(BUILDDIR)/*.o $(BUILDDIR)/*.opp
	rm -f $(BUILDROOT)/$(TARGET)

# after a distclean you'll need to run configure again
distclean: clean
	rm -rf $(BUILDROOT)/
	rm -rf config.mk
	
install: $(TARGET)
	./install.sh

#include the generated dependency files
-include $(addprefix $(DEPDIR)/,$(subst .c,.d,$(C_SOURCES)))
-include $(addprefix $(DEPDIR)/,$(subst .cpp,.dpp,$(CPP_SOURCES)))



