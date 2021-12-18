mkdir -p build/terminal
cd  build/terminal
cmake ../.. -GNinja
cmake --build . --parallel 8
