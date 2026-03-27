/**
 * @file skin_registry.h
 * Skin registry -- register and look up gauge skins by name.
 */

#ifndef SKIN_REGISTRY_H
#define SKIN_REGISTRY_H

#include "skin.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SKIN_REGISTRY_MAX 16

/**
 * Register a skin.  Call at startup for each compiled-in skin.
 * @return 0 on success, -1 if registry is full.
 */
int skin_registry_register(const gauge_skin_t *skin);

/**
 * Look up a skin by name.
 * @return Pointer to the skin, or NULL if not found.
 */
const gauge_skin_t *skin_registry_find(const char *name);

/**
 * Get skin by index.
 * @return Pointer to the skin, or NULL if index out of range.
 */
const gauge_skin_t *skin_registry_get(int index);

/** Get total number of registered skins. */
int skin_registry_count(void);

#ifdef __cplusplus
}
#endif

#endif /* SKIN_REGISTRY_H */
