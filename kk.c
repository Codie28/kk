#include <stdio.h>
#include <ncurses.h>
#include <stdlib.h>
#include <stdbool.h>

enum hm {
  NONE,
  STATUS,
  PAGE
};

typedef struct  {
  char* content;   // Content as null terminated string
  size_t line;     // current starting line 0 indexed
  size_t x_offset; // x offset (to the left)
  enum hm help;    // help mode, 0: none 1: statusline 2: help page
} Context;

char* read_entire_stream(FILE* f) {
  fseek(f, 0, SEEK_END); 
  long size = ftell(f);
  fseek(f, 0, SEEK_SET);
  // duno what happens
  if (size == -1) size = 1024*1024;

  char* fcontent = malloc(size);
  fread(fcontent, 1, size, f);
  return fcontent;
}

void draw_status(Context *ctx, WINDOW *win) {
  // TODO: get term size
  move(49, 0);
  printw("-- kk -- %zu", ctx->line);
  if (ctx->help == 1) {
    printw("  j: down  k: up  q: quit");
  }
}

void draw(Context *ctx, WINDOW *win) {
  clear();
  char c = 0;
  int lcount = 0;
  int i = 0;
  int maxline = getmaxy(win) - 2 + ctx->line;
  while ((c = ctx->content[i]) != 0) {
    i++;
    if (c == '\n') {
      if (++lcount == ctx->line && ctx->line > 0 ) continue;
    }
    if (lcount > maxline) break;
    // TODO hacky solution
    if (lcount < ctx->line) continue;
    printw("%c", c);
  }
  draw_status(ctx, win);
  refresh();
  return;
}

int main(int argc, char *argv[]) {

  char* content;

  if (argc == 2) {
    // file
    FILE *f = fopen(argv[1], "r");
    if (f == NULL) {
      printf("fatal: counldnt open file");
      return 1;
    }
    content = read_entire_stream(f);
    fclose(f);
  } else {
    // stdin
    content = read_entire_stream(stdin);
  }

  if (content == NULL) {
    printf("fatal: null content stream\n\r");
    return 1;
  }

  Context c = {content, 0, 0, NONE};

  WINDOW *win = initscr();
  draw(&c, win);

  int k;
  // TODO: kinda wacky - https://stackoverflow.com/questions/69037629/how-to-read-stdin-then-keyboard-input-in-c
  FILE *tty = fopen("/dev/tty", "r");
  if (tty == NULL) {
    printf("fatal: tty couldnt open\n\r");
    return 1;
  }

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

// #include <ncurses.h>
// #include <stdio.h>
// int main(int argc, char** argv) {
//   initscr();
//   FILE* f = fopen(argv[1], "r");
//   int c;
//   while ((c = getc(f)) != EOF) printw("%c", c);
//   fclose(f);
//   refresh();
//   getc(stdin);
//   endwin();
//   return 0;
// }
