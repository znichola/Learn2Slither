NAME	= snake
LIBNAME = libsnake.a

CC		= clang++
CFLAGS	= -Wall -Wextra
CFLAGS	+= -Werror
CFLAGS	+= -std=c++20 #-pedantic
#CFLAGS	+=  -g3 -O0

ifdef DEBUG
CFLAGS	+= -g3 -fsanitize=address
else
ifdef DEBUGL
CFLAGS	+= -g3
endif
endif

LEAKS_CHECK = valgrind

EXAMPLE_FILE = example_file.txt

FILES	= Board

MAIN_SRC	= src/main.cpp
MAIN_OBJ	= obj/main.o

OBJS_PATH = obj/
SRCS_PATH = src/
INCS_PATH = include

SRCS	= $(addprefix $(SRCS_PATH), $(addsuffix .cpp, $(FILES)))
OBJS	= $(addprefix $(OBJS_PATH), $(addsuffix .o, $(FILES)))

GIT_COMMIT	:= $(shell git rev-parse --short HEAD 2>/dev/null)
BUILD_DATE := $(shell date -u +"%Y-%m-%d %H:%M:%S UTC")
BUILD_INFO	:= -DGIT_COMMIT="$(GIT_COMMIT)" -DBUILD_DATE="$(BUILD_DATE)"

all	: $(NAME)

$(OBJS_PATH)%.o: $(SRCS_PATH)%.cpp
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $(BUILD_INFO) -c -I$(INCS_PATH) $(GV_INCS) -o $@ $<

$(NAME)	: $(OBJS) $(MAIN_OBJ)
	$(CC) $(CFLAGS) $(BUILD_INFO) $(OBJS) $(MAIN_OBJ) $(GV_LIBS) -o $@

clean	:
	-rm $(OBJS) $(MAIN_OBJ)

fclean	: clean
	-rm $(NAME) $(LIBNAME)

re	: fclean all

leaks : re
	$(LEAKS_CHECK) ./$(NAME) $(EXAMPLE_FILE)

run : all
	$(LEAKS_CHECK) ./$(NAME) $(EXAMPLE_FILE)

# Rule to archive object files into a static library.
# Useful for testing
$(LIBNAME): $(OBJS)
	ar rcs $(LIBNAME) $(OBJS)

.PHONY: all clean fclean re leaks run
