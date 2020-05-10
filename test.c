#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "microui.h"

extern void canvas_clear();
extern void canvas_clip(int x, int y, int w, int h);
extern void canvas_setFillRGB(unsigned char r, unsigned char g, unsigned char b, unsigned char a);
extern void canvas_rect(int x, int y, int w, int h);
extern void canvas_text(char* str, int x, int y);

extern int measureText(const char* text, int len);

static int text_width(mu_Font font, const char *text, int len) {
	if (len < 0) {
		len = strlen(text);
	}

	return measureText(text, len);
}

int _textHeight;
static int text_height(mu_Font font) {
	return _textHeight;
}

mu_Context* ctx;

void init(int textHeight) {
	_textHeight = textHeight;

	ctx = malloc(sizeof(mu_Context));
	mu_init(ctx);
	ctx->text_width = text_width;
  	ctx->text_height = text_height;
}

void mouseMove(int x, int y) {
	mu_input_mousemove(ctx, x, y);
}

void mouseDown(int x, int y) {
	mu_input_mousedown(ctx, x, y, MU_MOUSE_LEFT);
}

void mouseUp(int x, int y) {
	mu_input_mouseup(ctx, x, y, MU_MOUSE_LEFT);
}

void keyDown(int key) {
	mu_input_keydown(ctx, key);
}

void keyUp(int key) {
	mu_input_keyup(ctx, key);
}

char textInputBuf[1024];
void textInput() {
	mu_input_text(ctx, textInputBuf);
}

void scroll(int x, int y) {
	mu_input_scroll(ctx, x, y);
}

static  char logbuf[32000];
static   int logbuf_updated = 0;

void frame() {
	mu_begin(ctx);

	if (mu_begin_window(ctx, "My Window", mu_rect(10, 10, 300, 400))) {
		if (mu_button(ctx, "x")) {
			printf("Pressed button!");
		}

		if (mu_header(ctx, "Window Info")) {
			mu_Container *win = mu_get_current_container(ctx);
			char buf[64];
			mu_layout_row(ctx, 2, (int[]) { 54, -1 }, 0);
			mu_label(ctx,"Position:");
			sprintf(buf, "%d, %d", win->rect.x, win->rect.y); mu_label(ctx, buf);
			mu_label(ctx, "Size:");
			sprintf(buf, "%d, %d", win->rect.w, win->rect.h); mu_label(ctx, buf);
		}

		/* labels + buttons */
    if (mu_header_ex(ctx, "Test Buttons", MU_OPT_EXPANDED)) {
      mu_layout_row(ctx, 3, (int[]) { 86, -110, -1 }, 0);
      mu_label(ctx, "Test buttons 1:");
      if (mu_button(ctx, "Button 1")) { printf("Pressed button 1"); }
      if (mu_button(ctx, "Button 2")) { printf("Pressed button 2"); }
      mu_label(ctx, "Test buttons 2:");
      if (mu_button(ctx, "Button 3")) { printf("Pressed button 3"); }
      if (mu_button(ctx, "Popup")) { mu_open_popup(ctx, "Test Popup"); }
      if (mu_begin_popup(ctx, "Test Popup")) {
        mu_button(ctx, "Hello");
        mu_button(ctx, "World");
        mu_end_popup(ctx);
      }
    }

    /* tree */
    if (mu_header_ex(ctx, "Tree and Text", MU_OPT_EXPANDED)) {
      mu_layout_row(ctx, 2, (int[]) { 140, -1 }, 0);
      mu_layout_begin_column(ctx);
      if (mu_begin_treenode(ctx, "Test 1")) {
        if (mu_begin_treenode(ctx, "Test 1a")) {
          mu_label(ctx, "Hello");
          mu_label(ctx, "world");
          mu_end_treenode(ctx);
        }
        if (mu_begin_treenode(ctx, "Test 1b")) {
          if (mu_button(ctx, "Button 1")) { printf("Pressed button 1"); }
          if (mu_button(ctx, "Button 2")) { printf("Pressed button 2"); }
          mu_end_treenode(ctx);
        }
        mu_end_treenode(ctx);
      }
      if (mu_begin_treenode(ctx, "Test 2")) {
        mu_layout_row(ctx, 2, (int[]) { 54, 54 }, 0);
        if (mu_button(ctx, "Button 3")) { printf("Pressed button 3"); }
        if (mu_button(ctx, "Button 4")) { printf("Pressed button 4"); }
        if (mu_button(ctx, "Button 5")) { printf("Pressed button 5"); }
        if (mu_button(ctx, "Button 6")) { printf("Pressed button 6"); }
        mu_end_treenode(ctx);
      }
      if (mu_begin_treenode(ctx, "Test 3")) {
        static int checks[3] = { 1, 0, 1 };
        mu_checkbox(ctx, "Checkbox 1", &checks[0]);
        mu_checkbox(ctx, "Checkbox 2", &checks[1]);
        mu_checkbox(ctx, "Checkbox 3", &checks[2]);
        mu_end_treenode(ctx);
      }
      mu_layout_end_column(ctx);

      mu_layout_begin_column(ctx);
      mu_layout_row(ctx, 1, (int[]) { -1 }, 0);
      mu_text(ctx, "Lorem ipsum dolor sit amet, consectetur adipiscing "
        "elit. Maecenas lacinia, sem eu lacinia molestie, mi risus faucibus "
        "ipsum, eu varius magna felis a nulla.");
      mu_layout_end_column(ctx);
    }

		mu_end_window(ctx);
	}

	if (mu_begin_window(ctx, "Log Window", mu_rect(350, 40, 300, 200))) {
		/* output text panel */
		mu_layout_row(ctx, 1, (int[]) { -1 }, -25);
		mu_begin_panel(ctx, "Log Output");
		mu_Container *panel = mu_get_current_container(ctx);
		mu_layout_row(ctx, 1, (int[]) { -1 }, -1);
		// mu_text(ctx, logbuf);
		mu_end_panel(ctx);
		if (logbuf_updated) {
		  panel->scroll.y = panel->content_size.y;
		  logbuf_updated = 0;
		}

		/* input textbox + submit button */
		static char buf[128];
		int submitted = 0;
		mu_layout_row(ctx, 2, (int[]) { -70, -1 }, 0);
		if (mu_textbox(ctx, buf, sizeof(buf)) & MU_RES_SUBMIT) {
		  mu_set_focus(ctx, ctx->last_id);
		  submitted = 1;
		}
		if (mu_button(ctx, "Submit")) { submitted = 1; }
		if (submitted) {
		  buf[0] = '\0';
		}

		mu_end_window(ctx);
	}

	mu_end(ctx);

	canvas_clear();
	mu_Command *cmd = NULL;
    while (mu_next_command(ctx, &cmd)) {
		switch (cmd->type) {
			// case MU_COMMAND_TEXT: r_draw_text(cmd->text.str, cmd->text.pos, cmd->text.color); break;
			// case MU_COMMAND_RECT: r_draw_rect(cmd->rect.rect, cmd->rect.color); break;
			// case MU_COMMAND_ICON: r_draw_icon(cmd->icon.id, cmd->icon.rect, cmd->icon.color); break;
			// case MU_COMMAND_CLIP: r_set_clip_rect(cmd->clip.rect); break;
			case MU_COMMAND_TEXT: {
				mu_Vec2 pos = cmd->text.pos;
				mu_Color color = cmd->text.color;
				canvas_setFillRGB(color.r, color.g, color.b, color.a);
				canvas_text(cmd->text.str, pos.x, pos.y);
				break;
			}
			case MU_COMMAND_RECT: {
				mu_Color color = cmd->rect.color;
				mu_Rect rect = cmd->rect.rect;
				canvas_setFillRGB(color.r, color.g, color.b, color.a);
				canvas_rect(rect.x, rect.y, rect.w, rect.h);
				break;
			}
			// case MU_COMMAND_ICON: r_draw_icon(cmd->icon.id, cmd->icon.rect, cmd->icon.color); break;
			case MU_COMMAND_CLIP: {
				mu_Rect rect = cmd->clip.rect;
				canvas_clip(rect.x, rect.y, rect.w, rect.h);
				break;
			}
		}
    }
}
