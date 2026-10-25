ifeq ($(OS),Windows_NT)
    HOST_OS := windows

    ifeq ($(PROCESSOR_ARCHITECTURE),AMD64)
        HOST_ARCH := x86_64
    else ifeq ($(PROCESSOR_ARCHITEW6432),AMD64)
        HOST_ARCH := x86_64
    else ifeq ($(PROCESSOR_ARCHITECTURE),x86)
        HOST_ARCH := x86
    else ifeq ($(PROCESSOR_ARCHITEW6432),x86)
        HOST_ARCH := x86
    endif

    RED    :=
    GREEN  :=
    YELLOW :=
    BLUE   :=
    NC     :=
else
    RAW_OS   := $(shell uname -s 2>/dev/null || echo Unknown)
    RAW_ARCH := $(shell uname -m 2>/dev/null || echo Unknown)

    ifeq ($(RAW_OS),Darwin)
        $(error "Darwin host is not yet implemented.")
    else ifeq ($(RAW_OS),Linux)
        HOST_OS := linux
    else
        $(error "Unsupported OS: $(RAW_OS)")
    endif

    ifneq (,$(filter aarch64 arm64,$(RAW_ARCH)))
        $(error "arm64 host architecture is not yet implemented.")
    else ifneq (,$(filter %64,$(RAW_ARCH)))
        HOST_ARCH := x86_64
    else ifneq (,$(filter %86,$(RAW_ARCH)))
        HOST_ARCH := x86
    else
        $(error "Unsupported Architecture: $(RAW_ARCH)")
    endif

    RED    := \033[0;31m
    GREEN  := \033[0;32m
    YELLOW := \033[0;33m
    BLUE   := \033[0;34m
    NC     := \033[0m
endif

INPUT_OS   := $(strip $(OS))
INPUT_ARCH := $(strip $(ARCH))

ifeq ($(INPUT_OS),)
    INPUT_OS := $(HOST_OS)
endif
ifeq ($(INPUT_ARCH),)
    INPUT_ARCH := $(HOST_ARCH)
endif

ifneq (,$(filter win win32 windows Windows_NT,$(INPUT_OS)))
    TARGET_OS := windows
else ifneq (,$(filter linux Linux,$(INPUT_OS)))
    TARGET_OS := linux
else ifneq (,$(filter mac macos darwin Darwin,$(INPUT_OS)))
    $(error "Darwin target is not yet implemented.")
else
    TARGET_OS := $(INPUT_OS)
endif

ifneq (,$(filter x64 amd64 x86_64,$(INPUT_ARCH)))
    TARGET_ARCH := x86_64
else ifneq (,$(filter x32 x86 i386 i686,$(INPUT_ARCH)))
    TARGET_ARCH := x86
else ifneq (,$(filter arm arm64 aarch64,$(INPUT_ARCH)))
    $(error "arm64 target is not yet implemented.")
else
    TARGET_ARCH := $(INPUT_ARCH)
endif

CC       := gcc
CXX      := g++
AR       := ar
STRIP    := strip
BIN_EXT  :=
LIB_EXT  :=

ifeq ($(TARGET_OS),windows)
    BIN_EXT := .exe
    LIB_EXT := .dll

    RUNTIME ?= msvcrt

    ifneq ($(HOST_OS),windows)
        ifeq ($(TARGET_ARCH),x86_64)
            ifeq ($(RUNTIME),ucrt)
                CC    := x86_64-w64-mingw32ucrt-gcc
                CXX   := x86_64-w64-mingw32ucrt-g++
                AR    := x86_64-w64-mingw32ucrt-ar
                STRIP := x86_64-w64-mingw32ucrt-strip
            else
                CC    := x86_64-w64-mingw32-gcc
                CXX   := x86_64-w64-mingw32-g++
                AR    := x86_64-w64-mingw32-ar
                STRIP := x86_64-w64-mingw32-strip
            endif
        else ifeq ($(TARGET_ARCH),x86)
            CC    := i686-w64-mingw32-gcc
            CXX   := i686-w64-mingw32-g++
            AR    := i686-w64-mingw32-ar
            STRIP := i686-w64-mingw32-strip
        endif
    endif
else ifeq ($(TARGET_OS),linux)
    BIN_EXT  :=
    LIB_EXT  := .so
endif

ifeq ($(TARGET_ARCH),x86)
    ARCH_FLAGS := -m32
else
    ARCH_FLAGS := -m64
endif

BIN_NAME   := $(NAME)$(BIN_EXT)
LIB_NAME   := lib$(NAME)$(LIB_EXT)

SRC_DIR   := src
INC_DIR   := include

LIB_DIR   := lib/$(TARGET_OS)-$(TARGET_ARCH)
BIN_DIR   := bin/$(TARGET_OS)-$(TARGET_ARCH)
BUILD_DIR := build/$(TARGET_OS)-$(TARGET_ARCH)

OBJ_DIR   := $(BUILD_DIR)/obj
DEP_DIR   := $(BUILD_DIR)/dep

TARGET_BIN := $(BIN_DIR)/$(BIN_NAME)
TARGET_LIB := $(LIB_DIR)/$(LIB_NAME)

CFLAGS  := $(ARCH_FLAGS) -Wall -Wextra -Wformat=2 -Werror=format-security -I$(INC_DIR) -DVERSION=\"$(VERSION)\" $(CFLAGS)
LDFLAGS := $(ARCH_FLAGS) -static-libgcc -static-libstdc++ -L$(LIB_DIR) $(LDFLAGS)
DEPFLAGS = -MT $@ -MMD -MP -MF $(DEP_DIR)/$*.d

ifeq ($(TARGET_OS),windows)
    CFLAGS  += -D__USE_MINGW_ANSI_STDIO=1
    LDFLAGS += -Wl,--dynamicbase -Wl,--nxcompat
    LDLIBS  += -lmingw32 -mwindows -lbcrypt
    OSFLAG  :=
else ifeq ($(TARGET_OS),linux)
    CFLAGS  += -fPIC -D_GNU_SOURCE
    LDFLAGS += -Wl,--disable-new-dtags -Wl,-rpath='$$ORIGIN:$$ORIGIN/../lib'
    LDLIBS  += -lm -ldl -pthread
    OSFLAG  := -pie
endif

LDLIBS += -lSDL2 -lSDL2_image -lSDL2_mixer

BUILD ?= debug
ifeq ($(BUILD),debug)
    CFLAGS  += -Og -g
else
    CFLAGS  += -O2 -g -fstack-protector-strong -fstack-clash-protection -fcf-protection -D_FORTIFY_SOURCE=2 -DNDEBUG
    ifeq ($(TARGET_OS),linux)
        LDFLAGS += -Wl,-z,relro,-z,now
    endif
endif

CCFLAGS  := $(CFLAGS) -std=c17
CXXFLAGS := $(CFLAGS) -std=c++20

SRC_MAIN := $(wildcard $(SRC_DIR)/main.c) $(wildcard $(SRC_DIR)/main.cpp)

OBJ_MAIN := $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(filter %.c, $(SRC_MAIN))) \
            $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(filter %.cpp, $(SRC_MAIN)))

SRCS_C   := $(filter-out $(SRC_MAIN), $(shell find $(SRC_DIR) -type f -name "*.c"))
SRCS_CPP := $(filter-out $(SRC_MAIN), $(shell find $(SRC_DIR) -type f -name "*.cpp"))

OBJS_CORE := $(SRCS_C:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o) \
             $(SRCS_CPP:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)

DEPS      := $(OBJS_CORE:$(OBJ_DIR)/%.o=$(DEP_DIR)/%.d) \
             $(OBJ_MAIN:$(OBJ_DIR)/%.o=$(DEP_DIR)/%.d)

EXEC := $(TARGET_BIN)

ifeq ($(TARGET_OS),windows)
    ifneq ($(HOST_OS),windows)
        EXEC := WINEDEBUG=-all wine $(TARGET_BIN)
    endif
endif

define POST_BUILD_STEP
    @if [ "$(TARGET_OS)" = "windows" ]; then \
        echo "$(BLUE)[DLL] Copying dynamic libraries...$(NC)"; \
        cp $(LIB_DIR)/*.dll $(BIN_DIR)/ 2>/dev/null || true; \
        if [ "$(BUILD)" = "release" ]; then \
            echo "$(BLUE)[STRIP] Stripping executables and DLLs in bin/...$(NC)"; \
            $(STRIP) $(BIN_DIR)/*.dll $(BIN_DIR)/*.exe 2>/dev/null || true; \
        fi; \
    elif [ "$(TARGET_OS)" = "linux" ]; then \
        echo "$(BLUE)[SO] Copying dynamic libraries...$(NC)"; \
        cp -P $(LIB_DIR)/*.so* $(BIN_DIR)/ 2>/dev/null || true; \
        if [ "$(BUILD)" = "release" ]; then \
            echo "$(BLUE)[STRIP] Stripping binaries in bin/...$(NC)"; \
            $(STRIP) $(BIN_DIR)/*.so* $(BIN_DIR)/$(BIN_NAME) 2>/dev/null || true; \
        fi; \
    fi
endef

DIST_DIR := dist/$(NAME)-$(VERSION)-$(TARGET_OS)-$(TARGET_ARCH)

.PHONY: all lib run debug clean fclean distclean re release info

.DELETE_ON_ERROR:

all: $(TARGET_BIN)

lib: $(TARGET_LIB)

$(OBJ_DIR) $(DEP_DIR) $(BIN_DIR) $(LIB_DIR):
	@mkdir -p $@

$(TARGET_BIN): $(OBJ_MAIN) $(TARGET_LIB) | $(BIN_DIR)
	@echo "$(BLUE)[LD] Linking: $@$(NC)"
	@$(CXX) $(OBJ_MAIN) $(OSFLAG) $(LDFLAGS) -l$(NAME) $(LDLIBS) -o $@
	$(POST_BUILD_STEP)

$(TARGET_LIB): $(OBJS_CORE) | $(LIB_DIR)
	@echo "$(BLUE)[SHARED] Building Dynamic Library: $@$(NC)"
	@$(CXX) -shared $(OBJS_CORE) $(LDFLAGS) $(LDLIBS) -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR) $(DEP_DIR)
	@echo "$(YELLOW)[CC] $<$(NC)"
	@$(CC) $(CCFLAGS) $(DEPFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR) $(DEP_DIR)
	@echo "$(YELLOW)[CXX] $<$(NC)"
	@$(CXX) $(CXXFLAGS) $(DEPFLAGS) -c $< -o $@

-include $(DEPS)

run: all
	@echo "$(GREEN)[RUN] Launching...$(NC)"
	@$(EXEC) $(ARGS)

debug:
	@$(MAKE) --no-print-directory all BUILD=debug
	@gdb $(TARGET_BIN)

clean:
	@rm -rf $(BUILD_DIR)
	@echo "$(RED)[CLEAN] Build objects removed.$(NC)"

fclean: clean
	@rm -rf $(TARGET_BIN) $(TARGET_LIB)
	@echo "$(RED)[FCLEAN] Binaries and libraries removed.$(NC)"

distclean:
	@rm -rf build bin
	@rm -f lib/*/$(LIB_NAME)
	@echo "$(RED)[DISTCLEAN] Entire build environment wiped.$(NC)"

re:
	@$(MAKE) --no-print-directory fclean
	@$(MAKE) --no-print-directory all

release:
	@echo "$(YELLOW)[RELEASE] Starting Universal Release build...$(NC)"
	@$(MAKE) --no-print-directory fclean
	@$(MAKE) --no-print-directory all BUILD=release
	@echo "$(BLUE)[RELEASE] Creating directory structure...$(NC)"
	@mkdir -p $(DIST_DIR)/bin $(DIST_DIR)/lib $(DIST_DIR)/$(INC_DIR)
	@echo "$(BLUE)[RELEASE] Exporting public headers...$(NC)"
	@$(eval EXPORT_HDRS := $(if $(PUBLIC_HEADERS),$(PUBLIC_HEADERS),$(INC_DIR)/*))
	@cp -r $(EXPORT_HDRS) $(DIST_DIR)/$(INC_DIR)/ 2>/dev/null || true
ifeq ($(TARGET_OS),windows)
	@echo "$(BLUE)[RELEASE] Exporting Windows binaries...$(NC)"
	@cp -P $(BIN_DIR)/* $(DIST_DIR)/bin/ 2>/dev/null || true
	@cp -P $(LIB_DIR)/*.dll $(DIST_DIR)/lib/ 2>/dev/null || true
else ifeq ($(TARGET_OS),linux)
	@echo "$(BLUE)[RELEASE] Exporting Linux binaries...$(NC)"
	@cp -P $(BIN_DIR)/* $(DIST_DIR)/bin/ 2>/dev/null || true
	@cp -P $(LIB_DIR)/*.so* $(DIST_DIR)/lib/ 2>/dev/null || true
endif
	@echo "$(BLUE)[RELEASE] Attaching assets and documentation...$(NC)"
	@if [ -d "assets" ]; then cp -r assets $(DIST_DIR)/; fi
	@cp *.md $(DIST_DIR)/ 2>/dev/null || true
	@echo "$(BLUE)[RELEASE] Compressing to ZIP archive...$(NC)"
	@cd dist && zip -ryq $(notdir $(DIST_DIR)).zip $(notdir $(DIST_DIR)) 2>/dev/null || echo "$(YELLOW)[WARNING] 'zip' failed.$(NC)"
	@cd dist && tar -cJf $(notdir $(DIST_DIR)).tar.xz $(notdir $(DIST_DIR)) 2>/dev/null || echo "$(YELLOW)[WARNING] 'tar' failed.$(NC)"
	@echo "$(GREEN)==================================================$(NC)"
	@echo "$(GREEN)[SUCCESS] Release packaged in: $(DIST_DIR)$(NC)"
	@echo "$(GREEN)[SUCCESS] Archive created:     dist/$(notdir $(DIST_DIR)).zip$(NC)"
	@echo "$(GREEN)                               dist/$(notdir $(DIST_DIR)).tar.xz$(NC)"
	@echo "$(GREEN)==================================================$(NC)"

info:
	@echo "$(BLUE)==================================================$(NC)"
	@echo "$(GREEN)                 BUILD CONFIGURATION             $(NC)"
	@echo "$(BLUE)==================================================$(NC)"
	@echo "$(YELLOW)[Project]$(NC)"
	@echo "  Name     : $(NAME)"
	@echo "  Version  : $(VERSION)"
	@echo "  Build    : $(BUILD)"
	@echo ""
	@echo "$(YELLOW)[Environment]$(NC)"
	@echo "  Host OS  : $(HOST_OS) ($(HOST_ARCH))"
	@echo "  Target   : $(TARGET_OS) ($(TARGET_ARCH))"
	@echo ""
	@echo "$(YELLOW)[Toolchain]$(NC)"
	@echo "  C Compiler   : $(CC)"
	@echo "  C++ Compiler : $(CXX)"
	@echo "  Archiver     : $(AR)"
	@echo "  Stripper     : $(STRIP)"
	@echo ""
	@echo "$(YELLOW)[Directories]$(NC)"
	@echo "  Sources  : $(SRC_DIR)/"
	@echo "  Includes : $(INC_DIR)/"
	@echo "  Objects  : $(OBJ_DIR)/"
	@echo "  Library  : $(LIB_DIR)/"
	@echo "  Binary   : $(BIN_DIR)/"
	@echo ""
	@echo "$(YELLOW)[Flags]$(NC)"
	@echo "  CCFLAGS  : $(CCFLAGS)"
	@echo "  CXXFLAGS : $(CXXFLAGS)"
	@echo "  LDFLAGS  : $(LDFLAGS)"
	@echo "  LDLIBS   : $(LDLIBS)"
	@echo "$(BLUE)==================================================$(NC)"

$(OBJS_CORE): CCFLAGS  += -D$(NAME)_BUILD_DLL
$(OBJS_CORE): CXXFLAGS += -D$(NAME)_BUILD_DLL
