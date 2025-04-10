mkdir -p build
cd build || exit
# To use Visual Studio generator, replace -G "Ninja Multi-Config" with -A x64
cmake -G "Ninja Multi-Config" ..
cd ..
# Add optional -- -j 8
cmake --build build --target package --config Release
