NAME    := VOID
VERSION := 0.1.0-alpha

include common.mk

PUBLIC_HEADERS := $(INC_DIR)/void.h \
                  $(INC_DIR)/api.h \
                  $(INC_DIR)/types.h

ifeq ($(TARGET_OS),windows)
    CCFLAGS += -Dmain=SDL_main
    LDLIBS  := $(subst -lSDL2,-lSDL2main -lSDL2,$(LDLIBS))
endif
