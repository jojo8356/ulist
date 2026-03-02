#include "ulist.h"
#include <stdio.h>

int main(void)
{
    UVec *v = uvec_new(sizeof(int));
    printf("UList Phase 0 OK — UVec created (size=%d, capacity=%d)\n",
           uvec_size(v), v->capacity);
    uvec_free(v);
    return 0;
}
