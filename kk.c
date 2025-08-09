#include <stdio.h>
#include <ncurses.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/param.h>

#define ROW 1024
#define COL 1024

typedef struct  {
  char** content;  // Content as array of strings
  int size;        // size of the content
  ssize_t line;    // current starting line 0 indexed
  size_t x_offset; // x offset (to the left)
  enum hm {        // ...
    NONE,          // ...
    STATUS,        // ...
    PAGE           // ...
  } help;          // help mode to display
} Context;

// returns the size of the array
int read_entire_stream(FILE* f, char** c, int s) {
  for (int i=0 ; i<s; i++) {
    if ((c[i] = malloc(sizeof(char) * COL)) == NULL) {
        printf("unable to allocate memory \n");
        return -1;
      }
  }

  int i = 0;
  while(fgets(c[i], COL, f)) { i++; }
  return i;
}

// loads help.txt file as context
Context load_help_page() {
  char *c[ROW];
  // TODO: fix the path
  FILE* f = fopen("/home/petal/programming/kk/help.txt", "r");

  int size = read_entire_stream(f, c, ROW);

  Context h = {c, size, 0, 0, PAGE};
  return h;
}

void draw_status(Context *ctx, WINDOW *win) {
  attron(COLOR_PAIR(1));
  int lastline = getmaxy(win) -1;
  move(lastline, 0);

  if (ctx->help == PAGE) {
    printw("HELP -- press q or ESC to exit help");
  } else {
    printw("-- kk -- %d/%zd", ctx->size, ctx->line);
  }
  if (ctx->help == STATUS) {
    printw("  j e: down  k y: up");
  }
  attroff(COLOR_PAIR(1));
}

void draw(Context *ctx, WINDOW *win) {
  clear();
  int maxline = getmaxy(win) - 1 + ctx->line;
  int i = ctx->line;
  for (; i < COL; i++) {
    if (i >= maxline) break;
    if (i >= ctx->size) break; printw("-%s", ctx->content[i]);
  }

  while (i++ < maxline) {
    printw("~ \n\r");
  }
  draw_status(ctx, win);
  refresh();
  return;
}

void handle_key(WINDOW *win, Context c, Context hp) {

  // TODO: kinda wacky - https://stackoverflow.com/questions/69037629/how-to-read-stdin-then-keyboard-input-in-c
  FILE *tty = fopen("/dev/tty", "r");
  if (tty == NULL) {
    printf("fatal: tty couldnt open\n\r");
    return;
  }

  int window = getmaxy(win) - 1;
  int half_window = window / 2;
  Context* cp = &c;

  int k;
  while((k = getc(tty))) {
    switch (k) {
      case 'q': 
      case 'Q': 
      case 0x1b: // ESC
        if (cp == &hp) cp = &c;
        else return;
      case 'j':
      case 'e':
      case 0x05: // ^E
      case 0x0d: // CR aka \n
        if (cp->line != c.size - window) c.line++;
        if (cp == &c) cp->help = NONE;
        draw(cp, win);
        break;
      case 'k':
      case 'y':
      case 0x19: // ^Y
        if (cp->line != 0) c.line--;
        if (cp == &c) cp->help = NONE;
        draw(cp, win);
        break;
      case 'u':
      case 0x15: // ^U
        cp->line = MAX(0, c.line - half_window);
        if (cp == &c) cp->help = NONE;
        draw(cp, win);
        break;
      case 'd':
      case 0x04: // ^D
        // TODO: segfault cuz cp->line get set to -5123912yui987123 on small files
        cp->line = MIN(c.size - window, c.line + half_window);
        if (cp == &c) cp->help = NONE;
        draw(cp, win);
        break;
      case 'z':
        cp->line = MIN(c.size - window, c.line + window);
        draw(cp, win);
        break;
      case 'w':
        cp->line = MAX(0, c.line - window);
        draw(cp, win);
        break;
      case 'h':
        if (cp == &c) cp = &hp;
        else cp = &c;
        draw(cp, win);
        break;
      default:
        if (cp == &c) cp->help = STATUS;
        draw_status(cp, win);
        refresh();
        break;
    }
  };

}

int main(int argc, char *argv[]) {

  Context hp = load_help_page();

  char *content[ROW];
  int size;

  if (argc == 2) {
    // file
    FILE *f = fopen(argv[1], "r");
    if (f == NULL) {
      printf("fatal: counldnt open file");
      return 1;
    }
    size = read_entire_stream(f, content, ROW);
    fclose(f);
  } else {
    // stdin
    size = read_entire_stream(stdin, content, ROW);
  }

  WINDOW *win = initscr();
  start_color();
  use_default_colors();

  // status bar
  init_pair(1, COLOR_BLACK, COLOR_WHITE);

  Context c = {content, size, 0, 0, NONE};

  draw(&c, win);
  handle_key(win, c, hp);
  endwin();

  return 1;
}
