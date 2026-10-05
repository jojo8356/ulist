#include "ulist.h"
#include "ugc.h"
#include <stdio.h>

int main(void)
{
    /* --- Structure classique (gestion mémoire manuelle) --- */
    UVec *v = uvec_new(sizeof(int));
    uvec_add_val(v, 42);
    printf("UList OK — UVec created (size=%d, capacity=%d)\n",
           uvec_size(v), v->capacity);
    uvec_free(v);

    /* --- Garbage collector (conservateur, opt-in) --- */
    ugc_init();   /* le plus tôt possible = meilleure capture de la pile */
    printf("\n--- UGC demo (moteur: %s) ---\n", ugc_backend_name());

    UVec *gv = uvec_new_gc(sizeof(int));   /* tracké : pas de free manuel */
    for (int i = 0; i < 1000; i++)
        uvec_add_val(gv, i);

    ugc_collect();          /* gv vit sur la pile : il EST une racine */
    printf("apres collect: %zu objets trackes, %zu bytes, vec[999]=%d\n",
           ugc_count(), ugc_bytes(), uvec_get_as(gv, 999, int));

    uvec_free(gv);          /* réclamation synchrone du conteneur    */
    gv = NULL;              /* (sinon la racine pile le garderait)   */
    ugc_collect();
    ugc_collect();
    printf("apres free+collect: %zu objets, %zu bytes\n",
           ugc_count(), ugc_bytes());

    ugc_shutdown();                         /* nettoie tout le reste */
    return 0;
}
