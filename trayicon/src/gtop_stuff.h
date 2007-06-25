#include <glib.h>

#define DCCM_BIN "vdccm"

gboolean
dccm_is_running ();

gboolean
send_signal_dccm(int sig);
