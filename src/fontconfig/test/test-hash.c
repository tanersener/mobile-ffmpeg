#include "../src/fchash.c"
#include "../src/fcstr.c"

FcChar8 *
FcLangNormalize (const FcChar8 *lang)
{
    return NULL;
}

FcChar8 *
FcConfigHome (void)
{
    return NULL;
}

typedef struct _Test
{
    FcHashTable *table;
} Test;

static Test *
init (void)
{
    Test *ret;

    ret = malloc (sizeof (Test));
    if (ret)
    {
	ret->table = FcHashTableCreate ((FcHashFunc) FcStrHashIgnoreCase,
					(FcCompareFunc) FcStrCmp,
					FcHashStrCopy,
					FcHashUuidCopy,
					(FcDestroyFunc) FcStrFree,
					FcHashUuidFree);
    }

    return ret;
}

static void
fini (Test *test)
{
    FcHashTableDestroy (test->table);
    free (test);
}

static FcBool
test_add (Test *test, FcChar8 *key, FcBool replace)
{
    uuid_t uuid;
    void *u;
    FcBool (*hash_add) (FcHashTable *, void *, void *);
    FcBool ret = FcFalse;

    uuid_generate_random (uuid);
    if (replace)
	hash_add = FcHashTableReplace;
    else
	hash_add = FcHashTableAdd;
    if (!hash_add (test->table, key, uuid))
	return FcFalse;
    if (!FcHashTableFind (test->table, key, &u))
	return FcFalse;
    ret = (uuid_compare (uuid, u) == 0);
    FcHashUuidFree (u);

    return ret;
}

static FcBool
test_remove (Test *test, FcChar8 *key)
{
    void *u;

    if (!FcHashTableFind (test->table, key, &u))
	return FcFalse;
    FcHashUuidFree (u);
    if (!FcHashTableRemove (test->table, key))
	return FcFalse;
    if (FcHashTableFind (test->table, key, &u))
	return FcFalse;

    return FcTrue;
}

int
main (void)
{
    Test *test;
    uuid_t uuid;
    int ret = 0;

    test = init ();
    /* first op to add */
    if (!test_add (test, "foo", FcFalse))
    {
	ret = 1;
	goto bail;
    }
    /* second op to add */
    if (!test_add (test, "bar", FcFalse))
    {
	ret = 1;
	goto bail;
    }
    /* dup not allowed */
    if (test_add (test, "foo", FcFalse))
    {
	ret = 1;
	goto bail;
    }
    /* replacement */
    if (!test_add (test, "foo", FcTrue))
    {
	ret = 1;
	goto bail;
    }
    /* removal */
    if (!test_remove (test, "foo"))
    {
	ret = 1;
	goto bail;
    }
    /* not found to remove */
    if (test_remove (test, "foo"))
    {
	ret = 1;
	goto bail;
    }
    /* complex op in pointer */
    if (!test_add (test, "foo", FcFalse))
    {
	ret = 1;
	goto bail;
    }
    if (test_add (test, "foo", FcFalse))
    {
	ret = 1;
	goto bail;
    }
    if (!test_remove (test, "foo"))
    {
	ret = 1;
	goto bail;
    }
    if (!test_add (test, "foo", FcFalse))
    {
	ret = 1;
	goto bail;
    }
    if (!test_remove (test, "bar"))
    {
	ret = 1;
	goto bail;
    }
    /* completely remove */
    if (!test_remove (test, "foo"))
    {
	ret = 1;
	goto bail;
    }
    /* completely remove from the last one */
    if (!test_add (test, "foo", FcFalse))
    {
	ret = 1;
	goto bail;
    }
    if (!test_add (test, "bar", FcFalse))
    {
	ret = 1;
	goto bail;
    }
    if (!test_remove (test, "bar"))
    {
	ret = 1;
	goto bail;
    }
    if (!test_remove (test, "foo"))
    {
	ret = 1;
	goto bail;
    }
bail:
    fini (test);

    return ret;
}
