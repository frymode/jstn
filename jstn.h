#ifndef __JSTN_H_
#define __JSTN_H_

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    JSTN_NULL,
    JSTN_PRIMITIVE,
    JSTN_STRING,
    JSTN_MAP_KEY,
    JSTN_OBJECT_BEGIN,
    JSTN_OBJECT_END,
    JSTN_ARRAY_BEGIN,
    JSTN_ARRAY_END
} jstntype_t;

typedef enum {
    JSTN_SUCCESS,
    JSTN_ERROR_NOMEM,
    JSTN_ERROR_INVAL,
    JSTN_ERROR_NEED_MORE
} jstnres_t;

typedef struct {
    jstntype_t type;
    const char* value;
    size_t len;
} jstn_token_t;

typedef enum {
    JSTN_STATE_IDLE,
    JSTN_STATE_PARSE_PRIMITIVE_START,
    JSTN_STATE_STRING_START,
    JSTN_STATE_STRING_ESCAPE,
    JSTN_STATE_MAP_KEY_START,
    JSTN_STATE_MAP_KEY_END,
    JSTN_STATE_MAP_KEY_ESCAPE,
} jstn_parser_state_t;

typedef struct {
    char *buf;
    size_t buf_size;
    jstn_token_t token;

    jstn_parser_state_t state;
} jstn_parser;


void jstn_init(jstn_parser *parser, char *value_buf, size_t value_buf_size);

jstnres_t jstn_read(jstn_parser *parser, const char **chunk, const char* chunk_end);
jstn_token_t jstn_get_token(jstn_parser *parser);
int jstn_eof_allowed(jstn_parser *parser);

#ifdef __cplusplus
}
#endif

#endif /* __JSTN_H_ */
