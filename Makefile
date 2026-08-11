# ============================================================================
#   Native (clang /gcc w/ linux)
# ============================================================================

NAME    = snake

CC      = c++
CFLAGS  = -Wall -Wextra
CFLAGS  += -Werror
CFLAGS  += -std=c++20
CFLAGS  += -O3

ifdef DEBUG
CFLAGS  += -g3 -fsanitize=address
else
ifdef DEBUGL
CFLAGS  += -g3
endif
endif

LEAKS_CHECK = valgrind
EXAMPLE_FILE = example_file.txt

FILES = environment interpreter agent trainer bridge configParser agentIO

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

all: elm.js wasm-serve

$(OBJS_PATH)%.o: $(SRCS_PATH)%.cpp
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $(BUILD_INFO) -c -I$(INCS_PATH) $(GV_INCS) -o $@ $<

$(NAME): $(OBJS) $(MAIN_OBJ)
	$(CC) $(CFLAGS) $(BUILD_INFO) $(OBJS) $(MAIN_OBJ) $(GV_LIBS) -o $@

clean:
	-rm $(OBJS) $(MAIN_OBJ)

fclean: clean wasm-clean test-clean elm-clean
	-rm $(NAME)

re: fclean all

leaks: re
	$(LEAKS_CHECK) ./$(NAME) $(EXAMPLE_FILE)

run: all
	$(LEAKS_CHECK) ./$(NAME) $(EXAMPLE_FILE)

.PHONY: all clean fclean re leaks run


# ============================================================================
#   Test Setup
# ============================================================================

TEST_DIR       = test/
TEST_OBJS_PATH = test/$(OBJS_PATH)
TEST_FILES     = environment interpreter trainer

TEST_OBJS = $(addprefix $(TEST_DIR)obj/test-, $(addsuffix .o, $(TEST_FILES)))
TEST_BINS = $(addprefix test-, $(TEST_FILES))

$(TEST_OBJS_PATH)%.o: $(TEST_DIR)%.cpp
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $(BUILD_INFO) -c -I$(INCS_PATH) -o $@ $<

test-interpreter: $(TEST_OBJS_PATH)test-interpreter.o $(OBJS)
	$(CC) $(CFLAGS) $(BUILD_INFO) $(OBJS) $< -o $@

test-environment: $(TEST_OBJS_PATH)test-environment.o $(OBJS)
	$(CC) $(CFLAGS) $(BUILD_INFO) $(OBJS) $< -o $@

test-trainer: $(TEST_OBJS_PATH)test-trainer.o $(OBJS)
	$(CC) $(CFLAGS) $(BUILD_INFO) $(OBJS) $< -o $@

test-clean:
	-rm $(TEST_OBJS)
	-rm $(TEST_BINS)

test: $(TEST_BINS)
	@echo "Running all tests..."
	@fail=0; \
	for t in $(TEST_BINS); do \
		echo "==> Running $$t"; \
		./$$t || fail=1; echo; \
	done; \
	exit $$fail

.PHONY: test-clean test

# ============================================================================
#  Elm app and ui
# ============================================================================

ELM_NAME=elm.js

ELM_FLAGS=--optimize

deps/elm:
	mkdir -p deps && cd deps \
	&& curl -L -o elm.gz https://github.com/elm/compiler/releases/download/0.19.1/binary-for-linux-64-bit.gz \
	&& gzip -d elm.gz \
	&& chmod +x elm

elm.js: elm/src/Main.elm elm/src/App/Config.elm elm/src/App/Board.elm deps/elm
	cd elm && ../deps/elm make $(ELM_FLAGS) src/Main.elm --output=../elm.js

elm-clean:
	-rm elm.js

.PHONY: elm-clean

# ============================================================================
#   WebAssembly Build (emscripten w/ docker)
# ============================================================================

EMCC_DOCKER_IMG = emscripten/emsdk

WASM_NAME=$(NAME).js

$(WASM_NAME) : $(SRCS) $(MAIN_SRC)
	./update_index.sh index.html "$(GIT_COMMIT)" "$(BUILD_DATE)"
	docker run --rm -v $(PWD):/app -w /app $(EMCC_DOCKER_IMG) \
		em++ $(CFLAGS) \
		-s WASM=1 \
		-s MODULARIZE=1 \
		-s EXPORT_ES6=1 \
		-s ENVIRONMENT=web \
		-s ALLOW_MEMORY_GROWTH=1 \
		-s ASYNCIFY \
		-s "ASYNCIFY_IMPORTS=['js_wait_for_input']" \
		-s "EXPORTED_RUNTIME_METHODS=['ccall','cwrap','callMain']" \
		-s "EXPORTED_FUNCTIONS=['_main','_bridge_deliver','_free', 'stringToNewUTF8','lengthBytesUTF8']" \
		-s NO_DISABLE_EXCEPTION_CATCHING \
		$(BUILD_INFO) \
		-I$(INCS_PATH) $^ \
		-o $(WASM_NAME)

wasm-clean:
	rm -f $(WASM_NAME) $(NAME).wasm

wasm-re: wasm-clean $(WASM_NAME)

# Serve WASM at http://localhost:8080
wasm-serve: $(WASM_NAME)
	docker run --rm -p 8080:8080 -v $(PWD):/app -w /app $(EMCC_DOCKER_IMG) \
		emrun --no_browser --port 8080 .

wasm-shell:
	docker run -it --rm -v $(PWD):/app -w /app $(EMCC_DOCKER_IMG) bash

.PHONY: wasm-clean wasm-re wasm-serve wasm-shell

