################################
#            Config            #
################################
CFLAGS := -Wall -pedantic -ggdb -O0 -fdata-sections -ffunction-sections
CINCS := `pkg-config --cflags sdl2 SDL2_image`
CLIBS := `pkg-config --libs sdl2 SDL2_image` -lm
ifeq ($(CC),tcc)
	CINCS += -DSDL_DISABLE_IMMINTRIN_H
else
	LDFLAGS := -Wl,--gc-sections
endif
ifeq ($(OS),Windows_NT)
	EXE_EXT := .exe
else
	EXE_EXT :=
endif

################################
#             App              #
################################
APP=minesweeper$(EXE_EXT)

SRC=main.c rng.c data.gen.c
HDR=main.h rng.h data.gen.h

OBJ := $(SRC:.c=.o)

$(APP): $(OBJ)
	$(CC) -o $@ $^ $(CLIBS) $(CFLAGS) $(LDFLAGS)

%.o: %.c $(HDR)
	$(CC) -c -o $@ $< $(CINCS) $(CFLAGS)

run: $(APP)
	./$<

analyze:
	$(MAKE) clean
	scan-build --exclude ./Nuklear/ $(MAKE)

debug: $(APP)
	env \
		DEBUGINFOD_URLS= \
		gdb -ex='set confirm on' -ex run -ex quit --args ./$<

memcheck: $(APP)
	valgrind --tool=memcheck ./$<

memcheck_full: $(APP)
	valgrind --tool=memcheck --leak-check=full ./$<

RESOURCES := tile_closed.png tile_open.png mine.png mine_flagged.png flag.png numbers.png game_over.png victory.png difficulties.png

data.gen.c data.gen.h: tools/embed $(addprefix resources/,$(RESOURCES))
	rm -f data.gen.c data.gen.h
	@printf "/* AUTOMATICALLY GENERATED SOURCE FILE */\n\n" >> data.gen.c
	@printf "/* AUTOMATICALLY GENERATED HEADER FILE */\n\n" >> data.gen.h
	@printf "#ifndef __DATA_GEN_H__\n" >> data.gen.h
	@printf "#define __DATA_GEN_H__\n\n" >> data.gen.h
	for i in $(RESOURCES); do \
		./tools/embed -ndata_$$i -cdata.gen.c -hdata.gen.h -t$$i resources/$$i; \
	done
	@printf "\n#endif /* __DATA_GEN_H__ */" >> data.gen.h

################################
#            Tools             #
################################
TOOLS := embed

_TOOLS := $(addsuffix $(EXE_EXT),$(addprefix tools/,$(TOOLS)))

tools: $(_TOOLS)

tools/%$(EXE_EXT): tools/%.c
	$(CC) -o $@ $< $(CFLAGS) $(LDFLAGS)

################################
#           Testing            #
################################
define test_single
if ./$(1); then \
	echo "Passed test $(1)."; \
else \
	echo "Failed test $(1)!"; \
	exit 1; \
fi
endef

define test_single_leaks
printf "Checking for leaks in $(1)..."; \
if valgrind -q --error-exitcode=1 --leak-check=full --show-leak-kinds=all --track-origins=yes ./$(1); then \
	echo "OK"; \
else \
	echo "Valgrind reported one or more errors!"; \
	exit 1; \
fi
endef

TESTS :=

_TESTS := $(addsuffix $(EXE_EXT),$(addprefix tests/,$(TESTS)))

.PHONY: test test_leaks test_full test_debug

test: $(_TESTS)
	@for i in $(_TESTS); do $(call test_single,$$i); done
	@echo "All tests passed."

test_leaks: $(_TESTS)
	@for i in $(_TESTS); do $(call test_single_leaks,$$i); done
	@echo "All tests passed."

test_full: clean $(_TESTS)
	@for i in $(_TESTS); do $(call test_single,$$i); done
	@for i in $(_TESTS); do $(call test_single_leaks,$$i); done
	@echo "All tests passed."

test_debug: tests/$(TEST_TO_DEBUG)
ifdef TEST_TO_DEBUG
	gdb -q --args ./tests/$(TEST_TO_DEBUG)
else
	@echo "Please run with TEST_TO_DEBUG=<test>."
	@echo "Options are: $(TESTS)"
endif

tests/%$(EXE_EXT): tests/%.c $(HDR)
	$(CC) -o $@ $< $(CINCS) $(CLIBS) $(CFLAGS) $(LDFLAGS)

################################
#           General            #
################################
.PHONY: clean format

format:
	clang-format --style=WebKit -i \
		$(filter-out data.gen.c,$(SRC)) \
		$(filter-out data.gen.h,$(HDR)) \
		$(addprefix tools/,$(addsuffix .c,$(TOOLS))) \
		$(addprefix tests/,$(addsuffix .c,$(TESTS)))

clean:
	rm -f $(OBJ) $(APP) $(_TESTS) $(_TOOLS) data.gen.c data.gen.h
