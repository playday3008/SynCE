#include <stdio.h>
#include <time.h>
#include <network_lib.h>
#include <list_lib.h>
#include <select_lib.h>
#include <timer_lib.h>

#include <synce_socket.h>


int read_local(local_p local, void *data_v)
{
    char data[256];
    int n;
    
    n = local_read(local, data, 256);
    
    data[n] = '\0';
    
    printf("Data: %s\n", data);
}


int main(int argc, char *argv[])
{
    local_p local;
    char *path = NULL;
    char control_socket_path[MAX_PATH];
    select_p sel;

    sel = select_create();
    
    if (!synce_get_directory(&path)) {
        printf("Faild to get configuration directory name.\n");
        exit (1);
    }

    snprintf(control_socket_path, sizeof(control_socket_path), "%s/%s", path, 
            "csock");

    local = local_connect_socket(control_socket_path);
    
    if (local != NULL) {
        local_select_addto_rlist(sel, local, read_local, NULL);

        while (1) {
            select_select(sel, NULL);
        }
    } else {
        printf("Error not connected\n");
    }
}