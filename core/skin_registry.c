/**
 * @file skin_registry.c
 * Skin registry implementation.
 */

#include "skin_registry.h"
#include <string.h>

static const gauge_skin_t *s_skins[SKIN_REGISTRY_MAX];
static int s_count = 0;

int skin_registry_register(const gauge_skin_t *skin)
{
    if (!skin || !skin->name) return -1;
    if (s_count >= SKIN_REGISTRY_MAX) return -1;
    s_skins[s_count++] = skin;
    return 0;
}

const gauge_skin_t *skin_registry_find(const char *name)
{
    if (!name) return NULL;
    for (int i = 0; i < s_count; i++) {
        if (strcmp(s_skins[i]->name, name) == 0) {
            return s_skins[i];
        }
    }
    return NULL;
}

const gauge_skin_t *skin_registry_get(int index)
{
    if (index < 0 || index >= s_count) return NULL;
    return s_skins[index];
}

int skin_registry_count(void)
{
    return s_count;
}
