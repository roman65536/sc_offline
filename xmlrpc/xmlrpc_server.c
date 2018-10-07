/* A simple standalone XML-RPC server program written in C. */

/* This server knows one RPC class (besides the system classes):
   "sample.add".

   The program takes one argument: the HTTP port number on which the server
   is to accept connections, in decimal.

   You can use the example program 'xmlrpc_sample_add_client' to send an RPC
   to this server.

   Example:

   $ ./xmlrpc_sample_add_server 8080&
   $ ./xmlrpc_sample_add_client

   For more fun, run client and server in separate terminals and turn on
   tracing for each:

   $ export XMLRPC_TRACE_XML=1
*/

#define WIN32_LEAN_AND_MEAN  /* required by xmlrpc-c/server_abyss.h */

#include <stdlib.h>
#include <stdio.h>
#ifdef _WIN32
#  include <windows.h>
#  include <winsock2.h>
#else
#  include <unistd.h>
#endif

#include <xmlrpc-c/base.h>
#include <xmlrpc-c/server.h>
#include <xmlrpc-c/server_abyss.h>

#include "rpsc.h"

//#include "config.h"  /* information about this build environment */


#ifdef _WIN32
  #define SLEEP(seconds) SleepEx(seconds * 1000, 1);
#else
  #define SLEEP(seconds) sleep(seconds);
#endif

struct roman * p;


static xmlrpc_value *
sample_add(xmlrpc_env *   const envP,
           xmlrpc_value * const paramArrayP,
           void *         const serverInfo,
           void *         const channelInfo) {

    xmlrpc_int32 x, y, z;

    /* Parse our argument array. */
    xmlrpc_decompose_value(envP, paramArrayP, "(ii)", &x, &y);
    if (envP->fault_occurred)
        return NULL;

    /* Add our two numbers. */
    z = x + y;

    /* Sometimes, make it look hard (so client can see what it's like
       to do an RPC that takes a while).
    */
    if (y == 1)
        SLEEP(5);

    /* Return our result. */
    return xmlrpc_build_value(envP, "i", z);
}

static xmlrpc_value *
getcell(xmlrpc_env *   const envP,
           xmlrpc_value * const paramArrayP,
           void *         const serverInfo,
           void *         const channelInfo) {

    const char * str;
    size_t len;
    xmlrpc_int32 x, y, z;
    struct Ent *t;
    char *label;
    int str_len;
    

    /* Parse our argument array. */
    xmlrpc_decompose_value(envP, paramArrayP, "(sii)",&str, &x, &y);
    if (envP->fault_occurred)
        return NULL;
    printf("%s: %s %d %d\n",__FUNCTION__,str,x,y);
    t=lookat(Search_sheet(p,str),x,y);
    /* Return our result. */
    printf("ret: %lf %x\n",t->val,t->flag);
    if(t->label==0) {label="NULL"; str_len=4;} else {label=t->label; str_len=strlen(label);}  
    return xmlrpc_build_value(envP, "(dis#)", t->val,t->flag,label,str_len);
}


static xmlrpc_value *
getsheets(xmlrpc_env *   const envP,
           xmlrpc_value * const paramArrayP,
           void *         const serverInfo,
           void *         const channelInfo) {

    xmlrpc_value * myArrayP;
    xmlrpc_value * itemP;
    struct Sheet *sh;
     myArrayP = xmlrpc_array_new(envP);
    sh=p->first_sh;
    for(sh;sh!=0 ;sh =sh->next) {
	//	xmlrpc_build_value(envP,"(s)", sh->name);
        itemP = xmlrpc_string_new(envP, sh->name);
         xmlrpc_array_append_item(envP, myArrayP, itemP);
	 xmlrpc_DECREF(itemP);
    }
    //   xmlrpc_DECREF(itemP);
    return myArrayP;



}



int
main(int           const argc,
     const char ** const argv) {

    struct xmlrpc_method_info3 const methodInfo = {
        /* .methodName     = */ "sample.add",
        /* .methodFunction = */ &sample_add,
    };
    
    xmlrpc_server_abyss_parms serverparm;
    xmlrpc_registry * registryP;
    xmlrpc_env env;

    if (argc-1 != 1) {
        fprintf(stderr, "You must specify 1 argument:  The TCP port "
                "number on which the server will accept connections "
                "for RPCs (8080 is a common choice).  "
                "You specified %d arguments.\n",  argc-1);
        exit(1);
    }


    slab_allocator_init();
    ExpressionInit();
    init_lib();
    init_plugin();

    load_plugin("html");
    load_plugin("xlsx");
     p = (struct roman *) malloc(sizeof(struct roman));
    struct Ent *tl;

    p->name=NULL;
    p->open=0; // we are not loading a file here..
    p->first_sh=p->last_sh=p->cur_sh=0;
    p->cache=0;
    p->cache_nr=0;

    // create a sheet
    struct Sheet * sh = new_sheet(p, "roman");
    growtbl(sh, GROWNEW, 0, 0);
    p->cur_sh = sh;
    // set a value of a cell
    struct Ent * t = lookat(p->cur_sh, 0, 0);
    t->flag |= VAL;
    t->val = 4;

  // create a sheet
    struct Sheet * sh1 = new_sheet(p, "roman1");
    growtbl(sh1, GROWNEW, 0, 0);
    p->cur_sh = sh1;
    // set a value of a cell
    struct Ent * t1 = lookat(p->cur_sh, 0, 0);
    t1->flag |= VAL;
    t1->val = 400;
    
    xmlrpc_env_init(&env);

    registryP = xmlrpc_registry_new(&env);
    if (env.fault_occurred) {
        printf("xmlrpc_registry_new() failed.  %s\n", env.fault_string);
        exit(1);
    }

    xmlrpc_registry_add_method3(&env, registryP, &methodInfo);
    if (env.fault_occurred) {
        printf("xmlrpc_registry_add_method3() failed.  %s\n",
               env.fault_string);
        exit(1);
    }

    struct xmlrpc_method_info3 const methodInfo1 = {
        /* .methodName     = */ "getcell",
        /* .methodFunction = */ &getcell,
    };

 xmlrpc_registry_add_method3(&env, registryP, &methodInfo1);
    if (env.fault_occurred) {
        printf("xmlrpc_registry_add_method3() failed.  %s\n",
               env.fault_string);
        exit(1);
    }
 struct xmlrpc_method_info3 const methodInfo2 = {
        /* .methodName     = */ "getsheets",
        /* .methodFunction = */ &getsheets,
    };

 xmlrpc_registry_add_method3(&env, registryP, &methodInfo2);
    if (env.fault_occurred) {
        printf("xmlrpc_registry_add_method3() failed.  %s\n",
               env.fault_string);
        exit(1);
    }

    
    
    serverparm.config_file_name = NULL;   /* Select the modern normal API */
    serverparm.registryP        = registryP;
    serverparm.port_number      = atoi(argv[1]);
    serverparm.log_file_name    = "/tmp/xmlrpc_log";

    printf("Running XML-RPC server...\n");

    xmlrpc_server_abyss(&env, &serverparm, XMLRPC_APSIZE(log_file_name));
    if (env.fault_occurred) {
        printf("xmlrpc_server_abyss() failed.  %s\n", env.fault_string);
        exit(1);
    }
    /* xmlrpc_server_abyss() never returns unless it fails */

    return 0;
}


