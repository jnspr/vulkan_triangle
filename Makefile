NAME = vulkan_triangle
PKGS = vulkan glfw3
SRCS = $(wildcard *.cpp)
HDRS = $(wildcard *.hpp)
OBJS = $(SRCS:.cpp=.o)

# Compiler settings
CFLAGS += -Ofast -DGLFW_INCLUDE_NONE

# Dependencies via pkg-config
CFLAGS += $(shell pkg-config $(PKGS) --cflags)
LIBS += $(shell pkg-config $(PKGS) --libs)

# Dependencies via submodules
CFLAGS += -I external/glfwpp/include

# Rule for building all components
all: $(NAME)

# Rule for cleaning build artifacts and results
clean:
	find . -type f -name '*.o' -delete
	rm -f vulkan_triangle

# Rule for building the $(NAME) executable
$(NAME): $(OBJS)
	$(CXX) -o $(NAME) $(OBJS) $(LIBS)

# Rule for building object files
%.o: %.cpp $(HDRS)
	$(CXX) $(CFLAGS) -o $@ -c $<

.PHONY: all clean
