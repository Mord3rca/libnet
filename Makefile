CC=gcc
CXX=g++
AR=ar
RM=rm -f
CPPFLAGS=-std=c++11 -Wall -I./include
LIBS=

SRCS= src/tcp/proxy.cpp src/tcp/server.cpp\
			src/tcp/socket.cpp src/tcp/utils.cpp\
			src/http/server.cpp src/http/worker.cpp

OBJS=$(SRCS:.cpp=.o)

all: libnet.a

libnet.a: $(OBJS)
	$(AR) rcu libnet.a $(OBJS)

.cpp.o:
	$(CXX) $(CPPFLAGS) -c $< -o $@

clean:
	$(RM) libnet.a src/http/*.o src/tcp/*.o

install:
	cp ng-backend /usr/lib/
