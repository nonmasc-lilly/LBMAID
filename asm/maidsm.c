#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

typedef enum {
        TOKEN_TYPE_NULL,
        TOKEN_TYPE_IMMEDIATE,
        TOKEN_TYPE_IDENTIFIER,
        TOKEN_TYPE_LABEL,
        TOKEN_TYPE_MEM,
        TOKEN_TYPE_DB,
        TOKEN_TYPE_DW,
        TOKEN_TYPE_DD,
        TOKEN_TYPE_DOLLARSIGN,
        TOKEN_TYPE_PERCENT,
        TOKEN_TYPE_ACC,
        TOKEN_TYPE_B,
        TOKEN_TYPE_C,
        TOKEN_TYPE_XI,
        TOKEN_TYPE_YI,
        TOKEN_TYPE_SP,
        TOKEN_TYPE_BP,
        TOKEN_TYPE_FLG,
        TOKEN_TYPE_IP,
        TOKEN_TYPE_SHORT,
        TOKEN_TYPE_NEAR,
        TOKEN_TYPE_FAR,
        TOKEN_TYPE_BYTE,
        TOKEN_TYPE_WORD,
        TOKEN_TYPE_DWORD,
        TOKEN_TYPE_PLUS,
        TOKEN_TYPE_SHIFT,
        TOKEN_TYPE_ADD,
        TOKEN_TYPE_ADV,
        TOKEN_TYPE_AND,
        TOKEN_TYPE_CALL,
        TOKEN_TYPE_CMP,
        TOKEN_TYPE_CMPS,
        TOKEN_TYPE_CURPOS,
        TOKEN_TYPE_FADD,
        TOKEN_TYPE_FSUB,
        TOKEN_TYPE_GETCHAR,
        TOKEN_TYPE_HLT,
        TOKEN_TYPE_JC,
        TOKEN_TYPE_JNC,
        TOKEN_TYPE_JZ,
        TOKEN_TYPE_JNZ,
        TOKEN_TYPE_JO,
        TOKEN_TYPE_JS,
        TOKEN_TYPE_JMP,
        TOKEN_TYPE_LODS,
        TOKEN_TYPE_LOOP,
        TOKEN_TYPE_LOOPZ,
        TOKEN_TYPE_LOOPNZ,
        TOKEN_TYPE_LOD,
        TOKEN_TYPE_LDI,
        TOKEN_TYPE_MOV,
        TOKEN_TYPE_MOVS,
        TOKEN_TYPE_NOT,
        TOKEN_TYPE_OR,
        TOKEN_TYPE_POP,
        TOKEN_TYPE_POPF,
        TOKEN_TYPE_PUSH,
        TOKEN_TYPE_REP,
        TOKEN_TYPE_REPZ,
        TOKEN_TYPE_REPNZ,
        TOKEN_TYPE_RET,
        TOKEN_TYPE_SCAS,
        TOKEN_TYPE_STO,
        TOKEN_TYPE_STOS,
        TOKEN_TYPE_SUB,
        TOKEN_TYPE_SUV,
        TOKEN_TYPE_TEST,
        TOKEN_TYPE_XOR,
        TOKEN_TYPE_ZERO,
        TOKEN_TYPE_PUTCHAR,
        TOKEN_TYPE_PUSHF,
        TOKEN_TYPE_ORG,
        TOKEN_TYPE__MAX
} TOKEN_TYPE;

const char *token_type_str[TOKEN_TYPE__MAX] = {
        "NULL",         "immediate",    "identifier",
        "LABEL",        "MEM",          "DB",
        "DW",           "DD",           "$",
        "%",            "ACC",          "B",
        "C",            "XI",           "YI",
        "SP",           "BP",           "FLG",
        "IP",           "SHORT",        "NEAR",
        "FAR",          "BYTE",         "WORD",
        "DWORD",        "+",            "<",
        "ADD",          "ADV",          "AND",
        "CALL",         "CMP",          "CMPS",
        "CURPOS",       "FADD",         "FSUB",
        "GETCHAR",      "HLT",          "JC",
        "JNC",          "JZ",           "JNZ",
        "JO",           "JS",           "JMP",
        "LODS",         "LOOP",         "LOOPZ",
        "LOOPNZ",       "LOD",          "LDI",
        "MOV",          "MOVS",         "NOT",
        "OR",           "POP",          "POPF",
        "PUSH",         "REP",          "REPZ",
        "REPNZ",        "RET",          "SCAS",
        "STO",          "STOS",         "SUB",
        "SUV",          "TEST",         "XOR",
        "ZERO",         "PUTCHAR",      "PUSHF",
        "ORG"
};

struct token {
        union {
                char *lexeme;
                unsigned immediate;
        } value;
        TOKEN_TYPE type;
        unsigned char has_lexeme;
};

struct tokens {
        struct token *contents;
        unsigned length;
};

void create_tokens(struct tokens *ret);
void tokens_append(struct tokens *ret, TOKEN_TYPE type, char *lexeme, unsigned immediate);
void destroy_tokens(struct tokens *ret);
void tokenize(struct tokens *ret, const char *content);

void assemble(unsigned char **ret, unsigned *length, const struct tokens *input);

int main(int argc, char **argv) {
        FILE    *fp;
        char    *content;
        unsigned length, i;
        struct tokens tokens;

        if(argc < 3) {
                printf("Usage: %s <input file> <output file>\n", argv[0]);
                exit(1);
        }

        fp = fopen(argv[1], "r");
        fseek(fp, 0L, SEEK_END);
        length  = ftell(fp);
        content = calloc(1, length + 1);
        fseek(fp, 0L, SEEK_SET);
        fread(content, 1, length, fp);
        fclose(fp);

        create_tokens(&tokens);
        tokenize(&tokens, content);
        for(i = 0; i < tokens.length; i++) {
                printf("%04X: %s :: ", i, token_type_str[tokens.contents[i].type]);
                if(tokens.contents[i].has_lexeme) printf("%s\n", tokens.contents[i].value.lexeme);
                else printf("%04x\n", tokens.contents[i].value.immediate);
        }

        destroy_tokens(&tokens);
        exit(0);
}

void create_tokens(struct tokens *ret) {
        ret->contents = malloc(1);
        ret->length = 0;
}
void tokens_append(struct tokens *ret, TOKEN_TYPE type, char *lexeme, unsigned immediate) {
        ret->contents = realloc(ret->contents, ++ret->length * sizeof(*ret->contents));
        ret->contents[ret->length - 1].has_lexeme = 0;
        ret->contents[ret->length - 1].type = type;
        if(lexeme) {
                ret->contents[ret->length - 1].value.lexeme = malloc(strlen(lexeme)+1);
                strcpy(ret->contents[ret->length - 1].value.lexeme, lexeme);
                ret->contents[ret->length - 1].has_lexeme = 1;
        } else ret->contents[ret->length - 1].value.immediate = immediate;
}
void destroy_tokens(struct tokens *ret) {
        unsigned i;
        for(i = 0; i < ret->length; i++)
                if(ret->contents[i].has_lexeme) free(ret->contents[i].value.lexeme);
        free(ret->contents);
}
void tokenize(struct tokens *ret, const char *content) {
        unsigned i, j;
        long tmp;
        char *buffer, *tbuffer, *eptr;
        buffer = calloc(1,1);
        for(i = 0; content[i]; i++) {
                if(!isspace(content[i]) && content[i] != '+' && content[i] != '%' && content[i] != '$' && content[i] != '<' && content[i] != '\'' &&
                                content[i] != ';' && (content[i] != '/' && content[i+1] != '*')) {
                        buffer = realloc(buffer, strlen(buffer)+2);
                        buffer[strlen(buffer)+1] =          0;
                        buffer[strlen(buffer)]   = content[i];
                        continue;
                }
                if(*buffer) {
                        tbuffer = malloc(strlen(buffer)+1);
                        strcpy(tbuffer, buffer);
                        for(j = 0; buffer[j]; j++) buffer[j] = toupper(buffer[j]);
                        if(!strcmp(buffer, "LABEL"))            tokens_append(ret, TOKEN_TYPE_LABEL,    NULL, 0);
                        else if(!strcmp(buffer, "MEM"))         tokens_append(ret, TOKEN_TYPE_MEM,      NULL, 0);
                        else if(!strcmp(buffer, "DB"))          tokens_append(ret, TOKEN_TYPE_DB,       NULL, 0);
                        else if(!strcmp(buffer, "DW"))          tokens_append(ret, TOKEN_TYPE_DW,       NULL, 0);
                        else if(!strcmp(buffer, "DD"))          tokens_append(ret, TOKEN_TYPE_DD,       NULL, 0);
                        else if(!strcmp(buffer, "ACC"))         tokens_append(ret, TOKEN_TYPE_ACC,      NULL, 0);
                        else if(!strcmp(buffer, "B"))           tokens_append(ret, TOKEN_TYPE_B,        NULL, 0);
                        else if(!strcmp(buffer, "C"))           tokens_append(ret, TOKEN_TYPE_C,        NULL, 0);
                        else if(!strcmp(buffer, "XI"))          tokens_append(ret, TOKEN_TYPE_XI,       NULL, 0);
                        else if(!strcmp(buffer, "YI"))          tokens_append(ret, TOKEN_TYPE_YI,       NULL, 0);
                        else if(!strcmp(buffer, "SP"))          tokens_append(ret, TOKEN_TYPE_SP,       NULL, 0);
                        else if(!strcmp(buffer, "BP"))          tokens_append(ret, TOKEN_TYPE_BP,       NULL, 0);
                        else if(!strcmp(buffer, "FLG"))         tokens_append(ret, TOKEN_TYPE_FLG,      NULL, 0);
                        else if(!strcmp(buffer, "IP"))          tokens_append(ret, TOKEN_TYPE_IP,       NULL, 0);
                        else if(!strcmp(buffer, "SHORT"))       tokens_append(ret, TOKEN_TYPE_SHORT,    NULL, 0);
                        else if(!strcmp(buffer, "NEAR"))        tokens_append(ret, TOKEN_TYPE_NEAR,     NULL, 0);
                        else if(!strcmp(buffer, "FAR"))         tokens_append(ret, TOKEN_TYPE_FAR,      NULL, 0);
                        else if(!strcmp(buffer, "BYTE"))        tokens_append(ret, TOKEN_TYPE_BYTE,     NULL, 0);
                        else if(!strcmp(buffer, "WORD"))        tokens_append(ret, TOKEN_TYPE_WORD,     NULL, 0);
                        else if(!strcmp(buffer, "DWORD"))       tokens_append(ret, TOKEN_TYPE_DWORD,    NULL, 0);
                        else if(!strcmp(buffer, "ADD"))         tokens_append(ret, TOKEN_TYPE_ADD,      NULL, 0);
                        else if(!strcmp(buffer, "ADV"))         tokens_append(ret, TOKEN_TYPE_ADV,      NULL, 0);
                        else if(!strcmp(buffer, "AND"))         tokens_append(ret, TOKEN_TYPE_AND,      NULL, 0);
                        else if(!strcmp(buffer, "CALL"))        tokens_append(ret, TOKEN_TYPE_CALL,     NULL, 0);
                        else if(!strcmp(buffer, "CMP"))         tokens_append(ret, TOKEN_TYPE_CMP,      NULL, 0);
                        else if(!strcmp(buffer, "CMPS"))        tokens_append(ret, TOKEN_TYPE_CMPS,     NULL, 0);
                        else if(!strcmp(buffer, "CURPOS"))      tokens_append(ret, TOKEN_TYPE_CURPOS,   NULL, 0);
                        else if(!strcmp(buffer, "FADD"))        tokens_append(ret, TOKEN_TYPE_FADD,     NULL, 0);
                        else if(!strcmp(buffer, "FSUB"))        tokens_append(ret, TOKEN_TYPE_FSUB,     NULL, 0);
                        else if(!strcmp(buffer, "GETCHAR"))     tokens_append(ret, TOKEN_TYPE_GETCHAR,  NULL, 0);
                        else if(!strcmp(buffer, "HLT"))         tokens_append(ret, TOKEN_TYPE_HLT,      NULL, 0);
                        else if(!strcmp(buffer, "JC"))          tokens_append(ret, TOKEN_TYPE_JC,       NULL, 0);
                        else if(!strcmp(buffer, "JNC"))         tokens_append(ret, TOKEN_TYPE_JNC,      NULL, 0);
                        else if(!strcmp(buffer, "JZ"))          tokens_append(ret, TOKEN_TYPE_JZ,       NULL, 0);
                        else if(!strcmp(buffer, "JNZ"))         tokens_append(ret, TOKEN_TYPE_JNZ,      NULL, 0);
                        else if(!strcmp(buffer, "JO"))          tokens_append(ret, TOKEN_TYPE_JS,       NULL, 0);
                        else if(!strcmp(buffer, "JS"))          tokens_append(ret, TOKEN_TYPE_JO,       NULL, 0);
                        else if(!strcmp(buffer, "JMP"))         tokens_append(ret, TOKEN_TYPE_JMP,      NULL, 0);
                        else if(!strcmp(buffer, "LODS"))        tokens_append(ret, TOKEN_TYPE_LODS,     NULL, 0);
                        else if(!strcmp(buffer, "LOOP"))        tokens_append(ret, TOKEN_TYPE_LOOP,     NULL, 0);
                        else if(!strcmp(buffer, "LOOPZ"))       tokens_append(ret, TOKEN_TYPE_LOOPZ,    NULL, 0);
                        else if(!strcmp(buffer, "LOOPNZ"))      tokens_append(ret, TOKEN_TYPE_LOOPNZ,   NULL, 0);
                        else if(!strcmp(buffer, "LOD"))         tokens_append(ret, TOKEN_TYPE_LOD,      NULL, 0);
                        else if(!strcmp(buffer, "LDI"))         tokens_append(ret, TOKEN_TYPE_LDI,      NULL, 0);
                        else if(!strcmp(buffer, "MOV"))         tokens_append(ret, TOKEN_TYPE_MOV,      NULL, 0);
                        else if(!strcmp(buffer, "MOVS"))        tokens_append(ret, TOKEN_TYPE_MOVS,     NULL, 0);
                        else if(!strcmp(buffer, "NOT"))         tokens_append(ret, TOKEN_TYPE_NOT,      NULL, 0);
                        else if(!strcmp(buffer, "OR"))          tokens_append(ret, TOKEN_TYPE_OR,       NULL, 0);
                        else if(!strcmp(buffer, "POP"))         tokens_append(ret, TOKEN_TYPE_POP,      NULL, 0);
                        else if(!strcmp(buffer, "POPF"))        tokens_append(ret, TOKEN_TYPE_POPF,     NULL, 0);
                        else if(!strcmp(buffer, "PUSH"))        tokens_append(ret, TOKEN_TYPE_PUSH,     NULL, 0);
                        else if(!strcmp(buffer, "PUSHF"))       tokens_append(ret, TOKEN_TYPE_PUSHF,    NULL, 0);
                        else if(!strcmp(buffer, "REP"))         tokens_append(ret, TOKEN_TYPE_REP,      NULL, 0);
                        else if(!strcmp(buffer, "REPZ"))        tokens_append(ret, TOKEN_TYPE_REPZ,     NULL, 0);
                        else if(!strcmp(buffer, "REPNZ"))       tokens_append(ret, TOKEN_TYPE_REPNZ,    NULL, 0);
                        else if(!strcmp(buffer, "RET"))         tokens_append(ret, TOKEN_TYPE_RET,      NULL, 0);
                        else if(!strcmp(buffer, "SCAS"))        tokens_append(ret, TOKEN_TYPE_SCAS,     NULL, 0);
                        else if(!strcmp(buffer, "STO"))         tokens_append(ret, TOKEN_TYPE_STO,      NULL, 0);
                        else if(!strcmp(buffer, "STOS"))        tokens_append(ret, TOKEN_TYPE_STOS,     NULL, 0);
                        else if(!strcmp(buffer, "SUB"))         tokens_append(ret, TOKEN_TYPE_SUB,      NULL, 0);
                        else if(!strcmp(buffer, "SUV"))         tokens_append(ret, TOKEN_TYPE_SUV,      NULL, 0);
                        else if(!strcmp(buffer, "TEST"))        tokens_append(ret, TOKEN_TYPE_TEST,     NULL, 0);
                        else if(!strcmp(buffer, "XOR"))         tokens_append(ret, TOKEN_TYPE_XOR,      NULL, 0);
                        else if(!strcmp(buffer, "ZERO"))        tokens_append(ret, TOKEN_TYPE_ZERO,     NULL, 0);
                        else if(!strcmp(buffer, "PUTCHAR"))     tokens_append(ret, TOKEN_TYPE_PUTCHAR,  NULL, 0);
                        else if(!strcmp(buffer, "ORG"))         tokens_append(ret, TOKEN_TYPE_ORG,      NULL, 0);
                        else {
                                if(buffer[0] == '0' && buffer[1] == 'X') tmp = strtol(buffer + 2, &eptr, 0x10);
                                else if(buffer[strlen(buffer) - 1] == 'H') {
                                        buffer[strlen(buffer) - 1] = 0;
                                        tmp = strtol(buffer, &eptr, 0x10);
                                } else tmp = strtol(buffer, &eptr, 0x0A);
                                if(!*eptr) tokens_append(ret, TOKEN_TYPE_IMMEDIATE, NULL, *(unsigned*)&tmp);
                                else tokens_append(ret, TOKEN_TYPE_IDENTIFIER, tbuffer, 0);
                        }
                        free(tbuffer);
                        *buffer = 0;
                }

                switch(content[i]) {
                case '+':  tokens_append(ret,       TOKEN_TYPE_PLUS, NULL, 0); break;
                case '%':  tokens_append(ret,    TOKEN_TYPE_PERCENT, NULL, 0); break;
                case '$':  tokens_append(ret, TOKEN_TYPE_DOLLARSIGN, NULL, 0); break;
                case '<':  tokens_append(ret,      TOKEN_TYPE_SHIFT, NULL, 0); break;
                case '\'':
                        for(i++; content[i] != '\'' && content[i] != 0; i++)
                                tokens_append(ret, TOKEN_TYPE_IMMEDIATE, NULL, content[i]);
                        break;
                case ';': while(content[i] != '\n') i++; break;
                case '/': if(content[i+1] == '*') {
                                while(content[i] != '/' || content[i - 1] != '*') i++;
                        } break;
                }
        }
}

struct label {
        char *identifier;
        unsigned immediate;
};

struct labels {
        struct label *contents;
        unsigned length;
};

void create_labels(struct labels *ret) {
        ret->contents = malloc(1);
        ret->length   = 0;
}
void labels_append(struct labels *ret, const char *iden, unsigned addr) {
        ret->contents = realloc(ret->contents, ++ret->length * sizeof(*ret->contents));
        ret->contents[ret->length - 1].identifier = malloc(strlen(iden)+1);
        strcpy(ret->contents[ret->length - 1].identifier, iden);
        ret->contents[ret->length - 1].immediate = addr;
}
void destroy_labels(struct labels *ret) {
        unsigned i;
        for(i = 0; i < ret->length; i++)
                free(ret->contents[i].identifier);
        free(ret->contents);
}

void s_assert(unsigned char cond, const char *msg, ...) {
        va_list args;
        if(!cond) {
                va_start(args, msg);
                vprintf(msg, args);
                va_end(args);
                exit(1);
        }
}

void push(unsigned char **ret, unsigned *length, unsigned char *x, unsigned x_len) {
        *ret = realloc(*ret, *length + x_len);
        memcpy(*ret + *length, x, x_len);
}

void assemble(unsigned char **ret, unsigned *length, const struct tokens *input) {
        unsigned i, memory_len = 0x4000;
        struct labels labels;
        create_labels(&labels);
        *ret = malloc(1);
        *length = 0;
        
        for(i = 0; i < input->length; i++) {
                switch(input->contents[i].type) {
                case TOKEN_TYPE_LABEL:
                        s_assert(input->contents[++i].type == TOKEN_TYPE_IDENTIFIER, "label expects identifier (token %04x)\n", i);
                        labels_append(&labels, input->contents[i].value.lexeme, cur_addr);
                        break;
                case TOKEN_TYPE_MEM:
                        s_assert(input->contents[++i].type == TOKEN_TYPE_IMMEDIATE, "mem expects immediate (token %04x)\n", i);
                        if(memory_len < input->contents[i].value.immediate) memory_len = input->contents[i].value.immediate;
                        break;
                case TOKEN_TYPE_DB:
                        for(++i; input->contents[i].type != TOKEN_TYPE_PERCENT; i++) {
                                s_assert(input->contents[i].type == TOKEN_TYPE_IMMEDIATE || input->contents[i].type != TOKEN_TYPE_IDENTIFIER,
                                        "db expected immediate or label (token %04x", i);
                                if(input->contents[i].type == TOKEN_TYPE_push(ret, length, input->contents[i].value.immediate
                        }
                }
        }

        destroy_labels(&labels);
}
