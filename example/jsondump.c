#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "../jstn.h"

#define READ_BUF_SIZE 1
#define VALUE_BUF_SIZE 10

static void print_indent(int depth)
{
    if (depth > 0)
    {
        printf("%*c", depth * 2, ' ');
    }
}

/*
 * An example of reading JSON from stdin and printing its content to stdout.
 */
int main(int argc, char* argv[])
{
    char buf[READ_BUF_SIZE];
    char value_buf[VALUE_BUF_SIZE];

    jstntype_t prev_type = JSTN_NULL;
    jstnres_t r = JSTN_SUCCESS;
    int depth = 0;

    jstn_parser p;
    jstn_init(&p, value_buf, sizeof(value_buf));

    for (;;)
    {
        int n = fread(buf, 1, sizeof(buf), stdin);
        if (n < 0)
        {
            fprintf(stderr, "fread(): %d, errno=%d\n", n, errno);
            return 1;
        }
        else if (n == 0)
        {
            if (jstn_eof_allowed(&p))
            {
                return 0;
            }
            else
            {
                fprintf(stderr, "fread(): unexpected EOF\n");
                return 2;
            }
        }

        const char *chunk_start = buf;
        const char *chunk_end = buf + n;

        while ((r = jstn_read(&p, &chunk_start, chunk_end)) == JSTN_SUCCESS)
        {
            switch (p.token.type)
            {
                case JSTN_OBJECT_BEGIN:
                case JSTN_ARRAY_BEGIN:
                    if (prev_type == JSTN_MAP_KEY)
                    {
                        printf("\n");
                    }
                    depth++;
                    break;
                case JSTN_OBJECT_END:
                case JSTN_ARRAY_END:
                    depth--;
                    break;
                case JSTN_MAP_KEY:
                    print_indent(depth);
                    printf("\"%.*s\" : ", p.token.len, p.token.value);
                    break;
                case JSTN_STRING:
                    if (prev_type != JSTN_MAP_KEY)
                    {
                        print_indent(depth);
                    }
                    printf("\"%.*s\"\n", p.token.len, p.token.value);
                    break;
                case JSTN_PRIMITIVE:
                    if (prev_type != JSTN_MAP_KEY)
                    {
                        print_indent(depth);
                    }
                    printf("%.*s\n", p.token.len, p.token.value);
                    break;
            }

            prev_type = p.token.type;
        }
        fflush(stdout);

        if (r != JSTN_ERROR_NEED_MORE)
        {
            fprintf(stderr, "parse error: %d\n", r);
            return 1;
        }
    }

    return 0;
}
