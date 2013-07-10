#include <synce.h>
#include <rapi2.h>

typedef struct _REG_KEY_INFO
{
  gchar *name;
  gchar *class;
} REG_KEY_INFO;

typedef struct _REG_VALUE_INFO
{
  gchar *name;
  DWORD type;
  gpointer data;
  DWORD data_len;
} REG_VALUE_INFO;

GList* enum_registry_key(IRAPISession *session, GList *list, char *key_name, GtkWidget *progressbar); 

GList* enum_registry_values(IRAPISession *session, GList *list, gchar *key_path);
