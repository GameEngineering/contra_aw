#!bin/sh

rm -rf bin
mkdir bin
cd bin

proj_root_dir=$(pwd)/../

flags=(
	-std=c++11
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
	-L ../third_party/lib/osx/
)

fworks=(
	-framework OpenGL
	-framework CoreFoundation 
	-framework CoreVideo 
	-framework IOKit 
	-framework Cocoa 
	-framework Carbon
)

libs=(
	-lgunslinger
)

# Build
g++ -O3 ${lib_dirs[*]} ${libs[*]} ${fworks[*]} ${inc[*]} ${src[*]} ${flags[*]} -o Contra3

cd ..



