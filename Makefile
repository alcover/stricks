CC = gcc
FLAGS = -std=c11 -Wall -g
OPTIM = -O0
COMP = $(CC) $(FLAGS) -c $< -o $@
LINK = $(CC) $(FLAGS) -lm $^ -o $@

lib		= bin/libstx
unit 	= bin/unit
test 	= bin/test
bench 	= bin/bench
sds 	= bin/sds
example	= example/forum

.PHONY: all check clean bench

all: $(lib) $(unit) $(test) $(bench)

$(lib): src/stx.c src/stx.h src/util.c
	$(COMP) $(OPTIM)
	
$(unit): src/unit.c $(lib)
	$(LINK)

$(example): example/forum.c $(lib)
	$(LINK)

$(test): src/test.c $(lib)
	$(LINK)

$(bench): src/bench.c $(lib) $(sds)
	$(LINK)

$(sds): sds/sds.c sds/sds.h
	$(CC) -std=c99 -Wall $(OPTIM) -c $< -o $@

check:
	@ ./$(unit)

bench:
	@ ./$(bench)

clean:
	@ rm -f bin/* $(example)