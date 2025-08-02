RAYLIB ?= ./extern/raylib/src/
JSON ?= ./extern/json/

all:
	g++ src/main.cpp -std=c++17 -I $(RAYLIB) -I include -I $(JSON)/include -I extern -L $(RAYLIB) -o boxscratch -lraylib -lgdi32 -lwinmm -lzip -ladvapi32 -lbz2 -llzma -lzstd -lbcrypt -lz