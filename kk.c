#include <stdio.h>
#include <ncurses.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/param.h>

#define ROW 1024
#define COL 1024

typedef struct  {
  char** content;  // Content as array of strings
  ssize_t line;    // current starting line 0 indexed
  size_t x_offset; // x offset (to the left)
  enum hm {        // ...
    NONE,          // ...
    STATUS,        // ...
    PAGE           // ...
  } help;          // help mode, 0: none 1: statusline 2: help page
} Context;

void read_entire_stream(FILE* f, char** c, int s) {
  for (int i=0 ; i<s; i++) {
    if ((c[i] = malloc(sizeof(char) * COL)) == NULL) {
        printf("unable to allocate memory \n");
        return;
      }
  }

  int i = 0;
  while(fgets(c[i], COL, f)) { i++; }
  c[i++] = NULL;
}

void draw_status(Context *ctx, WINDOW *win) {
  attron(COLOR_PAIR(1));
  int lastline = getmaxy(win) -1;
  move(lastline, 0);
  printw("-- kk --");
  if (ctx->help == 1) {
    printw("  j d: down; k u: up; q: quit");
  }
  attroff(COLOR_PAIR(1));
}

void draw(Context *ctx, WINDOW *win) {
  clear();
  int maxline = getmaxy(win) - 1 + ctx->line;
  int i = ctx->line;
  for (; i < COL; i++) {
    if (i >= maxline) break;
    if (ctx->content[i] == NULL) break;
    printw("-%s", ctx->content[i]);
  }

  // while ((c = ctx->content[i]) != 0) {
  //   i++;
  //   if (c == '\n') {
  //     if (++lcount == ctx->line && ctx->line > 0 ) continue;
  //   }
  //   if (lcount > maxline) break;
  //   // TODO hacky solution
  //   if (lcount < ctx->line) continue;
  //   printw("%c", c);
  // }
  while (i++ < maxline) {
    printw("~ \n\r");
  }
  draw_status(ctx, win);
  refresh();
  return;
}

int main(int argc, char *argv[]) {

  char *content[ROW];

  if (argc == 2) {
    // file
    FILE *f = fopen(argv[1], "r");
    if (f == NULL) {
      printf("fatal: counldnt open file");
      return 1;
    }
    read_entire_stream(f, content, ROW);
    fclose(f);
  } else {
    // stdin
    read_entire_stream(stdin, content, ROW);
  }

  if (content == NULL) {
    printf("fatal: null content stream\n\r");
    return 1;
  }

  // TODO: line caps

  Context c = {content, 0, 0, NONE};

  WINDOW *win = initscr();
  start_color();
  use_default_colors();

  // status bar
  init_pair(1, COLOR_BLACK, COLOR_WHITE);

  draw(&c, win);

  int k;
  // TODO: kinda wacky - https://stackoverflow.com/questions/69037629/how-to-read-stdin-then-keyboard-input-in-c
  FILE *tty = fopen("/dev/tty", "r");
  if (tty == NULL) {
    printf("fatal: tty couldnt open\n\r");
    return 1;
  }

  int stepsize = 10;

  while((k = getc(tty))) {
    switch (k) {
      case 'q': 
        endwin();
        return 0;
      case 'j':
        // TODO get line count
        c.line++;
        c.help = NONE;
        draw(&c, win);
        break;
      case 'k':
        if (c.line != 0) c.line--;
        c.help = NONE;
        draw(&c, win);
        break;
      case 'u':
      case 0x15: // ^U
        c.line = MAX(0, c.line - stepsize);
        c.help = NONE;
        draw(&c, win);
        break;
      case 'd':
      case 0x04: // ^D
        c.line += stepsize;
        c.help = NONE;
        draw(&c, win);
        break;
      default:
        c.help = STATUS;
        draw_status(&c, win);
        refresh();
        break;
    }
  };

  // ho did we get here
  endwin();
  return 1;
}
