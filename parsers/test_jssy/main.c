#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include "jssy.h"

typedef enum testStatus {ERROR, PASS, FAIL} TestStatus;

static void append(char **buf, size_t *pos, size_t *max, const char *str, size_t len)
{
    size_t oldpos = *pos;
    *pos += len;
    if(*pos >= *max)
    {
        while(*pos >= *max)
        {
            *max *= 2;
        }
        *buf = realloc(*buf, *max);
    }
    strncat(*buf + oldpos, str, len);
}

static int serializeRecursively(jssytok_t *token, char **buf, size_t *pos, size_t *max, size_t limit)
{
    if(token == NULL || limit == 0)
    {
        return 0;
    }
    switch(token->type)
    {
        case JSSY_DICT:
        case JSSY_ARRAY:
            {
                append(buf, pos, max, token->type == JSSY_DICT ? "{" : "[", 1);
                serializeRecursively(token->subval, buf, pos, max, token->size);
                append(buf, pos, max, token->type == JSSY_DICT ? "}" : "]", 1);
            }
            break;

        case JSSY_DICT_KEY:
        case JSSY_STRING:
            {
                size_t idx = 0;
                char *str = malloc(token->size * 2 + 2);
                str[idx] = '"';
                ++idx;
                for(size_t i = 0; i < token->size; ++i)
                {
                    char c = token->value[i];
                    if(c == '"' || c == '\\')
                    {
                        str[idx] = '\\';
                        ++idx;
                    }
                    str[idx] = c;
                    ++idx;
                }
                str[idx] = '"';
                ++idx;
                append(buf, pos, max, str, idx);
                free(str);
                if(token->type == JSSY_DICT_KEY)
                {
                    append(buf, pos, max, ":", 1);
                    serializeRecursively(token->subval, buf, pos, max, 1);
                }
            }
            break;

        case JSSY_PRIMITIVE:
            {
                char *str = NULL;
                asprintf(&str, "%g", token->numval);
                append(buf, pos, max, str, strlen(str));
                free(str);
            }
            break;

        default:
            fprintf(stderr, "Unknown token type: %d\n", token->type);
            return 1;
            break;
    }
    if(token->next != NULL && limit > 1)
    {
        append(buf, pos, max, ",", 1);
        serializeRecursively(token->next, buf, pos, max, limit - 1);
    }
    return 0;
}

static const char* serializeJson(jssytok_t *tokens)
{
    size_t pos = 0,
           max = 1024;
    char *buf = malloc(max);
    if(serializeRecursively(tokens, &buf, &pos, &max, ~0) != 0)
    {
        return NULL;
    }
    buf[pos] = '\0';
    return buf;
}

/* Parse text to JSON, then render back to text, and print! */
TestStatus parseData(char *data, long len, int printParsingResults) {
    long ret;
    ret = jssy_parse(data, len, NULL, 0);
    size_t tokensSize = ret * sizeof(jssytok_t);
    jssytok_t *tokens = malloc(tokensSize);
    ret = jssy_parse(data, len, tokens, tokensSize);

    if (ret < 0) {
        printf("-- no json\n");
        return FAIL;
    }

    if (printParsingResults) {
        printf("--  in: %s\n", data);
    }

    const char *out = serializeJson(tokens);
    if(!out) {
        return ERROR;
    }

    if (printParsingResults) {
        printf("-- out: %s\n", out);
    }
    return PASS;
}

/* Read a file, parse, render back, etc. */
TestStatus testFile(const char *filename, int printParsingResults) {

    FILE *f=fopen(filename,"rb");
    if(f == NULL) { return ERROR; };
    fseek(f,0,SEEK_END);
    long len=ftell(f);
    fseek(f,0,SEEK_SET);
    char *data=(char*)malloc(len+1);
    fread(data,1,len,f);
    data[len]='\0';
    fclose(f);
    TestStatus status = parseData(data, len, printParsingResults);
    free(data);
    return status;
}

int main(int argc, const char * argv[]) {

    const char* path = argv[1];

    int printParsingResults = 1;

    int result = testFile(path, printParsingResults);

    printf("-- result: %d\n", result);

    if (result == PASS) {
        return 0;
    } else {
        return 1;
    }
}
