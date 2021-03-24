CC = gcc
STD = c11
OPTIM = -O0
WARN = -Wno-pedantic # -Wall -Wno-unused-function -Wno-unused-variable
CP = $(CC) -std=$(STD) $(OPTIM) $(WARN) -g
COMP = $(CP) -c $< -o $@
LINK = $(CP) $^ -o $@

lib		= bin/libstx
unit 	= bin/unit
bench 	= bin/bench
example	= example/forum
sandbox = bin/sandbox
sds 	= bin/sds

.PHONY: all check clean bench

all: $(lib) $(unit) $(bench) $(example) $(sandbox)
	
$(lib): src/stx.c src/stx.h src/util.c
	@ echo $@
	@ $(COMP)
# 	@ ./$(unit)

$(unit): src/unit.c $(lib) src/util.c
	@ echo $@
	@ $(CP) $< $(lib) -o $@
# 	@ ./$(unit)

$(bench): src/bench.c $(lib) $(sds)
	@ echo $@
	@ $(LINK) -lm

$(example): example/forum.c $(lib)
	@ echo $@
	@ $(LINK)

$(sandbox): sandbox.c $(lib)
	@ echo $@
	@ $(LINK)

$(sds): sds/sds.c sds/sds.h
	@ echo $@
	@ $(CC) -std=c99 -Wall $(OPTIM) -c $< -o $@

check:
	@ ./$(unit)

bench:
	@ ./$(bench)

clean:
	@ rm -f bin/* $(example)