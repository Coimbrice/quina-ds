BLOCKSDS	?= /opt/blocksds/core

NAME		:= quina
GAME_TITLE	:= Quina
GAME_SUBTITLE	:= Jogo de palavras
GAME_AUTHOR	:= Coimbrice

include $(BLOCKSDS)/sys/default_makefiles/rom_arm9/Makefile

HOSTCC		:= gcc

.PHONY: test

test:
	@$(MKDIR) -p build
	$(V)$(HOSTCC) -std=c11 -Wall -Wextra -Isource -o build/test_game \
		tests/test_game.c source/game.c
	$(V)build/test_game
