SIM := $(wildcard src/sim/*.c)
TESTS := $(wildcard tests/test_*.c)
BINS := $(TESTS:tests/%.c=tests/bin/%)
CFLAGS := -std=c11 -Wall -Wextra -g -Isrc -fsanitize=address,undefined -fno-sanitize-recover=all

.PHONY: test
test: $(BINS)
	@for b in $(BINS); do echo "== $$b"; $$b || exit 1; done
	@echo ALL PASS

tests/bin/%: tests/%.c $(SIM) $(wildcard src/sim/*.h) tests/t.h
	@mkdir -p tests/bin
	clang $(CFLAGS) -o $@ $< $(SIM)
