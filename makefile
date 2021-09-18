EXE=expandpass
SRCDIR=src
SRC=$(SRCDIR)/util.cpp $(SRCDIR)/expansion.cpp $(SRCDIR)/expand.cpp $(SRCDIR)/parse.cpp $(SRCDIR)/expandpass.cpp 
NOWARN=-Wno-write-strings
ARGS=

make: $(EXE)
	

run: password.txt
	

frun:
	./$(EXE) $(ARGS)

$(EXE).dSYM:
	gcc $(NOWARN) -ggdb3 $(SRC) -o $(EXE)

builddebug: $(EXE).dSYM
	

debug: $(EXE).dSYM
#	lldb -- ./$(EXE) $(ARGS)
	gdb --args ./$(EXE) $(ARGS)

$(EXE): $(SRC) seed.txt
	gcc $(NOWARN) $(SRC) -o $(EXE)

password.txt: $(EXE)
	./$(EXE) $(ARGS)

test: $(EXE)
	tests/run.sh

tags: $(SRC)
	ctags ./*

clean:
	if [ -f $(EXE) ];      then rm    $(EXE);      fi
	if [ -d $(EXE).dSYM ]; then rm -r $(EXE).dSYM; fi

replace: $(EXE)
	cp $(EXE) `which $(EXE)`

