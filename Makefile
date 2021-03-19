CC = gcc
FLAGS = -std=c11 -Wall -Wno-unused-function -g
OPTIM = -O0
COMP = $(CC) $(FLAGS) -c $< -o $@
LINK = $(CC) $(FLAGS) $^ -o $@

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
	
$(unit): src/unit.c $(lib) src/util.c
# 	$(LINK)
	$(CC) $(FLAGS) src/unit.c $(lib) -o $@

$(example): example/forum.c $(lib)
	$(LINK)

$(test): src/test.c $(lib)
	$(LINK)

$(bench): src/bench.c $(lib) $(sds)
	$(LINK) -lm

$(sds): sds/sds.c sds/sds.h
	$(CC) -std=c99 -Wall $(OPTIM) -c $< -o $@

check:
	@ ./$(unit)

bench:
	@ ./$(bench)

clean:
	@ rm -f bin/* $(example)