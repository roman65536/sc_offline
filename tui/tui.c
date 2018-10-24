/* INCLUDES */
#include <signal.h> /* for signal */
#include <wchar.h>
#include <ncurses.h>
#include <stdlib.h> /* for malloc */
/* #include <wctype.h> */


/* DEFINES */
#define TIMEOUT_CURSES   300   // ms  curses input timeout
#define BUFFERSIZE      1024
#define sc_debug(x, ...)     ui_sc_msg(x, DEBUG_MSG, ##__VA_ARGS__)
#define DEBUG_MSG         19
#define RESROW           2     // rows reserved for prompt, error, and column numbers
#define RESCOL           4     // default terminal columns reserved for row numbers


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
void ui_show_header();
void ui_clr_header(int i);
void ui_show_sc_row_headings(WINDOW * win, int mxrow);
int calc_offscr_sc_rows();
void ui_show_sc_col_headings(WINDOW * win, int mxcol);
void ui_update(int header);
char * coltoa(int col);

/* GLOBAL VARIABLES */
WINDOW * main_win;
WINDOW * input_win;
SCREEN * sstdout;

struct block * buffer;
int return_value;          // return value of getch()
static wint_t wd;          // char read from stdin
static int d;              // char read from stdin

int currow = 0; /* Current row of the selected cell */
int curcol = 0; /* Current column of the selected cell */
int rescol = RESCOL; /* terminal columns reserved for displaying row numbers */
int shall_quit = 0;
int offscr_sc_rows = 0, offscr_sc_cols = 0; /* off screen spreadsheet rows and columns */



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

void ui_show_header() {
    ui_clr_header(0);
    ui_clr_header(1);
    mvwprintw(input_win, 0, rescol, " %d %d", currow, curcol);
    wrefresh(input_win);
    return;
}


void ui_clr_header(int i) {
    int row_orig, col_orig;
    getyx(input_win, row_orig, col_orig);
    if (col_orig > COLS) col_orig = COLS - 1;

    wmove(input_win, i, 0);
    wclrtoeol(input_win);

    // Return cursor to previous position
    wmove(input_win, row_orig, col_orig);

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
        case L'l':
            curcol++;
            //sc_debug("l");
            break;

        case L'h':
            if (curcol) curcol--;
            //sc_debug("h");
            break;

        case L'k':
            if (currow) currow--;
            //sc_debug("k");
            break;

        case L'j':
            currow++;
            //sc_debug("j");
            break;

        case L'q':
            shall_quit = 1;
            break;
    }
    ui_show_header();
    ui_update(1);
}


void ui_sc_msg(char * s, int type, ...) {
    char t[BUFFERSIZE];
    va_list args;
    va_start(args, type);
    vsprintf (t, s, args);


    mvwprintw(input_win, 1, 0, "%s", t);
    wclrtoeol(input_win);
    wrefresh(input_win);
    va_end(args);
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

void ui_update(int header) {
    /*
     * Calculate offscreen rows and columns
     *
     * mxcol is the last visible column in screen grid.
     * for instance, if mxcol is 8, last visible column would be I
     * mxrow is the last visible row in screen grid
     *
     * offscr_sc_cols are the number of columns left at the left of start of grid.
     * for instance, if offscr_sc_cols is 4. the first visible column in grid would be column E.
     *
     * there is a special behaviour when frozen columns or rows exists.
     * center_hidden_rows and center_hidden_columns are the number of rows and columns between
     * a frozen range and the first column or row visible.
     * example: if columns A and B are frozen, and center_hidden_cols is 4,
     * your grid would start with columns A, B, G, H..
     */
    int off_cols = calc_offscr_sc_cols();
    int off_rows = calc_offscr_sc_rows();
    int mxcol = offscr_sc_cols + off_cols - 1;
    int mxrow = offscr_sc_rows + off_rows - 1;


    //ui_show_content(main_win, mxrow, mxcol);

    // Show sc_col headings: A, B, C, D..
    ui_show_sc_col_headings(main_win, mxcol);

    // Show sc_row headings: 0, 1, 2, 3..
    ui_show_sc_row_headings(main_win, mxrow);

    // Refresh curses windows
    wrefresh(main_win);

}


void ui_show_sc_row_headings(WINDOW * win, int mxrow) {
    int row = 0;
    int i;
    for (i = 0; i <= mxrow; i++) {
        if (i == currow) wattron(win, A_REVERSE);
        mvwprintw (win, row+1, 0, "%*d ", rescol-1, i);
        wattroff(win, A_REVERSE);
        row++;
    }
    return;
}


void ui_show_sc_col_headings(WINDOW * win, int mxcol) {
    int FIXED_COLWIDTH = 12;
    int i, col = rescol;
    wmove(win, 0, 0);
    wclrtoeol(win);

    for (i = 0; i <= mxcol; i++) {
        int k = FIXED_COLWIDTH / 2;
        if (i == curcol) wattron(win, A_REVERSE);
        mvwprintw(win, 0, col, "%*s%-*s", k-1, " ", FIXED_COLWIDTH - k + 1, coltoa(i));

        col += FIXED_COLWIDTH;
        if (i == mxcol && COLS - col > 0) wclrtoeol(win);

        wattroff(win, A_REVERSE);
    }
}


int calc_offscr_sc_rows() {
    offscr_sc_rows = 0;
    return LINES - RESROW;
}

int calc_offscr_sc_cols() {
    offscr_sc_cols = 0;
    return COLS - RESCOL;
}


char * coltoa(int col) {
    static char rname[3];
    register char *p = rname;

    if (col > 25) {
        *p++ = col/26 + 'A' - 1;
        col %= 26;
    }
    *p++ = col+'A';
    *p = '\0';
    return (rname);
}

