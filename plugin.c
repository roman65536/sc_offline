#include <dlfcn.h>
#include <string.h>
#include <math.h>

#include "rpsc.h"
#include "Parser.h"
#include "Lexer.h"
#include "sheet.h"


static 	struct  plugin *plug_ins;


void init_plugin()
{
 plug_ins=0;

}


int load_plugin(char *name)
{
 void *handle;
 struct plugin *new;
 int (*init_p)(struct plugin *);
 char f_name[255];
	sprintf(f_name,"./%s.so",name);

	handle = dlopen(f_name, RTLD_LAZY|RTLD_GLOBAL|RTLD_DEEPBIND );
           if (!handle) {
		printf("Failed load %s \n",name);
		return -1;
	 	}
 	new=(struct plugin *) calloc(1,sizeof(struct plugin));
	new->next=plug_ins;
	plug_ins=new;
	sprintf(f_name,"init_%s",name);
	init_p=dlsym(handle,f_name);
	init_p(new); //plugin fills out the plugin struct
	

 return 0; 
}


read_plugin(struct roman *p_t, char *file,char *name )
{
 struct plugin *p=plug_ins;
 
 for(;p!=0; p=p->next)
 if ((p !=0 )  && (strcmp(name,p->name)==0 ))
	return p->read(p_t,file);
}


write_plugin(struct roman *p_t, char *file,char *name )
{
 struct plugin *p=plug_ins;

 for(;p!=0; p=p->next)
 if ((p !=0 )  && (strcmp(name,p->name)==0 ))
        return p->write(p_t,file);
}


