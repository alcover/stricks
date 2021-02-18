CC = gcc
FLAGS = -std=c99 -Wall -Wno-unused-variable -g -O1
COMP = $(CC) $(FLAGS) -c $< -o $@
LINK = $(CC) $(FLAGS) $^ -o $@

lib = bin/libstx
unit = bin/unit
example = example/forum
test = bin/test

.PHONY: all check clean

all: $(lib) $(unit) $(example) $(test) 

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