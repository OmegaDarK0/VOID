NAME    := VOID
VERSION := 0.1.0-beta

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
    BUILD_OS := windows
else ifneq (,$(filter linux Linux,$(INPUT_OS)))
    BUILD_OS := linux
else ifneq (,$(filter mac macos darwin Darwin,$(INPUT_OS)))
    $(error "Darwin build is not yet implemented.")
else
    BUILD_OS := $(INPUT_OS)
endif

ifneq (,$(filter x64 amd64 x86_64,$(INPUT_ARCH)))
    BUILD_ARCH := x86_64
else ifneq (,$(filter x32 x86 i386 i686,$(INPUT_ARCH)))
    BUILD_ARCH := x86
else ifneq (,$(filter arm arm64 aarch64,$(INPUT_ARCH)))
    $(error "arm64 build is not yet implemented.")
else
    BUILD_ARCH := $(INPUT_ARCH)
endif

CC       := gcc
CXX      := g++
AR       := ar
STRIP    := strip
BIN_EXT  :=
LIB_EXT  :=

MODE       ?= debug
GIT_COMMIT := $(shell git rev-parse --short HEAD 2>/dev/null || echo unknown)
CLEAN_TAG  := $(strip $(TAG))
TAG_SUFFIX := $(if $(CLEAN_TAG),-$(CLEAN_TAG),)

ifeq ($(BUILD_OS),windows)
    BIN_EXT := .exe
    LIB_EXT := .dll

    ifeq ($(strip $(RUNTIME)),)
        ifeq ($(CLEAN_TAG),enhanced)
            RUNTIME := ucrt
        else
            RUNTIME := msvcrt
        endif
    endif

    ifneq ($(HOST_OS),windows)
        ifeq ($(BUILD_ARCH),x86_64)
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
        else ifeq ($(BUILD_ARCH),x86)
            CC    := i686-w64-mingw32-gcc
            CXX   := i686-w64-mingw32-g++
            AR    := i686-w64-mingw32-ar
            STRIP := i686-w64-mingw32-strip
        endif
    endif
else ifeq ($(BUILD_OS),linux)
    BIN_EXT  :=
    LIB_EXT  := .so
endif

ifeq ($(BUILD_ARCH),x86)
    ARCH_FLAGS := -m32
else
    ARCH_FLAGS := -m64
endif

ifeq ($(BUILD_OS),linux)
    BASE_TRIPLET := $(if $(filter x86,$(BUILD_ARCH)),i686-linux-gnu,x86_64-linux-gnu)
    ifneq ($(strip $(GLIBC)),)
        BUILD_ABI := $(strip $(GLIBC))
    else ifeq ($(CLEAN_TAG),legacy)
        BUILD_ABI := 2.17
    else ifeq ($(CLEAN_TAG),enhanced)
        BUILD_ABI := 2.31
    else
        BUILD_ABI := default
    endif
    VENDOR_TRIPLET := $(if $(filter default,$(BUILD_ABI)),$(BASE_TRIPLET),$(BASE_TRIPLET).$(BUILD_ABI))
else ifeq ($(BUILD_OS),windows)
    BUILD_ABI := $(if $(filter x86,$(BUILD_ARCH)),msvcrt,$(RUNTIME))
    ifeq ($(BUILD_ARCH),x86)
        VENDOR_TRIPLET := i686-w64-mingw32
    else ifeq ($(BUILD_ABI),ucrt)
        VENDOR_TRIPLET := x86_64-w64-mingw32ucrt
    else
        VENDOR_TRIPLET := x86_64-w64-mingw32
    endif
endif

SRC_DIR    := src
INC_DIR    := include
VENDOR_DIR := vendor
VENDOR_INC := $(VENDOR_DIR)/include
VENDOR_LIB := $(VENDOR_DIR)/lib/$(VENDOR_TRIPLET)

BUILD_DIR  := build/$(BUILD_OS)-$(BUILD_ARCH)$(TAG_SUFFIX)
BIN_DIR    := $(BUILD_DIR)/bin
LIB_DIR    := $(BUILD_DIR)/lib
OBJ_DIR    := $(BUILD_DIR)/obj-$(MODE)
DEP_DIR    := $(BUILD_DIR)/dep-$(MODE)
MODE_STAMP := $(BUILD_DIR)/.mode-$(MODE)-$(BUILD_ABI)

DIST_NAME  := $(NAME)-$(VERSION)-$(BUILD_OS)-$(BUILD_ARCH)$(TAG_SUFFIX)
DIST_DIR   := dist/$(DIST_NAME)

BIN_NAME   := $(NAME)$(BIN_EXT)
LIB_NAME   := lib$(NAME)$(LIB_EXT)
BUILD_BIN  := $(BIN_DIR)/$(BIN_NAME)

STUB_NAME  := $(NAME)_Launcher$(BIN_EXT)
BUILD_STUB := $(BUILD_DIR)/$(STUB_NAME)

ifeq ($(BUILD_OS),windows)
    BUILD_LIB   := $(BIN_DIR)/$(LIB_NAME)
    IMPLIB_FLAG := -Wl,--out-implib,$(LIB_DIR)/lib$(NAME).dll.a
else
    BUILD_LIB   := $(LIB_DIR)/$(LIB_NAME)
    IMPLIB_FLAG :=
endif

CFLAGS  := $(ARCH_FLAGS) -Wall -Wextra -Wformat=2 -Werror=format-security \
           -I$(INC_DIR) -I$(VENDOR_INC) \
           -DNAME=\"$(NAME)\" -DVERSION=\"$(VERSION)\" \
           -DMODE=\"$(MODE)\" -DGIT_COMMIT=\"$(GIT_COMMIT)\" $(CFLAGS)
LDFLAGS := $(ARCH_FLAGS) -static-libgcc -static-libstdc++ -L$(LIB_DIR) -L$(VENDOR_LIB) $(LDFLAGS)
DEPFLAGS = -MT $@ -MMD -MP -MF $(DEP_DIR)/$*.d

ifeq ($(BUILD_OS),windows)
    CFLAGS  += -D__USE_MINGW_ANSI_STDIO=1
    LDFLAGS += -Wl,--dynamicbase -Wl,--nxcompat
    LDLIBS  += -lbcrypt -Wl,-Bstatic -lwinpthread -Wl,-Bdynamic
else ifeq ($(BUILD_OS),linux)
    CFLAGS  += -fvisibility=hidden -fPIC -D_GNU_SOURCE
    LDFLAGS += -Wl,--disable-new-dtags -Wl,-rpath='$$ORIGIN:$$ORIGIN/../lib'
    LDLIBS  += -lm -ldl -pthread
    OSFLAG  := -pie
endif

LDLIBS += -lSDL2 -lSDL2_image -lSDL2_mixer

ifeq ($(MODE),debug)
    CFLAGS  += -Og -g
    ifeq ($(BUILD_OS),windows)
        OSFLAG := -mconsole
    endif
else
    CFLAGS  += -O2 -g -fstack-protector-strong -fstack-clash-protection -fcf-protection -D_FORTIFY_SOURCE=2 -DNDEBUG
    ifeq ($(BUILD_OS),windows)
        OSFLAG := -mwindows
    else ifeq ($(BUILD_OS),linux)
        LDFLAGS += -Wl,-z,relro,-z,now
    endif
endif

CCFLAGS  := $(CFLAGS) -std=c17
CXXFLAGS := $(CFLAGS) -std=c++20

SRC_MAIN := $(wildcard $(SRC_DIR)/main.c) $(wildcard $(SRC_DIR)/main.cpp)
SRC_STUB := $(wildcard $(SRC_DIR)/launcher.c)

OBJ_MAIN := $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(filter %.c, $(SRC_MAIN))) \
            $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(filter %.cpp, $(SRC_MAIN)))
OBJ_STUB := $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRC_STUB))

SRCS_C   := $(filter-out $(SRC_MAIN) $(SRC_STUB), $(shell find $(SRC_DIR) -type f -name "*.c"))
SRCS_CPP := $(filter-out $(SRC_MAIN), $(shell find $(SRC_DIR) -type f -name "*.cpp"))

OBJS_CORE := $(SRCS_C:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o) \
             $(SRCS_CPP:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)

DEPS     := $(OBJS_CORE:$(OBJ_DIR)/%.o=$(DEP_DIR)/%.d) \
            $(OBJ_MAIN:$(OBJ_DIR)/%.o=$(DEP_DIR)/%.d) \
            $(OBJ_STUB:$(OBJ_DIR)/%.o=$(DEP_DIR)/%.d)

EXEC := $(BUILD_BIN)

ifeq ($(BUILD_OS),windows)
    ifneq ($(HOST_OS),windows)
        EXEC := WINEDEBUG=-all wine $(BUILD_BIN)
    endif
endif

define SYNC_VENDOR
    @if [ -d "$(VENDOR_LIB)" ]; then \
        if [ "$(BUILD_OS)" = "windows" ]; then \
            cp -u $(VENDOR_LIB)/*.dll $(BIN_DIR)/ 2>/dev/null || true; \
        elif [ "$(BUILD_OS)" = "linux" ]; then \
            cp -u -P $(VENDOR_LIB)/*.so* $(LIB_DIR)/ 2>/dev/null || true; \
        fi; \
    fi
endef

.PHONY: all lib stub run debug clean fclean distclean purge re prerelease release info

all: $(BUILD_BIN) $(BUILD_STUB)

lib: $(BUILD_LIB)

stub: $(BUILD_STUB)

$(BUILD_DIR) $(OBJ_DIR) $(DEP_DIR) $(BIN_DIR) $(LIB_DIR):
	@mkdir -p $@

$(MODE_STAMP): | $(BUILD_DIR)
	@rm -f $(BUILD_DIR)/.mode-* $(LIB_DIR)/*.so* $(BIN_DIR)/*.dll
	@touch $@

$(BUILD_BIN): $(OBJ_MAIN) $(BUILD_LIB) $(MODE_STAMP) | $(BIN_DIR)
	@echo "$(BLUE)[LD] Linking: $@$(NC)"
	@$(CXX) $(OBJ_MAIN) $(OSFLAG) $(LDFLAGS) -l$(NAME) $(LDLIBS) -o $@

$(BUILD_LIB): $(OBJS_CORE) $(MODE_STAMP) | $(LIB_DIR) $(BIN_DIR)
	@echo "$(BLUE)[SHARED] Building Dynamic Library: $@$(NC)"
	@$(CXX) -shared $(OBJS_CORE) $(IMPLIB_FLAG) $(LDFLAGS) $(LDLIBS) -o $@
	$(SYNC_VENDOR)

$(BUILD_STUB): $(OBJ_STUB) $(MODE_STAMP) | $(BUILD_DIR)
	@echo "$(BLUE)[LD] Linking Stub: $@$(NC)"
	@$(CC) $(OBJ_STUB) $(OSFLAG) $(LDFLAGS) -o $@

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
	@$(MAKE) --no-print-directory all MODE=debug
	@gdb $(BUILD_BIN)

clean:
	@rm -rf $(OBJ_DIR) $(DEP_DIR) $(MODE_STAMP)
	@echo "$(RED)[CLEAN] Removed objects and dependencies for $(BUILD_OS)-$(BUILD_ARCH)$(TAG_SUFFIX) [$(MODE)].$(NC)"

fclean:
	@rm -rf $(BUILD_DIR)
	@echo "$(RED)[FCLEAN] Wiped entire build tree for $(BUILD_OS)-$(BUILD_ARCH)$(TAG_SUFFIX).$(NC)"

distclean:
	@rm -rf dist
	@echo "$(RED)[DISTCLEAN] Wiped all release packages and archives.$(NC)"

purge:
	@rm -rf build dist
	@echo "$(RED)[PURGE] Wiped all build targets and release archives.$(NC)"

re:
	@$(MAKE) --no-print-directory fclean
	@$(MAKE) --no-print-directory all

prerelease:
	@echo "$(YELLOW)[PRERELEASE] Building package ($(DIST_NAME))...$(NC)"
	@$(MAKE) --no-print-directory all MODE=release
	@echo "$(BLUE)[PRERELEASE] Creating directory structure...$(NC)"
	@rm -rf $(DIST_DIR)
	@mkdir -p $(DIST_DIR)/bin $(DIST_DIR)/lib $(DIST_DIR)/$(INC_DIR)
	@echo "$(BLUE)[PRERELEASE] Exporting public headers...$(NC)"
	@cp -r $(INC_DIR)/* $(DIST_DIR)/$(INC_DIR)/ 2>/dev/null || true
ifeq ($(BUILD_OS),windows)
	@echo "$(BLUE)[PRERELEASE] Exporting Windows binaries...$(NC)"
	@cp -P $(BUILD_STUB) $(DIST_DIR)/ 2>/dev/null || true
	@cp -P $(BUILD_BIN) $(DIST_DIR)/bin/ 2>/dev/null || true
	@cp -P $(BIN_DIR)/*.dll $(DIST_DIR)/bin/ 2>/dev/null || true
	@cp -P $(LIB_DIR)/lib$(NAME).dll.a $(DIST_DIR)/lib/ 2>/dev/null || true
else ifeq ($(BUILD_OS),linux)
	@echo "$(BLUE)[PRERELEASE] Exporting Linux binaries...$(NC)"
	@cp -P $(BUILD_STUB) $(DIST_DIR)/ 2>/dev/null || true
	@cp -P $(BUILD_BIN) $(DIST_DIR)/bin/ 2>/dev/null || true
	@cp -P $(LIB_DIR)/*.so* $(DIST_DIR)/lib/ 2>/dev/null || true
endif
	@echo "$(BLUE)[PRERELEASE] Attaching assets and documentation...$(NC)"
	@if [ -d "assets" ]; then cp -r assets $(DIST_DIR)/; fi
	@cp *.md $(DIST_DIR)/ 2>/dev/null || true
	@echo "$(GREEN)[SUCCESS] Prerelease ready in: $(DIST_DIR)$(NC)"

release: prerelease
	@echo "$(YELLOW)[RELEASE] Finalizing release ($(DIST_NAME))...$(NC)"
ifeq ($(BUILD_OS),windows)
	@echo "$(BLUE)[RELEASE] Stripping Windows binaries...$(NC)"
	@$(STRIP) $(DIST_DIR)/$(STUB_NAME) $(DIST_DIR)/bin/*.exe $(DIST_DIR)/bin/*.dll 2>/dev/null || true
else ifeq ($(BUILD_OS),linux)
	@echo "$(BLUE)[RELEASE] Stripping Linux binaries...$(NC)"
	@$(STRIP) $(DIST_DIR)/$(STUB_NAME) $(DIST_DIR)/bin/$(BIN_NAME) $(DIST_DIR)/lib/*.so* 2>/dev/null || true
endif
	@echo "$(BLUE)[RELEASE] Compressing archives...$(NC)"
	@cd dist && zip -ryq $(DIST_NAME).zip $(DIST_NAME) 2>/dev/null || echo "$(YELLOW)[WARNING] 'zip' failed.$(NC)"
	@cd dist && tar -czf $(DIST_NAME).tar.gz $(DIST_NAME) 2>/dev/null || echo "$(YELLOW)[WARNING] 'tar' failed.$(NC)"
	@echo "$(GREEN)==================================================$(NC)"
	@echo "$(GREEN)[SUCCESS] Release packaged in: $(DIST_DIR)$(NC)"
	@echo "$(GREEN)[SUCCESS] Archives created:    dist/$(DIST_NAME).zip$(NC)"
	@echo "$(GREEN)                               dist/$(DIST_NAME).tar.gz$(NC)"
	@echo "$(GREEN)==================================================$(NC)"

info:
	@echo "$(BLUE)==================================================$(NC)"
	@echo "$(GREEN)                 BUILD CONFIGURATION             $(NC)"
	@echo "$(BLUE)==================================================$(NC)"
	@echo "$(YELLOW)[Project]$(NC)"
	@echo "  Name       : $(NAME)"
	@echo "  Version    : $(VERSION) ($(GIT_COMMIT))"
	@echo "  Mode       : $(MODE)"
	@echo "  Tag        : $(if $(CLEAN_TAG),$(CLEAN_TAG),none)"
	@echo "  Dist       : $(DIST_DIR)"
	@echo ""
	@echo "$(YELLOW)[Environment]$(NC)"
	@echo "  Host OS    : $(HOST_OS) ($(HOST_ARCH))"
	@echo "  Build OS   : $(BUILD_OS) ($(BUILD_ARCH))"
	@echo ""
	@echo "$(YELLOW)[Toolchain]$(NC)"
	@echo "  C Compiler   : $(CC)"
	@echo "  C++ Compiler : $(CXX)"
	@echo "  Archiver     : $(AR)"
	@echo "  Stripper     : $(STRIP)"
	@echo ""
	@echo "$(YELLOW)[Directories]$(NC)"
	@echo "  Sources     : $(SRC_DIR)/"
	@echo "  Includes    : $(INC_DIR)/"
	@echo "  Library     : $(LIB_DIR)/"
	@echo "  Binary      : $(BIN_DIR)/"
	@echo ""
	@echo "$(YELLOW)[Flags]$(NC)"
	@echo "  CCFLAGS  : $(CCFLAGS)"
	@echo "  CXXFLAGS : $(CXXFLAGS)"
	@echo "  LDFLAGS  : $(LDFLAGS)"
	@echo "  LDLIBS   : $(LDLIBS)"
	@echo "  OSFLAG   : $(OSFLAG)"
	@echo "$(BLUE)==================================================$(NC)"

$(OBJS_CORE): CCFLAGS  += -D$(NAME)_BUILD_DLL
$(OBJS_CORE): CXXFLAGS += -D$(NAME)_BUILD_DLL
