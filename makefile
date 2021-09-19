EXE=expandpass
SRCDIR=src
SRC=gen.cpp
NOWARN=-Wno-write-strings
CFLAGS=$(NOWARN) -O3
ARGS=

make: $(EXE)
	

run: password.txt
	

frun:
	./$(EXE) $(ARGS)

$(EXE).dSYM:
	gcc $(CFLAGS) -ggdb3 $(SRC) -o $(EXE)

builddebug: $(EXE).dSYM
	

debug: $(EXE).dSYM
#	lldb -- ./$(EXE) $(ARGS)
	gdb --args ./$(EXE) $(ARGS)

$(EXE): $(SRC) seed.txt
	gcc $(CFLAGS) $(SRC) -o $(EXE)

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

