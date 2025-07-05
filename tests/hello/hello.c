#include <stdio.h>

int main(void){
	printf("hello\n");
	return 0;
}

// Strace ./hello give all system calls by lines
// kill, read, man 2
// /proc/kallsyms -> holds all the symbols that the kernel know 
// Maybe important to know to read/write a file in /proc subsystem

