# GraphLab Makefile
# Based on Dear ImGui GLFW + OpenGL3 Example Makefile
# Adapted for GraphLab Project Structure

EXE = graphlab
BUILD_DIR = build
OBJ_DIR = $(BUILD_DIR)/objs
BIN_DIR = $(BUILD_DIR)/bin

IMGUI_DIR = thirdparty/imgui
GLFW_DIR = thirdparty/glfw

SOURCES = src/main.cpp
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
CXXFLAGS += -Isrc -I$(IMGUI_DIR) -I$(IMGUI_DIR)/backends -I$(GLFW_DIR)/include

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
## BUILD FLAGS PER PLATFORM
##---------------------------------------------------------------------

ifeq ($(IS_WINDOWS), 1)
    ECHO_MESSAGE = Windows (MinGW/MSYS2)
    LIBS = $(GLFW_DIR)/win64/libglfw3.a -lopengl32 -lgdi32 -limm32
    TARGET = $(BIN_DIR)/$(EXE).exe
else ifeq ($(UNAME_S), Linux)
    ECHO_MESSAGE = Linux
    LIBS = -lglfw -lGL -ldl -lpthread
    TARGET = $(BIN_DIR)/$(EXE)
else ifeq ($(UNAME_S), Darwin)
    ECHO_MESSAGE = Mac OS X
    LIBS = -lglfw -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo
    TARGET = $(BIN_DIR)/$(EXE)
endif

##---------------------------------------------------------------------
## BUILD RULES
##---------------------------------------------------------------------

all: $(TARGET)
	@echo 'Build complete for $(ECHO_MESSAGE)'
	@echo 'Output binary: $(TARGET)'

# Pattern rules to compile object files into build/objs/
$(OBJ_DIR)/%.o: src/%.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(OBJ_DIR)/%.o: $(IMGUI_DIR)/%.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(OBJ_DIR)/%.o: $(IMGUI_DIR)/backends/%.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

# Create directories
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

# Linking rule
$(TARGET): $(OBJS) | $(BIN_DIR)
	$(CXX) -o $@ $^ $(CXXFLAGS) $(LIBS)

clean:
	rm -rf $(BUILD_DIR)
	rm -f *.o *.exe $(EXE) $(EXE).exe
