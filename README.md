BinaryParser
============

Reads 12 bit values from a binary file and writes the largest &amp; last 32 values to a txt file. Special considerations for space/time/stack optimization.


My build command:
gcc -g -ansi -pedantic -Wall -o binaryParser binaryParser.c -I ./

How to run it:
./binaryParser <inputFile> <outputFile>

Some info about my compiler:
Rushi$ gcc -v
Configured with: --prefix=/Library/Developer/CommandLineTools/usr --with-gxx-include-dir=/usr/include/c++/4.2.1
Apple LLVM version 5.0 (clang-500.2.79) (based on LLVM 3.3svn)
Target: x86_64-apple-darwin13.0.0
Thread model: posix