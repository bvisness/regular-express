#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "microui.h"

extern void printInt(int num);
extern void printLong(long num);
extern void printDouble(double num);

int values[] = { 88, 56, 100, 2, 25 };

int cmpfunc(const void * a, const void * b) {
   return (*(int*)a - *(int*)b);
}

int fib(int n) {
	if (n <= 2) {
		return 1;
	}

	return fib(n-2) + fib(n-1);
}

void boo() {
	for (int i = 0; i < 5; i++) {
		printInt(values[i]);
	}

	qsort(values, 5, sizeof(int), cmpfunc);

	for (int i = 0; i < 5; i++) {
		printInt(values[i]);
	}
}

float blam(int a, int b) {
	return a - b;
}

void goober() {
	printLong(strlen("hello, world"));
}

void foop(char* s) {
	sprintf(s, "abc %d", 12);
}

void glamp() {
	char* ptr;
	printDouble(strtod("123.456", &ptr));
}
