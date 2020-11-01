TARGET := example

GCC := gcc-5
GXX := g++-5

SOURCE_PATH := $(PWD)/../../
SOURCES := $(wildcard $(SOURCE_PATH)/*.cpp)

INCLUDE_PATH := -I$(PWD)/../../../libThread/inc
LIBRARY_PATH := $(PWD)/../../linux

LIBS := -L$(LIBRARY_PATH) -lthread

CFLAGS :=
CXXFLAGS := -O3 -DNDEBUG -Wall -W -Wextra -Wno-unused-parameter -std=c++11 -pthread -Wl,-Bsymbolic
LDFLAGS := -Wl,-Bsymbolic -Wl,-rpath=. -Wl,-rpath=$(LIBRARY_PATH)

BUILD_PATH := $(PWD)/build
$(shell mkdir -p $(BUILD_PATH))


OBJECTS := $(patsubst %.cpp, $(BUILD_PATH)/%.o, $(notdir $(SOURCES)))

BIN_PATH := $(PWD)/../../linux/
$(shell mkdir -p $(BIN_PATH))


TARGET_PATH := $(BIN_PATH)/$(TARGET)

$(TARGET_PATH) : $(OBJECTS)
	@echo .....linking......
	$(GXX) $(LDFLAGS) $< -o $@ $(LIBS)

$(BUILD_PATH)/%.o : $(SOURCES)
	$(GXX) $(CXXFLAGS) -c $(INCLUDE_PATH) $< -o $@


.PHONY: clean
	
clean:
	rm -rf $(BIN_PATH) $(BUILD_PATH)

