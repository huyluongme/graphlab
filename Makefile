# GraphLab Makefile
# Target Platforms: Windows (MinGW/MSYS2) & Linux (GCC)

EXE = graphlab
BUILD_DIR = build
OBJ_DIR = $(BUILD_DIR)/objs
BIN_DIR = $(BUILD_DIR)/bin

IMGUI_DIR = thirdparty/imgui
GLFW_DIR = thirdparty/glfw

SOURCES = src/main.cpp \
          src/core/app.cpp \
          src/ui/main_menu_bar.cpp \
          src/ui/sidebar_panel.cpp \
          src/ui/graph_canvas.cpp \
          src/math/evaluator.cpp \
          src/math/expression.cpp \
          src/math/analysis.cpp \
          src/utils/file_dialog.cpp \
          src/utils/project_serializer.cpp

SOURCES += $(IMGUI_DIR)/imgui.cpp \
           $(IMGUI_DIR)/imgui_demo.cpp \
           $(IMGUI_DIR)/imgui_draw.cpp \
           $(IMGUI_DIR)/imgui_tables.cpp \
           $(IMGUI_DIR)/imgui_widgets.cpp \
           $(IMGUI_DIR)/backends/imgui_impl_glfw.cpp \
           $(IMGUI_DIR)/backends/imgui_impl_opengl3.cpp

# Generate object file paths inside build/objs/
OBJS = $(addprefix $(OBJ_DIR)/, $(addsuffix .o, $(basename $(notdir $(SOURCES)))))

UNAME_S := $(shell uname -s 2>/dev/null || echo Windows_NT)

CXXFLAGS = -std=c++17 -Wall -Wformat
CXXFLAGS += -Isrc -I$(IMGUI_DIR) -I$(IMGUI_DIR)/backends -I$(GLFW_DIR)/include -Ithirdparty/stb

# Detect Windows Environment (MSYS2, MinGW, Cygwin, Native Windows)
IS_WINDOWS = 0
ifeq ($(OS), Windows_NT)
    IS_WINDOWS = 1
else ifneq ($(findstring MSYS,$(UNAME_S)),)
    IS_WINDOWS = 1
else ifneq ($(findstring MINGW,$(UNAME_S)),)
    IS_WINDOWS = 1
else ifneq ($(findstring CYGWIN,$(UNAME_S)),)
    IS_WINDOWS = 1
endif

##---------------------------------------------------------------------
## PLATFORM SPECIFIC CONFIGURATION (WINDOWS / LINUX)
##---------------------------------------------------------------------

ifeq ($(IS_WINDOWS), 1)
    ECHO_MESSAGE = Windows (MinGW/MSYS2)
    LIBS = $(GLFW_DIR)/win64/libglfw3.a -lopengl32 -lgdi32 -limm32 -lcomdlg32
    ifneq ($(DEBUG), 1)
        LIBS += -mwindows
    endif
    TARGET = $(BIN_DIR)/$(EXE).exe
    RES_OBJ = $(OBJ_DIR)/resource.o
else
    ECHO_MESSAGE = Linux
    LIBS = -lglfw -lGL -ldl -lpthread
    TARGET = $(BIN_DIR)/$(EXE)
    RES_OBJ =
endif

##---------------------------------------------------------------------
## BUILD RULES
##---------------------------------------------------------------------

all: $(TARGET)
	@echo 'Build complete for $(ECHO_MESSAGE)'
	@echo 'Output binary: $(TARGET)'

# Compile source files from src/ and subdirectories
$(OBJ_DIR)/%.o: src/%.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(OBJ_DIR)/%.o: src/core/%.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(OBJ_DIR)/%.o: src/ui/%.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(OBJ_DIR)/%.o: src/math/%.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(OBJ_DIR)/%.o: src/utils/%.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(OBJ_DIR)/%.o: $(IMGUI_DIR)/%.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(OBJ_DIR)/%.o: $(IMGUI_DIR)/backends/%.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

# Compile Windows Resource Script (Icon)
$(OBJ_DIR)/resource.o: assets/resource.rc | $(OBJ_DIR)
	windres $< -o $@

# Create Directories
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

# Link Executable
$(TARGET): $(OBJS) $(RES_OBJ) | $(BIN_DIR)
	$(CXX) -o $@ $^ $(CXXFLAGS) $(LIBS)

clean:
	rm -rf $(BUILD_DIR)
	rm -f *.o *.exe $(EXE) $(EXE).exe
