AR= ar rc
RANLIB= ranlib
RM= rm -f
CC= gcc

EFFEKSEER_DIR=../Effekseer/Dev/Cpp
BGFX_DIR=../bgfx
BX_DIR=../bx

CFLAGS=-O2 -Wall -std=c++17 -Wno-unused-but-set-variable -Wno-maybe-uninitialized -Wno-sign-compare
SHARED=--shared
EFFEKSEER_INC=-I $(EFFEKSEER_DIR)/Effekseer -I $(EFFEKSEER_DIR)
BGFX_INC=-I $(BGFX_DIR)/include -I $(BX_DIR)/include
LIBS=-lws2_32 -lstdc++
BGFX_LIBS=-L$(BGFX_DIR)/.build/win64_mingw-gcc/bin -lbgfx-shared-libRelease

EFK_SOURCE_DIRS:=$(wildcard $(EFFEKSEER_DIR)/Effekseer/Effekseer/*/)
SOURCE_DIRS:=\
  $(EFFEKSEER_DIR)/Effekseer/Effekseer/\
  $(EFFEKSEER_DIR)/EffekseerMaterial/\
  $(EFFEKSEER_DIR)/EffekseerRendererCommon/\
  $(EFK_SOURCE_DIRS)

find_files=$(wildcard $(dir)*.cpp)
SOURCES:=$(foreach dir,$(SOURCE_DIRS),$(find_files))
OBJECTS:=$(SOURCES:.cpp=.o)

all : efkbgfx.dll

$(OBJECTS) : %.o : %.cpp
	$(CC) -c $(CFLAGS) -o $@ $^ $(EFFEKSEER_INC)

libeffekseer.a : $(OBJECTS)
	$(AR) $@ $^ && $(RANLIB) $@


EFKMATC_SOURCES:=$(wildcard efkmatc/*.cpp)
EFKBGFX_SOURCES:=$(wildcard renderer/*.cpp)

#efkmatc.dll : $(EFKMATC_SOURCES) libeffekseer.a
#	$(CC) -o $@ $(SHARED) $(CFLAGS) -o $@ $^ $(EFFEKSEER_INC) $(LIBS)


efkbgfx.dll : $(EFKBGFX_SOURCES) libeffekseer.a
	$(CC) -o $@ $(SHARED) $(CFLAGS) -o $@ $^ $(EFFEKSEER_INC) $(BGFX_INC) $(BGFX_LIBS) $(LIBS)


clean :
	$(RM) libeffekseer.a efkbgfx.dll $(OBJECTS)
