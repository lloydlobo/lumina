#ifndef CF17C85F_D892_43F8_B47B_0BA265F170E6
#define CF17C85F_D892_43F8_B47B_0BA265F170E6

#include <asm-generic/errno-base.h>
#include <assert.h>
#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ERR_DBG(msg) fprintf(stderr, "--%d-- %s ", __LINE__, msg);
#define ERR_DBG_LN(msg) fprintf(stderr, "--%d-- %s\n", __LINE__, msg);

/*
 * Define macros to convert enum values to strings
 */
#define ENUM_TO_STR(e) #e
#define CASE_ENUM_TO_STR(e) \
    case e: \
        return #e

/* TokenType */
enum TokenType {
    TEXIT,
    TINT_LIT,
    TSEMICOLON,
    TPAREN_OPEN,
    TPAREN_CLOSE,
    TCURLY_OPEN,
    TCURLY_CLOSE,
    TIDENT,
    TLET,
    TEQUAL
};

const char *tokentype_to_str(enum TokenType self)
{
    switch (self) {
        CASE_ENUM_TO_STR(TEXIT);
        CASE_ENUM_TO_STR(TINT_LIT);
        CASE_ENUM_TO_STR(TSEMICOLON);
        CASE_ENUM_TO_STR(TPAREN_OPEN);
        CASE_ENUM_TO_STR(TPAREN_CLOSE);
        CASE_ENUM_TO_STR(TCURLY_OPEN);
        CASE_ENUM_TO_STR(TCURLY_CLOSE);
        CASE_ENUM_TO_STR(TIDENT);
        CASE_ENUM_TO_STR(TLET);
        CASE_ENUM_TO_STR(TEQUAL);
    }
    return "Unknown"; // Handle the case where self is not recognized.
}

/** token */
struct token {
    enum TokenType type;
    char *value;
};

/** tokenizer */
struct tokenizer {
    char *m_src;
    size_t m_index;
};

static int buf_clear(char array[], size_t *array_length)
{
    if (array == NULL)
        return -EBUSY; // see linux style guide
    for (int i = 0; i < *array_length; i++) {
        array[i] = '\0';
    }
    *array_length = 0;

    return 0; // success
}

/* impl struct token */

void token_free_tokens(struct token *self, int token_count)
{
    if (self == NULL)
        return;

    for (int i = 0; i < token_count; i++) { // clang-format off
        printf( "--%d-- Token %d: %u %s %s\n", __LINE__, i, self[i].type,
            tokentype_to_str(self[i].type), self[i].value); // clang-format on
        if (self[i].value != NULL)
            free(self[i].value);
    }
    free(self);
}

/* impl struct tokenizer */

struct tokenizer *t_init(const char *src)
{
    size_t size = sizeof(struct tokenizer);
    struct tokenizer *self = (struct tokenizer *)malloc(size);
    if (self == NULL) {
        perror("Failed to allocate memory for tokenizer\n");
        goto err_cleanup;
    }
    self->m_index = 0;
    self->m_src = strdup(src);
    if (self->m_src == NULL) {
        perror("Failed to duplicate source string\n");
        goto err_cleanup;
    }
    return self;

err_cleanup:

    if (self != NULL)
        free(self);
    return NULL;
}

void t_free(struct tokenizer *self)
{
    if (self == NULL)
        return;

    if (self->m_src != NULL)
        free(self->m_src);

    free(self);
}

/**
 * Returns `'\0'`(null character) or actual peeked char at offset (default is 0)
 */
static char t_peek(const struct tokenizer *self, int offset)
{
    if (self->m_index + offset >= (int)strlen(self->m_src))
        return '\0';

    return self->m_src[self->m_index + offset];
}

/**
 * Return `char` at `self->m_index` and increment `self->m_index` by 1
 * - Assertion context to protect against maintenace
 * - Also check if garbage value is not returned
 */
static char t_consume(struct tokenizer *self)
{
    assert(self->m_src[(self->m_index)] && "Is validated by peek caller");
    assert(
        (self->m_index + 1 <= strlen(self->m_src))
        && "Does not exceed source bounds");

    return self->m_src[(self->m_index)++];
}

struct token *t_tokenize(struct tokenizer *self, int *token_count)
{
    *token_count = 0;
    size_t m_src_length = strlen(self->m_src);
    size_t size = m_src_length * sizeof(struct token);

    struct token *tokens = (struct token *)malloc(size);
    if (tokens == NULL) {
        perror("Failed to allocate memory for tokens while tokenizing\n");
        return NULL;
    }

    char buf[m_src_length + 1];
    int offset = 0;
    size_t i = 0;

    while (t_peek(self, offset) != '\0') {
        if (isalpha(t_peek(self, offset))) {
            buf[i++] = t_consume(self);
            while (t_peek(self, offset) != '\0'
                   && isalnum(t_peek(self, offset))) {
                buf[i++] = t_consume(self);
            }
            buf[i] = '\0'; // Null-terminate the token buffer
            if (strcmp(buf, "exit") == 0) {
                tokens[(*token_count)++] = (struct token) { .type = TEXIT };
                buf_clear(buf, &i);
            }
            else if (strcmp(buf, "let") == 0) {
                tokens[(*token_count)++] = (struct token) { .type = TLET };
                buf_clear(buf, &i);
            }
            else {
                (tokens[(*token_count)++]
                 = (struct token) { .type = TIDENT, .value = strdup(buf) });
                buf_clear(buf, &i);
            }
        }
        else if (isdigit(t_peek(self, offset))) {
            buf[i++] = t_consume(self);
            while (t_peek(self, offset) != '\0'
                   && isdigit(t_peek(self, offset))) {
                buf[i++] = t_consume(self);
            }
            buf[i] = '\0';
            (tokens[(*token_count)++]
             = (struct token) { .type = TINT_LIT, .value = strdup(buf) });
            buf_clear(buf, &i);
        }
        else if (t_peek(self, offset) == '(') {
            t_consume(self);
            (tokens[(*token_count)++] = (struct token) { .type = TPAREN_OPEN });
        }
        else if (t_peek(self, offset) == ')') {
            t_consume(self);
            (tokens[(*token_count)++]
             = (struct token) { .type = TPAREN_CLOSE });
        }
        else if (t_peek(self, offset) == ';') {
            t_consume(self);
            tokens[(*token_count)++] = (struct token) { .type = TSEMICOLON };
        }
        else if (t_peek(self, offset) == '=') {
            t_consume(self);
            (tokens[(*token_count)++] = (struct token) { .type = TEQUAL });
        }
        else if (isspace(t_peek(self, offset))) {
            t_consume(self);
        }
        else {
            perror("You messed up while peeking!\n");
            exit(EXIT_FAILURE);
        }
        if (i > m_src_length) {
            perror("Temporary buffer stack overflowed while tokenizing\n");
            exit(EXIT_FAILURE);
        }
    }
    self->m_index = 0;
    return tokens;
}

// NOTE : free after this scope token_array_free(tokens, token_count); //
// Token[] cleanup
/* NOTE: TEMPORARY FUNCTION skips parsing and generation */
char *token_array_to_asm(struct token self[], size_t token_count)
{
    char output[1000];
    for (int i = 0; i < token_count; i++) {
        printf(
            "--%d-- Token %d: %u %s\n",
            __LINE__,
            i,
            self[i].type,
            self[i].value);

        if (self[i].type == TEXIT) {
            // char *output_asm = NULL; // Dynamic buffer
            // static size_t MAX_ASM_SIZE = 10000; // PERF: generate
            // // required memory based on input file
            // size_t len_token = token_count;
            //
            // output_asm = (char *)malloc(MAX_ASM_SIZE * sizeof(char)); //
            // adjust size as needed if (output_asm == NULL) {
            //     fprintf(stderr, "Failed to allocate memory to assembly
            //     output\n"); exit(EXIT_FAILURE); // return NULL; // if in
            //     a function
            // }
            // output_asm[0] = '\0'; // initialize buffer
            // strcat(output_asm, "global _start\n_start:\n");
            //
            // for (int i = 0; i < len_token; i++) {
            //     if (tokens[i].type == TOK_EXIT) {
            //         if (i + 1 < len_token && tokens[i + 1].type ==
            //         TOK_INT_LIT) {
            //             if (i + 2 < len_token && tokens[i + 2].type ==
            //             TOK_SEMICOLON) {
            //                 strcat(output_asm, "    mov rax, 60\n"); //
            //                 exit code strcat(output_asm, "    mov rdi,
            //                 "); // register expr char int_lit_str[20];
            //                 sprintf(int_lit_str, "%s", tokens[i +
            //                 1].value); // register no. strcat(output_asm,
            //                 int_lit_str); strcat(output_asm, "\n");
            //                 strcat(output_asm, "    syscall\n");
            //             }
            //         }
            //     }
            // }
        }
    }
    return "\0";
}

#endif /* CF17C85F_D892_43F8_B47B_0BA265F170E6 */