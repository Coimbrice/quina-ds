BLOCKSDS	?= /opt/blocksds/core

NAME		:= quina
GAME_TITLE	:= Quina
VERSION		:= 0.2
GAME_SUBTITLE	:= Jogo de palavras
GAME_AUTHOR	:= Coimbrice
GAME_ICON	:= icon.png

INCLUDEDIRS	:= source
SOURCES_C	:= build/words.c build/licenses.c
DEFINES		:= -DGAME_VERSION=\"$(VERSION)\"
AUDIODIRS	:= audio

LIBS		:= -lmm9 -lnds9
LIBDIRS		:= $(BLOCKSDS)/libs/maxmod

include $(BLOCKSDS)/sys/default_makefiles/rom_arm9/Makefile

HOSTCC		:= gcc
HOSTCFLAGS	:= -std=c11 -Wall -Wextra -Isource

build/words.c: tools/wordlist.py data/answers.txt data/allowed.txt
	@echo "  WORDS   $@"
	@$(MKDIR) -p $(@D)
	$(V)python3 tools/wordlist.py data/answers.txt data/allowed.txt $@

LICENSE_FILES	:= LICENSE THIRD_PARTY.txt OFL.txt data/LICENSE

build/licenses.c: tools/licenses.py $(LICENSE_FILES)
	@echo "  LICENSE $@"
	@$(MKDIR) -p $(@D)
	$(V)python3 tools/licenses.py $@ $(LICENSE_FILES)

# The version comes from this file.
$(BUILDDIR)/source/menu.c.o: Makefile

.PHONY: test memcheck format lint

C_FILES		:= $(filter-out source/font_data.c,$(wildcard source/*.c source/*.h tests/*.c))

test: build/words.c
	$(V)$(HOSTCC) $(HOSTCFLAGS) -o build/test_game tests/test_game.c source/game.c
	$(V)$(HOSTCC) $(HOSTCFLAGS) -o build/test_words tests/test_words.c source/words.c build/words.c
	$(V)$(HOSTCC) $(HOSTCFLAGS) -o build/test_daily tests/test_daily.c source/daily.c source/words.c build/words.c
	$(V)$(HOSTCC) $(HOSTCFLAGS) -o build/test_save tests/test_save.c source/save.c
	$(V)build/test_game
	$(V)build/test_words
	$(V)build/test_daily
	$(V)build/test_save

# Runs the tests with the memory and undefined behavior checkers on.
memcheck: HOSTCFLAGS += -g -fsanitize=address,undefined -fno-sanitize-recover=all
memcheck: test

format:
	clang-format -i $(C_FILES)
	ruff format tools

lint:
	clang-format --dry-run -Werror $(C_FILES)
	cppcheck --enable=warning,performance,portability --std=c11 --quiet --error-exitcode=1 \
		--suppress=missingIncludeSystem --suppress='*:source/font_data.c' -Isource \
		'-DALIGN(n)=' '-DGAME_VERSION="$(VERSION)"' source tests
	ruff check tools
	ruff format --check tools
