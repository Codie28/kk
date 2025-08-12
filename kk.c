#include <stdio.h>
#include <ncurses.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/param.h>
#include <unistd.h>
#include <sys/ioctl.h>

#define FLAG_IMPLEMENTATION
#include "./thidparty/flag.h"

// 1 2 3 4 5 6 7 8 9 a b c d e 
// 0 0 0 0 0 s t r i n g 0 g k
// char key = 's';
// char *line = 6;
// while(line[i] != 0)
// print(line[i])
// i++
//
// 1 2 3 4 5 6 7 8 9 a b c d e f
// 2 5 9 0 o n e 0 s e c o n d 0
// char **content = 2;
// void change(char **string) {
//   string[0] = 't';
// }
// change(&content[1]) -> change(9)
// 1 2 3 4 5 6 7 8 9 a b c d e f
// 2 5 9 0 o n e 0 t e c o n d 0
//
// maybe possibl;y proly not working asm hello world programm
// __asm__(
//     hello: db "hello, world"
//     mov [hello], rax
//
//     print:
//     xor rbx, rbx
//     loop:
//     mov ral, 0x2h
//     int 12
//     inc rbx
//     mov rax, [rax + rbx]
//     mov rax, rcx
//     jnz loop
// )

#define ROW 1024
#define COL 1024

#define REG_START 97 // a
#define REG_COUNT 25

static bool FLAGcolor;
static bool FLAGdiff;

typedef struct  {
  char **content;        // Content as array of strings
  int size;              // size of the content
  ssize_t line;          // current starting line 0 indexed
  size_t x_offset;       // x offset (to the left)
  enum hm {              // ...
    NONE,                // ...
    STATUS,              // ...
    PAGE                 // ...
  } help;                // help mode to display
  int marks[REG_COUNT];  // *dict* of marks (indexes are lc letters)
} Context;

// returns the size of the array
int read_entire_stream(FILE* f, char **c, int s) {
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

void draw_status(Context *ctx, WINDOW *win, bool input) {
  int lastline = getmaxy(win) -1;
  move(lastline, 0);
  clrtoeol();

  attron(COLOR_PAIR(10));

  if (ctx->help == PAGE) {
    printw("HELP -- press q or ESC to exit help");
  } else {
    printw("-- kk -- %d/%zd", ctx->size, ctx->line);
  }
  if (input) {
    printw(" ...");
  }
  if (ctx->help == STATUS) {
    printw("  h: help  q Q: quit  j e: down  k y: up");
  }
  attroff(COLOR_PAIR(10));
}

void print_with_ansi(char *str) {
  char *p = str;
  while (*p) {
    if (*p == '\033' && *(p + 1) == '[') {
      p += 2;
      const char *start = p;
      while (*p && *(p + 1) != 'm') p++;
      // TODO: this loop doesnt terminate when it cant find m
      int color_code = strtol(start, &p, 10);
      if (color_code >= 30 && color_code <= 37) {
        attron(COLOR_PAIR(color_code));
      }
    } else {
      printw("%c", *p);
    }
    p++;
  }

  attroff(COLOR_PAIR(1));
}


void draw(Context *ctx, WINDOW *win) {
  clear();
  int maxline = getmaxy(win) - 1 + ctx->line;
  int i = ctx->line;
  for (; i < COL; i++) {
    if (i >= maxline) break;
    if (i >= ctx->size) break; 

    char column = '-';

    // color codes
    char f = ctx->content[i][5];
    // printw("f:%c!", f);

    // marks
    for (int m = 0 ; m < REG_COUNT; m++) {
      if (i == ctx->marks[m]) {
        column = (char) m + REG_START - 32;
        break;
      }
    }

    // maybe complicate it alot more and make only one printw("%c", column) line
    if (FLAGdiff ) {
      if (!(f == '-' || f == '+' || f == '@')) {
        if (column != '-') printw("%c", column);
        else printw("#");
      }
    } else {
      printw("%c", column);
    }


    if (FLAGcolor) {
      print_with_ansi(ctx->content[i]);
    } else {
      printw("%s", ctx->content[i]);
    }
  }

  while (i++ < maxline) {
    printw("~ \n\r");
  }
  draw_status(ctx, win, false);
  refresh();
  return;
}

void set_mark(Context *ctx, char c, int line) {
  ctx->marks[(int) c - REG_START] = line;
}

// only realy used for marks
char await_input(Context *ctx, WINDOW *win, FILE *tty, char d) {
  draw_status(ctx, win, true);
  refresh();
  char m = getc(tty);
  if (m == 0x1b /* ESC */) {
    return d;
  }
  if (m < 97 || m > 122) {
    return d;
  }
  return m;
}

void handle_key(WINDOW *win, Context c, Context hp) {

  // TODO: kinda wacky - https://stackoverflow.com/questions/69037629/how-to-read-stdin-then-keyboard-input-in-c
  FILE *tty = fopen("/dev/tty", "r");
  if (tty == NULL) {
    printf("fatal: tty couldnt open\n\r");
    return;
  }

  // TODO: make them update on resize
  int window = getmaxy(win) - 1;
  int half_window = window / 2;
  Context* cp = &c;

  char k;
  char m;
  while((k = getc(tty))) {
    switch (k) {

      /// META IDK

      case 'q': 
      case 'Q': 
      case 0x1b: // ESC
        if (cp == &hp) {
          cp = &c;
          draw(cp, win);
          break;
        }
        else return;
      case 'h':
      case 'H':
        if (cp == &c) cp = &hp;
        else cp = &c;
        draw(cp, win);
        break;

      /// MOVING

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
        // TODO: goes offscreen on files smaller than window
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

      /// JUMPING

      case 'g':
        cp->line = 0;
        draw(cp, win);
        break;
      case 'G':
        cp->line = cp->size - window;
        draw(cp, win);
        break;

      /// MARKING

      case 'm':
        m = await_input(cp, win, tty, 'a');
        set_mark(cp, m, cp->line);
        draw(cp, win);
        break;
      case 'M':
        m = await_input(cp, win, tty, 'a');
        set_mark(cp, m, MIN(cp->size, cp->line + window - 1));
        draw(cp, win);
        break;

      // goto mark
      case '\'':
        m = await_input(cp, win, tty, 'a');
        cp->line = cp->marks[m - REG_START];
        draw(cp, win);
        break;

      default:
        if (cp == &c) cp->help = STATUS;
        draw_status(cp, win, false);
        refresh();
        break;
    }
  }
}

void usage(FILE *stream) {
  fprintf(stream, "Usage: ./kk [OPTIONS] [--] [FILE]\n");
  fprintf(stream, "OPTIONS:\n");
  flag_print_options(stream);
}

int main(int argc, char **argv) {

    bool *Fhelp = flag_bool("h", false, "Print help and exit");
    bool *Fcolor = flag_bool("r", false, "Enable color support");
    bool *Fdiff = flag_bool("d", false, "Enable diff mode, only for 'git diff'");
    bool *Fdo_paging_for_small_files = flag_bool("s", false, "Do paging even for small inputs");

    if (!flag_parse(argc, argv)) {
        usage(stderr);
        flag_print_error(stderr);
        exit(1);
    }

    FLAGcolor = *Fcolor;
    FLAGdiff = *Fdiff;

    if (*Fhelp) {
        usage(stdout);
        exit(0);
    }

    int rest_argc = flag_rest_argc();
    char **rest_argv = flag_rest_argv();

  char *content[ROW];
  int size;

  if (rest_argc >= 1 && strcmp(rest_argv[0], "-") != 0) {
    // file
    FILE *f = fopen(rest_argv[0], "r");
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

  // why negation???
  if (!*Fdo_paging_for_small_files) {
    struct winsize w;

    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);

    if (size < w.ws_row) {
      for (int i; i < size; i++) {
        printf("%s", content[i]);
      }
      return 0;
    }
  }

  WINDOW *win = initscr();
  start_color();
  use_default_colors();

  // Define color pairs for ansi
  init_pair(31, COLOR_RED, -1);
  init_pair(32, COLOR_GREEN, -1);
  init_pair(33, COLOR_YELLOW, -1);
  init_pair(34, COLOR_BLUE, -1);
  init_pair(35, COLOR_MAGENTA, -1);
  init_pair(36, COLOR_CYAN, -1);
  init_pair(37, COLOR_WHITE, -1);

  // status bar
  init_pair(10, COLOR_BLACK, COLOR_WHITE);



  Context c = {content, size, 0, 0, NONE};
  Context hp = load_help_page();

  draw(&c, win);
  handle_key(win, c, hp);
  endwin();

  return 1;
}
