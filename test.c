#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "microui.h"

static int text_width(mu_Font font, const char *text, int len) {
  return strlen(text);
}

static int text_height(mu_Font font) {
  return r_get_text_height();
}

mu_Context* ctx;

void init() {
	int* foo = (int*) malloc(sizeof(int));
	*foo = 12345678;
	long* blam = (long*) malloc(sizeof(long));
	*blam = 234567876;
	malloc(12);
	ctx = malloc(sizeof(mu_Context));

	mu_init(ctx);
}

void frame() {
	if (mu_begin_window(ctx, "My Window", mu_rect(10, 10, 300, 400))) {
		/* process ui here... */
		mu_end_window(ctx);
	}

	mu_end(ctx);
}
