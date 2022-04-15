CXX = g++
CFLAGS = -std=c++14 -O2 -Wall -g 

TARGET = test
OBJS = ../codes/log/*.cpp ../codes/pool/*.cpp ../codes/timer/*.cpp \
       ../codes/http/*.cpp ../codes/server/*.cpp \
       ../codes/buffer/*.cpp ../test/test.cpp

all: $(OBJS)
	$(CXX) $(CFLAGS) $(OBJS) -o $(TARGET)  -pthread -lmysqlclient

clean:
	rm -rf ../bin/$(OBJS) $(TARGET)