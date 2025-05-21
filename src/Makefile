CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -I../include
TARGET = taskmanager

all: $(TARGET)

$(TARGET): main.cpp task_manager.cpp ../include/task_manager.h
	$(CXX) $(CXXFLAGS) main.cpp task_manager.cpp -o $(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: all clean
