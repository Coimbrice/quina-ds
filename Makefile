BLOCKSDS	?= /opt/blocksds/core

NAME		:= quina
GAME_TITLE	:= Quina
GAME_SUBTITLE	:= Jogo de palavras
GAME_AUTHOR	:= Coimbrice

INCLUDEDIRS	:= source
SOURCES_C	:= build/words.c

include $(BLOCKSDS)/sys/default_makefiles/rom_arm9/Makefile

HOSTCC		:= gcc
HOSTCFLAGS	:= -std=c11 -Wall -Wextra -Isource

build/words.c: tools/wordlist.py data/answers.txt data/allowed.txt
	@echo "  WORDS   $@"
	@$(MKDIR) -p $(@D)
	$(V)python3 tools/wordlist.py data/answers.txt data/allowed.txt $@

.PHONY: test

test:
	@$(MKDIR) -p build
	$(V)$(HOSTCC) $(HOSTCFLAGS) -o build/test_game tests/test_game.c source/game.c
	$(V)build/test_game
