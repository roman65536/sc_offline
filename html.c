#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <libxml/HTMLparser.h>

#include "rpsc.h"


#define MAX(a,b) (((a)>(b))?(a):(b))

static struct plugin *pl;

#define new_sheet_pl(...) (*pl->new_sheet)( __VA_ARGS__)
#define lookat_pl(...) (*pl->lookat)( __VA_ARGS__)

char * get_cell_content(xmlNode *cur, char  **str)
{
 xmlNode *ptr;
 for(ptr=cur->children; ptr != NULL; ptr =ptr->next)
 {
  if(ptr->type == XML_TEXT_NODE)
   strcat(*str,ptr->content);
  if(ptr->children != 0) get_cell_content(ptr,str);

 }
}


char * get_cell_content_len(xmlNode *cur, int *len)
{
 xmlNode *ptr;
 for(ptr=cur->children; ptr != NULL; ptr =ptr->next)
 {
  if(ptr->type == XML_TEXT_NODE)
   *len+=strlen(ptr->content); 
  if(ptr->children != 0) get_cell_content_len(ptr,len);

 } 
}

void html_read_row(struct roman *p , xmlNode *cur, int row)
{
 int col=0;
 xmlNode *ptr;

 for(ptr=cur->children; ptr != NULL ; ptr = ptr->next)
{
 if (ptr->type != XML_ELEMENT_NODE)
        continue;
 if (xmlStrEqual(ptr->name,"td"))
        {
	 if(ptr->children != 0 )
 	{
	 struct Ent *ent;
	 char *next;
	 int len=0;
	 char *str;
	 get_cell_content_len(ptr,&len);
	 str=calloc(1,len+1);
	 get_cell_content(ptr,&str);
	 //printf("String len %d [%s]\n",len,str); 
 	 printf("CELL %d;%d [%s]\n",row,col,str);
	 ent=lookat_pl(p->cur_sh,row,col);
	 p->cur_sh->maxcol=MAX(p->cur_sh->maxcol,col);
         p->cur_sh->maxrow=MAX(p->cur_sh->maxrow,row);
	 ent->val=strtod(str,&next);
	 if ((str == next) )
		{
		/* not a number */
		ent->label=strdup(str);
		ent->flag |= RP_LABEL;
		}
		else ent->flag |= VAL;

	 
	 free(str);
	 }
	 col++;
        }

}

}


void html_read_rows(struct roman *p , xmlNode *cur, int row)
{
 xmlNode *ptr;

 for(ptr=cur->children; ptr != NULL ; ptr = ptr->next)
{
 if (ptr->type != XML_ELEMENT_NODE)
        continue;
 if (xmlStrEqual(ptr->name,"tr"))
        {
          html_read_row(p, ptr,row);
          row++;
        }

}

}


void html_read_table( struct roman *p ,  xmlNode *cur)
{
 
 int row=0;
 xmlNode *ptr;

 for(ptr=cur->children; ptr != NULL ; ptr = ptr->next)
{
 if (ptr->type != XML_ELEMENT_NODE)
	continue;
 if(xmlStrEqual(ptr->name,"caption"))
	printf(" TABLE NAME [%s]\n",ptr->children->content);
 		
 else if (xmlStrEqual(ptr->name,"tr"))
	{
	 printf(" TR \n");
 	  html_read_rows(p, cur,row);	
	  break;
	} 

}

}

void traverse_dom_trees(struct roman *p, xmlNode * a_node)
{
    xmlNode *cur_node = NULL;
    int tbl_nr=1;

    if(NULL == a_node)
    {
        //printf("Invalid argument a_node %p\n", a_node);
        return;
    }

    for (cur_node = a_node; cur_node; cur_node = cur_node->next) 
    {
        if (cur_node->type == XML_ELEMENT_NODE) 
        {
            /* Check for if current node should be exclude or not */
//            printf("Node type: Text, name: %s\n", cur_node->name);
	    if(xmlStrEqual(cur_node->name, "table"))
		{ 
			xmlNode *pt=cur_node->children;
			int flag=0;
			char *tbl_name;
			printf("****** Found Table\n");
			for(;pt!=0 ;pt=pt->next)
			{
				if (pt->type != XML_ELEMENT_NODE)
				        continue;
 				if(xmlStrEqual(pt->name,"caption"))
					{
        				printf("Found tbl name [%s]\n",pt->children->content);
					tbl_name=pt->children->content;
					flag=1;
					}
			}
			if(flag == 1) p->cur_sh=new_sheet_pl(p,tbl_name);
			else {
				char tbl_name1[32];
				sprintf(tbl_name1,"html_table%d",tbl_nr++); 
				printf(" tbl name [%s] will used", tbl_name1);
				p->cur_sh=new_sheet_pl(p,tbl_name1);
			}
			html_read_table(p , cur_node);
	        }
        }
        else if(cur_node->type == XML_TEXT_NODE)
        {
            /* Process here text node, It is available in cpStr :TODO: */
//            printf("node type: Text, node content: %s,  content length %d\n", (char *)cur_node->content, strlen((char *)cur_node->content));
        }
        traverse_dom_trees(p, cur_node->children);
    }
}

#if 0
int main(int argc, char **argv) 
{
    htmlDocPtr doc;
    xmlNode *roo_element = NULL;

    if (argc != 2)  
    {
        printf("\nInvalid argument\n");
        return(1);
    }

    /* Macro to check API for match with the DLL we are using */
    LIBXML_TEST_VERSION    

    doc = htmlReadFile(argv[1], NULL, HTML_PARSE_NOBLANKS | HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING | HTML_PARSE_NONET);
    if (doc == NULL) 
    {
        fprintf(stderr, "Document not parsed successfully.\n");
        return 0;
    }

    roo_element = xmlDocGetRootElement(doc);

    if (roo_element == NULL) 
    {
        fprintf(stderr, "empty document\n");
        xmlFreeDoc(doc);
        return 0;
    }

    printf("Root Node is %s\n", roo_element->name);
    traverse_dom_trees(roo_element);

    xmlFreeDoc(doc);       // free document
    xmlCleanupParser();    // Free globals
    return 0;
}

#endif

static html_read(struct roman *p,char *name)
{
 htmlDocPtr doc;
    xmlNode *roo_element = NULL;

/* Macro to check API for match with the DLL we are using */
    LIBXML_TEST_VERSION

    doc = htmlReadFile(name, NULL, HTML_PARSE_NOBLANKS | HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING | HTML_PARSE_NONET);
    if (doc == NULL)
    {
        fprintf(stderr, "Document not parsed successfully.\n");
        return 0;
    }

    roo_element = xmlDocGetRootElement(doc);

    if (roo_element == NULL)
    {
        fprintf(stderr, "empty document\n");
        xmlFreeDoc(doc);
        return 0;
    }

    printf("Root Node is %s\n", roo_element->name);
    traverse_dom_trees(p, roo_element);

    xmlFreeDoc(doc);       // free document
    xmlCleanupParser();    // Free globals
    return 0;


}



html_write(struct roman *p, char *name)
{

 FILE *fd=fopen(name,"w+");
 fprintf(fd,"<html>\n");
 for(p->cur_sh=p->first_sh;p->cur_sh  !=0  ; p->cur_sh=p->cur_sh->next)
 {
 fprintf(fd,"<h3>%s</h3\n",p->cur_sh->name);
  export(p, fd, "<table>","</table>\n","<tr>","</tr>\n","<td>","</td>");

 } 
 fprintf(fd,"</html>\n");
 fclose(fd);
}

static char *html_ending[]= {
{".html"},
{".htm"},
{0}
};

static char *html_name = "html";


int init_html(struct plugin *ptr)
{
ptr->ending=html_ending;
ptr->type=PLUG_IN;
ptr->read=html_read;
ptr->write=html_write;
ptr->name=html_name;
pl=ptr;
}





