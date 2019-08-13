#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
 #include <curl/curl.h>
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






struct MemoryStruct {
  char *memory;
  size_t size;
};




static size_t
WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
  size_t realsize = size * nmemb;
  struct MemoryStruct *mem = (struct MemoryStruct *)userp;

  char *ptr = realloc(mem->memory, mem->size + realsize + 1);
  if(ptr == NULL) {
    /* out of memory! */
    printf("not enough memory (realloc returned NULL)\n");
    return 0;
  }

  mem->memory = ptr;
  memcpy(&(mem->memory[mem->size]), contents, realsize);
  mem->size += realsize;
  mem->memory[mem->size] = 0;

  return realsize;
}





static html_read(struct roman *p,char *name)
{
 htmlDocPtr doc;
    xmlNode *roo_element = NULL;

     CURL *curl_handle;
  CURLcode res;

  struct MemoryStruct chunk;
  
/* Macro to check API for match with the DLL we are using */
    LIBXML_TEST_VERSION

    
 chunk.memory = malloc(1);  /* will be grown as needed by the realloc above */
  chunk.size = 0;    /* no data at this point */

  curl_global_init(CURL_GLOBAL_ALL);

  /* init the curl session */
  curl_handle = curl_easy_init();

  /* specify URL to get */
  curl_easy_setopt(curl_handle, CURLOPT_URL, argn[1]);

  /* send all data to this function  */
  curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);

  /* we pass our 'chunk' struct to the callback function */
  curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);

  /* some servers don't like requests that are made without a user-agent
     field, so we provide one */
  curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");
  curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, (long)1);

  /* get it! */
  res = curl_easy_perform(curl_handle);

  /* check for errors */
  if(res != CURLE_OK) {
    fprintf(stderr, "curl_easy_perform() failed: %s\n",
            curl_easy_strerror(res));
  }
  else {
    /*
     * Now, our chunk.memory points to a memory block that is chunk.size
     * bytes big and contains the remote file.
     *
     * Do something nice with it!
     */

    printf("%lu bytes retrieved\n", (unsigned long)chunk.size);
  }




	
    doc = xmlReadMemory(chunk.memory, chunk.size, "noname.xml", NULL, 0);
    
    //doc = htmlReadFile(name, NULL, HTML_PARSE_NOBLANKS | HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING | HTML_PARSE_NONET);
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

      /* cleanup curl stuff */
  curl_easy_cleanup(curl_handle);

  free(chunk.memory);

  /* we're done with libcurl, so clean it up */
  curl_global_cleanup();
    
    return 0;


}



html_write(struct roman *p, char *name)
{

 FILE *fd=fopen(name,"w+");
 fprintf(fd,"<html>\n");
 for(p->cur_sh=p->first_sh;p->cur_sh  !=0  ; p->cur_sh=p->cur_sh->next)
 {
 fprintf(fd,"<h3>%s</h3>\n",p->cur_sh->name);
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





