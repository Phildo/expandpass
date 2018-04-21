make: expandpass
	

run: password.txt
	

frun:
	./expandpass

debug:
	gcc -Wno-write-strings -ggdb3 gen.cpp -o expandpass && lldb ./expandpass

expandpass: gen.cpp seed.txt
	gcc -Wno-write-strings gen.cpp -o expandpass

password.txt: expandpass
	./expandpass

clean:
	rm expandpass

