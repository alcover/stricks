CC = gcc
FLAGS = -std=c11 -Wall -g
COMP = $(CC) $(FLAGS) -lm -c $< -o $@
LINK = $(CC) $(FLAGS)  $^ -o $@

lib  = bin/libstx
unit = bin/unit
test = bin/test
example = example/forum

.PHONY: all check clean

all: $(lib) $(test) $(unit)

$(lib): src/stx.c src/stx.h src/util.c
	$(COMP)
	
$(unit): src/unit.c $(lib)
	$(LINK)
	@ ./$(unit)

$(example): example/forum.c $(lib)
	$(LINK)

$(test): src/test.c $(lib)
	$(LINK)

check:
	@ ./$(unit)

clean:
	@ rm -f bin/* $(example)