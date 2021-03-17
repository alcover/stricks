CC = gcc
FLAGS = -std=c11 -Wall -g
OPTIM = -O3
COMP = $(CC) $(FLAGS) $(OPTIM) -lm -c $< -o $@
LINK = $(CC) $(FLAGS)  $^ -o $@

lib  = bin/libstx
unit = bin/unit
test = bin/test
example = example/forum
bench = bin/bench
sds = bin/sds

.PHONY: all check clean

all: $(lib) $(unit) $(test) $(bench)

$(lib): src/stx.c src/stx.h src/util.c
	$(COMP)
	
$(unit): src/unit.c $(lib)
	$(LINK)
	@ ./$(unit)

$(example): example/forum.c $(lib)
	$(LINK)

$(test): src/test.c $(lib)
	$(LINK)

$(bench): src/bench.c $(lib) $(sds)
	$(CC) -std=c11 -Wall -lm $^ -o $@

$(sds): sds/sds.c sds/sds.h
	$(CC) -std=c99 -Wall $(OPTIM) -c $< -o $@ -O2

check:
	@ ./$(unit)

clean:
	@ rm -f bin/* $(example)