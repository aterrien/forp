 /*
  +----------------------------------------------------------------------+
  | PHP Version 5                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2011 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author: Anthony Terrien <forp@anthonyterrien.com>                    |
  +----------------------------------------------------------------------+
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "SAPI.h"

#include "forp.h"
#include "php_forp.h"
#include "forp_string.h"
#include "forp_json.h"

#ifdef ZTS
#include "TSRM.h"
#endif

#if defined(PHP_WIN32)
double round(double d)
{
  return floor(d + 0.5);
}
#endif

/* {{{ forp_json
 */
void forp_json(TSRMLS_D) {
    int i, j;
    // TODO Win path
    char psep[] = "/";
    char epsep[] = "\\/";
    char nssep[] = "\\";
    char enssep[] = "\\\\";
    forp_node_t *n;

    /*
    {
                    utime : 4000,
                    stime : 4000,
                    stack : [
                        {
                            file : "\/var\/www\/app\/myfile.php",
                            usec : 100,
                            bytes : 64,
                            function : "{main}",
                            caption : "This is the caption of {main}.",
                            level : 0
                        },
                        {
                            file : "\/var\/www\/app\/myfile.php",
                            lineno : 2,
                            usec : 50,
                            bytes : 64,
                            class : "MyClass",
                            function : "myFunction",
                            caption : "This is the caption of an entry.",
                            groups : ["Example", "Foo"],
                            level : 1,
                            parent : 0
                        }
                    ]
                }
    */

    if (FORP_G(started)) {
        forp_end(TSRMLS_C);
    }

    if(FORP_G(stack_len)) {

        php_printf("{");
        php_printf("\"utime\":%0.0f,", FORP_G(utime));
        php_printf("\"stime\":%0.0f,", FORP_G(stime));
        php_printf("\"stack\":[");

        for (i = 0; i < FORP_G(stack_len); ++i) {

            php_printf("{");

            n = FORP_G(stack)[i];

            if (n->function.filename) {
                php_printf(
                    "\"%s\":\"%s\",",
                    FORP_DUMP_ASSOC_FILE,
                    forp_str_replace(psep,epsep,n->function.filename TSRMLS_CC)
                );
            } else if (n->filename) {
                php_printf(
                    "\"%s\":\"%s\",",
                    FORP_DUMP_ASSOC_FILE,
                    forp_str_replace(psep,epsep,n->filename TSRMLS_CC)
                );
            } else {
                php_printf(
                    "\"%s\":\"**internal**\",",
                    FORP_DUMP_ASSOC_FILE
                );
            }

            if (n->function.class) {
                php_printf(
                    "\"%s\":\"%s\",",
                    FORP_DUMP_ASSOC_CLASS,
                    forp_str_replace(nssep,enssep,n->function.class TSRMLS_CC)
                );
            }

            if (n->alias) {
                php_printf("\"%s\":\"%s\",", FORP_DUMP_ASSOC_FUNCTION, n->alias);
            } else if (n->function.function) {
                php_printf("\"%s\":\"%s\",", FORP_DUMP_ASSOC_FUNCTION, n->function.function);
            }

            if (n->lineno)
                php_printf("\"%s\":%d,", FORP_DUMP_ASSOC_LINENO, n->lineno);

            if (n->function.groups && n->function.groups_len > 0) {
                j = 0;
                php_printf("\"%s\":[", FORP_DUMP_ASSOC_GROUPS);
                while(j < n->function.groups_len) {
                    php_printf("\"%s\"", n->function.groups[j]);
                    if(j < n->function.groups_len - 1)
                        php_printf(",");
                    j++;
                }
                php_printf("],");
            }


            if (n->caption) {
                php_printf("\"%s\":\"%s\",", FORP_DUMP_ASSOC_CAPTION, n->caption);
            }

            if(FORP_G(flags) & FORP_FLAG_TIME) {
                php_printf("\"%s\":%0.0f,", FORP_DUMP_ASSOC_DURATION, round(n->time * 1000000.0) / 1000000.0);
                php_printf("\"%s\":%0.0f,", FORP_DUMP_ASSOC_PROFILERTIME, round(n->profiler_duration * 1000000.0) / 1000000.0);
            }

            if(FORP_G(flags) & FORP_FLAG_MEMORY) {
                php_printf("\"%s\":%d,", FORP_DUMP_ASSOC_MEMORY, n->mem);
            }

            if (n->parent) {
                php_printf("\"%s\":%d,", FORP_DUMP_ASSOC_PARENT, n->parent->key);
                //add_assoc_long(entry, FORP_DUMP_ASSOC_PARENT, n->parent->key);
            }

            php_printf("\"%s\":%d", FORP_DUMP_ASSOC_LEVEL, n->level);

            php_printf("}");

            if(i < FORP_G(stack_len) - 1) php_printf(",");
        }
        if(FORP_G(inspect_len)) {
            php_printf("],\"inspect\":{");
            for (i = 0; i < FORP_G(inspect_len); i++) {
                php_printf("\"%s\": {",FORP_G(inspect)[i]->name);
                forp_json_inspect(FORP_G(inspect)[i] TSRMLS_CC);
                if ( i + 1 < FORP_G(inspect_len) )  {
                    php_printf("},");
                } else {
                    php_printf("}");
                }
            }
            php_printf("}}");
        } else {
            php_printf("]}");
        }
    }
}

/* {{{ forp_json_inspect
 * Recursive output inspect structure
 */
void forp_json_inspect(forp_var_t *var TSRMLS_DC) {
		uint i;
		if(var->stack_idx > -1) php_printf("\"stack_idx\":%d,", var->stack_idx);
    if(var->type) php_printf("\"type\":\"%s\",", var->type);
    if(var->level) php_printf("\"level\":\"%s\",", var->level);
		if(var->class) php_printf("\"class\":\"%s\",", var->class);
		if(var->is_ref) {
        php_printf("\"is_ref\": true,");
        if(var->refcount > 1) php_printf("\"refcount\":%d,", var->refcount);
    }
		if(var->arr_len) {
				if(strcmp(var->type, "object") == 0) {
						php_printf("\"properties\":{");
				} else {
						php_printf("\"value\":{");
				}
				for(i = 0; i < var->arr_len; i++) {
						php_printf("\"%s\": {", forp_addslashes(var->arr[i]->key TSRMLS_CC));
            forp_json_inspect(var->arr[i] TSRMLS_CC);
						if ( i + 1 < var->arr_len ) {
								php_printf("},");
						} else {
								php_printf("}");
						}
        }
				php_printf("}");
		} else {
				php_printf("\"value\":\"%s\"", forp_addslashes(var->value TSRMLS_CC));
		}
}

/* {{{ forp_json_google_tracer
 */
void forp_json_google_tracer(char *filepath TSRMLS_DC) {
    FILE *fp;

    int i, j;
    // TODO Win path
    char psep[] = "/";
    char epsep[] = "\\/";
    char nssep[] = "\\";
    char enssep[] = "\\\\";
    forp_node_t *n;

    /*
    {
   "traceEvents":[
      {
         "name":"main",
         "cat":"PHP",
         "ts":1407355698849,
         "pid":0,
         "tid":0,
         "ph":"B"
      },
      ...
    */

    if (FORP_G(started)) {
        forp_end(TSRMLS_C);
    }

    if(FORP_G(stack_len)) {
        // First, delete existing file
        unlink(filepath);
        // Then, open and append everything
        fp = fopen(filepath, "a");

        fprintf(fp,"{");
        fprintf(fp,"\"traceEvents\":[");
        // Some metadata
        fprintf(fp,"{\"args\": {\"name\": \"%s\"}, \"cat\": \"__metadata\", \"name\": \"process_name\", \"ph\": \"M\", \"pid\": 0, \"tid\": 0, \"ts\": 0 },", sapi_module.name);
        fprintf(fp,"{\"args\": {\"name\": \"PHP\"}, \"cat\": \"__metadata\", \"name\": \"thread_name\", \"ph\": \"M\", \"pid\": 0, \"tid\": 0, \"ts\": 0 },");
        for (i = 0; i < FORP_G(stack_len); ++i) {

            fprintf(fp,"{");
            fprintf(fp,"\"%s\":\"%s\",", "ph", "X"); // "Complete event"
            fprintf(fp,"\"%s\":\"%s\",", "pid", "0"); // Fake PID
            fprintf(fp,"\"%s\":\"%s\",", "tid", "0"); // Fake TID
            n = FORP_G(stack)[i];

            if (n->alias) {
                fprintf(fp,"\"%s\":\"%s\",", "name", n->alias);
                } else if (n->function.function) {
                if (n->function.class) {
                    // class and method
                    fprintf(fp,
                        "\"%s\":\"%s::%s\",",
                        "name",
                        forp_str_replace(nssep,enssep,n->function.class TSRMLS_CC),
                        n->function.function
                        );
                    }
                else {
                    // simple function
                    fprintf(fp,"\"%s\":\"%s\",", "name", n->function.function);
                    }
            }

            if (n->function.groups && n->function.groups_len > 0) {
                j = 0;
                fprintf(fp,"\"%s\":\"", "cat");
                while(j < n->function.groups_len) {
                    fprintf(fp,"%s", n->function.groups[j]);
                    if(j < n->function.groups_len - 1){
                        fprintf(fp,",");
                        }
                    j++;
                }
                fprintf(fp,"\",");
                }
            else {
                fprintf(fp,"\"%s\":\"%s\",", "cat", "PHP");
                }

            if(FORP_G(flags) & FORP_FLAG_TIME) {
                fprintf(fp,"\"%s\":%0.0f,", "dur", round(n->time * 1000000.0) / 1000000.0);
                fprintf(fp,"\"%s\":%0.0f", "ts", n->time_begin);
                // fprintf(fp,"\"%s\":%0.0f,", FORP_DUMP_ASSOC_PROFILERTIME, round(n->profiler_duration * 1000000.0) / 1000000.0);
            }

            fprintf(fp,"}");
            if(i < FORP_G(stack_len) - 1) {
                fprintf(fp,",");
                }
        }

        fprintf(fp,"]}");
        fclose(fp);
        return;
    }
}
