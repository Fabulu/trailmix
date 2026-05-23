CXX      := g++
# -I. for test headers; -Inds so that #include <nds.h> finds our stub
CXXFLAGS := -std=c++17 -Wall -Wextra -g -I. -Inds
LDFLAGS  :=

# On Windows the executable extension is .exe
EXE := .exe

TESTS := test_types test_rng test_entities test_pills

.PHONY: all clean run

all: $(addsuffix $(EXE),$(TESTS))

test_types$(EXE): test_types.cpp nds_stub.h nds/nds.h test_framework.h
	$(CXX) $(CXXFLAGS) -o $@ $<

test_rng$(EXE): test_rng.cpp nds_stub.h nds/nds.h test_framework.h
	$(CXX) $(CXXFLAGS) -o $@ $<

test_entities$(EXE): test_entities.cpp nds_stub.h nds/nds.h test_framework.h
	$(CXX) $(CXXFLAGS) -o $@ $<

test_pills$(EXE): test_pills.cpp nds_stub.h nds/nds.h audio_stub.h test_framework.h
	$(CXX) $(CXXFLAGS) -o $@ $<

run: all
	@echo ""; echo "Running all tests..."; echo ""
	@failed=0; \
	for t in $(TESTS); do \
		echo "----------------------------------------"; \
		./$$t$(EXE) || failed=1; \
	done; \
	echo "----------------------------------------"; \
	if [ $$failed -eq 0 ]; then echo "ALL TEST SUITES PASSED"; else echo "SOME TESTS FAILED"; exit 1; fi

clean:
	rm -f $(addsuffix $(EXE),$(TESTS))
