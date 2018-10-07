/* A simple synchronous XML-RPC client program written in C, as an example of
   an Xmlrpc-c client.  This invokes the sample.add procedure that the
   Xmlrpc-c example xmlrpc_sample_add_server.c server provides.  I.e. it adds
   two numbers together, the hard way.

   This sends the RPC to the server running on the local system ("localhost"),
   HTTP Port 8080.

   This program uses the Xmlrpc-c global client, which uses the default
   client XML transport.
*/

#include <stdlib.h>
#include <stdio.h>

#include <xmlrpc-c/base.h>
#include <xmlrpc-c/client.h>

//#include "config.h"  /* information about this build environment */

#define NAME "Xmlrpc-c Test Client"
#define VERSION "1.0"

const char * const serverUrl = "http://localhost:8083/RPC2";




static void
dieIfFaultOccurred (xmlrpc_env * const envP) {
    if (envP->fault_occurred) {
        fprintf(stderr, "ERROR: %s (%d)\n",
                envP->fault_string, envP->fault_code);
	//  exit(1);
    }
}




getcell(xmlrpc_env env, int x, int y)
{
 xmlrpc_value * resultP;
    xmlrpc_value * ar;
    xmlrpc_double sum;
    xmlrpc_int32 flag;
    const char *name;
    xmlrpc_value * firstElementP;
const char * const methodName = "getcell";

  /* Make the remote procedure call */
    resultP = xmlrpc_client_call(&env, serverUrl, methodName,
                                 "(sii)","roman", (xmlrpc_int32) 0, (xmlrpc_int32) 0);
    dieIfFaultOccurred(&env);

     printf("Array has %u elements\n", xmlrpc_array_size(&env, resultP));
        /* prints "Array has 2 elements" */
    
   
     xmlrpc_array_read_item(&env,resultP,0,&ar);    
    /* Get our sum and print it out. */
    xmlrpc_read_double(&env, ar, &sum);
    xmlrpc_DECREF(ar);
     xmlrpc_array_read_item(&env,resultP,1,&ar); 
    xmlrpc_read_int(&env, ar, &flag);
    xmlrpc_DECREF(ar);
     xmlrpc_array_read_item(&env,resultP,2,&ar); 
    xmlrpc_read_string(&env,ar,&name);
    xmlrpc_DECREF(ar);
    dieIfFaultOccurred(&env);
    printf("The sum is %lf %x %s\n", sum, flag, name);

    /* Dispose of our result value. */
    xmlrpc_DECREF(resultP);

    
    
}


getsheets(xmlrpc_env env)
{
 xmlrpc_value * resultP;
    xmlrpc_value * ar;
    xmlrpc_double sum;
    xmlrpc_int32 flag;
    const char *name;
    xmlrpc_value * firstElementP;
const char * const methodName = "getsheets";
 int a,cnt=0;
  /* Make the remote procedure call */
    resultP = xmlrpc_client_call(&env, serverUrl, methodName,
                                 "()",0);
    dieIfFaultOccurred(&env);

    cnt=xmlrpc_array_size(&env, resultP);
     printf("Array has %u elements\n", cnt);
        /* prints "Array has 2 elements" */
    
     for( a=0;a<cnt;a++){
     xmlrpc_array_read_item(&env,resultP,a,&ar); 
    xmlrpc_read_string(&env,ar,&name);
    xmlrpc_DECREF(ar);
    dieIfFaultOccurred(&env);
    printf("The Names %s\n", name);
     }
    /* Dispose of our result value. */
    xmlrpc_DECREF(resultP);

    
    
}



int
main(int           const argc,
     const char ** const argv) {

    xmlrpc_env env;
    xmlrpc_value * resultP;
    xmlrpc_value * ar;
    xmlrpc_double sum;
    xmlrpc_int32 flag;
    const char *name;
    xmlrpc_value * firstElementP;
    const char * const serverUrl = "http://localhost:8083/RPC2";
    const char * const methodName = "getcell";
    // xmlrpc_value * const myArrayP =
    //    xmlrpc_build_value(envP, "(dis)", 5, 7);

    if (argc-1 > 0) {
        fprintf(stderr, "This program has no arguments\n");
        exit(1);
    }

    /* Initialize our error-handling environment. */
    xmlrpc_env_init(&env);

    /* Create the global XML-RPC client object. */
    xmlrpc_client_init2(&env, XMLRPC_CLIENT_NO_FLAGS, NAME, VERSION, NULL, 0);
    dieIfFaultOccurred(&env);

    printf("Making XMLRPC call to server url '%s' method '%s' "
           "to request the sum "
           "of 5 and 7...\n", serverUrl, methodName);

    #if 0
    /* Make the remote procedure call */
    resultP = xmlrpc_client_call(&env, serverUrl, methodName,
                                 "(sii)","roman", (xmlrpc_int32) 0, (xmlrpc_int32) 0);
    dieIfFaultOccurred(&env);

     printf("Array has %u elements\n", xmlrpc_array_size(&env, resultP));
        /* prints "Array has 2 elements" */
    
   
     xmlrpc_array_read_item(&env,resultP,0,&ar);    
    /* Get our sum and print it out. */
    xmlrpc_read_double(&env, ar, &sum);
    xmlrpc_DECREF(ar);
     xmlrpc_array_read_item(&env,resultP,1,&ar); 
    xmlrpc_read_int(&env, ar, &flag);
    xmlrpc_DECREF(ar);
     xmlrpc_array_read_item(&env,resultP,2,&ar); 
    xmlrpc_read_string(&env,ar,&name);
    xmlrpc_DECREF(ar);
    dieIfFaultOccurred(&env);
    printf("The sum is %lf %x %s\n", sum, flag, name);

    /* Dispose of our result value. */
    xmlrpc_DECREF(resultP);

#endif
    getcell(env,0,0);
    getsheets(env);
    /* Clean up our error-handling environment. */
    xmlrpc_env_clean(&env);

    /* Shutdown our XML-RPC client library. */
    xmlrpc_client_cleanup();

    return 0;
}


