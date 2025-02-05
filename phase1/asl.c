#include "../h/const.h"
#include "../h/types.h"
#include "../h/asl.h"

HIDDEN semd_t *semd_h; /* Head of the active semaphore list */ 
HIDDEN semd_t *semdFree_h; /* Head of the free semaphore list */

