e exe:
	g++ *.cpp

r run:
	./a.out

.PHONY: ttt
ttt: exe
	./a.out tic-tac-toe.s