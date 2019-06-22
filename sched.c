#include "global.h"
#include "proc.h"

/* choose ONE process to run */
struct proc* pick_proc() { return &proc_table[0]; }
