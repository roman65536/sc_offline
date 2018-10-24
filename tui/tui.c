/* INCLUDES */
#include <signal.h> /* for signal */
#include <wchar.h>
#include <ncurses.h>
#include <stdlib.h> /* for malloc */
/* #include <wctype.h> */


/* DEFINES */
#define RESROW           2     // rows reserved for prompt, error, and column numbers
#define TIMEOUT_CURSES   300   // ms  curses input timeout
#define BUFFERSIZE      1024
#define sc_debug(x, ...)     ui_sc_msg(x, DEBUG_MSG, ##__VA_ARGS__)
#define DEBUG_MSG         19


/* STRUCTS */
struct block { /* Block of buffer */
  wint_t value;
  struct block * pnext;
};


/* PROTOTYPES */
void sig_int();
void ui_start_screen();
void ui_stop_screen();

struct block * create_buf();
void addto_buf(struct block * buf, wint_t d);
void flush_buf (struct block * buf);
void erase_buf (struct block * buf);

void handle_input(struct block * buffer);
void do_normalmode(struct block * buf);
void ui_sc_msg(char * s, int type, ...);


/* GLOBAL VARIABLES */
WINDOW * main_win;
WINDOW * input_win;
SCREEN * sstdout;

struct block * buffer;
int shall_quit = 0;

int currow = 0; /* Current row of the selected cell */
int curcol = 0; /* Current column of the selected cell */
int return_value;          // return value of getch()
static wint_t wd;          // char read from stdin
static int d;              // char read from stdin


int main (int argc, char ** argv) {
    signal(SIGINT, sig_int);

    ui_start_screen();
    wchar_t stdin_buffer [BUFFERSIZE] = { L'\0' };

    buffer = (struct block *) create_buf(); // this should only take place if curses ui


    while ( ! shall_quit) {
        handle_input(buffer);
    }

    erase_buf(buffer);

    ui_stop_screen();

    return 0;
}


void ui_start_screen() {
    sstdout = newterm(NULL, stdout, stdin);
    set_term(sstdout);

    main_win = newwin(LINES - RESROW, COLS, RESROW, 0);
    input_win = newwin(RESROW, COLS, 0, 0); // just 2 rows (RESROW = 2)

    wtimeout(input_win, TIMEOUT_CURSES);
    noecho();
    curs_set(0);

    cbreak();
    keypad(input_win, 1);
    return;
}


void ui_stop_screen() {
    set_term(sstdout);
    endwin();
    return;
}

void sig_int() {
    shall_quit = 1;
    return;
}


void handle_input(struct block * buffer) {
    return_value = ui_getch(&wd);
    d = wd;

    if (return_value != -1) {
       addto_buf(buffer, wd);
    }

    do_normalmode(buffer);
    flush_buf(buffer);        // Flush the buffer
}


void do_normalmode(struct block * buf) {
    switch (buf->value) {
        case L'j':
            sc_debug("j");
            break;

        case L'q':
            shall_quit = 1;
            break;
    }
}


void ui_sc_msg(char * s, int type, ...) {
    mvwprintw(input_win, 1, 0, "%s", s);
    wclrtoeol(input_win);
}

/**
 * Asks the user input from stdin (non-blocking)
 * This function asks the user for input from stdin. Should be
 * non-blocking and should return -1 when no key was pressed; 0 when
 * key was pressed. It receives wint_t as a parameter. When a valid
 * key is pressed, its value is then updated in that win_t variable.
 *
 * \param[in] wd
 *
 * \return 0 on key press; -1 otherwise
 */

int ui_getch(wint_t * wd) {
    return wget_wch(input_win, wd);
}


/* Create buffer as list of blocks */
struct block * create_buf() {
    struct block * b = (struct block *) malloc(sizeof(struct block));
    b->value = '\0';
    b->pnext = NULL;
    return b;
}


/* Add a wint_t to a buffer */
void addto_buf(struct block * buf, wint_t d) {
    struct block * aux = buf;

    if (buf->value == '\0') {
        buf->value = d;
    } else {
        struct block * b = (struct block *) malloc(sizeof(struct block));
        b->value = d;
        b->pnext = NULL;

        while (aux->pnext != NULL)
            aux = aux->pnext;
        aux->pnext = b;
    }
    return;
}


void flush_buf (struct block * buf) {
    if (buf == NULL) return;

    struct block * aux, * np;
    for (aux = buf->pnext; aux != NULL; aux = np)
    {
        np = aux->pnext;
        free(aux);
    }
    buf->value = '\0';
    buf->pnext = NULL;

    return;
}


/* Delete all blocks of a buffer including the initial node */
void erase_buf (struct block * buf) {
    flush_buf(buf);
    free(buf);
    return;
}


/* Delete the first element in a buffer */
struct block * dequeue (struct block * buf) {
    if (buf == NULL) return buf;
    struct block * sig;
    if (buf->value == '\0') return buf;

    if (buf->pnext == NULL) {
       buf->value = '\0';
    } else {
        sig = buf->pnext;
        //free(buf);
        buf = sig;
    }
    return buf;
}
