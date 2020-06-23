mkdir -p build
cd build
cmake ..
cd ..
cmake --build build --config Release
cmake --build build --target PACKAGE --config Release
