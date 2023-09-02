NAME = vulkan_triangle
PKGS = vulkan glfw3 shaderc
SRCS = $(wildcard *.cpp)
HDRS = $(wildcard *.hpp)
OBJS = $(SRCS:.cpp=.o)

# Compiler settings
CFLAGS += -DGLFW_INCLUDE_NONE -std=c++17

# Build in release mode by default
ifeq ($(BUILD_MODE),debug)
	CFLAGS += -g -DENABLE_VALIDATION
else
	CFLAGS += -Ofast -DNDEBUG
endif

# Dependencies via pkg-config
CFLAGS += $(shell pkg-config $(PKGS) --cflags)
LIBS += $(shell pkg-config $(PKGS) --libs)

# Dependencies via submodules
CFLAGS += -I external/glfwpp/include -I external/glm

# Rule for building all components
all: $(NAME)

# Rule for cleaning build artifacts and results
clean:
	find . -type f -name '*.o' -delete
	rm -f pch.hpp.gch vulkan_triangle

# Rule for building the $(NAME) executable
$(NAME): pch.hpp.gch $(OBJS)
	$(CXX) -o $(NAME) $(OBJS) $(LIBS)

# Rule for building object files
%.o: %.cpp $(HDRS) pch.hpp.gch
	$(CXX) $(CFLAGS) -include pch.hpp -o $@ -c $<

# Rule for building precompiled header
pch.hpp.gch:
	$(CXX) $(CFLAGS) -o pch.hpp.gch pch.hpp

.PHONY: all clean
