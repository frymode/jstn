#include "jstn.h"
#include <stdlib.h>
#include <string.h>

static jstnres_t jstn_parse_next(jstn_parser *parser, const char **chunk, const char* chunk_end);
static jstnres_t jstn_parse_primitive(jstn_parser *parser, const char **chunk, const char* chunk_end);
static jstnres_t jstn_parse_string(jstn_parser *parser, const char **chunk, const char* chunk_end);
static jstnres_t jstn_set_token_value(jstn_parser *parser, const char *start, const char* end);
static jstnres_t jstn_set_token_partial_value(jstn_parser *parser, const char *start, const char* end);

void jstn_init(jstn_parser *parser, char *value_buf, size_t value_buf_size)
{
    parser->buf = value_buf;
    parser->buf_size = value_buf_size;
    parser->state = JSTN_STATE_IDLE;

    parser->token.type = JSTN_PRIMITIVE;
    parser->token.value = NULL;
    parser->token.len = 0;
}

jstn_token_t jstn_get_token(jstn_parser *parser)
{
    return parser->token;
}

int jstn_eof_allowed(jstn_parser *parser)
{
    return parser->token.type != JSTN_OBJECT_BEGIN &&
           parser->token.type != JSTN_ARRAY_BEGIN;
}

jstnres_t jstn_read(jstn_parser *parser, const char **chunk, const char* chunk_end)
{
    jstnres_t res = jstn_parse_next(parser, chunk, chunk_end);
    if (res == JSTN_SUCCESS)
    {
        if (parser->token.type != JSTN_MAP_KEY &&
            parser->token.type != JSTN_PRIMITIVE)
        {
            ++*chunk;
        }
    }

    return res;
}

static jstnres_t jstn_parse_next(jstn_parser *parser, const char **chunk, const char* chunk_end)
{
    switch (parser->state)
    {
        case JSTN_STATE_STRING_START:
        case JSTN_STATE_STRING_ESCAPE:
        case JSTN_STATE_MAP_KEY_START:
        case JSTN_STATE_MAP_KEY_END:
            return jstn_parse_string(parser, chunk, chunk_end);

        case JSTN_STATE_PARSE_PRIMITIVE_START:
            return jstn_parse_primitive(parser, chunk, chunk_end);
    }

    jstn_token_t* token = &parser->token;
    token->len = 0;
    for ( ; *chunk != chunk_end; ++*chunk)
    {
        switch (**chunk)
        {
            case ' ':
            case '\t':
            case '\r':
            case '\n':
            case ':':
            case ',':
                break;

            case '{':
                token->type = JSTN_OBJECT_BEGIN;
                return JSTN_SUCCESS;

            case '}':
                token->type = JSTN_OBJECT_END;
                return JSTN_SUCCESS;

            case '[':
                token->type = JSTN_ARRAY_BEGIN;
                return JSTN_SUCCESS;

            case ']':
                token->type = JSTN_ARRAY_END;
                return JSTN_SUCCESS;

            case '\"':
                ++*chunk;
                if (token->type == JSTN_MAP_KEY)
                {
                    parser->state = JSTN_STATE_STRING_START;
                }
                else
                {
                    parser->state = JSTN_STATE_MAP_KEY_START;
                }
                token->type = JSTN_STRING;
                return jstn_parse_string(parser, chunk, chunk_end);

            default:
                parser->state = JSTN_STATE_PARSE_PRIMITIVE_START;
                token->type = JSTN_PRIMITIVE;
                return jstn_parse_primitive(parser, chunk, chunk_end);
        }
    }

    return JSTN_ERROR_NEED_MORE;
}

static jstnres_t jstn_parse_primitive(jstn_parser *parser, const char **chunk, const char* chunk_end)
{
    const char* start = *chunk;
    for ( ; *chunk != chunk_end; ++*chunk)
    {
        char c = **chunk;
        switch (c)
        {
            case '\t':
            case '\r':
            case '\n':
            case ' ':
            case ',':
            case ']':
            case '}':
                return jstn_set_token_value(parser, start, *chunk);
        }

        if (c < 32 || c >= 127)
        {
            return JSTN_ERROR_INVAL;
	}
    }

    jstnres_t res = jstn_set_token_partial_value(parser, start, *chunk);
    if (res != JSTN_SUCCESS)
    {
        return res;
    }

    return JSTN_ERROR_NEED_MORE;
}

static jstnres_t jstn_parse_string(jstn_parser *parser, const char **chunk, const char* chunk_end)
{
    jstnres_t res;
    const char* start = *chunk;
    for ( ; *chunk != chunk_end; ++*chunk)
    {
        char c = **chunk;
        switch (parser->state)
        {
            case JSTN_STATE_STRING_START:
            case JSTN_STATE_MAP_KEY_START:
                switch (c)
                {
                    case '\"':
                        res = jstn_set_token_value(parser, start, *chunk);
                        if (res != JSTN_SUCCESS)
                        {
                            return res;
                        }

                        if (parser->state == JSTN_STATE_STRING_START)
                        {
                            return res;
                        }
                        else
                        {
                            parser->state = JSTN_STATE_MAP_KEY_END;
                        }
                        break;

                    case '\\':
                        res = jstn_set_token_partial_value(parser, start, *chunk);
                        if (res != JSTN_SUCCESS)
                        {
                            return res;
                        }
                        parser->state = parser->state == JSTN_STATE_STRING_START
                                        ? JSTN_STATE_STRING_ESCAPE
                                        : JSTN_STATE_MAP_KEY_ESCAPE;
                        break;
                }
                break;
            case JSTN_STATE_MAP_KEY_END:
                switch (c)
                {
                    case ' ':
                    case '\t':
                    case '\r':
                    case '\n':
                        break;
                    case ':':
                        parser->token.type = JSTN_MAP_KEY;
                        parser->state = JSTN_STATE_IDLE;
                        return JSTN_SUCCESS;
                    default:
                        parser->state = JSTN_STATE_IDLE;
                        return JSTN_SUCCESS;
                }
                break;
            case JSTN_STATE_STRING_ESCAPE:
            case JSTN_STATE_MAP_KEY_ESCAPE:
                switch (c)
                {
                    case '/':
                    case '\"':
                    case '\\':
                        break;
                    case 'b':
                        c = '\b';
                        break;
                    case 'f':
                        c = '\f';
                        break;
                    case 'r':
                        c = '\r';
                        break;
                    case 'n':
                        c = '\n';
                        break;
                    case 't':
                        c = '\t';
                        break;
                    default:
                        return JSTN_ERROR_INVAL;
                }

                res = jstn_set_token_partial_value(parser, &c, &c + 1);
                if (res != JSTN_SUCCESS)
                {
                    return res;
                }

                start = *chunk + 1;
                parser->state = parser->state == JSTN_STATE_STRING_ESCAPE 
                                ? JSTN_STATE_STRING_START
                                : JSTN_STATE_MAP_KEY_START;
                break;
        }
    }

    switch (parser->state)
    {
        case JSTN_STATE_STRING_START:
        case JSTN_STATE_MAP_KEY_START:
            res = jstn_set_token_partial_value(parser, start, *chunk);
            if (res != JSTN_SUCCESS)
            {
                return res;
            }
            break;
    }

    return JSTN_ERROR_NEED_MORE;
}

static jstnres_t jstn_set_token_value(jstn_parser *parser, const char *start, const char* end)
{
    parser->state = JSTN_STATE_IDLE;
    jstn_token_t* token = &parser->token;
    if (token->value != parser->buf || token->len == 0)
    {
        token->value = start;
        token->len = end - start;
        return JSTN_SUCCESS;
    }
    else
    {
        return jstn_set_token_partial_value(parser, start, end);
    }
}

static jstnres_t jstn_set_token_partial_value(jstn_parser *parser, const char *start, const char* end)
{
    jstn_token_t* token = &parser->token;
    token->value = parser->buf;

    size_t chunk_size = end - start;
    if (token->len + chunk_size >= parser->buf_size)
    {
        return JSTN_ERROR_NOMEM;
    }
    else
    {
        memcpy(parser->buf + token->len, start, chunk_size);
        token->len += chunk_size;
        return JSTN_SUCCESS;
    }
}