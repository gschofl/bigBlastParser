# a compiler with c++11 support is needed
# tested with gcc-4.8

EXEC		= bigBlastParser

CXX 		= g++
RM 			= rm -f
CXXFLAGS	= -std=c++11
CPPFLAGS	= -g -pthread -Wall
LDFLAGS		= -s
LDLIBS		= -lxerces-c -lsqlite3

SRCS		= bigBlastParser.cpp Blast.cpp BlastSAXHandler.cpp SQLite.cpp
OBJS		= $(subst .cpp,.o,$(SRCS))

all: $(EXEC)

$(EXEC): $(OBJS)
	g++ $(LDFLAGS) -o $(EXEC) $(OBJS) $(LDLIBS)

depend: .depend

.depend: $(SRCS)
	rm -f ./.depend
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MM $^>>./.depend;

clean:
	$(RM) $(OBJS)

dist-clean: clean
	$(RM) *~ .depend $(EXEC)

include .depend