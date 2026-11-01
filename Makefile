BLOCKSDS	?= /opt/blocksds/core

NAME		:= quina
GAME_TITLE	:= Quina
GAME_SUBTITLE	:= Jogo de palavras
GAME_AUTHOR	:= Coimbrice
GAME_ICON	:= icon.png

INCLUDEDIRS	:= source
SOURCES_C	:= build/words.c build/licenses.c
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

.PHONY: test

test: build/words.c
	$(V)$(HOSTCC) $(HOSTCFLAGS) -o build/test_game tests/test_game.c source/game.c
	$(V)$(HOSTCC) $(HOSTCFLAGS) -o build/test_words tests/test_words.c source/words.c build/words.c
	$(V)$(HOSTCC) $(HOSTCFLAGS) -o build/test_daily tests/test_daily.c source/daily.c source/words.c build/words.c
	$(V)$(HOSTCC) $(HOSTCFLAGS) -o build/test_save tests/test_save.c source/save.c
	$(V)build/test_game
	$(V)build/test_words
	$(V)build/test_daily
	$(V)build/test_save
