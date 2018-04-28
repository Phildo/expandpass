EXE=expandpass
SRC=gen.cpp
NOWARN=-Wno-write-strings

make: $(EXE)
	

run: password.txt
	

frun:
	./$(EXE)

debug:
	gcc $(NOWARN) -ggdb3 $(SRC) -o $(EXE) && lldb -- ./$(EXE) -fa -f\#

$(EXE): $(SRC) seed.txt
	gcc $(NOWARN) $(SRC) -o $(EXE)

password.txt: $(EXE)
	./$(EXE)

clean:
	rm -r $(EXE) $(EXE).dSYM

replace: $(EXE)
	cp $(EXE) `which $(EXE)`

