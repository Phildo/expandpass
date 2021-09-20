EXE=expandpass
CSRC=gen.cpp
SRCDIR=src
DEP=$(SRCDIR)/util.cpp $(SRCDIR)/util.cpp $(SRCDIR)/expansion.cpp $(SRCDIR)/parse.cpp $(SRCDIR)/expand.cpp $(SRCDIR)/validate.cpp $(SRCDIR)/expandpass.cpp $(CSRC)
NOWARN=-Wno-write-strings
CFLAGS=$(NOWARN)# -O3
ARGS=

make: $(EXE)
	

run: password.txt
	

frun:
	./$(EXE) $(ARGS)

$(EXE).dSYM: $(DEP)
	gcc $(CFLAGS) -ggdb3 $(CSRC) -o $(EXE)

builddebug: $(EXE).dSYM
	

debug: $(EXE).dSYM
	lldb -- ./$(EXE) $(ARGS)
#	gdb --args ./$(EXE) $(ARGS)

$(EXE): $(DEP) seed.txt
	gcc $(CFLAGS) $(CSRC) -o $(EXE)

password.txt: $(EXE)
	./$(EXE) $(ARGS)

test: $(EXE)
	tests/run.sh

tags: $(DEP)
	ctags ./*

clean:
	if [ -f $(EXE) ];      then rm    $(EXE);      fi
	if [ -d $(EXE).dSYM ]; then rm -r $(EXE).dSYM; fi

replace: $(EXE)
	cp $(EXE) `which $(EXE)`

