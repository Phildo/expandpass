EXE=expandpass
SRC=gen.cpp
NOWARN=-Wno-write-strings
ARGS=

make: $(EXE)
	

run: password.txt
	

frun:
	./$(EXE) $(ARGS)

debug:
	gcc $(NOWARN) -ggdb3 $(SRC) -o $(EXE) && lldb -- ./$(EXE) $(ARGS)

$(EXE): $(SRC) seed.txt
	gcc $(NOWARN) $(SRC) -o $(EXE)

password.txt: $(EXE)
	./$(EXE) $(ARGS)

test:
	tests/run.sh

tags:
	ctags ./*

clean:
	rm -r $(EXE) $(EXE).dSYM

replace: $(EXE)
	cp $(EXE) `which $(EXE)`

