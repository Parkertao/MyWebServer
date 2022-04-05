CXX = g++
CFLAGS = -std=c++14 -Wall -g


TARGET = server
OBJS = ../codes/log/*.cpp ../codes/pool/*.cpp ../codes/timer/*.cpp \
       ../codes/http/*.cpp ../codes/server/*.cpp \
       ../codes/buffer/*.cpp ../codes/main.cpp

all: $(OBJS)
	$(CXX) $(CFLAGS) $(OBJS) -o ../bin/$(TARGET)  -pthread -lmysqlclient

clean:
	rm -rf ../bin/$(OBJS) $(TARGET)