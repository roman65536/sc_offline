/* INCLUDES */
#include <signal.h> /* for signal */
#include <wchar.h>
#include <ncurses.h>
#include <stdlib.h> /* for malloc */
#include <sys/socket.h>
#include <msgpack.h>
//#include <stdio.h>
//#include <string.h>
//#include <netinet/in.h>
//#include <fcntl.h>
#include <unistd.h>   /* for read */
/* #include <wctype.h> */


/* DEFINES */
#define TIMEOUT_CURSES   300     // ms  curses input timeout
#define BUFFERSIZE      1024
#define sc_debug(x, ...)     ui_sc_msg(x, DEBUG_MSG, ##__VA_ARGS__)
#define DEBUG_MSG         19
#define RESROW             2     // rows reserved for prompt, error
#define RESCOL             4     // default terminal columns reserved for row numbers
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
void handle_input(struct block * buffer);
void do_normalmode(struct block * buf);
void ui_sc_msg(char * s, int type, ...);
void ui_show_header();
void ui_clr_header(int i);
void ui_show_sc_row_headings(WINDOW * win, int mxrow);
void calc_offscr_sc_rows();
void calc_offscr_sc_cols();
void ui_show_sc_col_headings(WINDOW * win, int mxcol);
void ui_update(int header);
char * coltoa(int col);
void ui_show_content(WINDOW * win, int mxrow, int mxcol);

void send_helo();
void create_sheet();
void remove_sheet();
int get_val(int row, int col, float * res);
int set_val(int row, int col, float val);
int set_label(int row, int col, char * val);
int get_label(int row, int col, char * val);
int bye();
int connect_to_server();
void unpack_msg(msgpack_object o, msg * m);
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
int rescol = RESCOL; /* terminal columns reserved for displaying row numbers */
int shall_quit = 0;
int offscr_sc_rows = 0, offscr_sc_cols = 0; /* off screen spreadsheet rows and columns */
int mxcol = 0;
int mxrow = 0;

msgpack_sbuffer sbuf; /* buffer */
msgpack_packer pk;    /* packer */
msgpack_zone mempool;
int id_session, sock;
int valread;
char buffer[1024] = {0};

/***************************************************************************/
int connect_to_server() {
    struct sockaddr_in address;
    struct sockaddr_in serv_addr;

    sock = 0;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("Socket creation error\n");
        return -1;
    }

    memset(&serv_addr, '0', sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr)<=0) {
        printf("Invalid address/ Address not supported\n");
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("Connection Failed\n");
        return -1;
    }

    /* Put the socket in non-blocking mode:
    if(fcntl(sock, F_SETFL, fcntl(sock, F_GETFL) | O_NONBLOCK) < 0) {
        printf("cannot set socket in non blocking mode");
        return -1;
    }*/

    valread = read(sock, buffer, 1024);
    if (valread != -1) {
        printf("got server inf.: %s\n", buffer);
        buffer[0]='\0';
    } else
        printf("valread :%d\n", valread);

    msgpack_sbuffer_init(&sbuf); /* initialize buffer */
    msgpack_packer_init(&pk, &sbuf, msgpack_sbuffer_write); /* initialize packer */

    /* deserialize the buffer into msgpack_object instance. */
    /* deserialized object is valid during the msgpack_zone instance alive. */
    msgpack_zone_init(&mempool, 1024);
}


int main (int argc, char ** argv) {

    // first of all, handle SIGINT
    signal(SIGINT, sig_int);

    // conect to server
    if (connect_to_server() == -1) return -1;

    //printf("sending helo\n");
    send_helo();

    //printf("creating sheet\n");
    create_sheet();

    //printf("\nsetting value\n");
    set_val(3, 2, 12.4);
    set_val(0, 0, 4);
    set_val(1, 0, 3);
    set_val(2, 2, 6.7);
    set_label(4, 4, "OKIDOKI");

    //sleep(5);
    //float f;
    //int res = get_val(0, 0, &f);
    //if (res == 0) printf("\ngetting value: %f.\n", f);

    // start ncurses
    ui_start_screen();
    offscr_sc_cols = 0;
    offscr_sc_rows = 0;

    // first update
    ui_update(1);

    // handle input from keyboard
    wchar_t stdin_buffer [BUFFERSIZE] = { L'\0' };
    sbuffer = (struct block *) create_buf(); // this should only take place if curses ui
    while ( ! shall_quit) {
        handle_input(sbuffer);
    }
    erase_buf(sbuffer);

    // stop ncurses
    ui_stop_screen();

    //printf("deleting sheet\n");
    remove_sheet();

    // send bye to server
    bye();

    // free structures
    msgpack_zone_destroy(&mempool);
    msgpack_sbuffer_destroy(&sbuf);

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
    mvwprintw(input_win, 0, rescol, " (%d, %d)", currow, curcol);
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
            {
            clean_cursor(main_win);
            curcol++;
            int i = offscr_sc_cols;
            calc_offscr_sc_cols();
            if (i != offscr_sc_cols) ui_show_content(main_win, mxrow, mxcol);
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
            if (i != offscr_sc_cols) ui_show_content(main_win, mxrow, mxcol);
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
            if (before != offscr_sc_rows) ui_show_content(main_win, mxrow, mxcol);
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
            if (before != offscr_sc_rows) ui_show_content(main_win, mxrow, mxcol);
            high_cursor(main_win);

            // highlight new currow and curcol
            ui_show_sc_row_headings(main_win, mxrow);
            ui_show_sc_col_headings(main_win, mxcol);

            // update cell in header bar
            ui_show_header();
            }
            break;

        case L'u':
            ui_update(1);
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
    calc_offscr_sc_cols();
    calc_offscr_sc_rows();

    ui_show_content(main_win, mxrow, mxcol);

    // Show sc_col headings: A, B, C, D..
    ui_show_sc_col_headings(main_win, mxcol);

    // Show sc_row headings: 0, 1, 2, 3..
    ui_show_sc_row_headings(main_win, mxrow);

    // Refresh curses windows
    wrefresh(main_win);

}


void ui_show_content(WINDOW * win, int mxrow, int mxcol) {

    // Clean from top to bottom
    wmove(win, 0, 0);
    wclrtobot(win);

    int ri = offscr_sc_rows;
    int ci = offscr_sc_cols;
    int r, c;

    msgpack_unpacker unp;
    bool result;

    for (r=ri; r<=mxrow; r++) {
        for (c=ci; c<=mxcol; c++) {
            result = msgpack_unpacker_init(&unp, 1024);

            msgpack_pack_map(&pk, 3);

            msgpack_pack_str(&pk, 2);
            msgpack_pack_str_body(&pk, "id", 2);
            msgpack_pack_short(&pk, id_session);

            msgpack_pack_str(&pk, 6);
            msgpack_pack_str_body(&pk, "method", 6);
            msgpack_pack_str(&pk, 9);
            msgpack_pack_str_body(&pk, "get_cell", 9);

            msgpack_pack_str(&pk, 6);
            msgpack_pack_str_body(&pk, "params", 6);
            msgpack_pack_map(&pk, 2);
            //msgpack_pack_map(&pk, 4);

            msgpack_pack_str(&pk, 3);
            msgpack_pack_str_body(&pk, "row", 3);
            msgpack_pack_int(&pk, r);

            msgpack_pack_str(&pk, 3);
            msgpack_pack_str_body(&pk, "col", 3);
            msgpack_pack_int(&pk, c);

            /*
            msgpack_pack_str(&pk, 6);
            msgpack_pack_str_body(&pk, "to_row", 6);
            msgpack_pack_int(&pk, mxrow);

            msgpack_pack_str(&pk, 6);
            msgpack_pack_str_body(&pk, "to_col", 6);
            msgpack_pack_int(&pk, mxcol);
            */

            send(sock, sbuf.data, sbuf.size, 0 );
            msgpack_sbuffer_clear(&sbuf);

            //if (result) sc_debug("\nok init\n");

            // get cells content
            valread = read(sock, buffer, 1024);
            if (valread > 0) {
                memcpy(msgpack_unpacker_buffer(&unp), buffer, 1024);
                msgpack_unpacker_buffer_consumed(&unp, 1024); //FIXME
                msgpack_unpacked und;
                msgpack_unpack_return ret;
                msgpack_unpacked_init(&und);
                ret = msgpack_unpacker_next(&unp, &und);
                //if (ret == MSGPACK_UNPACK_SUCCESS) sc_debug("ok unpacked\n");
                msgpack_object q = und.data;

                //msgpack_object_print(stdout, q);
                msg m;
                initialize_msg(&m);
                if (q.type == MSGPACK_OBJECT_MAP) {
                    unpack_msg(q, &m); // unpack object received (q) to (msg) m
                    wattroff(win, A_REVERSE);
                    if ((currow == r) && (curcol == c)) wattron(win, A_REVERSE);

                    //sc_debug("ri:%d, ci:%d, mxrow:%d, mxcol:%d", ri, ci, mxrow, mxcol);
                    if (*(m.ret) == 0 && m.val != NULL) {
                        r = *(m.row);
                        c = *(m.col);
                        mvwprintw(win, r - offscr_sc_rows + RESROW - 1, FIXED_COLWIDTH * (c - offscr_sc_cols) + RESCOL, "%f", *(m.val));
                    } else if (*(m.ret) == 0 && m.label != NULL) {
                        r = *(m.row);
                        c = *(m.col);
                        mvwprintw(win, r - offscr_sc_rows + RESROW - 1, FIXED_COLWIDTH * (c - offscr_sc_cols) + RESCOL, "%s", m.label);
                    } else if (currow == r && curcol == c) {
                        wchar_t w = L' ';
                        int i;
                        for (i = 0; i < FIXED_COLWIDTH; i++)
                            mvwprintw(win, r - offscr_sc_rows + RESROW - 1, FIXED_COLWIDTH * (c - offscr_sc_cols) + i + RESCOL, "%lc", w);
                    }
                    wclrtoeol(win);
                }
                free_msg(&m);
                msgpack_unpacked_destroy(&und);

            }
            msgpack_unpacker_destroy(&unp);
            wattroff(win, A_REVERSE);
        }
    }
    wrefresh(win);
    return;
}


int bye() {
    msgpack_pack_map(&pk, 2);

    msgpack_pack_str(&pk, 2);
    msgpack_pack_str_body(&pk, "id", 2);
    msgpack_pack_short(&pk, id_session);

    msgpack_pack_str(&pk, 6);
    msgpack_pack_str_body(&pk, "method", 6);
    msgpack_pack_str(&pk, 3);
    msgpack_pack_str_body(&pk, "bye", 3);

    send(sock, sbuf.data, sbuf.size, 0 );
    msgpack_sbuffer_clear(&sbuf);

    //get OK return status
    valread = read(sock, buffer, 1024);
    if (valread > 0) {
        msgpack_object p;

        //FIXME sizeof(..) may differ on sender and receiver machine
        msgpack_unpack(buffer, sizeof(buffer), NULL, &mempool, &p);

        //msgpack_object_print(stdout, p);

        // return 0
        if ( p.type == MSGPACK_OBJECT_MAP && p.via.map.size == 2 &&
           ! strncmp(p.via.map.ptr->key.via.str.ptr, "ret", 3) &&
           ((int) p.via.map.ptr->val.via.u64 == 0)) {
            return 0;
        }
    } else if (valread < 0) {
        sc_debug("nothing to read in socket");
        return -2;
    }
    return -1;
}

// TODO should return 0 when 0 return status is received from server
int set_val(int row, int col, float val) {
    msgpack_pack_map(&pk, 3);

    msgpack_pack_str(&pk, 2);
    msgpack_pack_str_body(&pk, "id", 2);
    msgpack_pack_short(&pk, id_session);

    msgpack_pack_str(&pk, 6);
    msgpack_pack_str_body(&pk, "method", 6);
    msgpack_pack_str(&pk, 7);
    msgpack_pack_str_body(&pk, "set_val", 7);

    msgpack_pack_str(&pk, 6);
    msgpack_pack_str_body(&pk, "params", 6);
    msgpack_pack_map(&pk, 3);

    msgpack_pack_str(&pk, 3);
    msgpack_pack_str_body(&pk, "row", 3);
    msgpack_pack_int(&pk, row);

    msgpack_pack_str(&pk, 3);
    msgpack_pack_str_body(&pk, "col", 3);
    msgpack_pack_int(&pk, col);

    msgpack_pack_str(&pk, 3);
    msgpack_pack_str_body(&pk, "val", 3);
    msgpack_pack_float(&pk, val);
    send(sock, sbuf.data, sbuf.size, 0 );
    msgpack_sbuffer_clear(&sbuf);

    //get OK return status
    valread = read(sock, buffer, 1024);
    if (valread > 0) {
        msgpack_object p;

        //FIXME sizeof(..) may differ on sender and receiver machine
        msgpack_unpack(buffer, sizeof(buffer), NULL, &mempool, &p);

        //msgpack_object_print(stdout, p);

        // return 0
        if ( p.type == MSGPACK_OBJECT_MAP && p.via.map.size == 2 &&
           ! strncmp(p.via.map.ptr->key.via.str.ptr, "ret", 3) &&
           ((int) p.via.map.ptr->val.via.u64 == 0)) {
            return 0;
        }
    } else if (valread < 0) {
        sc_debug("nothing to read in socket\n");
        return -2;
    }
    return -1;
}

// TODO return 0 when ok. -1 when not found/error
int get_val(int row, int col, float * res) {
    int found=0;

    msgpack_pack_map(&pk, 3);

    msgpack_pack_str(&pk, 2);
    msgpack_pack_str_body(&pk, "id", 2);
    msgpack_pack_short(&pk, id_session);

    msgpack_pack_str(&pk, 6);
    msgpack_pack_str_body(&pk, "method", 6);
    msgpack_pack_str(&pk, 7);
    msgpack_pack_str_body(&pk, "get_val", 7);

    msgpack_pack_str(&pk, 6);
    msgpack_pack_str_body(&pk, "params", 6);
    msgpack_pack_map(&pk, 2);

    msgpack_pack_str(&pk, 3);
    msgpack_pack_str_body(&pk, "row", 3);
    msgpack_pack_int(&pk, row);

    msgpack_pack_str(&pk, 3);
    msgpack_pack_str_body(&pk, "col", 3);
    msgpack_pack_int(&pk, col);

    send(sock, sbuf.data, sbuf.size, 0 );
    msgpack_sbuffer_clear(&sbuf);

    msgpack_unpacker unp;
    bool result = msgpack_unpacker_init(&unp, 1024);
    //if (result) sc_debug("\nok init\n");

    // get value
    valread = read(sock, buffer, 1024);
    if (valread > 0) {
        memcpy(msgpack_unpacker_buffer(&unp), buffer, 1024);
        msgpack_unpacker_buffer_consumed(&unp, 1024);
        msgpack_unpacked und;
        msgpack_unpack_return ret;
        msgpack_unpacked_init(&und);
        ret = msgpack_unpacker_next(&unp, &und);
        //if (ret == MSGPACK_UNPACK_SUCCESS) sc_debug("ok unpacked\n");
        msgpack_object q = und.data;

        //msgpack_object_print(stdout, q);

        msg m;
        initialize_msg(&m);
        if (q.type == MSGPACK_OBJECT_MAP) {
            unpack_msg(q, &m); /* unpack object received (q) to (msg) m */
            *res = *(m.val);
            found = 1;
        }
        free_msg(&m);

        msgpack_unpacked_destroy(&und);
    }
    msgpack_unpacker_destroy(&unp);
    return found ? 0 : -1;
}

// TODO should return 0 when 0 return status is received from server
int set_label(int row, int col, char * label) {
    msgpack_pack_map(&pk, 3);

    msgpack_pack_str(&pk, 2);
    msgpack_pack_str_body(&pk, "id", 2);
    msgpack_pack_short(&pk, id_session);

    msgpack_pack_str(&pk, 6);
    msgpack_pack_str_body(&pk, "method", 6);
    msgpack_pack_str(&pk, 9);
    msgpack_pack_str_body(&pk, "set_label", 9);

    msgpack_pack_str(&pk, 6);
    msgpack_pack_str_body(&pk, "params", 6);
    msgpack_pack_map(&pk, 3);

    msgpack_pack_str(&pk, 3);
    msgpack_pack_str_body(&pk, "row", 3);
    msgpack_pack_int(&pk, row);

    msgpack_pack_str(&pk, 3);
    msgpack_pack_str_body(&pk, "col", 3);
    msgpack_pack_int(&pk, col);

    msgpack_pack_str(&pk, 5);
    msgpack_pack_str_body(&pk, "label", 5);
    msgpack_pack_str(&pk, strlen(label));
    msgpack_pack_str_body(&pk, label, strlen(label));

    send(sock, sbuf.data, sbuf.size, 0 );
    msgpack_sbuffer_clear(&sbuf);

    //get OK return status
    valread = read(sock, buffer, 1024);
    if (valread > 0) {
        msgpack_object p;

        //FIXME sizeof(..) may differ on sender and receiver machine
        msgpack_unpack(buffer, sizeof(buffer), NULL, &mempool, &p);

        //msgpack_object_print(stdout, p);

        // return 0
        if ( p.type == MSGPACK_OBJECT_MAP && p.via.map.size == 2 &&
           ! strncmp(p.via.map.ptr->key.via.str.ptr, "ret", 3) &&
           ((int) p.via.map.ptr->val.via.u64 == 0)) {
            return 0;
        }
    } else if (valread < 0) {
        sc_debug("nothing to read in socket\n");
        return -2;
    }
    return -1;
}

// TODO return 0 when ok. -1 when not found/error
int get_label(int row, int col, char * label) {
    int found=0;

    msgpack_pack_map(&pk, 3);

    msgpack_pack_str(&pk, 2);
    msgpack_pack_str_body(&pk, "id", 2);
    msgpack_pack_short(&pk, id_session);

    msgpack_pack_str(&pk, 6);
    msgpack_pack_str_body(&pk, "method", 6);
    msgpack_pack_str(&pk, 9);
    msgpack_pack_str_body(&pk, "get_label", 9);

    msgpack_pack_str(&pk, 6);
    msgpack_pack_str_body(&pk, "params", 6);
    msgpack_pack_map(&pk, 2);

    msgpack_pack_str(&pk, 3);
    msgpack_pack_str_body(&pk, "row", 3);
    msgpack_pack_int(&pk, row);

    msgpack_pack_str(&pk, 3);
    msgpack_pack_str_body(&pk, "col", 3);
    msgpack_pack_int(&pk, col);

    send(sock, sbuf.data, sbuf.size, 0 );
    msgpack_sbuffer_clear(&sbuf);

    msgpack_unpacker unp;
    bool result = msgpack_unpacker_init(&unp, 1024);
    //if (result) sc_debug("\nok init\n");

    // get value
    valread = read(sock, buffer, 1024);
    if (valread > 0) {
        memcpy(msgpack_unpacker_buffer(&unp), buffer, 1024);
        msgpack_unpacker_buffer_consumed(&unp, 1024);
        msgpack_unpacked und;
        msgpack_unpack_return ret;
        msgpack_unpacked_init(&und);
        ret = msgpack_unpacker_next(&unp, &und);
        //if (ret == MSGPACK_UNPACK_SUCCESS) sc_debug("ok unpacked\n");
        msgpack_object q = und.data;

        //msgpack_object_print(stdout, q);

        msg m;
        initialize_msg(&m);
        if (q.type == MSGPACK_OBJECT_MAP) {
            unpack_msg(q, &m); /* unpack object received (q) to (msg) m */
            label = malloc(sizeof(char) * strlen(m.label)+1);
            strcpy(label, m.label);
            found = 1;
        }
        free_msg(&m);

        msgpack_unpacked_destroy(&und);
    }
    msgpack_unpacker_destroy(&unp);
    return found ? 0 : -1;
}







// TODO return 0 when ok. -1 when not found/error
void send_helo() {
    // pack HELO
    msgpack_pack_str(&pk, 4);
    msgpack_pack_str_body(&pk, "HELO", 4); // this requests a new session

    // send HELO
    send(sock, sbuf.data, sbuf.size, 0 );
    msgpack_sbuffer_clear(&sbuf);

    // get session ID
    valread = read(sock, buffer, 1024);
    if (valread > 0) {
        msgpack_object o;

        //FIXME sizeof(..) may differ on sender and receiver machine
        msgpack_unpack(buffer, sizeof(buffer), NULL, &mempool, &o);

        //msgpack_object_print(stdout, o);

        if (o.type == MSGPACK_OBJECT_MAP && o.via.map.size == 1 && ! strncmp(o.via.map.ptr->key.via.str.ptr, "id", 2)) {
            id_session = (int) o.via.map.ptr->val.via.u64;
            printf("got session #: %d.\n", id_session);
        }
    }
}

// TODO return 0 when ok. -1 when not found/error
void create_sheet() {
    //msgpack_sbuffer_clear(&sbuf);
    msgpack_pack_map(&pk, 3);
    msgpack_pack_str(&pk, 2);
    msgpack_pack_str_body(&pk, "id", 2);
    msgpack_pack_int(&pk, id_session);
    msgpack_pack_str(&pk, 6);
    msgpack_pack_str_body(&pk, "method", 6);
    msgpack_pack_str(&pk, 12);
    msgpack_pack_str_body(&pk, "create_sheet", 12);
    msgpack_pack_str(&pk, 6);
    msgpack_pack_str_body(&pk, "params", 6);
    msgpack_pack_map(&pk, 1);
    msgpack_pack_str(&pk, 4);
    msgpack_pack_str_body(&pk, "name", 4);
    msgpack_pack_str(&pk, 6);
    msgpack_pack_str_body(&pk, "sheet1", 6);

    send(sock, sbuf.data, sbuf.size, 0 );
    msgpack_sbuffer_clear(&sbuf);

    // get OK
    valread = read(sock, buffer, 1024);
    if (valread > 0) {
        msgpack_object o;
        msgpack_unpack(buffer, sizeof(buffer), NULL, &mempool, &o);
        //msgpack_object_print(stdout, o);
    }
}

// TODO return 0 when ok. -1 when not found/error
void remove_sheet() {
    msgpack_pack_map(&pk, 3);
    msgpack_pack_str(&pk, 2);
    msgpack_pack_str_body(&pk, "id", 2);
    msgpack_pack_int(&pk, id_session);
    msgpack_pack_str(&pk, 6);
    msgpack_pack_str_body(&pk, "method", 6);
    msgpack_pack_str(&pk, 12);
    msgpack_pack_str_body(&pk, "delete_sheet", 12);
    msgpack_pack_str(&pk, 6);
    msgpack_pack_str_body(&pk, "params", 6);
    msgpack_pack_map(&pk, 1);
    msgpack_pack_str(&pk, 4);
    msgpack_pack_str_body(&pk, "name", 4);
    msgpack_pack_str(&pk, 6);
    msgpack_pack_str_body(&pk, "sheet1", 6);

    send(sock, sbuf.data, sbuf.size, 0 );
    msgpack_sbuffer_clear(&sbuf);

    // get OK
    valread = read(sock, buffer, 1024);
    if (valread > 0) {
        msgpack_object o;
        msgpack_unpack(buffer, sizeof(buffer), NULL, &mempool, &o);
        //msgpack_object_print(stdout, o);
    }
    return;
}

/* unpack a msg from a map in a msgpack object */
void unpack_msg(msgpack_object o, msg * m) {
    int size = o.via.map.size;

    msgpack_object_kv * p = o.via.map.ptr;
    msgpack_object_kv * pend = o.via.map.ptr + o.via.map.size;

    while (p < pend) {
        if (p->key.type == MSGPACK_OBJECT_STR) {
        //printf("key: %.*s\n", p->key.via.str.size, p->key.via.str.ptr);

            if (! strncmp(p->key.via.str.ptr, "id", p->key.via.str.size)) {
                m->id = (int *) malloc(sizeof(int));
                *(m->id) = p->val.via.u64;
            } else if (! strncmp(p->key.via.str.ptr, "row", p->key.via.str.size)) {
                m->row = (int *) malloc(sizeof(int));
                *(m->row) = p->val.via.u64;
            } else if (! strncmp(p->key.via.str.ptr, "ret", p->key.via.str.size)) {
                m->ret = (int *) malloc(sizeof(int));
                *(m->ret) = p->val.via.u64;
            } else if (! strncmp(p->key.via.str.ptr, "col", p->key.via.str.size)) {
                m->col = (int *) malloc(sizeof(int));
                *(m->col) = p->val.via.u64;
          /*} else if (! strncmp(p->key.via.str.ptr, "to_row", p->key.via.str.size)) {
                m->to_row = (int *) malloc(sizeof(int));
                *(m->to_row) = p->val.via.u64;
            } else if (! strncmp(p->key.via.str.ptr, "to_col", p->key.via.str.size)) {
                m->to_col = (int *) malloc(sizeof(int));
                *(m->to_col) = p->val.via.u64;*/
            } else if (! strncmp(p->key.via.str.ptr, "bye", p->key.via.str.size)) {
                m->bye = (short *) malloc(sizeof(short));
                *(m->bye) = p->val.via.u64;
            } else if (! strncmp(p->key.via.str.ptr, "val", p->key.via.str.size)) {
                m->val = (double *) malloc(sizeof(double));
                *(m->val) = p->val.via.f64;
            } else if (! strncmp(p->key.via.str.ptr, "flag", p->key.via.str.size)) {
                m->flag = (short *) malloc(sizeof(short));
                *(m->flag) = p->val.via.u64;
            } else if (! strncmp(p->key.via.str.ptr, "formula", p->key.via.str.size)) {
                m->formula = (char *) malloc(sizeof(char) * (p->val.via.str.size + 1));
                sprintf(m->formula, "%.*s", p->val.via.str.size, p->val.via.str.ptr);
            } else if (! strncmp(p->key.via.str.ptr, "method", p->key.via.str.size)) {
                m->method = (char *) malloc(sizeof(char) * (p->val.via.str.size + 1));
                sprintf(m->method, "%.*s", p->val.via.str.size, p->val.via.str.ptr);
            } else if (! strncmp(p->key.via.str.ptr, "name", p->key.via.str.size)) {
                m->name = (char *) malloc(sizeof(char) * (p->val.via.str.size + 1));
                sprintf(m->name, "%.*s", p->val.via.str.size, p->val.via.str.ptr);
            } else if (! strncmp(p->key.via.str.ptr, "label", p->key.via.str.size)) {
                m->label = (char *) malloc(sizeof(char) * (p->val.via.str.size + 1));
                sprintf(m->label, "%.*s", p->val.via.str.size, p->val.via.str.ptr);
            } else if (! strncmp(p->key.via.str.ptr, "cell", p->key.via.str.size)) {
                unpack_msg(p->val, m);
            }

        } else if (p->val.type == MSGPACK_OBJECT_MAP) {
            unpack_msg(p->val, m);
        }
        p++;
    }
}

void free_msg(msg * m) {
    if (m->method != NULL)  free(m->method);
    if (m->name != NULL)    free(m->name);
    if (m->id  != NULL)     free(m->id);
    if (m->ret != NULL)     free(m->ret);
    if (m->row != NULL)     free(m->row);
    if (m->col != NULL)     free(m->col);
    if (m->to_row != NULL)  free(m->to_row);
    if (m->to_col != NULL)  free(m->to_col);
    if (m->bye != NULL)     free(m->bye);
    if (m->val != NULL)     free(m->val);
    if (m->label != NULL)   free(m->label);
    if (m->formula != NULL) free(m->formula);
    if (m->flag != NULL)    free(m->flag);
    return;
}

void initialize_msg(msg * m) {
    m->method = NULL;
    m->name = NULL;
    m->id  = NULL;
    m->ret = NULL;
    m->row = NULL;
    m->col = NULL;
    m->to_row = NULL;
    m->to_col = NULL;
    m->val = NULL;
    m->label = NULL;
    m->formula = NULL;
    m->flag = NULL;
    m->bye = NULL;
    return;
}




























void clean_cursor(WINDOW * win) {
    wattroff(win, A_REVERSE);

    cchar_t cht[FIXED_COLWIDTH];
    wchar_t w;
    int i, j;
    for (i = 0; i < FIXED_COLWIDTH; ) {
        w = L' ';
        j = mvwin_wchnstr (win, currow - offscr_sc_rows + RESROW - 1, (FIXED_COLWIDTH * (curcol - offscr_sc_cols)) + RESCOL + i, cht, 1);
        if (j == OK && cht[0].chars[0] != L'\0') w = cht[0].chars[0];
        mvwprintw(win, currow - offscr_sc_rows + RESROW - 1, (FIXED_COLWIDTH * (curcol - offscr_sc_cols)) + RESCOL + i, "%lc", w);
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
        j = mvwin_wchnstr (win, currow - offscr_sc_rows + RESROW - 1, FIXED_COLWIDTH * (curcol - offscr_sc_cols) + RESCOL + i, cht, 1);
        if (j == OK && cht[0].chars[0] != L'\0') w = cht[0].chars[0];
        mvwprintw(win, currow - offscr_sc_rows + RESROW - 1, (FIXED_COLWIDTH * (curcol - offscr_sc_cols)) + RESCOL + i, "%lc", w);
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
        mvwprintw (win, row+1, 0, "%*d ", rescol-1, i);
        wattroff(win, A_REVERSE);
    }
    wrefresh(win);
    return;
}


void ui_show_sc_col_headings(WINDOW * win, int mxcol) {
    int i, col = rescol;
    wmove(win, 0, 0);
    wclrtoeol(win);

    //sc_debug("curcol:%d, off:%d mxcol:%d", curcol, offscr_sc_cols, mxcol);
    for (i = offscr_sc_cols; i <= mxcol; i++) {
        int k = FIXED_COLWIDTH / 2;
        if (i == curcol) wattron(win, A_REVERSE);
        mvwprintw(win, 0, col, "%*s%-*s", k-1, " ", FIXED_COLWIDTH - k + 1, coltoa(i));

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
    mxrow = LINES - RESROW - 2;
    for (; currow > mxrow; offscr_sc_rows++, mxrow++) ;
    return;
}

void calc_offscr_sc_cols() {
    if (offscr_sc_cols && curcol >= offscr_sc_cols && curcol <= mxcol) return;
    else if (offscr_sc_cols && curcol == offscr_sc_cols - 1) { offscr_sc_cols--; mxcol--; return; }
    offscr_sc_cols = 0;
    mxcol = (COLS - RESCOL) / FIXED_COLWIDTH - 1;
    for (; curcol > mxcol; offscr_sc_cols++, mxcol++) ;
    return;
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

