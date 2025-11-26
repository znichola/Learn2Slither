# ============================================================================
#   Native (clang w/ linux)
# ============================================================================

NAME    = snake
LIBNAME = libsnake.a

CC      = clang++
CFLAGS  = -Wall -Wextra
CFLAGS  += -Werror
CFLAGS  += -std=c++20

ifdef DEBUG
CFLAGS  += -g3 -fsanitize=address
else
ifdef DEBUGL
CFLAGS  += -g3
endif
endif

LEAKS_CHECK = valgrind
EXAMPLE_FILE = example_file.txt

FILES = environment interpreter

MAIN_SRC = src/main.cpp
MAIN_OBJ = obj/main.o

OBJS_PATH = obj/
SRCS_PATH = src/
INCS_PATH = include

SRCS = $(addprefix $(SRCS_PATH), $(addsuffix .cpp, $(FILES)))
OBJS = $(addprefix $(OBJS_PATH), $(addsuffix .o, $(FILES)))

GIT_COMMIT := $(shell git rev-parse --short HEAD 2>/dev/null)
BUILD_DATE := $(shell date -u +"%Y-%m-%d %H:%M:%S UTC")
BUILD_INFO := -DGIT_COMMIT="$(GIT_COMMIT)" -DBUILD_DATE="$(BUILD_DATE)"

all: $(NAME)

$(OBJS_PATH)%.o: $(SRCS_PATH)%.cpp
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $(BUILD_INFO) -c -I$(INCS_PATH) $(GV_INCS) -o $@ $<

$(NAME): $(OBJS) $(MAIN_OBJ)
	$(CC) $(CFLAGS) $(BUILD_INFO) $(OBJS) $(MAIN_OBJ) $(GV_LIBS) -o $@

clean:
	-rm $(OBJS) $(MAIN_OBJ)

fclean: clean wasm-clean
	-rm $(NAME) $(LIBNAME)

re: fclean all

leaks: re
	$(LEAKS_CHECK) ./$(NAME) $(EXAMPLE_FILE)

run: all
	$(LEAKS_CHECK) ./$(NAME) $(EXAMPLE_FILE)

# Static library
$(LIBNAME): $(OBJS)
	ar rcs $(LIBNAME) $(OBJS)


.PHONY: all clean fclean re leaks run

# ============================================================================
#   WebAssembly Build (emscripten w/ docker)
# ============================================================================

EMCC_DOCKER_IMG = emscripten/emsdk

wasm:
	docker run --rm -v $(PWD):/app -w /app $(EMCC_DOCKER_IMG) \
		em++ -Wall -Wextra -Werror -std=c++20 \
		-s WASM=1 \
		-s MODULARIZE=1 \
		-s EXPORT_ES6=1 \
		-s ENVIRONMENT=web \
		-s ALLOW_MEMORY_GROWTH=1 \
		-s EXPORTED_RUNTIME_METHODS=['ccall','cwrap','callMain'] \
		$(BUILD_INFO) \
		-I$(INCS_PATH) $(SRCS) $(MAIN_SRC) \
		-o snake.js

wasm-clean:
	rm -f snake.js snake.wasm

wasm-re: wasm-clean wasm

# Serve WASM at http://localhost:8080
wasm-serve:
	docker run --rm -p 8080:8080 -v $(PWD):/app -w /app $(EMCC_DOCKER_IMG) \
		emrun --no_browser --port 8080 .

wasm-shell:
	docker run -it --rm -v $(PWD):/app -w /app $(EMCC_DOCKER_IMG) bash

PHONY: wasm wasm-clean wasm-re wasm-serve wasm-shell

