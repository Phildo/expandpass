EXE=expandpass
SRC=gen.cpp
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
	lldb -- ./$(EXE) $(ARGS)

$(EXE): $(SRC) seed.txt
	gcc $(NOWARN) $(SRC) -o $(EXE)

password.txt: $(EXE)
	./$(EXE) $(ARGS)

test: $(EXE)
	tests/run.sh

tags:
	ctags ./*

clean:
	if [ -f $(EXE) ];      then rm    $(EXE);      fi
	if [ -d $(EXE).dSYM ]; then rm -r $(EXE).dSYM; fi

replace: $(EXE)
	cp $(EXE) `which $(EXE)`

