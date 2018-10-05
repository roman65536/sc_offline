#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>   // for isdigit
#include <stdlib.h>  // for atoi

#ifdef XLSX
#include <zip.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>

#include "rpsc.h"
#include "xlsx.h"
#include "sheet.h"


#define MAX(a,b) (((a)>(b))?(a):(b))


/*
 * xlsx import requires:
 * requires libzip-dev
 * requires libxml2-dev
 *
 * xlsx export requires
* libxlsxwriter
 */

//struct Ent ***tbl;
//extern struct Sheet *first_sh;
//extern struct Sheet *last_sh;
//extern struct Sheet *cur_sh;




#define sc_error printf


int atocol(char *string, int len) {
    register int col;

    col = (toupper(string[0])) - 'A';

    if (len == 2)        /* has second char */
        col = ((col + 1) * 26) + ((toupper(string[1])) - 'A');

    return (col);
}




/*
 * this functions takes the DOM of the sharedStrings file
 * and based on a position, it returns the according string
 * note that 0 is the first string.
 */
char * get_xlsx_string(xmlDocPtr doc, int pos) {

    xmlNode * cur_node = xmlDocGetRootElement(doc)->xmlChildrenNode;
    xmlXPathContextPtr context;
    xmlXPathObjectPtr result;
    int i;
    char * keyword;
    char query[64];
    char *str_ret=calloc(1,4096);

    
    context=xmlXPathNewContext(doc);
    xmlXPathRegisterNs(context,"a","http://schemas.openxmlformats.org/spreadsheetml/2006/main");
    sprintf(query,"/a:sst/a:si[%d]//a:t",pos+1);
    result = xmlXPathEvalExpression(query, context);
    if(xmlXPathNodeSetIsEmpty(result->nodesetval)){
		xmlXPathFreeObject(result);
                printf("No result\n");
		return NULL;
    }
       for (i=0; i < result->nodesetval->nodeNr; i++) {
        keyword = xmlNodeListGetString(doc, result->nodesetval->nodeTab[i]->xmlChildrenNode, 1);
		//printf("keyword: %s\n", keyword);
		strcat(str_ret,keyword);
	        xmlFree(keyword);
	}
       keyword=strdup(str_ret);
       free(str_ret);
       return keyword;
#if 0    
    xmlNode * cur_node = xmlDocGetRootElement(doc)->xmlChildrenNode;
    xmlNode * father;
    char * result = malloc(4096);

    while (pos--) cur_node = cur_node->next;

    father = cur_node->xmlChildrenNode;
    cur_node = father->xmlChildrenNode;

    while (father != NULL) {  // traverse children
        while (cur_node != NULL) {  // traverse relatives
            if ( ! xmlStrcmp(cur_node->name, (const xmlChar *) "t")
                && cur_node->xmlChildrenNode != NULL
                && cur_node->xmlChildrenNode->content != NULL
               ) {
                strcat(result, (char *) cur_node->xmlChildrenNode->content);
                //sc_debug("%s %s", cur_node->name, result);
                //return result;
            }
            cur_node = cur_node->next;
        }

        father = father->next;
        if (father != NULL) cur_node = father->xmlChildrenNode;
    }

    return result;
#endif
}

/*
 * this functions takes the DOM of the styles file
 * and based on a position, it returns the according numFmtId
 * IMPORTANT: note that 0 is the first "xf".
 */
char * get_xlsx_styles(xmlDocPtr doc_styles, int pos) {
    // we go forward up to styles data
    xmlNode * cur_node = xmlDocGetRootElement(doc_styles)->xmlChildrenNode;
    while (cur_node != NULL && !(cur_node->type == XML_ELEMENT_NODE && !strcmp((char *) cur_node->name, "cellXfs")))
        cur_node = cur_node->next;

    cur_node = cur_node->xmlChildrenNode;
    // we go forward up to desidered numFmtId
    while (pos--) cur_node = cur_node->next;
    char * id = (char *) xmlGetProp(cur_node, (xmlChar *) "numFmtId");
    return id;
}

char * get_xlsx_number_format_by_id(xmlDocPtr doc_styles, int id) {
    if (doc_styles == NULL || !((id >= 165 && id <= 180) || id == 100)) 
        return NULL;

    // we go forward up to numFmts section
    xmlNode * cur_node = xmlDocGetRootElement(doc_styles)->xmlChildrenNode;
    while (cur_node != NULL && !(cur_node->type == XML_ELEMENT_NODE && !strcmp((char *) cur_node->name, "numFmts")))
        cur_node = cur_node->next;

    cur_node = cur_node->xmlChildrenNode;
    // we go forward up to desidered format
    char * idFile = (char *) xmlGetProp(cur_node, (xmlChar *) "numFmtId");
    while (atoi(idFile) != id) {
        cur_node = cur_node->next;
        free(idFile);
        idFile = (char *) xmlGetProp(cur_node, (xmlChar *) "numFmtId");
    }

    if (atoi(idFile) == id) {
        free(idFile);
        return (char *) xmlGetProp(cur_node, (xmlChar *) "formatCode");
    } else {
        free(idFile);
        return NULL;
    }
}

#define FBUFLEN 1024


// this function takes the sheetfile DOM and builds the tbl spreadsheet (SC-IM format)
void get_sheet_data(struct roman *p, xmlDocPtr doc, xmlDocPtr doc_strings, xmlDocPtr doc_styles) {
    xmlNode * cur_node = xmlDocGetRootElement(doc)->xmlChildrenNode;
    xmlNode * child_node = NULL;

    char line_interp[FBUFLEN] = "";
    int r, c;
    struct Ent *ent;

    // we go forward up to sheet data
    while (cur_node != NULL && !(cur_node->type == XML_ELEMENT_NODE && !strcmp((char *) cur_node->name, "sheetData")))
        cur_node = cur_node->next;

    cur_node = cur_node->xmlChildrenNode;       // this is sheetdata
    while (cur_node != NULL) {
        child_node = cur_node->xmlChildrenNode; // this are rows
        while (child_node != NULL) {            // this are cols

            // We get r y c
            char * row = (char *) xmlGetProp(cur_node, (xmlChar *) "r");
            r = atoi(row);
            char * col = (char *) xmlGetProp(child_node, (xmlChar *) "r");
            while (isdigit(col[strlen(col)-1])) col[strlen(col)-1]='\0';
             c = atocol(col, strlen(col));

            char * s = (char *) xmlGetProp(child_node, (xmlChar *) "t"); // type
            char * style = NULL;
            style = (char *) xmlGetProp(child_node, (xmlChar *) "s");    // style
            char * fmtId = style == NULL ? NULL : get_xlsx_styles(doc_styles, atoi(style)); // numfmtId by style number
            char * numberFmt = NULL;
            if (fmtId != NULL && atoi(fmtId) != 0) {
                numberFmt = get_xlsx_number_format_by_id(doc_styles, atoi(fmtId));
            }

            // string
            if ( s != NULL && ! strcmp(s, "s") ) {
                char * st = NULL;
                char * strvalue =  get_xlsx_string(doc_strings, atoi((char *) child_node->xmlChildrenNode->xmlChildrenNode->content));
                if (strvalue != NULL && strvalue[0] != '\0') {
		  //      st = str_replace (strvalue, "\"", "''");
		  //clean_carrier(st); // we handle padding
                    snprintf(line_interp, FBUFLEN, "label %s%d=\"%s\"", col, r, strvalue);
                    //printf(line_interp);
		     ent=lookat(p->cur_sh,r,c);
		     p->cur_sh->maxcol=MAX(p->cur_sh->maxcol,c);
		     p->cur_sh->maxrow=MAX(p->cur_sh->maxrow,r);
		     ent->label=strdup(strvalue);
		    ent->flag|=RP_LABEL;
                    free(st);
                }

            // inlinestring
            } else if ( s != NULL && ! strcmp(s, "inlineStr") ) {
                char * st = NULL;
                char * strvalue = (char *) child_node->xmlChildrenNode->xmlChildrenNode->content;
                if (strvalue != NULL && strvalue[0] != '\0') {
		  //st = str_replace (strvalue, "\"", "''");
		  //clean_carrier(st); // we handle padding
                    snprintf(line_interp, FBUFLEN, "label %s%d=\"%s\"", col, r, st);
                    //printf(line_interp);
		     ent=lookat(p->cur_sh,r,c);
		     p->cur_sh->maxcol=MAX(p->cur_sh->maxcol,c);
		     p->cur_sh->maxrow=MAX(p->cur_sh->maxrow,r);
		     ent->label=strdup(strvalue);
		    ent->flag|=RP_LABEL;
                    free(st);
                }

            // numbers (can be dates, results from formulas or simple numbers)
            } else {
                // date value in v
                if (fmtId != NULL && child_node->xmlChildrenNode != NULL &&
                ! strcmp((char *) child_node->xmlChildrenNode->name, "v")
		&& (s != NULL  && strcmp((char *) s, "n"))
                && (
                (atoi(fmtId) >= 14 && atoi(fmtId) <= 17) || 
                atoi(fmtId) == 278 || atoi(fmtId) == 185 ||
                atoi(fmtId) == 196 ||
                atoi(fmtId) == 217 || atoi(fmtId) == 326 ||
                (((atoi(fmtId) >= 165 && atoi(fmtId) <= 180) ||
                atoi(fmtId) == 100) && numberFmt != NULL // 100,165-180 are user defined formats!!
		 // && str_in_str(numberFmt, "/") != -1)
		 ))) {
                    long l = strtol((char *) child_node->xmlChildrenNode->xmlChildrenNode->content, (char **) NULL, 10);

                    snprintf(line_interp, FBUFLEN, "let1 %s%d=%.15ld %ld", col, r, (l - 25569) * 86400 - atoi("1"),l);
                    //printf(line_interp);
		    ent=lookat(p->cur_sh,r,c);
		     p->cur_sh->maxcol=MAX(p->cur_sh->maxcol,c);
		     p->cur_sh->maxrow=MAX(p->cur_sh->maxrow,r);
		    ent->val=(l-25569)*86400-1;
		    ent->flag|=VAL;
		    //                    struct ent * n = lookat(r, c);
                    //n->format = 0;
                    char * stringFormat = malloc((unsigned)(strlen("%d/%m/%Y") + 2));
                    sprintf(stringFormat, "%c", 'd');
                    strcat(stringFormat, "%d/%m/%Y");
                    //n->format = stringFormat;

                // time value in v
                } else if (fmtId != NULL && child_node->xmlChildrenNode != NULL &&
                ! strcmp((char *) child_node->xmlChildrenNode->name, "v")
                && (
                (atoi(fmtId) >= 18 && atoi(fmtId) <= 21)
                )) {
                    double l = atof((char *) child_node->xmlChildrenNode->xmlChildrenNode->content);
                    snprintf(line_interp, FBUFLEN, "let2 %s%d=%.15f", col, r, (l - atoi("1") * 1.0 / 60 / 60 / 24) * 86400);
                    //printf(line_interp);
                    //struct ent * n = lookat(r, c);
                    //n->format = 0;
                    char * stringFormat = malloc((unsigned)(strlen("%H:%M:%S") + 2));
                    sprintf(stringFormat, "%c", 'd');
                    strcat(stringFormat, "%H:%M:%S");
                    //n->format = stringFormat;

                // v - straight int value
                } else if (//fmtId != NULL &&
                child_node->xmlChildrenNode != NULL &&
                ! strcmp((char *) child_node->xmlChildrenNode->name, "v") ){
                    double l = atof((char *) child_node->xmlChildrenNode->xmlChildrenNode->content);
                    snprintf(line_interp, FBUFLEN, "let3 %s%d=%.15f", col, r, l);
                    //printf(line_interp);

		    ent=lookat(p->cur_sh,r,c);
		     p->cur_sh->maxcol=MAX(p->cur_sh->maxcol,c);
		     p->cur_sh->maxrow=MAX(p->cur_sh->maxrow,r);
		    ent->val=l;
		    ent->flag|=VAL;

		    
                // f - numeric value that is a result from formula
                } else if (//fmtId != NULL &&
                child_node->xmlChildrenNode != NULL && ! strcmp((char *) child_node->xmlChildrenNode->name, "f")) {

                    // handle the formula if that is whats desidered!!
                    if (atoi("1")) {
                        char * formula = (char *) child_node->xmlChildrenNode->xmlChildrenNode->content;
                        char * strf;

                        // we take some excel common function and adds a @ to them
                        // we replace count sum avg with @count, @sum, @prod, @avg, @min, @max 
                        //strf = str_replace (formula, "COUNT","@COUNT");
                        //strcpy(formula, strf);
                        //free(strf);
                        //strf = str_replace (formula, "SUM","@SUM");
                        //strcpy(formula, strf);
                        //free(strf);
                        //strf = str_replace (formula, "PRODUCT","@PROD");
                        //strcpy(formula, strf);
                        //free(strf);
                        //strf = str_replace (formula, "AVERAGE","@AVG");
                        //strcpy(formula, strf);
                        //free(strf);
                        //strf = str_replace (formula, "MIN","@MIN");
                        //strcpy(formula, strf);
                        //free(strf);
                        //strf = str_replace (formula, "MAX","@MAX");
                        //strcpy(formula, strf);
                        //free(strf);
                        //strf = str_replace (formula, "ABS","@ABS");
                        //strcpy(formula, strf);
                        //free(strf);
                        //strf = str_replace (formula, "STDEV","@STDDEV");
                        //strcpy(formula, strf);
                        //free(strf);

                        // we send the formula to the interpreter and hope to resolve it!
                        snprintf(line_interp, FBUFLEN, "let4 %s%d=%s", col, r, formula);
			 ent=lookat(p->cur_sh,r,c);
		     p->cur_sh->maxcol=MAX(p->cur_sh->maxcol,c);
		     p->cur_sh->maxrow=MAX(p->cur_sh->maxrow,r);
			 ent->formula=strdup(formula);
			 char *ptr=ent->formula;
			 ent->exp=getAST(ptr,p);
			 
			 if((ent->exp != 0) && (ent->exp->type == eVALUE)) {
			   ent->val=ent->exp->value;
			   deleteExpression(ent->exp);
			   ent->exp=0;
			 }
			 ent->flag |= VAL | RP_FORMULA;
			
                    } else {
                        double l = atof((char *) child_node->last->xmlChildrenNode->content);
                        snprintf(line_interp, FBUFLEN, "2let %s%d=%.15f", col, r, l);
                    }
                    //printf(line_interp);
                }
            }

            xmlFree(s);
            xmlFree(fmtId);
            xmlFree(style);
            xmlFree(numberFmt);

            child_node = child_node->next;
            xmlFree(col);
            xmlFree(row);
        }
        cur_node = cur_node->next;
    }
    return;
}

int open_xlsx(struct roman *p,char * fname, char * encoding) {
    struct zip * za;
    struct zip_file * zf;
    struct zip_stat sb, sb_strings, sb_styles, sb_sheets;
    char buf[100];
    int err;
    int len;

    // open zip file
    if ((za = zip_open(fname, 0, &err)) == NULL) {
        zip_error_to_str(buf, sizeof(buf), err, errno);
        sc_error("can't open zip archive `%s': %s", fname, buf);
        return -1;
    }

    // open xl/sharedStrings.xml
    char * name = "xl/sharedStrings.xml";
    zf = zip_fopen(za, name, ZIP_FL_UNCHANGED);
    char * strings = NULL;
    if (zf) {
        // some files may not have strings
        zip_stat(za, name, ZIP_FL_UNCHANGED, &sb_strings);
        strings = (char *) malloc(sb_strings.size);
        len = zip_fread(zf, strings, sb_strings.size);
        if (len < 0) {
            sc_error("cannot read file %s.\n", name);
            free(strings);
            return -1;
        }
        zip_fclose(zf);
    }

    // open xl/styles.xml
    name = "xl/styles.xml";
    zf = zip_fopen(za, name, ZIP_FL_UNCHANGED);
    if ( ! zf ) {
        sc_error("cannot open %s file.", name);
        if (strings != NULL) free(strings);
        return -1;
    }
    zip_stat(za, name, ZIP_FL_UNCHANGED, &sb_styles);
    char * styles = (char *) malloc(sb_styles.size);
    len = zip_fread(zf, styles, sb_styles.size);
    if (len < 0) {
        sc_error("cannot read file %s.", name);
        if (strings != NULL) free(strings);
        free(styles);
        return -1;
    }
    zip_fclose(zf);


    // XML parse for the sheet file
    xmlDoc * doc = NULL;
    xmlDoc * doc_strings = NULL;
    xmlDoc * doc_sheets = NULL;
    xmlDoc * doc_styles = NULL;

    // this initialize the library and check potential ABI mismatches
    // between the version it was compiled for and the actual shared
    // library used.
    LIBXML_TEST_VERSION

    // parse the file and get the DOM
    doc_strings = xmlReadMemory(strings, sb_strings.size, "noname.xml", NULL, XML_PARSE_NOBLANKS);
    doc_styles = xmlReadMemory(styles, sb_styles.size, "noname.xml", NULL, XML_PARSE_NOBLANKS);

    zf=zip_fopen(za,"xl/workbook.xml",ZIP_FL_UNCHANGED);
    char * sheets=NULL;
        if (zf) {
        // some files may not have strings
        zip_stat(za, "xl/workbook.xml", ZIP_FL_UNCHANGED, &sb_sheets);
        sheets = (char *) malloc(sb_sheets.size);
        len = zip_fread(zf, sheets, sb_sheets.size);
        if (len < 0) {
            error("cannot read file %s.\n", "xl/workbook.xml");
            free(strings);
            return -1;
        }
        zip_fclose(zf);
    }

xmlChar * xpath = "//a:workbook/a:sheets/*";
        //xmlDocPtr doc = xmlParseFile("xmlfile.xml");
        xmlDocPtr sdoc = xmlParseMemory(sheets,len);
        xmlXPathContextPtr context = xmlXPathNewContext(sdoc);
        xmlXPathRegisterNs(context,"a","http://schemas.openxmlformats.org/spreadsheetml/2006/main");

        xmlXPathObjectPtr result = xmlXPathEvalExpression(xpath, context);
        int nr_sheets=result->nodesetval->nodeNr;
        int a;
        for(a=0;a<nr_sheets;a++ ) {
            xmlNodePtr cur;
            cur=result->nodesetval->nodeTab[a];
            struct Sheet * this_sh;
            this_sh=new_sheet(p,xmlGetProp(cur,"name"));
            growtbl(this_sh,GROWNEW, 0, 0);
            p->cur_sh=this_sh;
        }

        for(a=0;a<nr_sheets;a++        ) {
	        xmlNodePtr cur;
            cur=result->nodesetval->nodeTab[a];
	/*
        printf("Name %s\n",cur->name);
        printf(" -> %s\n",xmlNodeListGetString(sdoc, cur->properties->children, 1));
        printf(" -> %s\n",xmlGetProp(cur,"name"));
        printf(" -> %s\n",xmlGetProp(cur,"sheetId"));
	*/

        



	struct Sheet *this_sh;
	this_sh=Search_sheet(p,xmlGetProp(cur,"name"));
	//growtbl(this_sh,GROWNEW, 0, 0);
	p->cur_sh=this_sh;
   
	char buf1[64];
    // open xl/worksheets/sheet1.xml
	//sprintf(buf1,"xl/worksheets/sheet%s.xml",xmlGetProp(cur,"sheetId"));
	sprintf(buf1,"xl/worksheets/sheet%d.xml",a+1);
    zf = zip_fopen(za, buf1, ZIP_FL_UNCHANGED);
    if ( ! zf ) {
        sc_error("cannot open %s file.", buf1);
        if (sheets != NULL) free(sheets);
        free(styles);
        return -1;
    }
    zip_stat(za, buf1, ZIP_FL_UNCHANGED, &sb);
    char * sheet = (char *) malloc(sb.size);
    len = zip_fread(zf, sheet, sb.size);
    if (len < 0) {
        sc_error("cannot read file %s.", name);
        if (strings != NULL) free(strings);
        free(styles);
        free(sheet);
        return -1;
    }
    zip_fclose(zf);






    doc = xmlReadMemory(sheet, sb.size, "noname.xml", NULL, XML_PARSE_NOBLANKS);

    if (doc == NULL) {
        sc_error("error: could not parse file");
        if (strings != NULL) free(strings);
        free(styles);
        free(sheet);
        return -1;
    }

    get_sheet_data(p,doc, doc_strings, doc_styles);

    // free the document
    xmlFreeDoc(doc);
} 

    
    xmlFreeDoc(doc_strings);
    xmlFreeDoc(doc_styles);

    // Free the global variables that may have been allocated by the parser
    xmlCleanupParser();

    // free both sheet and strings variables
    if (strings != NULL) free(strings);
    free(styles);
    //free(sheet);

    // close zip file
    if (zip_close(za) == -1) {
        sc_error("cannot close zip archive `%s'", fname);
        return -1;
    }

    //    auto_justify(0, maxcols, DEFWIDTH);
    // deleterow(currow, 1);
    return 0;
}


static exl_read(struct roman *p, char *name)
{

 open_xlsx(p, name,"");
}

static char *exc_ending[]= {
{".xlsx"},
{0}
};



static char exl_name[] ="xlsx";

 int init_xlsx(struct plugin *ptr)
{
ptr->ending=exc_ending;
ptr->type=PLUG_IN;
ptr->read=exl_read;
ptr->name=exl_name;



}



#endif

#ifdef XLSX_EXPORT
#include "xlsxwriter.h"
int export_xlsx(char * filename, int r0, int c0, int rn, int cn) {
    int row, col;
    register struct ent ** pp;

    lxw_workbook  * workbook  = workbook_new(filename);
    lxw_worksheet * worksheet = workbook_add_worksheet(workbook, NULL);

    int bkp_currow = currow;
e    currow = 0;
    insert_row(0); //add a row so that scim formulas apply to excel

    for (row = r0; row <= rn+1; row++)
        for (pp = ATBL(tbl, row, col = c0); col <= cn; col++, pp++)
            if (*pp) {
                // Check format here
                lxw_format * format = workbook_add_format(workbook);

                // handle alignment
                if ((*pp)->label && (*pp)->flags & is_label)          // center align
                    format_set_align(format, LXW_ALIGN_CENTER);
                else if ((*pp)->label && (*pp)->flags & is_leftflush) // left align
                    format_set_align(format, LXW_ALIGN_LEFT);
                else if ((*pp)->label)                                // right align
                    format_set_align(format, LXW_ALIGN_RIGHT);

                // handle bold and underline
                if ((*pp)->ucolor != NULL && (*pp)->ucolor->bold)
                    format_set_bold(format);
                else if ((*pp)->ucolor != NULL && (*pp)->ucolor->underline)
                    format_set_underline(format, LXW_UNDERLINE_SINGLE);

                // handle fg color
                if ((*pp)->ucolor != NULL && (*pp)->ucolor->fg) {
                    int fgcolor;
                    switch ((*pp)->ucolor->fg) {
                        case BLACK:
                            fgcolor = LXW_COLOR_BLACK;
                            break;
                        case RED:
                            fgcolor = LXW_COLOR_RED;
                            break;
                        case GREEN:
                            fgcolor = LXW_COLOR_GREEN;
                            break;
                        case YELLOW:
                            fgcolor = LXW_COLOR_YELLOW;
                            break;
                        case BLUE:
                            fgcolor = LXW_COLOR_BLUE;
                            break;
                        case MAGENTA:
                            fgcolor = LXW_COLOR_MAGENTA;
                            break;
                        case CYAN:
                            fgcolor = LXW_COLOR_CYAN;
                            break;
                        case WHITE:
                            fgcolor = LXW_COLOR_WHITE;
                            break;
                    }
                    format_set_font_color(format, fgcolor);
                }

                // handle bg color
                if ((*pp)->ucolor != NULL && (*pp)->ucolor->bg) {
                    int bgcolor;
                    switch ((*pp)->ucolor->bg) {
                        case BLACK:
                            bgcolor = LXW_COLOR_BLACK;
                            break;
                        case RED:
                            bgcolor = LXW_COLOR_RED;
                            break;
                        case GREEN:
                            bgcolor = LXW_COLOR_GREEN;
                            break;
                        case YELLOW:
                            bgcolor = LXW_COLOR_YELLOW;
                            break;
                        case BLUE:
                            bgcolor = LXW_COLOR_BLUE;
                            break;
                        case MAGENTA:
                            bgcolor = LXW_COLOR_MAGENTA;
                            break;
                        case CYAN:
                            bgcolor = LXW_COLOR_CYAN;
                            break;
                        case WHITE:
                            bgcolor = LXW_COLOR_WHITE;
                            break;
                    }
                    format_set_bg_color(format, bgcolor);
                }

                // dateformat
                if ((*pp) && (*pp)->format && (*pp)->format[0] == 'd') {
                    char sc_format[BUFFERSIZE];
                    char * st = NULL;
                    strcpy(sc_format, &((*pp)->format[1]));

                    st = str_replace(sc_format, "%Y", "yyyy");
                    strcpy(sc_format, st);
                    free(st);
                    st = str_replace(sc_format, "%y", "yy");
                    strcpy(sc_format, st);
                    free(st);
                    st = str_replace(sc_format, "%m", "mm");
                    strcpy(sc_format, st);
                    free(st);
                    st = str_replace(sc_format, "%d", "dd");
                    strcpy(sc_format, st);
                    free(st);
                    format_set_num_format(format, sc_format);
                    worksheet_write_number(worksheet, row-1, col, (((*pp)->v + atoi(get_conf_value("tm_gmtoff"))) / 86400 + 25569) , format);

                // formula
                } else if ((*pp) && (*pp)->expr && atoi(get_conf_value("xlsx_readformulas")))  {
                    linelim = 0;
                    editexp((*pp)->row, (*pp)->col);
                    linelim = -1;

                    char * strf;
                    char formula[BUFFERSIZE];
                    strcpy(formula, line);

                    strf = str_replace(formula, "@count","count");
                    strcpy(formula, strf);
                    free(strf);

                    strf = str_replace(formula, "@sum","sum");
                    strcpy(formula, strf);
                    free(strf);

                    strf = str_replace(formula, "@prod","product");
                    strcpy(formula, strf);
                    free(strf);

                    strf = str_replace(formula, "@avg","average");
                    strcpy(formula, strf);
                    free(strf);

                    strf = str_replace(formula, "@min","min");
                    strcpy(formula, strf);
                    free(strf);

                    strf = str_replace(formula, "@max","max");
                    strcpy(formula, strf);
                    free(strf);

                    strf = str_replace(formula, "@abs","abs");
                    strcpy(formula, strf);
                    free(strf);

                    strf = str_replace(formula, "@stddev","stdev");
                    strcpy(formula, strf);
                    free(strf);

                    add_char(formula, '=', 0);
                    worksheet_write_formula(worksheet, row-1, col, formula, NULL);

                // If a numeric value exists
                } else if ( (*pp)->flags & is_valid) {
                    worksheet_write_number(worksheet, row-1, col, (*pp)->v, format);

                } else if ((*pp)->label) {
                    worksheet_write_string(worksheet, row-1, col, (*pp)->label, format);
                }
                /* TODO: handle hidden rows and columns? */
            }
    int_deleterow(currow, 1); /* delete the added row */
    currow = bkp_currow;

    return workbook_close(workbook);
}
#endif
