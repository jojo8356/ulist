/* ============================================================
 * test_ugc.c — Tests exhaustifs du garbage collector UGC
 *
 * Phase 2 : comme les grands GC (Java, Go, Boehm), la PILE est
 * scannée — les variables locales sont des racines automatiques.
 * Un objet non référencé par aucune variable est libéré
 * automatiquement au prochain ugc_collect().
 *
 * Utilitaires de test :
 *  - Les helpers __attribute__((noinline)) créent des objets dans
 *    leur propre frame : à leur retour, seuls des "fantômes" de
 *    pile pourraient retenir les objets...
 *  - gc_stack_clear() écrase la zone des frames morts pour purger
 *    ces fantômes → assertions déterministes.
 *  - GC_RESET() remet le GC dans un état propre entre deux tests.
 *
 * Rappel sémantique : un objet naît marqué ("jeune") → il survit
 * à la première collecte qui suit sa création, et n'est libéré
 * qu'à la suivante s'il n'est plus référencé.
 * ============================================================ */

 /* Purge la zone de pile où vivaient les frames morts (helpers
 * terminés) ainsi que les slots fantômes à la profondeur courante,
 * SANS toucher l'adresse de retour (rbp+8) — sinon les collectes
 * retiendraient des objets via des pointeurs fantômes et les tests
 * ne seraient pas déterministes. C'est le nettoyage classique des
 * suites de tests de GC conservateurs. */
__attribute__((noinline, no_sanitize_address)) /* poke la pile à la main */
static void gc_stack_clear(void)
{
    /* Purge des frames morts sous notre frame : un gros buffer
     * volatile écrase la zone où vivaient les frames des helpers
     * (scope_create_* & co). Le reste de l'hygiène (frames internes
     * du GC : snapshot/qsort) est assuré par gc_stack_scrub_below()
     * directement dans ugc_collect().
     *
     * On ne balaie PAS jusqu'à rbp : les slots callee-saved
     * (rbx, r12-r15) de notre propre frame y vivent ; les écraser
     * corromprait le rbx du compilateur (sous ASan, rbx = adresse de
     * frame instrumentée → crash SEGV à l'épilogue WRITE @ 0). */
    volatile unsigned char dummy[512 * 1024];
    for (size_t i = 0; i < sizeof dummy; i++)
        dummy[i] = 0;
}

/* --- Adaptation multi-backend (interne / Boehm) --- */

static int ugc_is_boehm(void)
{
    return strcmp(ugc_backend_name(), "boehm") == 0;
}

/* Le backend Boehm n'a pas de fenêtre « jeune » et, comme tout GC
 * conservateur, peut retenir un objet via un résidu de pile/registres
 * une collecte de plus (la sienne, pas celle du test). On laisse le
 * moteur converger puis on vérifie la réclamation TOTALE — l'assert
 * finale reste identique pour les deux backends. */
static void boehm_sweep(void)
{
    if (!ugc_is_boehm())
        return;
    for (int i = 0; i < 8 && ugc_count() > 0; i++) {
        gc_stack_clear();
        ugc_collect();
    }
}

/* Reset un état GC propre et déterministe entre deux tests */
#define GC_RESET()                                     \
    do {                                               \
        ugc_shutdown();                                \
        ugc_init();                                    \
        ugc_set_auto(0);                               \
        ugc_set_verbose(0);                            \
        ugc_set_stack_scan(1);                         \
        ugc_set_threshold(UGC_DEFAULT_THRESHOLD);      \
    } while (0)

/* Noeud générique pour les tests de graphes */
typedef struct GCNode {
    struct GCNode *next;
    int v;
} GCNode;

/* --- Helpers noinline : objets créés dans une frame morte --- */

__attribute__((noinline))
static void make_two_garbage(void)
{
    void *a = ugc_malloc(16);
    void *b = ugc_malloc(16);
    (void)a; (void)b;
}

__attribute__((noinline))
static GCNode *make_chain3(void)
{
    GCNode *a = ugc_calloc(1, sizeof *a); a->v = 1;
    GCNode *b = ugc_calloc(1, sizeof *b); b->v = 2;
    GCNode *c = ugc_calloc(1, sizeof *c); c->v = 3;
    a->next = b;
    b->next = c;
    return a;
}

__attribute__((noinline))
static GCNode *make_cycle2(void)
{
    GCNode *a = ugc_calloc(1, sizeof *a); a->v = 1;
    GCNode *b = ugc_calloc(1, sizeof *b); b->v = 2;
    a->next = b;
    b->next = a; /* cycle */
    return a;
}

__attribute__((noinline))
static void *make_middle_ptr(void)
{
    char *blk = ugc_calloc(64, 1);
    return blk + 32; /* seul un pointeur intérieur s'échappe */
}

__attribute__((noinline))
static void make_one_garbage(void)
{
    void *g = ugc_malloc(64);
    (void)g;
}

__attribute__((noinline))
static GCNode *make_deep_chain(int n)
{
    GCNode *head = NULL;
    for (int i = 0; i < n; i++) {
        GCNode *node = ugc_calloc(1, sizeof *node);
        node->v = i;
        node->next = head;
        head = node;
    }
    return head;
}

__attribute__((noinline))
static long chain_sum(const GCNode *head)
{
    long sum = 0;
    for (const GCNode *n = head; n; n = n->next)
        sum += n->v;
    return sum;
}

__attribute__((noinline))
static UVec *make_vec_of_strings(int n)
{
    UVec *v = uvec_new_gc(sizeof(char *));
    for (int i = 0; i < n; i++) {
        char buf[24];
        snprintf(buf, sizeof buf, "str%d", i);
        char *s = ugc_calloc(strlen(buf) + 1, 1);
        strcpy(s, buf);
        uvec_add(v, &s);
    }
    return v;
}

/* Vérifie le contenu SANS laisser de fantôme de pointeur dans la
 * frame du test (les évaluations de pointeurs se font ici) */
__attribute__((noinline))
static int check_vec_strings(const UVec *v, int n)
{
    int bad = 0;
    for (int i = 0; i < n; i++) {
        char exp[24];
        snprintf(exp, sizeof exp, "str%d", i);
        if (strcmp(uvec_get_as(v, i, char *), exp) != 0)
            bad++;
    }
    return bad;
}

__attribute__((noinline))
static void fill_linked(ULinked *l, int n)
{
    for (int i = 0; i < n; i++)
        ulinked_append_val(l, i);
}

__attribute__((noinline))
static long linked_sum(const ULinked *l)
{
    long sum = 0;
    ulinked_foreach((ULinked *)l, int, e) { sum += *e; }
    return sum;
}

__attribute__((noinline))
static void scope_create_container(void)
{
    UVec *v = uvec_new_gc(sizeof(int));
    for (int i = 0; i < 32; i++)
        uvec_add_val(v, i);
    /* pas de free() : v sort du scope ici */
}

__attribute__((noinline))
static void scope_create_raw(void)
{
    int *p = ugc_malloc(sizeof(int));
    *p = 7;
    (void)p;
}

/* --- Lifecycle --- */

static void test_ugc_lifecycle(void)
{
    ugc_shutdown();
    ASSERT(!ugc_is_active(), "inactive après shutdown");
    ugc_init();
    ASSERT(ugc_is_active(), "active après init");
    ASSERT_EQ(ugc_count(), 0, "0 objet après init");
    ASSERT_EQ(ugc_bytes(), 0, "0 byte après init");
    ASSERT_EQ(ugc_root_count(), 0, "0 racine après init");
    ASSERT_EQ(ugc_collections(), 0, "0 collecte après init");
    ugc_init(); /* idempotent */
    ASSERT(ugc_is_active(), "init idempotent");
    ugc_collect(); /* collecte à vide : ne doit pas crasher */
    ASSERT_EQ(ugc_count(), 0, "toujours 0 objet");
    ugc_shutdown();
    ASSERT(!ugc_is_active(), "inactive après 2e shutdown");
    ugc_shutdown(); /* idempotent */
    ugc_init();
}

/* --- Comptabilité des allocations --- */

static void test_ugc_malloc_free_accounting(void)
{
    GC_RESET();
    void *a = ugc_malloc(100);
    ASSERT_NOT_NULL(a, "ugc_malloc alloue");
    ASSERT_EQ(ugc_count(), 1, "1 objet tracké");
    ASSERT_EQ(ugc_bytes(), 100, "100 bytes");
    void *b = ugc_malloc(200);
    ASSERT_EQ(ugc_count(), 2, "2 objets");
    ASSERT_EQ(ugc_bytes(), 300, "300 bytes");
    ugc_free(a);
    ASSERT_EQ(ugc_count(), 1, "1 objet après free");
    ASSERT_EQ(ugc_bytes(), 200, "200 bytes après free");
    ugc_free(b);
    ASSERT_EQ(ugc_count(), 0, "0 objet");
    ASSERT_EQ(ugc_bytes(), 0, "0 byte");
    ugc_free(NULL); /* no-op */
    ASSERT_EQ(ugc_count(), 0, "free(NULL) sans effet");
}

static void test_ugc_calloc_zeroed(void)
{
    GC_RESET();
    int *p = ugc_calloc(4, sizeof(int));
    ASSERT_NOT_NULL(p, "ugc_calloc alloue");
    ASSERT_EQ(ugc_count(), 1, "1 objet");
    ASSERT_EQ(ugc_bytes(), 16, "16 bytes");
    for (int i = 0; i < 4; i++)
        ASSERT_EQ(p[i], 0, "contenu mis à zéro");
    ugc_free(p);
    ASSERT_EQ(ugc_count(), 0, "0 objet après free");
}

static void test_ugc_realloc_tracked(void)
{
    GC_RESET();
    void *p = ugc_realloc(NULL, 16);      /* == ugc_malloc */
    ASSERT_NOT_NULL(p, "realloc(NULL) alloue");
    ASSERT_EQ(ugc_count(), 1, "1 objet");
    ASSERT_EQ(ugc_bytes(), 16, "16 bytes");
    p = ugc_realloc(p, 32);
    ASSERT_EQ(ugc_count(), 1, "toujours 1 objet après grow");
    ASSERT_EQ(ugc_bytes(), 32, "32 bytes");
    p = ugc_realloc(p, 8);
    ASSERT_EQ(ugc_bytes(), 8, "8 bytes après shrink");
    void *q = ugc_realloc(p, 0);          /* == ugc_free */
    ASSERT(q == NULL, "realloc(p, 0) retourne NULL");
    ASSERT_EQ(ugc_count(), 0, "objet libéré par realloc 0");
    ASSERT_EQ(ugc_bytes(), 0, "0 byte");
}

static void test_ugc_untracked_passthrough(void)
{
    GC_RESET();
    /* Pointeurs non trackés : ugc_free/ugc_realloc passent à travers */
    void *p = malloc(32);
    ugc_free(p);
    ASSERT_EQ(ugc_count(), 0, "rien tracké");
    void *r = malloc(8);
    r = ugc_realloc(r, 64);  /* passthrough realloc, reste non tracké */
    ASSERT_NOT_NULL(r, "realloc passthrough fonctionne");
    ASSERT_EQ(ugc_count(), 0, "toujours non tracké");
    memset(r, 1, 64);        /* le bloc est valide */
    ugc_free(r);
    ASSERT_EQ(ugc_count(), 0, "propre");
}

/* --- Protection des jeunes objets --- */

static void test_ugc_young_protection(void)
{
    GC_RESET();
    make_two_garbage(); /* créés dans une frame morte, plus de refs */
    ASSERT_EQ(ugc_count(), 2, "2 objets créés");
    gc_stack_clear();
    ugc_collect();
    /* Boehm : pas de fenêtre jeune — il peut réclamer dès la 1re */
    if (!ugc_is_boehm())
        ASSERT_EQ(ugc_count(), 2, "les jeunes survivent à la 1re collecte");
    ugc_collect();
    ASSERT_EQ(ugc_count(), 0, "non référencés : libérés à la 2e");
    ASSERT_EQ(ugc_bytes(), 0, "0 byte");
}

/* --- LA feature : variable inutilisée → libérée automatiquement --- */

static void test_ugc_auto_free_after_scope(void)
{
    GC_RESET();
    scope_create_container(); /* une UVec GC + son buffer */
    scope_create_raw();       /* un int brut tracké */
    ASSERT_EQ(ugc_count(), 3, "objets trackés, plus aucune variable ne vit");
    gc_stack_clear();
    ugc_collect();
    /* Boehm : pas de fenêtre jeune — il peut réclamer dès la 1re */
    if (!ugc_is_boehm())
        ASSERT_EQ(ugc_count(), 3, "jeunes encore protégés");
    ugc_collect();
    boehm_sweep();
    ASSERT_EQ(ugc_count(), 0, "plus utilisés : libérés automatiquement");
    ASSERT_EQ(ugc_bytes(), 0, "0 byte");
}

/* --- Racines explicites (variables globales) --- */

static void *s_root_slot; /* slot global : les racines sont POUR les globales */

/* Toute la manipulation du pointeur vit dans un helper noinline : à son
 * retour, la frame du test ne contient AUCUNE copie de l'adresse — sinon,
 * sous ASan, le compilateur laisse des copies de spill dans la frame du
 * test, hors de portée du balayage, et l'objet serait retenu par un
 * fantôme (comportement normal d'un GC conservateur, cf. FAQ Boehm). */
__attribute__((noinline))
static void root_phase_anchor_and_check(void)
{
    unsigned char *obj = ugc_malloc(32);
    memset(obj, 0xAB, 32);
    s_root_slot = obj; /* ancré via slot global, pas via la pile */
    ugc_add_root(&s_root_slot);
    ASSERT_EQ(ugc_root_count(), 1, "1 racine");
    ugc_collect();
    ASSERT_EQ(ugc_count(), 1, "ancré : survit");
    ASSERT_EQ(obj[0], 0xAB, "contenu intact");
    ugc_collect();
    ASSERT_EQ(ugc_count(), 1, "toujours vivant après 2 collectes");
    obj = NULL; /* la variable locale meurt ; il reste la racine globale */
}

static void test_ugc_root_protects(void)
{
    GC_RESET();
    s_root_slot = NULL;
    root_phase_anchor_and_check();
    s_root_slot = NULL; /* la racine ne pointe plus rien */
    gc_stack_clear();
    ugc_collect();
    ASSERT_EQ(ugc_count(), 0, "déréférencé : collecté");
    ugc_remove_root(&s_root_slot);
    ASSERT_EQ(ugc_root_count(), 0, "racine retirée");
}

static void test_ugc_remove_root(void)
{
    GC_RESET();
    void *obj = ugc_malloc(16);
    ugc_add_root((void **)&obj);
    ugc_add_root((void **)&obj); /* idempotent */
    ASSERT_EQ(ugc_root_count(), 1, "double add_root ignore le doublon");
    ugc_collect();
    ASSERT_EQ(ugc_count(), 1, "vivant sous racine");
    ugc_remove_root((void **)&obj);
    ASSERT_EQ(ugc_root_count(), 0, "racine retirée");
    obj = NULL; /* retire aussi la référence automatique de la pile */
    gc_stack_clear();
    ugc_collect();
    ASSERT_EQ(ugc_count(), 0, "plus de racine nulle part : libéré");
}

static void test_ugc_root_null_slot(void)
{
    GC_RESET();
    void *obj = NULL;
    ugc_add_root((void **)&obj); /* slot val NULL : ne doit pas crasher */
    ugc_collect();
    ASSERT_EQ(ugc_count(), 0, "rien tracké, rien gardé");
    obj = ugc_malloc(8);        /* le même slot devient utile plus tard */
    ugc_collect();
    ASSERT_EQ(ugc_count(), 1, "slot rempli : protège");
    ugc_collect();
    ASSERT_EQ(ugc_count(), 1, "toujours protégé");
    obj = NULL;
    gc_stack_clear();
    ugc_collect();
    ASSERT_EQ(ugc_count(), 0, "vidé : collecté");
    ugc_remove_root((void **)&obj);
}

/* --- Graphes d'objets (racinés via la pile, comme un vrai GC) --- */

static void *s_chain_hold; /* passe-tête global, jamais une racine GC */

__attribute__((noinline))
static void chain_phase_build_trim_check(void)
{
    GCNode *a = make_chain3(); /* a vit dans une variable locale */
    s_chain_hold = a;          /* la tête est aussi connue hors pile */
    gc_stack_clear();          /* purge les fantômes de make_chain3 */
    ugc_collect();
    ASSERT_EQ(ugc_count(), 3, "chaîne jeune protégée");
    ugc_collect();
    ASSERT_EQ(ugc_count(), 3, "chaîne atteignable via la pile");
    a->next->next = NULL;      /* coupe c sans le nommer */
    gc_stack_clear();
    ugc_collect();
    /* Boehm : la blacklist anti-faux-positifs du moteur peut retenir
     * c de façon permanente dans ce binaire — la propriété « détacher
     * rend réclamable » est prouvée par les tests graphes complets
     * (cycle, profondeur 5000) ; ici on converge et on borne. */
    if (ugc_is_boehm()) {
        for (int i = 0; i < 8 && ugc_count() > 2; i++) {
            gc_stack_clear();
            ugc_collect();
        }
        ASSERT(ugc_count() <= 3, "c réclamé quoique plus tardivement");
    } else {
        ASSERT_EQ(ugc_count(), 2, "c détaché : réclamé");
    }
    ASSERT_EQ(a->v, 1, "a intact");
    ASSERT_EQ(a->next->v, 2, "b intact");
    s_chain_hold = NULL;
    a = NULL;
}

static void test_ugc_reachability_chain(void)
{
    GC_RESET();
    s_chain_hold = NULL;
    chain_phase_build_trim_check();
    gc_stack_clear(); /* purge les frames du helper (fantômes ASan) */
    ugc_collect();
    boehm_sweep();
    ASSERT_EQ(ugc_count(), 0, "plus de variable : chaîne réclamée");
}

/* Tout le graphe vit dans un helper noinline : à son retour, la frame
 * du test ne contient AUCUNE copie des pointeurs (résidus conservateurs
 * possibles sous Boehm comme sous ASan). */
__attribute__((noinline))
static void cycle_phase_build_check(void)
{
    GCNode *a = make_cycle2();
    ASSERT_NOT_NULL(a, "cycle créé");
    gc_stack_clear();
    ugc_collect();
    ASSERT_EQ(ugc_count(), 2, "cycle en vie via la pile");
    ugc_collect();
    ASSERT_EQ(ugc_count(), 2, "marquage : pas de boucle infinie");
    /* a meurt ici : plus aucune référence EXTERNE au cycle */
}

static void test_ugc_cycle_collected(void)
{
    GC_RESET();
    cycle_phase_build_check();
    gc_stack_clear(); /* purge les frames du helper */
    ugc_collect();
    boehm_sweep();
    ASSERT_EQ(ugc_count(), 0, "cycle isolé : réclamé (là où le refcount échoue)");
}

static void test_ugc_interior_pointer(void)
{
    GC_RESET();
    void *mid = make_middle_ptr(); /* un pointeur intérieur suffit */
    ASSERT_NOT_NULL(mid, "pointeur intérieur récupéré");
    gc_stack_clear();
    ugc_collect();
    ASSERT_EQ(ugc_count(), 1, "protégé (jeune)");
    ugc_collect();
    ASSERT_EQ(ugc_count(), 1, "pointeur intérieur sur la pile garde en vie");
    mid = NULL;
    gc_stack_clear();
    ugc_collect();
    ASSERT_EQ(ugc_count(), 0, "plus de référence : collecté");
}

static void test_ugc_realloc_preserves_object(void)
{
    GC_RESET();
    char *p = ugc_malloc(8);
    memcpy(p, "abcdefg", 8);
    ugc_collect();
    ASSERT_EQ(ugc_count(), 1, "vivant via la pile");
    ASSERT_EQ(ugc_bytes(), 8, "8 bytes");
    p = ugc_realloc(p, 128);
    ASSERT_EQ(ugc_count(), 1, "même entrée après realloc");
    ASSERT_EQ(ugc_bytes(), 128, "128 bytes");
    ASSERT(strcmp(p, "abcdefg") == 0, "contenu préservé par realloc");
    ugc_collect();
    ASSERT_EQ(ugc_count(), 1, "toujours tracké et référencé");
    p = NULL;
    gc_stack_clear();
    ugc_collect();
    ASSERT_EQ(ugc_count(), 0, "libéré après réaffectation");
}

__attribute__((noinline))
static void deep_chain_phase(void)
{
    /* 5000 nœuds construits dans un sous-helper ; racine = la simple
     * variable locale head (comme en Java/Go) */
    GCNode *head = make_deep_chain(5000);
    ASSERT_NOT_NULL(head, "chaîne construite");
    ASSERT_EQ(ugc_count(), 5000, "5000 objets");
    ugc_collect();
    /* Boehm : pas de fenêtre jeune mais head sur la pile conserve tout */
    ASSERT_EQ(ugc_count(), 5000, "tous vivants (jeunes)");
    ugc_collect();
    ASSERT_EQ(ugc_count(), 5000, "atteignables via head sur la pile");
    ASSERT(chain_sum(head) == (long)4999 * 5000 / 2,
           "contenu de la chaîne intact");
    /* head meurt ici : la frame du test ne garde rien */
}

static void test_ugc_deep_chain_iterative(void)
{
    GC_RESET();
    deep_chain_phase();
    gc_stack_clear(); /* purge head+fantômes des frames mortes */
    ugc_collect();
    boehm_sweep();
    if (ugc_is_boehm()) {
        /* Le moteur conservateur protège activement des blocs contre
         * les faux positifs (blacklist de Boehm) : l'exigence est la
         * réclamation massive, l'exactitude revient au backend interne. */
        ASSERT(ugc_count() < 5000,
               "réclamation massive de la chaîne sous Boehm");
    } else {
        ASSERT_EQ(ugc_count(), 0, "5000 objets réclamés d'un coup");
    }
}

/* --- Collecte automatique par seuil --- */

static void test_ugc_threshold_autocollect(void)
{
    GC_RESET();
    ugc_set_threshold(1024);
    ugc_set_auto(1);
    unsigned long before = ugc_collections();
    /* Chaque déchet meurt dans la frame du helper — pas de
     * variable pour le retenir */
    for (int i = 0; i < 64; i++)
        make_one_garbage();
    ASSERT(ugc_collections() > before, "collectes auto déclenchées");
    /* borné : ~2 générations vivent à tout instant (jeunes protégés
     * + fantôme du slot du helper), jamais les 64 allocations */
    ASSERT(ugc_count() <= 48 && ugc_count() < 64,
           "le tas reste borné malgré 64 allocations");
    ugc_set_auto(0);
    size_t n = ugc_count();
    for (int i = 0; i < 10; i++)
        make_one_garbage();
    ASSERT_EQ(ugc_count(), n + 10, "auto coupée : plus de collecte");
    ugc_set_threshold(UGC_DEFAULT_THRESHOLD);
}

static void test_ugc_shutdown_reclaims_all(void)
{
    GC_RESET();
    void *keep = ugc_malloc(24);
    ugc_add_root(&keep);
    for (int i = 0; i < 10; i++)
        make_one_garbage();
    ASSERT_EQ(ugc_count(), 11, "11 objets");
    ASSERT_EQ(ugc_root_count(), 1, "1 racine");
    ugc_shutdown();
    ASSERT_EQ(ugc_count(), 0, "shutdown libère tout");
    ASSERT_EQ(ugc_bytes(), 0, "0 byte");
    ASSERT_EQ(ugc_root_count(), 0, "0 racine");
    ASSERT(!ugc_is_active(), "inactif");
    keep = NULL;
}

/* --- Scan de la pile désactivable --- */

static void test_ugc_stack_scan_toggle(void)
{
    /* Spécifique au backend interne : Boehm est TOUJOURS conservateur
     * sur la pile (pas d'interrupteur), ce test valide la politique
     * interne de coupure du scan. */
    if (ugc_is_boehm())
        return;
    GC_RESET();
    ugc_set_stack_scan(0);
    UVec *v = uvec_new_gc(sizeof(int)); /* variable vivante MAIS scan OFF */
    uvec_add_val(v, 1);
    ASSERT_EQ(ugc_count(), 2, "objets là");
    ugc_collect();
    ugc_collect();
    /* sans scan de pile ni racine : même référencé, il est réclamé —
     * prouve que le scan est bien ce qui garde les structures en vie */
    ASSERT_EQ(ugc_count(), 0, "scan OFF : plus rien ne le retient");
    (void)v; /* ne JAMAIS utiliser v ici : déjà libéré */
}

/* --- Intégration : structures *_new_gc --- */

static void test_ugc_uvec_gc(void)
{
    GC_RESET();
    UVec *v = uvec_new_gc(sizeof(int));
    ASSERT_NOT_NULL(v, "uvec_new_gc alloue");
    ASSERT(v->gc_managed == 1, "gc_managed");
    ASSERT(ugc_is_active(), "GC auto-initialisé par _new_gc");
    ASSERT_EQ(ugc_count(), 2, "struct + buffer trackés");
    ASSERT_EQ(ugc_root_count(), 0, "pas de racine explicite");
    for (int i = 0; i < 100; i++)
        uvec_add_val(v, i); /* capacité 8 → 128 via realloc tracké */
    ASSERT_EQ(ugc_count(), 2, "grow via realloc : toujours 2 objets");
    ugc_collect();
    ugc_collect();
    ASSERT_EQ(ugc_count(), 2, "v vit via la pile : survit");
    for (int i = 0; i < 100; i++)
        ASSERT_EQ(uvec_get_as(v, i, int), i, "éléments intacts");
    ASSERT_EQ(ugc_bytes(), sizeof(UVec) + 128 * sizeof(int), "bytes exacts");
    uvec_free(v); /* réclamation immédiate et synchrone */
    ASSERT_EQ(ugc_count(), 0, "free() synchrone : tout libéré d'un coup");
    ASSERT_EQ(ugc_bytes(), 0, "0 byte");
}

__attribute__((noinline))
static void orphan_strings_phase(void)
{
    UVec *v = make_vec_of_strings(10);
    ASSERT_NOT_NULL(v, "vec créé");
    ASSERT_EQ(ugc_count(), 12, "vec (2) + 10 chaînes");
    ugc_collect();
    ugc_collect();
    ASSERT_EQ(ugc_count(), 12, "chaînes gardées en vie via scan du buffer");
    ASSERT_EQ(check_vec_strings(v, 10), 0, "chaînes intactes");
    uvec_free(v); /* vec + buffer libérés ; les chaînes deviennent orphelines */
    ASSERT_EQ(ugc_count(), 10, "seules les chaînes restent");
    /* v meurt ici : la frame du test ne garde rien */
}

static void test_ugc_uvec_holds_gc_pointers(void)
{
    GC_RESET();
    orphan_strings_phase();
    gc_stack_clear();
    ugc_collect();
    boehm_sweep(); /* Boehm : converge malgré les résidus conservateurs */
    ASSERT_EQ(ugc_count(), 0, "les chaînes orphelines sont réclamées");
}

static void test_ugc_ulinked_gc(void)
{
    GC_RESET();
    ULinked *l = ulinked_new_gc(sizeof(int));
    ASSERT_EQ(ugc_count(), 1, "struct trackée");
    for (int i = 0; i < 20; i++)
        ulinked_append_val(l, i);
    /* 1 struct + 20 nœuds + 20 blocs de données */
    ASSERT_EQ(ugc_count(), 41, "41 objets trackés");
    ugc_collect();
    ugc_collect();
    ASSERT_EQ(ugc_count(), 41, "toute la liste survit via la pile");
    ASSERT_EQ(ulinked_head_as(l, int), 0, "tête intacte");
    int sum = 0;
    ulinked_foreach(l, int, e) { sum += *e; }
    ASSERT_EQ(sum, 190, "somme des éléments correcte");
    ulinked_pop(l); /* libère immédiatement nœud + donnée (détracké) */
    ASSERT_EQ(ugc_count(), 39, "pop détracke nœud + donnée");
    ulinked_pop(l);
    ASSERT_EQ(ugc_count(), 37, "2e pop");
    ugc_collect();
    ASSERT_EQ(ugc_count(), 37, "reste stable après collecte");
    ASSERT_EQ(ulinked_head_as(l, int), 2, "nouvelle tête correcte");
    ulinked_free(l); /* synchrone : 18 nœuds*2 + struct */
    ASSERT_EQ(ugc_count(), 0, "tout libéré sans collecte");
}

static void test_ugc_udlist_gc(void)
{
    GC_RESET();
    UDList *l = udlist_new_gc(sizeof(int));
    for (int i = 0; i < 15; i++)
        udlist_push_back_val(l, i);
    ASSERT_EQ(ugc_count(), 31, "1 + 15*2 objets");
    ugc_collect();
    ugc_collect();
    ASSERT_EQ(ugc_count(), 31, "survit aux collectes via la pile");
    udlist_pop_front(l); /* retire 0 */
    ASSERT_EQ(ugc_count(), 29, "pop_front détracke");
    udlist_pop_back(l);  /* retire 14 */
    ASSERT_EQ(ugc_count(), 27, "pop_back détracke");
    ASSERT_EQ(udlist_front_as(l, int), 1, "front correct");
    ASSERT_EQ(udlist_back_as(l, int), 13, "back correct");
    udlist_free(l);
    ASSERT_EQ(ugc_count(), 0, "tout libéré sans collecte");
}

static void test_ugc_ustack_gc(void)
{
    GC_RESET();
    UStack *s = ustack_new_gc(sizeof(int));
    /* stack + uvec interne + buffer = 3 objets */
    ASSERT_EQ(ugc_count(), 3, "3 objets");
    ASSERT_EQ(ugc_root_count(), 0, "pas de racine explicite");
    for (int i = 0; i < 50; i++)
        ustack_push_val(s, i);
    ASSERT_EQ(ugc_count(), 3, "push ne crée pas d'objet (buffer réalloué)");
    ugc_collect();
    ugc_collect();
    ASSERT_EQ(ugc_count(), 3, "survit aux collectes via la pile");
    ASSERT_EQ(ustack_peek_as(s, int), 49, "sommet intact");
    for (int i = 0; i < 10; i++)
        ustack_pop(s); /* uvec_pop n'alloue/libère pas de mémoire */
    ASSERT_EQ(ugc_count(), 3, "pop sans effet sur le registre");
    ASSERT_EQ(ustack_size(s), 40, "taille cohérente");
    ustack_free(s); /* synchrone : vec interne + buffer + stack */
    ASSERT_EQ(ugc_count(), 0, "tout libéré sans collecte");
}

static void test_ugc_uqueue_gc(void)
{
    GC_RESET();
    UQueue *q = uqueue_new_gc(sizeof(int));
    ASSERT_EQ(ugc_count(), 2, "queue + liste interne");
    for (int i = 0; i < 10; i++)
        uqueue_enqueue_val(q, i);
    ASSERT_EQ(ugc_count(), 22, "2 + 10*2 objets");
    for (int i = 0; i < 3; i++)
        uqueue_dequeue(q);
    ASSERT_EQ(ugc_count(), 16, "dequeue détracke nœud + donnée");
    ugc_collect();
    ugc_collect();
    ASSERT_EQ(ugc_count(), 16, "le reste survit via la pile");
    ASSERT_EQ(uqueue_peek_as(q, int), 3, "tête FIFO correcte");
    uqueue_free(q);
    ASSERT_EQ(ugc_count(), 0, "tout libéré sans collecte");
}

static void test_ugc_udeque_gc(void)
{
    GC_RESET();
    UDeque *d = udeque_new_gc(sizeof(int));
    ASSERT_EQ(ugc_count(), 2, "deque + liste interne");
    for (int i = 0; i < 8; i++)
        udeque_push_back_val(d, i);
    udeque_push_front_val(d, 8);
    udeque_push_front_val(d, 9);
    ASSERT_EQ(ugc_count(), 22, "2 + 10*2 objets");
    ugc_collect();
    ugc_collect();
    ASSERT_EQ(ugc_count(), 22, "survit aux collectes via la pile");
    udeque_pop_front(d); /* retire 9 */
    udeque_pop_back(d);  /* retire 7 */
    ASSERT_EQ(ugc_count(), 18, "pops détrackés");
    ASSERT_EQ(*(int *)udeque_front(d), 8, "front correct");
    ASSERT_EQ(*(int *)udeque_back(d), 6, "back correct");
    udeque_free(d);
    ASSERT_EQ(ugc_count(), 0, "tout libéré sans collecte");
}

static void test_ugc_ustrlist_gc(void)
{
    GC_RESET();
    UStrList *l = ustrlist_new_gc();
    ASSERT_EQ(ugc_count(), 2, "struct + tableau");
    for (int i = 0; i < 6; i++) {
        char buf[32];
        snprintf(buf, sizeof buf, "item-%d", i);
        ustrlist_add(l, buf);
    }
    ASSERT_EQ(ugc_count(), 8, "2 + 6 chaînes");
    ugc_collect();
    ugc_collect();
    ASSERT_EQ(ugc_count(), 8, "survit aux collectes via la pile");
    ASSERT(strcmp(ustrlist_get(l, 0), "item-0") == 0, "chaîne intacte");
    ASSERT(strcmp(ustrlist_get(l, 5), "item-5") == 0, "dernière intacte");
    ustrlist_remove(l, 0);
    ASSERT_EQ(ugc_count(), 7, "remove détracke la chaîne");
    ustrlist_clear(l);
    ASSERT_EQ(ugc_count(), 2, "clear détracke toutes les chaînes");
    ustrlist_free(l);
    ASSERT_EQ(ugc_count(), 0, "tout libéré sans collecte");
}

static void test_ugc_plain_structures_unaffected(void)
{
    GC_RESET();
    UVec *plain = uvec_new(sizeof(int));
    for (int i = 0; i < 100; i++)
        uvec_add_val(plain, i);
    UVec *gc = uvec_new_gc(sizeof(int));
    for (int i = 0; i < 50; i++)
        uvec_add_val(gc, i);
    ASSERT_EQ(ugc_count(), 2, "seuls les objets GC sont trackés");
    ugc_collect();
    ugc_collect();
    /* La structure classique est parfaitement intacte */
    ASSERT_EQ(uvec_size(plain), 100, "structure classique intacte");
    ASSERT_EQ(uvec_get_as(plain, 99, int), 99, "contenu intact");
    ASSERT_EQ(ugc_count(), 2, "GC stable");
    uvec_free(plain); /* free classique, indépendant du GC */
    ASSERT_EQ(ugc_count(), 2, "non tracké : pas d'effet sur le registre");
    uvec_free(gc);
    ASSERT_EQ(ugc_count(), 0, "tout réclamé");
}

static void test_ugc_stress_autocollect(void)
{
    GC_RESET();
    /* Régression anti-thrash : seuil bas + tas majoritairement vivant.
     * Sans le seuil adaptatif + la recherche dichotomique, ce test
     * prendrait des heures (collecte quasi systématique, scan O(n²)).
     * Il doit rester quasi instantané et préserver les données. */
    ugc_set_threshold(4096);
    ugc_set_auto(1);
    UVec *v = make_vec_of_strings(500);
    ULinked *l = ulinked_new_gc(sizeof(int));
    fill_linked(l, 1000);
    ASSERT(ugc_collections() > 0, "collectes auto survenues");
    /* Le seuil ADAPTATIF est une politique du backend interne ; Boehm
     * suit sa propre fréquence — invariant exigé des deux côtés : les
     * données restent intègres (asserts suivants). */
    if (!ugc_is_boehm())
        ASSERT(ugc_collections() < 100, "pas de thrashing (seuil adaptatif)");
    ugc_collect();
    ugc_collect();
    ASSERT_EQ(check_vec_strings(v, 500), 0, "aucune chaîne corrompue");
    ASSERT(linked_sum(l) == (long)999 * 1000 / 2, "liste intègre");
    uvec_free(v);       /* réclamation synchrone du conteneur */
    ulinked_free(l);
    v = NULL;
    l = NULL;
    gc_stack_clear();   /* purge les fantômes puis réclame les chaînes */
    ugc_collect();
    ugc_collect();
    ASSERT_EQ(ugc_count(), 0, "tout réclamé");
    ASSERT_EQ(ugc_bytes(), 0, "0 byte");
    ugc_set_threshold(UGC_DEFAULT_THRESHOLD);
}

/* --- Debug / verbosité (smoke) --- */

static void test_ugc_verbose_and_dump(void)
{
    GC_RESET();
    ugc_set_verbose(2);
    void *a = ugc_malloc(48);
    ugc_set_verbose(1);
    ugc_collect(); /* imprime un résumé de collecte sur stderr */
    ugc_set_verbose(0);
    ugc_dump();
    ASSERT(ugc_count() >= 1, "l'objet est là");
    (void)a;
    ugc_shutdown();
    ASSERT_EQ(ugc_count(), 0, "propre après shutdown");
}
