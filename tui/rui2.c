/* INCLUDES */
#include <signal.h> /* for signal */
//#include <wchar.h>
#include <curses.h>
#include <wchar.h>
#include <stdlib.h> /* for malloc */
//#include <msgpack.h>
//#include <stdio.h>
//#include <string.h>
//#include <netinet/in.h>
//#include <fcntl.h>
#include <unistd.h>   /* for read */
#include <wctype.h> 
#include <rpsc.h>


/* DEFINES */
#define TIMEOUT_CURSES   300     // ms  curses input timeout
#define BUFFERSIZE      1024
#define sc_debug(x, ...)     ui_sc_msg(x, DEBUG_MSG, ##__VA_ARGS__)
#define DEBUG_MSG         19
#define LRESROW             2     // rows reserved for prompt, error
#define LRESCOL             4     // default terminal columns reserved for row numbers
#define PORT 1234
#define FIXED_COLWIDTH    12


/* STRUCTS */
struct block { /* Block of buffer */
  wint_t value;
  struct block * pnext;
};

// this is a dirty struct to handle a message
// will be removed later on when code generator is ready
typedef struct {
    int * id;
    int * ret;
    char * method;
    char * name;      // sheet_name
    int * row;
    int * col;
    int * to_row;
    int * to_col;
    short * bye;
    double * val;     // ent value
    char * label;     // ent label
    short * flag;     // ent flag
    char * formula;   // ent formula
} msg;

/* PROTOTYPES */
void sig_int();
void ui_start_screen();
void ui_stop_screen();
struct block * create_buf();
void addto_buf(struct block * buf, wint_t d);
void flush_buf (struct block * buf);
void erase_buf (struct block * buf);
void handle_input(struct roman* ,struct block * buffer);
void do_normalmode(struct roman *, struct block * buf);
void ui_sc_msg(char * s, int type, ...);
void ui_show_header();
void ui_clr_header(int i);
void ui_show_sc_row_headings(WINDOW * win, int mxrow);
void calc_offscr_sc_rows();
void calc_offscr_sc_cols();
void ui_show_sc_col_headings(WINDOW * win, int mxcol);
void ui_update(struct roman *, int header);
char * lcoltoa(int col);
void ui_show_content(struct roman *p,WINDOW * win, int mxrow, int mxcol);

void send_helo();
struct Sheet *  create_sheet(struct roman *p, char * name);
void remove_sheet();
int get_val(struct roman *, int row, int col, float * res);
int set_val(struct roman *,int row, int col, float val);
int set_label(struct roman *,int row, int col, char * val);
int get_label(struct roman *, int row, int col, char * val);
int bye();
int connect_to_server();
//void unpack_msg(msgpack_object o, msg * m);
void free_msg(msg * m);
void initialize_msg(msg * m);
void high_cursor(WINDOW * win);
void clean_cursor(WINDOW * win);


/* GLOBAL VARIABLES */
WINDOW * main_win;
WINDOW * input_win;
SCREEN * sstdout;

struct block * sbuffer;
int return_value;          // return value of getch()
static wint_t wd;          // char read from stdin
static int d;              // char read from stdin

int currow = 0; /* Current row of the selected cell */
int curcol = 0; /* Current column of the selected cell */
int lrescol = LRESCOL; /* terminal columns reserved for displaying row numbers */
int shall_quit = 0;
int offscr_sc_rows = 0, offscr_sc_cols = 0; /* off screen spreadsheet rows and columns */
int mxcol = 0;
int mxrow = 0;

//msgpack_sbuffer sbuf; /* buffer */
//msgpack_packer pk;    /* packer */
//msgpack_zone mempool;
int id_session, sock;
int valread;
char buffer[1024] = {0};


int main (int argc, char ** argv) {

    // first of all, handle SIGINT
    signal(SIGINT, sig_int);


slab_allocator_init();
ExpressionInit();
init_lib();
init_plugin();

load_plugin("html");
load_plugin("xlsx");
    struct roman * p = (struct roman *) malloc(sizeof(struct roman));
    struct Sheet *sh;
    struct Ent *tl;



    sh=create_sheet(p,"New Sheet1");

    //printf("\nsetting value\n");
    set_val(p,3, 2, 12.4);
    set_val(p,0, 0, 4);
    set_val(p,1, 0, 3);
    set_val(p,2, 2, 6.7);
    set_label(p,4, 4, "OKIDOKI");

    //sleep(5);
    //float f;
    //int res = get_val(0, 0, &f);
    //if (res == 0) printf("\ngetting value: %f.\n", f);

    // start ncurses
    ui_start_screen();
    offscr_sc_cols = 0;
    offscr_sc_rows = 0;

    // first update
    ui_update(p,1);

    // handle input from keyboard
    wchar_t stdin_buffer [BUFFERSIZE] = { L'\0' };
    sbuffer = (struct block *) create_buf(); // this should only take place if curses ui
    while ( ! shall_quit) {
        handle_input(p, sbuffer);
    }
    erase_buf(sbuffer);

    // stop ncurses
    ui_stop_screen();

    //printf("deleting sheet\n");
    remove_sheet();

    // send bye to server
    bye();

    // free structures
    //    msgpack_zone_destroy(&mempool);
    //msgpack_sbuffer_destroy(&sbuf);

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
    mvwprintw(input_win, 0, lrescol, " (%d, %d)", currow, curcol);
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


void handle_input(struct roman *p, struct block * buffer) {
    return_value = ui_getch(&wd);
    d = wd;

    if (return_value != -1) {
       addto_buf(buffer, wd);
    }

    do_normalmode(p, buffer);
    flush_buf(buffer);        // Flush the buffer
}


void do_normalmode(struct roman *p, struct block * buf) {
    switch (buf->value) {
        case L'l':
            {
            clean_cursor(main_win);
            curcol++;
            int i = offscr_sc_cols;
            calc_offscr_sc_cols();
            if (i != offscr_sc_cols) ui_show_content(p, main_win, mxrow, mxcol);
            high_cursor(main_win);

            // highlight new currow and curcol
            ui_show_sc_row_headings(main_win, mxrow);
            ui_show_sc_col_headings(main_win, mxcol);

            // update cell in header bar
            ui_show_header();
            }
            break;

        case L'h':
            {
            clean_cursor(main_win);
            if (curcol) curcol--;
            int i = offscr_sc_cols;
            calc_offscr_sc_cols();
            if (i != offscr_sc_cols) ui_show_content(p, main_win, mxrow, mxcol);
            high_cursor(main_win);

            // highlight new currow and curcol
            ui_show_sc_row_headings(main_win, mxrow);
            ui_show_sc_col_headings(main_win, mxcol);

            // update cell in header bar
            ui_show_header();
            }
            break;

        case L'k':
            {
            clean_cursor(main_win);
            if (currow) currow--;
            int before = offscr_sc_rows;
            calc_offscr_sc_rows();
            if (before != offscr_sc_rows) ui_show_content(p, main_win, mxrow, mxcol);
            high_cursor(main_win);

            // highlight new currow and curcol
            ui_show_sc_row_headings(main_win, mxrow);
            ui_show_sc_col_headings(main_win, mxcol);

            // update cell in header bar
            ui_show_header();
            }
            break;

        case L'j':
            {
            clean_cursor(main_win);
            currow++;
            int before = offscr_sc_rows;
            calc_offscr_sc_rows();
            if (before != offscr_sc_rows) ui_show_content(p, main_win, mxrow, mxcol);
            high_cursor(main_win);

            // highlight new currow and curcol
            ui_show_sc_row_headings(main_win, mxrow);
            ui_show_sc_col_headings(main_win, mxcol);

            // update cell in header bar
            ui_show_header();
            }
            break;

        case L'u':
            ui_update(p,1);
            break;

        case L'a':
            /*
            ;
            calc_offscr_sc_cols();
            calc_offscr_sc_rows();
            ui_show_content(main_win, mxrow, mxcol);
            */
            break;
        case L'q':
            shall_quit = 1;
            break;
    }
}


void ui_sc_msg(char * s, int type, ...) {
    char t[BUFFERSIZE];
    va_list args;
    va_start(args, type);
    vsprintf (t, s, args);

    mvwprintw(input_win, 1, 0, "%s", t);
    wclrtoeol(input_win);
        wtimeout(input_win, -1);
        wgetch(input_win);
        wtimeout(input_win, TIMEOUT_CURSES);
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



void ui_update(struct roman *p, int header) {
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
    calc_offscr_sc_cols();
    calc_offscr_sc_rows();

    ui_show_content(p,main_win, mxrow, mxcol);

    // Show sc_col headings: A, B, C, D..
    ui_show_sc_col_headings(main_win, mxcol);

    // Show sc_row headings: 0, 1, 2, 3..
    ui_show_sc_row_headings(main_win, mxrow);

    // Refresh curses windows
    wrefresh(main_win);

}


void ui_show_content(struct roman *p, WINDOW * win, int mxrow, int mxcol) {

    // Clean from top to bottom
    wmove(win, 0, 0);
    wclrtobot(win);

    int ri = offscr_sc_rows;
    int ci = offscr_sc_cols;
    int r, c;
    struct Ent **pp;
   

    for (r=ri; r<=mxrow; r++) {
        for (c=ci; c<=mxcol; c++) {
	    struct Ent *t;
	    pp=ATBL(p->cur_sh,NULL,r,c);
	    if (pp == 0) continue;
	    t=*pp;
	    if (t == 0) continue;
	    //t=lookat(p->cur_sh,r,c);
	            wattroff(win, A_REVERSE);
                    if ((currow == r) && (curcol == c)) wattron(win, A_REVERSE);

                    //sc_debug("ri:%d, ci:%d, mxrow:%d, mxcol:%d", ri, ci, mxrow, mxcol);
                    if ( (t->flag & VAL)) {
                      
                        mvwprintw(win, r - offscr_sc_rows + LRESROW - 1, FIXED_COLWIDTH * (c - offscr_sc_cols) + LRESCOL, "%f", t->val);
                    } else if ( t->flag & RP_LABEL) {
                       
                        mvwprintw(win, r - offscr_sc_rows + LRESROW - 1, FIXED_COLWIDTH * (c - offscr_sc_cols) + LRESCOL, "%s", t->label);
                    }
		    /*else if (currow == r && curcol == c) {
                        wchar_t w = L' ';
                        int i;
                        for (i = 0; i < FIXED_COLWIDTH; i++)
                            mvwprintw(win, r - offscr_sc_rows + RESROW - 1, FIXED_COLWIDTH * (c - offscr_sc_cols) + i + RESCOL, "%lc", w);
                    }
		    */
                    wclrtoeol(win);
                }
               
            }
            
            wattroff(win, A_REVERSE);
      
    wrefresh(win);
    return;
}


int bye() {

}

// TODO should return 0 when 0 return status is received from server
int set_val(struct roman *p,int row, int col, float val) {
 struct Ent *t;

 t=lookat(p->cur_sh,row,col);
 t->val=val;
 t->flag |= VAL;

}

// TODO return 0 when ok. -1 when not found/error
int get_val(struct roman *p,int row, int col, float * res) {
   struct Ent *t;
    t=lookat(p->cur_sh,row,col);
    if (t->flag & VAL)
	{
	    *res=t->val;
	    return 1;
	}
    else return 0;
    
}

// TODO should return 0 when 0 return status is received from server
int set_label(struct roman *p, int row, int col, char * label) {

     struct Ent *t;
    t=lookat(p->cur_sh,row,col);
    t->label=strdup(label);
    t->flag |= RP_LABEL;

    
}

// TODO return 0 when ok. -1 when not found/error
int get_label(struct roman *p , int row, int col, char * label) {
     struct Ent *t;
    t=lookat(p->cur_sh,row,col);
    if (t->flag & RP_LABEL)
	{
	    label=t->label;
	    return 1;
	}
    else return 0;
}








// TODO return 0 when ok. -1 when not found/error
struct Sheet * create_sheet(struct roman *p, char * name) {
struct Sheet * new=new_sheet(p,name);
p->cur_sh=new;
return new;

}

// TODO return 0 when ok. -1 when not found/error
void remove_sheet() {

}





























void clean_cursor(WINDOW * win) {
    wattroff(win, A_REVERSE);

    cchar_t cht[FIXED_COLWIDTH];
    wchar_t w;
    int i, j;
    for (i = 0; i < FIXED_COLWIDTH; ) {
        w = L' ';
        j = mvwin_wchnstr (win, currow - offscr_sc_rows + LRESROW - 1, (FIXED_COLWIDTH * (curcol - offscr_sc_cols)) + LRESCOL + i, cht, 1);
        if (j == OK && cht[0].chars[0] != L'\0') w = cht[0].chars[0];
        mvwprintw(win, currow - offscr_sc_rows + LRESROW - 1, (FIXED_COLWIDTH * (curcol - offscr_sc_cols)) + LRESCOL + i, "%lc", w);
        i+= wcwidth(w);
    }
    return;
}


void high_cursor(WINDOW * win) {
    wattron(win, A_REVERSE);

    cchar_t cht[FIXED_COLWIDTH];
    wchar_t w;
    int i, j;
    //sc_debug("currow:%d off:%d", currow, offscr_sc_rows);

    for (i = 0; i < FIXED_COLWIDTH; ) {
        w = L' ';
        j = mvwin_wchnstr (win, currow - offscr_sc_rows + LRESROW - 1, FIXED_COLWIDTH * (curcol - offscr_sc_cols) + LRESCOL + i, cht, 1);
        if (j == OK && cht[0].chars[0] != L'\0') w = cht[0].chars[0];
        mvwprintw(win, currow - offscr_sc_rows + LRESROW - 1, (FIXED_COLWIDTH * (curcol - offscr_sc_cols)) + LRESCOL + i, "%lc", w);
        i+=wcwidth(w);
    }
    wattroff(win, A_REVERSE);
    wrefresh(win);
    return;
}

void ui_show_sc_row_headings(WINDOW * win, int mxrow) {
    int i, row = 0;
    for (i = offscr_sc_rows; i <= mxrow; i++, row++) {
        if (i == currow) wattron(win, A_REVERSE);
        mvwprintw (win, row+1, 0, "%*d ", lrescol-1, i);
        wattroff(win, A_REVERSE);
    }
    wrefresh(win);
    return;
}


void ui_show_sc_col_headings(WINDOW * win, int mxcol) {
    int i, col = lrescol;
    wmove(win, 0, 0);
    wclrtoeol(win);

    //sc_debug("curcol:%d, off:%d mxcol:%d", curcol, offscr_sc_cols, mxcol);
    for (i = offscr_sc_cols; i <= mxcol; i++) {
        int k = FIXED_COLWIDTH / 2;
        if (i == curcol) wattron(win, A_REVERSE);
        mvwprintw(win, 0, col, "%*s%-*s", k-1, " ", FIXED_COLWIDTH - k + 1, lcoltoa(i));

        col += FIXED_COLWIDTH;
        if (i == mxcol && COLS - col > 0) wclrtoeol(win);

        wattroff(win, A_REVERSE);
    }
    wrefresh(win);
}


void calc_offscr_sc_rows() {
    if (offscr_sc_rows && currow >= offscr_sc_rows && currow <= mxrow) return;
    else if (offscr_sc_rows && currow == offscr_sc_rows - 1) { offscr_sc_rows--; mxrow--; return; }
    offscr_sc_rows = 0;
    mxrow = LINES - LRESROW - 2;
    mxrow = 25;
    for (; currow > mxrow; offscr_sc_rows++, mxrow++) ;
    return;
}

void calc_offscr_sc_cols() {
    if (offscr_sc_cols && curcol >= offscr_sc_cols && curcol <= mxcol) return;
    else if (offscr_sc_cols && curcol == offscr_sc_cols - 1) { offscr_sc_cols--; mxcol--; return; }
    offscr_sc_cols = 0;
    mxcol = (COLS - LRESCOL) / FIXED_COLWIDTH - 1;
    for (; curcol > mxcol; offscr_sc_cols++, mxcol++) ;
    return;
}


char * lcoltoa(int col) {
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
