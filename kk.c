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

#define REG_START 95 // _
#define REG_COUNT 27

static bool FLAGcolor;
static bool FLAGdiff;
static bool FLAGlog;
static bool FLAGnum;

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

void set_mark(Context *ctx, char c, int line) {
  ctx->marks[(int) c - REG_START] = line + 1;
}

// loads help.txt file as context
Context load_help_page() {
  char *c[ROW];
  // TODO: fix the path
  FILE* f = fopen("/home/petal/programming/kk/help.txt", "r");

  int size = read_entire_stream(f, c, ROW);

  Context h = {c, size, 0, 0, PAGE};
  set_mark(&h, '_', size - 1);
  set_mark(&h, '`', 0);
  
  return h;
}

void draw_status(Context *ctx, WINDOW *win, bool input) {
  int window = getmaxy(win) - 1;
  move(window, 0);
  clrtoeol();

  attron(COLOR_PAIR(10));

  if (ctx->help == PAGE) {
    printw("HELP -- press q or ESC to exit help");
  } else {
    printw("-- kk -- %d/%zd %d", ctx->size, ctx->line, window);
  }
  if (input) {
    printw(" ...");
  }
  if (ctx->help == STATUS) {
    printw("  h: help  q Q: quit  j e: down  k y: up");
  }
  attroff(COLOR_PAIR(10));
}

const char* strip_ansi(const char *str) {
  size_t len = strlen(str);
  char *result = malloc(len + 1);
  if (!result) {
    return NULL;
  }

  char *ptr = result;
  const char *p = str;

  while (*p) {
    if (*p == '\033' && *(p + 1) == '[') {
      while (*p && *p != 'm') p++;
      if (*p == 'm') p++;
    } else {
      *ptr++ = *p;
      p++;
    }
  }

  *ptr = '\0';

  return result;
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

    if (FLAGnum) printw("%d", i);

    char column = '-';

    char f = strip_ansi(ctx->content[i])[0];

    // marks
    for (int m = 2 ; m < REG_COUNT; m++) {
      if (i + 1 == ctx->marks[m]) {
        column = (char) m + REG_START - 32;
        break;
      }
    }

    if (FLAGdiff) {
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

// only realy used for marks
char await_input(Context *ctx, WINDOW *win, FILE *tty, char d) {
  draw_status(ctx, win, true);
  refresh();
  char m = getc(tty);
  if (m == 0x1b /* ESC */) {
    return d;
  }
  if (m < REG_START || m > REG_START + REG_COUNT) {
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
  Context* ctx = &c;


  char k;
  char m;
  while((k = getc(tty))){

    int window = getmaxy(win) - 1;
    int half_window = window / 2;
    int limit = ctx->size;
    if (ctx->line <= ctx->size - window) {
      limit -= window;
    }

    switch (k) {

      /// META IDK

      case 'q': 
      case 'Q': 
      case 0x1b: // ESC
        if (ctx == &hp) {
          ctx = &c;
          draw(ctx, win);
          break;
        }
        else return;
      case 'h':
      case 'H':
        if (ctx == &c) ctx = &hp;
        else ctx = &c;
        draw(ctx, win);
        break;

      /// MOVING

      case 'j':
      case 'e':
      case 0x05: // ^E
      case 0x0d: // CR
        if (ctx->line != ctx->size - window && ctx->line < ctx->size) ctx->line++;
        if (ctx == &c) ctx->help = NONE;
        draw(ctx, win);
        break;
      case 'k':
      case 'y':
      case 0x19: // ^Y
        if (ctx->line != 0) ctx->line--;
        if (ctx == &c) ctx->help = NONE;
        draw(ctx, win);
        break;
      case 'd':
      case 0x04: // ^D
        ctx->line = MIN(limit, ctx->line + half_window);
        if (ctx == &c) ctx->help = NONE;
        draw(ctx, win);
        break;
      case 'u':
      case 0x15: // ^U
        ctx->line = MAX(0, ctx->line - half_window);
        if (ctx == &c) ctx->help = NONE;
        draw(ctx, win);
        break;
      case 'z':
        ctx->line = MIN(limit, ctx->line + window);
        draw(ctx, win);
        break;
      case 'w':
        ctx->line = MAX(0, ctx->line - window);
        draw(ctx, win);
        break;

      /// JUMPING

      case 'g':
        ctx->line = 0;
        draw(ctx, win);
        break;
      case 'G':
        ctx->line = ctx->size - window;
        draw(ctx, win);
        break;

      /// MARKING

      case 'm':
        m = await_input(ctx, win, tty, 'a');
        set_mark(ctx, m, ctx->line);
        draw(ctx, win);
        break;
      case 'M':
        m = await_input(ctx, win, tty, 'a');
        set_mark(ctx, m, MIN(ctx->size, ctx->line + window - 1));
        draw(ctx, win);
        break;
      // goto mark
      case '\'':
        m = await_input(ctx, win, tty, 'a');
        ctx->line = MAX(0, ctx->marks[m - REG_START] - 1);
        draw(ctx, win);
        break;
      case '"':
        m = await_input(ctx, win, tty, 'a');
        ctx->line = MAX(0, ctx->marks[m - REG_START] - window);
        draw(ctx, win);
        break;

      /// GIT STUFF
      case 'p':
        for (int i = ctx->line + 1; i < ctx->size; i++) {
          if (strncmp(strip_ansi(ctx->content[i]), "@@", 2) == 0) {
            ctx->line = i;
            draw(ctx, win);
            break;
          }
        }
        break;
      case 'P':
        for (int i = ctx->line - 1; i > -1; i--) {
          if (strncmp(strip_ansi(ctx->content[i]), "@@", 2) == 0) {
            ctx->line = i;
            draw(ctx, win);
            break;
          }
        }
        break;
      case 'l':
        for (int i = ctx->line + 1; i < ctx->size; i++) {
          if (strncmp(strip_ansi(ctx->content[i]), "commit", 6) == 0) {
            ctx->line = i;
            draw(ctx, win);
            break;
          }
        }
        break;
      case 'L':
        for (int i = ctx->line - 1; i > -1; i--) {
          if (strncmp(strip_ansi(ctx->content[i]), "commit", 6) == 0) {
            ctx->line = i;
            draw(ctx, win);
            break;
          }
        }
        break;

      default:
        if (ctx == &c) ctx->help = STATUS;
        draw_status(ctx, win, false);
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
    bool *Fnum  = flag_bool("l", false, "Enable line numbers");
    bool *Fdo_paging_for_small_files = flag_bool("s", false, "Do paging even for small inputs");

    if (!flag_parse(argc, argv)) {
        usage(stderr);
        flag_print_error(stderr);
        exit(1);
    }

    FLAGcolor = *Fcolor;
    FLAGdiff = *Fdiff;
    FLAGnum = *Fnum;

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

    // add a bit less space for promt and stuff
    if (size < w.ws_row - 5) {
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
  set_mark(&c, '_', size - 1);
  set_mark(&c, '`', 0);
  Context hp = load_help_page();

  draw(&c, win);
  handle_key(win, c, hp);
  endwin();

  return 1;
}
