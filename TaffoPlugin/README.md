# How to build and use the Taffo plugin
 - mkdir build
 - cd build
 - cmake ..
 - make
 - clang -c -Xclang -load -Xclang ./TaffoPlugin.so -Xclang -add-plugin -Xclang taffo-plugin main.c