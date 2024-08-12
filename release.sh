mkdir -p build
cd build || exit
# Replace -A x64 with -G "Ninja Multi-Config"
cmake -A x64 ..
cd ..
# Add optional -- -j 8
cmake --build build --config Release
cmake --build build --target package --config Release
