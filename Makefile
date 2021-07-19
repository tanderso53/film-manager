#####################################################################
#                                                                   #
#                            FILM MANAGER                           #
#                  A program for logging photographic               #
#                            film records                           #
#                                                                   #
#####################################################################

# Copyright 2021 Tyler J. Anderson

# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:

# 1. Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.

# 2. Redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in the
# documentation and/or other materials provided with the distribution.

# 3. Neither the name of the copyright holder nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.

# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
# FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
# COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
# ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.

# film-manager Makefile

srcdir		=	./src
OBJDIR		:=	./objdir

# BUILD SECTION
CXX		=	clang++
CFLAGS		=	-Wall -std=c++17 -I/usr/local/include
LDLIBS		=	
LDFLAGS		=	-L/usr/local/lib
APP		=	film-manager
CXX_SRCS	=	film-manager.cpp
CXX_OBJS	=	$(addprefix $(OBJDIR)/,$(CXX_SRCS:.cpp=.o))
OBJS		:=	$(CXX_OBJS)
INSTROBJ	:=	$(OBJS:.o=.oi)
HPP		=	
LICENSE		=	./LICENSE
.PHONY: all clean install coverage

$(OBJDIR)/%.o: $(srcdir)/%.cpp $(addprefix $(srcdir)/,$(HPP))
	@echo "*** BUILDING $@ ***"
	$(CXX) -c ${CFLAGS} -o $@ $<

$(OBJDIR)/%.oi: $(srcdir)/%.cpp $(addprefix $(srcdir/,$(HPP))
	@echo "*** BUILDING $@ ***"
	$(CXX) -c ${CFLAGS} -fprofile-instr-generate -fcoverage-mapping -o $@ $<

$(APP): $(OBJS)
	@echo "*** BUILDING $@ ***"
	$(CXX) ${CFLAGS} ${LDFLAGS} ${LDLIBS} -o $@ $(OBJS)
	@echo "Complete! Install with \"make install\""

all: $(APP)

clean:
	$(RM) $(APP)
	$(RM) -R $(OBJDIR)

coverage: $(INSTROBJ)
	@echo "*** BUILDING $@ ***"
	$(CXX) ${CFLAGS} ${LDFLAGS} ${LDLIBS} \
		-fprofile-instr-generate -fcoverage-mapping -o $@ $(OBJS)
	

$(OBJS): | $(OBJDIR)

$(OBJDIR):
	mkdir $(OBJDIR)

# INSTALL SECTION
INSTALL=install
INSTALL_PROGRAM=$(INSTALL)
INSTALL_DATA=$(INSTALL) -m 644
PREFIX=/usr/local
BINDIR=$(PREFIX)/bin
DATAROOTDIR=$(PREFIX)/share
DATADIR=$(DATAROOTDIR)
SYSCONFDIR=$(PREFIX)/etc
DOCDIR=$(DATAROOTDIR)/doc/$(APP)

install: all
	$(INSTALL_PROGRAM) $(APP) $(DESTDIR)$(BINDIR)/$(APP)
	$(INSTALL_DATA) $(LICENSE) $(DESTDIR)$(DATADIR)/$(APP)/LICENSE
