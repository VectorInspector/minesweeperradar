mineseer:
	g++ mineseer.cpp -o mineseer.exe -std=c++20 -I../sdl/include -L../sdl/lib -lSDL2main -lSDL2 ../lodepng/lodepng.cpp -I../lodepng