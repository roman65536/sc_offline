#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include "rpsc.h"

#define	isBlank(ch)	((ch) == ',') 


/*
 * Take a command string and break it up into an argc, argv list while
 * handling quoting and wildcards.  The returned argument list and
 * strings are in static memory, and so are overwritten on each call.
 * The argument list is ended with a NULL pointer for convenience.
 * Returns TRUE if successful, or FALSE on an error with a message
 * already output.
 */
BOOL makeArgs(const char * cmd, int * retArgc, const char *** retArgv) {
    const char * argument;
    char * cp;
    char * cpOut;
    char * newStrings;
    const char ** fileTable;
    const char ** newArgTable;
    int newArgTableSize;
    int fileCount;
    int len;
    int ch;
    int quote;
    BOOL quotedWildCards;
    BOOL unquotedWildCards;

    static int stringsLength;
    static char * strings;
    static int argCount;
    static int argTableSize;
    static const char ** argTable;

    /*
     * Clear the returned values until we know them.
     */
    argCount = 0;
    *retArgc = 0;
    *retArgv = NULL;
    stringsLength=0;
    strings=0;
    argTableSize=0;
    argTable=0;
    /*
     * Copy the command string into a buffer that we can modify,
     * reallocating it if necessary.
     */
    len = strlen(cmd) + 1;

    if (len > stringsLength)
    {
        newStrings = realloc(strings, len);

        if (newStrings == NULL)
        {
            fprintf(stderr, "Cannot allocate string\n");

            return FALSE;
        }

        strings = newStrings;
        stringsLength = len;
    }

    memcpy(strings, cmd, len);
    cp = strings;

    /*
     * Keep parsing the command string as long as there are any
     * arguments left.
     */
    while (*cp)
    {
        /*
         * Save the beginning of this argument.
         */
        argument = cp;
        cpOut = cp;

        /*
         * Reset quoting and wildcarding for this argument.
         */
        quote = '\0';
        quotedWildCards = FALSE;
        unquotedWildCards = FALSE;

        /*
         * Loop over the string collecting the next argument while
         * looking for quoted strings or quoted characters, and
         * remembering whether there are any wildcard characters
         * in the argument.
         */
        while (*cp)
        {
            ch = *cp++;

            /*
             * If we are not in a quote and we see a blank then
             * this argument is done.
             */
            if (isBlank(ch) && (quote == '\0'))
                break;

            if (ch == ' ')
                continue;
            /*
             * If we see a backslash then accept the next
             * character no matter what it is.
             */
            if (ch == '\\')
            {
                ch = *cp++;

                /*
                 * Make sure there is a next character.
                 */
                if (ch == '\0')
                {
                    fprintf(stderr,
                            "Bad quoted character\n");

                    return FALSE;
                }

                /*
                 * Remember whether the quoted character
                 * is a wildcard.
                 */


                *cpOut++ = ch;

                continue;
            }



            /*
             * If we were in a quote and we saw the same quote
             * character again then the quote is done.
             */
            if (ch == quote)
            {
                quote = '\0';

                continue;
            }

            /*
             * If we weren't in a quote and we see either type
             * of quote character, then remember that we are
             * now inside of a quote.
             */
            if ((quote == '\0') && ((ch == '\'') || (ch == '"')))
            {
                quote = ch;

                continue;
            }


            /*
             * Store the character.
             */
            *cpOut++ = ch;
        }

        /*
         * Make sure that quoting is terminated properly.
         */
        if (quote)
        {
            fprintf(stderr, "Unmatched quote character\n");

            return FALSE;
        }

        /*
         * Null terminate the argument if it had shrunk, and then
         * skip over all blanks to the next argument, nulling them
         * out too.
         */
        if (cp != cpOut)
            *cpOut = '\0';

        while (isBlank(*cp))
            *cp++ = '\0';

        fileTable = &argument;
        fileCount = 1;


        /*
         * Now reallocate the argument table to hold the file name.
         */
        if (argCount + fileCount >= argTableSize)
        {
            newArgTableSize = argCount + fileCount + 1;

            newArgTable = (const char **) realloc(argTable,
                    (sizeof(const char *) * newArgTableSize));

            if (newArgTable == NULL)
            {
                fprintf(stderr, "No memory for arg list\n");

                return FALSE;
            }

            argTable = newArgTable;
            argTableSize = newArgTableSize;
        }

        /*
         * Copy the new arguments to the end of the old ones.
         */
        memcpy((void *) &argTable[argCount], (const void *) fileTable,
                (sizeof(const char **) * fileCount));

        /*
         * Add to the argument count.
         */
        argCount += fileCount;
    }

    /*
     * Null terminate the argument list and return it.
     */
    argTable[argCount] = NULL;

    *retArgc = argCount;
    *retArgv = argTable;

    return TRUE;
}


void export(struct roman * p, char * start, char * end, char * start_col, char * end_col, char * start_row, char * end_row) {
    register struct Ent ** pp;
    int x, y;
    int a = 0;
    char tmp[3];


    if (start !=0 ) printf("%s",start);

    for (x=0;x<p->cur_sh->col;x++) {
        if (start_col != 0) printf("%s",start_col);
        for (y=0;y<p->cur_sh->row;y++) {
            coltoa(y,tmp);
            if (start_row != 0) printf("%s %s",start_row,tmp);
            pp = ATBL(p->cur_sh->tbl,y,x);
            if (*pp != 0) {
                if (((*pp)->flag & RP_FORMULA) == RP_FORMULA) {
                    char *ptr=(*pp)->formula;
                    printf("%g",(*pp)->val);

                }
                if (((*pp)->flag & RP_LABEL) == RP_LABEL) printf("%s",(*pp)->label);

            }
            if (end_row != 0) printf("%s",end_row);

        }
        if (end_col != 0) printf("%s",end_col);

    }
    if(end !=0 ) printf("%s",end);
}
