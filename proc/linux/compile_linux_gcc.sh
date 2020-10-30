#!bin/sh

rm -rf bin
mkdir bin
cd bin

proj_root_dir=$(pwd)/../

flags=(
	-std=c++11 -Wl,--no-as-needed -ldl -lGL -lX11 -pthread -lXi
)

# Include directories
inc=(
	-I ../source/
	-I ../third_party/include/
	-I ../third_party/include/gs/
	-I ../include/
)

# Source files
src=(
	../source/main.cpp
	../source/imgui/*.cpp
)

lib_dirs=(
	-L ../third_party/lib/linux/
)

fworks=(
)

libs=(
	-lGunslinger
)

# Build
g++ -O3 ${fworks[*]} ${inc[*]} ${src[*]} ${flags[*]} ${lib_dirs[*]} ${libs[*]} -lm -o Contra3

cd ..



