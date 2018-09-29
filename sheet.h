int coltoa(int col, char * rname);
int convert(int * col, int * row, char * s, int size);
struct Sheet * Search_sheet(struct roman *doc, char *name);
struct Sheet * new_sheet(struct roman * doc, char * name);
struct Ent * lookat(struct Sheet * sh, int row, int col);

int growtbl(struct Sheet * sh, int rowcol, int toprow, int topcol);
void checkbounds(struct Sheet *sh, int *rowp, int *colp);
