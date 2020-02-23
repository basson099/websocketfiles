CROSS = 
CC = $(CROSS)gcc
CXX = $(CROSS)g++
#DEBUG = -g -O2
DEBUG = -g -O0
CFLAGS = $(DEBUG) -Wall -c
RM = rm -rf

SRCPATH = ./src/
SRCS = $(wildcard $(SRCPATH)*.cpp)
OBJS = $(patsubst %.cpp, %.o, $(SRCS))

HEADER_PATH = -I./include
LIB_PATH = -L./ -L./lib/

LIBS = -luv

VERSION = 1.0.2
TARGET = wsfiles_server_uv.$(VERSION)

$(TARGET) : $(OBJS)
	$(CXX) $^ -o $@ $(LIB_PATH) $(LIBS)

$(OBJS):%.o : %.cpp
	$(CXX) $(CFLAGS) $< -o $@ $(HEADER_PATH)

clean:
	$(RM) $(TARGET) *.o 
	$(RM) $(SRCPATH)/*.o 
