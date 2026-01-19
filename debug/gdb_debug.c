#include <stdio.h>
#include "lexer/lexer.h"

int main(void)
{
    FILE *f = tmpfile();
    if (!f) {
        perror("tmpfile");
        return 1;
    }

    fputs("   \t  echo hello", f);
    rewind(f);

    struct lexer lx;
    lexer_init(&lx, f);

    struct token tok = lexer_next(&lx); 

    printf("token type = %d\n", tok.type);
    if (tok.value)
        printf("token value = '%s'\n", tok.value);

    fclose(f);
    return 0;
}
