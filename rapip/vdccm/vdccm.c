/***************************************************************************
 * Copyright (c) 2003 Volker Christian <voc@users.sourceforge.net>         *
 *                                                                         *
 * Permission is hereby granted, free of charge, to any person obtaining a *
 * copy of this software and associated documentation files (the           *
 * "Software"), to deal in the Software without restriction, including     *
 * without limitation the rights to use, copy, modify, merge, publish,     *
 * distribute, sublicense, and/or sell copies of the Software, and to      *
 * permit persons to whom the Software is furnished to do so, subject to   *
 * the following conditions:                                               *
 *                                                                         *
 * The above copyright notice and this permission notice shall be included *
 * in all copies or substantial portions of the Software.                  *
 *                                                                         *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS *
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF              *
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  *
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY    *
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,    *
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE       *
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                  *
 ***************************************************************************/


#include <stdio.h>
#include <time.h>
#include <dirent.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <stdarg.h>
#include <synce_socket.h>
#include <synce_log.h>
#include <net/if.h>
#include <sys/ioctl.h>

#include "network_lib.h"
#include "list_lib.h"
#include "select_lib.h"
#include "timer_lib.h"


#define DCCM_PID_FILE           "dccm.pid"
#define DCCM_PORT               5679
#define DCCM_PING_INTERVAL      5       /* seconds */
#define DCCM_PING               0x12345678
#define DCCM_MAX_PING_COUNT     3       /* max number of pings without reply */
#define DCCM_MAX_PACKET_SIZE	512
#define DCCM_MIN_PACKET_SIZE	0x24

#define MAX_PASSWORD_LENGTH    16


static bool is_daemon = true;
static bool use_ipaddress = false;
static int missing_ping_count = DCCM_MAX_PING_COUNT;
static int ping_delay = DCCM_PING_INTERVAL;
static char *password = NULL;


struct management_s
{
    SynceSocket *device_server;
    select_p sel;
    local_p client_server;
    list_p device_l;
    list_p client_l;
    list_p passwordpending_device_l;
};

typedef struct management_s management_t;
typedef management_t *manage_p;


struct device_s
{
    SynceSocket* socket;
    struct sockaddr_in address;
    int ping_count;
    bool locked;
    bool expect_password_reply;
    int key;
    char password[MAX_PASSWORD_LENGTH];
    char ip_str[16];
    char *name;
    char *class;
    char *hardware;
    uint16_t os_version;
    uint16_t build_number;
    uint16_t processor_type;
    uint32_t partner_id_1;
    uint32_t partner_id_2;
    manage_p manage;
    bool password_correct;
    bool is_main_device;
};

typedef struct device_s device_t;
typedef device_t *device_p;


static management_t management;
static int running = 1;


static void vdccm_remove_device(device_p);


static void vdccm_disconnect_active_devices()
{
    device_p device;

    list_iterator(management.device_l, device) {
        vdccm_remove_device(device);
    }
}


static void vdccm_disconnect_active_clients()
{
    local_p client;

    list_iterator(management.client_l, client) {
        local_write(client, (char*) "S", 1);
        local_close_socket(client);
    }
}


static void vdccm_handle_sighup(int n)
{
    vdccm_disconnect_active_devices();
}


static void vdccm_handle_terminating_signals(int n)
{
    char *path;
    char pid_file[MAX_PATH];

    running = false;
    vdccm_disconnect_active_devices();
    vdccm_disconnect_active_clients();

    if (!synce_get_directory(&path)) {
        synce_error("Faild to get configuration directory name.");
    } else {
        snprintf(pid_file, sizeof(pid_file), "%s/" DCCM_PID_FILE, path);
        unlink(pid_file);
    }
}


/*
 * Action is "connect" or "disconnect"
 */
static void vdccm_run_scripts(device_p device, char* action)
{
    char* directory = NULL;
    DIR* dir = NULL;
    struct dirent* entry = NULL;

    if (!synce_get_script_directory(&directory)) {
        synce_error("Failed to get script directory");
        goto exit;
    }

    dir = opendir(directory);
    if (!dir) {
        synce_error("Failed to open script directory");
        goto exit;
    }

    while ((entry = readdir(dir)) != NULL) {
        char path[MAX_PATH];
        char command[MAX_PATH];
        struct stat info;

        snprintf(path, sizeof(path), "%s/%s", directory, entry->d_name);

        if (lstat(path, &info) < 0)
            continue;

        if (!(info.st_mode & S_IFREG))
            continue;

        synce_trace("Running script: %s %s", path, action);

        if (device)
            snprintf(command, sizeof(command), "%s %s %s", path, action, device->name);
        else
            snprintf(command, sizeof(command), "%s %s", path, action);

        system(command);
    }

exit:
    if (directory)
        free(directory);

    if (dir)
        closedir(dir);
}


static bool vdccm_write_connection_file(char *filename, device_p device)
{
    bool success = false;
    FILE* file = NULL;

    synce_trace("Writing client-file: %s", filename);

    if ((file = fopen(filename, "w")) == NULL) {
        synce_error("Failed to open file for writing: %s", filename);
        goto exit;
    }

    fprintf(file,
            "# Modifications to this file will be lost next time a client connects to dccm\n"
            "\n"
            "[dccm]\n"
            "pid=%i\n"
            "\n"
            "[device]\n"
            "os_version=%i\n"
            "build_number=%i\n"
            "processor_type=%i\n"
            "partner_id_1=%i\n"
            "partner_id_2=%i\n"
            "name=%s\n"
            "class=%s\n"
            "hardware=%s\n"
            "ip=%s\n"
            "port=%i\n",
            getpid(),
            device->os_version,
            device->build_number,
            device->processor_type,
            device->partner_id_1,
            device->partner_id_2,
            device->name,
            device->class,
            device->hardware,
            device->ip_str,
            ntohs(device->address.sin_port));

    if (device->locked) {
        fprintf(file,
                "password=%s\n"
                "key=%i\n",
                device->password,
                device->key
               );
    }

    success = true;

exit:
    if (file)
        fclose(file);

    return success;
}


static bool vdccm_write_device_specific_connection_file(device_p device)
{
    char *path = NULL;
    char filename[MAX_PATH];

    if (!synce_get_directory(&path)) {
        synce_error("Faild to get configuration directory name.");
        exit (1);
    }

    if (!use_ipaddress) {
        snprintf(filename, sizeof(filename), "%s/%s", path, device->name);
    } else {
        snprintf(filename, sizeof(filename), "%s/%s", path, device->ip_str);
    }

    return vdccm_write_connection_file(filename, device);
}


static bool vdccm_write_active_connection_file(device_p device)
{
    bool ret = false;
    char *old_filename;

    if (!synce_get_connection_filename(&old_filename)) {
        synce_error("Unable to get connection filename");
    } else {
        if (old_filename) {
            ret = vdccm_write_connection_file(old_filename, device);
            free(old_filename);
        } else {
            synce_error("Default connection filename not valid");
        }
    }

    return ret;
}


static bool vdccm_write_connection_files(device_p device)
{
    if (!vdccm_write_device_specific_connection_file(device)) {
        synce_error("Could not write device-specific connection file");
        return false;
    }

    if (!vdccm_write_active_connection_file(device)) {
        synce_error("Could not write active_connection file");
        return false;
    }

    return true;
}


static void vdccm_send_connection_info_to_clients(device_p device, char *command)
{
    local_p local;
    char string[256];

    if (!use_ipaddress)
        snprintf(string, 256, "%s%s;", command, device->name);
    else
        snprintf(string, 256, "%s%s;", command, device->ip_str);

    list_iterator(device->manage->client_l, local) {
        if (local_write(local, string, strlen(string)) <= 0) {
            synce_error("Could not write to client");
            local_select_delfrom_rlist(local);
            list_delete_data(device->manage->client_l, local);
            free(local);
        }
    }
}


static void vdccm_notify_new_connected_client(local_p local, manage_p manage)
{
    device_p device;
    unsigned char count;

    count = list_get_count(manage->device_l);

    list_iterator(manage->device_l, device) {
        vdccm_send_connection_info_to_clients(device, (char *) "C");
    }
}


static int vdccm_read_from_client(local_p local, void *manage_v)
{
    manage_p manage = (manage_p) manage_v;
    char buffer[256];
    char *name;
    char *passwd;
    device_p device;
    int n;
    int send = 0;

    if ((n = local_read(local, buffer, 256)) <= 0) {
        local_select_delfrom_rlist(local);
        list_delete_data(manage->client_l, local);
        local_close_socket(local);
        return 0;
    }

    buffer[n] = '\0';

    synce_trace("Raki-Passwordreply: %s", buffer);

    switch(buffer[0]) {
    case 'R':
        passwd = &buffer[1];
        name = strsep(&passwd, "=");

        list_iterator(manage->passwordpending_device_l, device) {
            if (strcmp(device->name, name) == 0) {
                send = 1;
                synce_trace("Sending Password to: %s", device->name);
                snprintf(device->password, MAX_PASSWORD_LENGTH, "%s", passwd);
                if (!synce_password_send(device->socket, device->password, device->key)) {
                    synce_error("failed to send password to %s", device->name);
                }
                list_delete_data(manage->passwordpending_device_l, device);
            }
        }
        if (!send) {
            synce_trace("Got Password for %s from Raki but no device waiting", name);
        }
        break;
    default:
        printf("Unknown response from Raki");
        break;
    }

    return 0;
}


static int vdccm_accept_new_client(local_p control, void *manage_v)
{
    manage_p manage = (manage_p) manage_v;
    local_p new_client = local_accept_socket(control);
    list_insert_tail(manage->client_l, new_client);
    local_select_addto_rlist(manage->sel, new_client, vdccm_read_from_client, manage);
    vdccm_notify_new_connected_client(new_client, manage);

    return 0;
}


static void dump(const char *desc, void *data, size_t len)
{
    uint8_t *buf = data;
    unsigned int i, j;
    char hex[8 * 3 + 1];
    char chr[8 + 1];

    printf("%s (%d bytes):\n", desc, len);
    for (i = 0; i < len + 7; i += 8) {
        for (j = 0; j < 8; j++)
            if (j + i >= len) {
                hex[3*j+0] = ' ';
                hex[3*j+1] = ' ';
                hex[3*j+2] = ' ';
                chr[j] = ' ';
            } else {
                uint8_t c = buf[j + i];
                const char *hexchr = "0123456789abcdef";
                hex[3*j+0] = hexchr[(c >> 4) & 0xf];
                hex[3*j+1] = hexchr[c & 0xf];
                hex[3*j+2] = ' ';
                if (c > ' ' && c <= '~')
                    chr[j] = c;
                else
                    chr[j] = '.';
            }
        hex[8*3] = '\0';
        chr[8] = '\0';
        printf("  %04x: %s %s\n", i, hex, chr);
    }
}


static char* string_at(char* buffer, size_t size, size_t offset)
{
    size_t string_offset = letoh32(*(uint32_t*)(buffer + offset));

    if (string_offset < size) {
        return wstr_to_ascii((WCHAR*)(buffer + string_offset));
    } else {
        synce_error("String offset too large: 0x%08x", string_offset);
        return NULL;
    }
}


static void vdccm_remove_connection_files(device_p device)
{
    char *old_filename = NULL;
    char *path = NULL;
    char filename[MAX_PATH];

    if (!synce_get_directory(&path)) {
        synce_error("Faild to get configuration directory name.");
        exit (1);
    }

    if (!use_ipaddress)
        snprintf(filename, sizeof(filename), "%s/%s", path, device->name);
    else
        snprintf(filename, sizeof(filename), "%s/%s", path, device->ip_str);

    free(path);

    unlink(filename);

    if (device->is_main_device) {
        if (!synce_get_connection_filename(&old_filename)) {
            synce_error("Unable to get connection filename");
            goto exit;
        }

        unlink(old_filename);

    exit:
        if (old_filename)
            free(old_filename);
    }
}


static void free_device(device_p device)
{
    if (device->name)
        free(device->name);
    if (device->class)
        free(device->class);
    if (device->hardware)
        free(device->hardware);

    free(device);
}


static void vdccm_remove_device(device_p device)
{
    device_p last_device;
    select_delfrom_rlist(device->manage->sel, synce_socket_get_descriptor(device->socket));
    synce_socket_free(device->socket);
    if (list_delete_data(device->manage->device_l, device) == 0) {
        vdccm_remove_connection_files(device);
        vdccm_run_scripts(device, (char *) "disconnect");
        vdccm_send_connection_info_to_clients(device, (char *) "D");
        if (device->is_main_device == true) {
            last_device = list_get_last(device->manage->device_l);
            if (last_device != NULL) {
                if (!vdccm_write_active_connection_file(last_device)) {
                    synce_error("Could not write active_connection file");
                } else {
                    last_device->is_main_device = true;
                }
            }
        }
    }
    synce_trace("Removed %s", device->name);
    free_device(device);
}


static bool vdccm_add_device(device_p device)
{
    device_p tmp_device;
    device_p tmp_main_device = NULL;
    bool skipped = false;

    list_iterator(device->manage->device_l, tmp_device) {
        if (tmp_device->is_main_device) {
            tmp_device->is_main_device = false;
            tmp_main_device = tmp_device;
        }
        if (strcmp(tmp_device->name, device->name) == 0) {
            synce_trace("Device with name %s already exists. New device skipped");
            skipped = true;
            break;
        }
    }

    if (!skipped) {
        device->expect_password_reply = false;
        list_insert_tail(device->manage->device_l, device);
        vdccm_write_connection_files(device);
        device->is_main_device = true;
        vdccm_run_scripts(device, (char *) "connect");
        vdccm_send_connection_info_to_clients(device, (char *) "C");
        synce_trace("Connected %s", device->name);
    } else {
        if (tmp_main_device != NULL) {
            tmp_main_device->is_main_device = true;
        }
    }

    return !skipped;
}


static bool vdccm_read_from_device_real(device_p device)
{
    bool success = false;
    char* buffer = NULL;
    uint32_t header;
    int i;

    if (!synce_socket_read(device->socket, &header, sizeof(header))) {
        synce_error("Failed to read header");
        goto exit;
    }

    header = letoh32(header);

    synce_trace("Read header: 0x%08x", header);

    if (0 == header) {
        synce_trace("empty package");
    } else if (DCCM_PING == header) {
        synce_trace("this is a ping reply");
        device->ping_count = 0;
    } else if (header < DCCM_MAX_PACKET_SIZE) {
        synce_trace("this is an information message");

        if (header < DCCM_MIN_PACKET_SIZE) {
            synce_error("Packet is smaller than expected");
            goto exit;
        }

        buffer = (char *) malloc(header);

        if (!buffer) {
            synce_error("Failed to allocate %i (0x%08x) bytes", header, header);
            goto exit;
        }

        if (!synce_socket_read(device->socket, buffer, header)) {
            synce_error("Failed to read package");
            goto exit;
        }

        dump("info package", buffer, header);

        /* Offset 0000 is always 24 00 00 00 ? */
        /* Offset 0004 contains the OS version, for example 03 00 = 3.0 */
        device->os_version = letoh16(*(uint16_t*)(buffer + 0x04));
        /* Offset 0006 contains the build number, for example 0x2ba3 = 11171 */
        device->build_number = letoh16(*(uint16_t*)(buffer + 0x06));
        /* Offset 0008 contains the processor type, for example 0x0a11 = 2577 */
        device->processor_type = letoh16(*(uint16_t*)(buffer + 0x08));
        /* Offset 000c is always 00 00 00 00 ? */

        /* Offset 0010 contains the first partner id */
        device->partner_id_1 = letoh32(*(uint32_t*)(buffer + 0x10));
        /* Offset 0014 contains the second partner id */
        device->partner_id_2 = letoh32(*(uint32_t*)(buffer + 0x14));

        device->name      = (char *) string_at(buffer, header, 0x18);

        for (i = 0; i < strlen(device->name); i++) {
            device->name[i] = tolower(device->name[i]);
        }

        device->class     = (char *) string_at(buffer, header, 0x1c);
        device->hardware  = (char *) string_at(buffer, header, 0x20);

        synce_trace("name    : %s", device->name);
        synce_trace("class   : %s", device->class);
        synce_trace("hardware: %s", device->hardware);

        free(buffer);
        buffer = NULL;

        if (device->locked) {
            if (password) {
                sprintf(device->password, "%s", password);
                if (!synce_password_send(device->socket, password, device->key)) {
                    synce_error("Failed to send password");
                    goto exit;
                }
            } else {
                vdccm_send_connection_info_to_clients(device, (char *) "P");
            }
            device->expect_password_reply = true;
        } else {
            if (!vdccm_add_device(device)) {
                synce_socket_free(device->socket);
                free_device(device);
            }
        }
    } else {
        synce_trace("This is a password challenge");
        device->key = header & 0xff;
        device->locked = true;
        if (!password)
            list_insert_tail(device->manage->passwordpending_device_l, device);
    }

    success = true;

exit:
    if (buffer)
        free(buffer);

    return success;
}


static int vdccm_read_from_device(void *device_v)
{
    device_p device = (device_p) device_v;

    if (device->expect_password_reply) {
        if (!synce_password_recv_reply(device->socket, 2,
                                       &device->password_correct)) {
            synce_error("Failed to read password reply");
            select_delfrom_rlist(device->manage->sel, synce_socket_get_descriptor(device->socket));
            synce_socket_free(device->socket);
            free_device(device);
        } else {
            if (device->password_correct) {
                if (!vdccm_add_device(device)) {
                    synce_socket_free(device->socket);
                    free_device(device);
                }
            } else {
                synce_trace("Password not accepted");
                select_delfrom_rlist(device->manage->sel, synce_socket_get_descriptor(device->socket));
                synce_socket_free(device->socket);
                vdccm_send_connection_info_to_clients(device, (char *) "R");
                free_device(device);
            }
        }
    } else {
        if (!vdccm_read_from_device_real(device)) {
            synce_error("Failed to read from device");
            vdccm_remove_device(device);
        }
    }

    return 0;
}


static int vdccm_accept_new_device(void *manage_v)
{
    manage_p manage = (manage_p) manage_v;
    device_p device;

    if ((device = (device_p) malloc(sizeof(device_t))) == NULL) {
        synce_error("Out of memory by adding a device");
        return 0;
    }
    device->manage = manage;
    device->expect_password_reply = false;
    device->locked = false;
    device->password_correct = false;
    device->is_main_device = false;
    device->name = NULL;
    device->class = NULL;
    device->hardware = NULL;

    device->socket = synce_socket_accept(manage->device_server, &device->address);

    if (!device->socket) {
        synce_error("Error clients TCP/IP-connection not accept()ed");
        free_device(device);
    } else {
        if (!inet_ntop(AF_INET, &device->address.sin_addr, device->ip_str, sizeof(device->ip_str))) {
            synce_error("inet_ntop failed");
            synce_socket_free(device->socket);
            free_device(device);
        } else {
            select_addto_rlist(manage->sel, synce_socket_get_descriptor(device->socket),
                           vdccm_read_from_device, device);
        }
    }

    return 0;
}


static int vdccm_send_ping_to_devices( void *manage_v)
{
    manage_p manage = (manage_p) manage_v;
    device_p device;
    const uint32_t ping = htole32(DCCM_PING);

    list_iterator(manage->device_l, device) {
        if (!synce_socket_write(device->socket, &ping, sizeof(ping))) {
            synce_error("failed to send ping");
            vdccm_remove_device(device);
        } else {
            if (++device->ping_count == missing_ping_count) {
                vdccm_remove_device(device);
            }
        }
    }
    return 0;
}


static void vdccm_write_help(char *name)
{
    fprintf(
        stderr,
        "Syntax:\n"
        "\n"
        "\t%s [-d LEVEL] [-f] [-h] [-p PASSWORD] [-i] [-u count] [-s sec] \n"
        "\n"
        "\t-d LEVEL     Set debug log level\n"
        "\t                 0 - No logging\n"
        "\t                 1 - Errors only (default)\n"
        "\t                 2 - Errors and warnings\n"
        "\t                 3 - Everything\n", name);
    fprintf(
        stderr,
        "\t-f           Do not run as daemon\n"
        "\t-h           Show this help message\n"
        "\t-p PASSWORD  Use this password when device connects\n"
        "\t-i           Use ip-address of device for identification\n"
        "\t-u           Allowed numbers of unanswered pings (default 3)\n"
        "\t-s           Delay between pings in seconds (default 5)\n");
}


static bool vdccm_handle_parameters(int argc, char** argv)
{
    int c;
    int log_level = SYNCE_LOG_LEVEL_ERROR;

    while ((c = getopt(argc, argv, "d:fhp:iu:s:")) != -1) {
        switch (c) {
        case 'd':
            log_level = atoi(optarg);
            break;

        case 'f':
            is_daemon = false;
            break;

        case 'p':
            if (password)
                free(password);
            password = strdup(optarg);
            break;

        case 'i':
            use_ipaddress = true;
            break;

        case 'u':
            missing_ping_count = atoi(optarg);
            break;

        case 's':
            ping_delay = atoi(optarg);
            break;

        case 'h':
        default:
            vdccm_write_help(argv[0]);
            return false;
        }
    }

    synce_log_set_level(log_level);

    return true;
}


bool vdccm_check_vdccm_already_running(const char* filename, const char* socketpath)
{
    bool success = false;
    FILE* file = NULL;
    char pid_str[16];
    struct stat dummy;

    if (0 == stat(filename, &dummy)) {
        /* File exists */
        file = fopen(filename, "r");

        if (!file) {
            synce_error("Failed to open %s for reading.", filename);
            goto exit;
        }

        if (fgets(pid_str, sizeof(pid_str), file)) {
            pid_t pid = atoi(pid_str);
            if (0 == kill(pid, 0)) {
                if (local_connect_socket(socketpath)) {
                    synce_error("It seems like dccm is already running with PID %i. If this is wrong, please remove the file %s and run dccm again.",
                                pid, filename);
                    goto exit;
                }
            }
        }

        fclose(file);
        file = NULL;
    }

    success = true;

exit:
    return success;
}


bool vdccm_write_pid_file(const char* filename)
{
    bool success = false;
    char pid_str[16];
    FILE* file = NULL;

    file = fopen(filename, "w");

    if (!file) {
        synce_error("Failed to open %s for writing.", filename);
        goto exit;
    }

    snprintf(pid_str, sizeof(pid_str), "%i", getpid());

    fputs(pid_str, file);
    fclose(file);
    file = NULL;

    success = true;

exit:
    if (file)
        fclose(file);

    return success;
}


int main(int argc, char *argv[])
{
    char *path = NULL;
    char control_socket_path[MAX_PATH];
    struct timeval timeout;
    struct timeval ping_interval;
    timer_p timer;
    char pid_file[MAX_PATH];

    umask(0077);

    if (!vdccm_handle_parameters(argc, argv))
        return -1;

    if (getuid() == 0) {
        synce_error("You should not run dccm as root.");
        return -1;
    }

    if (!synce_get_directory(&path)) {
        synce_error("Faild to get configuration directory name.");
        return -1;
    }

    snprintf(pid_file, sizeof(pid_file), "%s/" DCCM_PID_FILE, path);

    if (password) {
        int i;
        char *p;

        /* Protect password */
        for (i = 0; i < argc; i++) {
            if (strcmp(argv[i], password) == 0) {
                p = argv[i];
                if (*p) {
                    *p = 'X';
                    p++;

                    for (; *p; p++)
                        *p = '\0';
                }
                break;
            }
        }
    }

    snprintf(control_socket_path, sizeof(control_socket_path), "%s/%s", path, "csock");
    free(path);

    if (!vdccm_check_vdccm_already_running(pid_file, control_socket_path)) {
        return -1;
    }

    ping_interval.tv_sec = ping_delay;
    ping_interval.tv_usec = 0;

    signal(SIGPIPE, SIG_IGN);
    signal(SIGHUP, vdccm_handle_sighup);
    signal(SIGTERM, vdccm_handle_terminating_signals);
    signal(SIGINT, vdccm_handle_terminating_signals);
    signal(SIGABRT, vdccm_handle_terminating_signals);
    signal(SIGSEGV, vdccm_handle_terminating_signals);
    signal(SIGQUIT, vdccm_handle_terminating_signals);

    management.device_server = NULL;
    management.client_server = NULL;

    if (!(management.sel = select_create())) {
        synce_error("COuld not create select-object");
        return -1;
    }

    if (!(management.device_l = list_create())) {
        synce_error("Could not create list");
        return -1;
    }

    if (!(management.client_l = list_create())) {
        synce_error("Could not create list");
    }

    if (!(management.passwordpending_device_l = list_create())) {
        synce_error("Could not create list");
        return -1;
    }

    if ((timer = voc_timer_create()) == NULL) {
        synce_error("Could not create timer");
        return -1;
    }

    if (voc_timer_add_continous_node(timer, ping_interval,
                                     vdccm_send_ping_to_devices,
                                     &management) == NULL) {
        synce_error("Could not set continous trigger");
        return -1;
    }

    if (!(management.device_server = synce_socket_new())) {
        synce_error("Could not create pda-server-socket");
        return -1;
    }

    if (!(synce_socket_listen(management.device_server, NULL, DCCM_PORT))) {
        synce_error("Could not set pda-server-socket to listen mode");
        return -1;
    }


    if (!(management.client_server = local_listen_socket(control_socket_path, 2))) {
        synce_error("Could not create client-server-socket");
        return -1;
    }

    if (local_select_addto_rlist(management.sel, management.client_server,
                                 vdccm_accept_new_client, &management) < 0) {
        synce_error("Could not add local socket");
        return -1;
    }

    if (select_addto_rlist(management.sel, synce_socket_get_descriptor(management.device_server),
                           vdccm_accept_new_device, &management) < 0) {
        synce_error("Could not add PDA-server-socket");
        return -1;
    }

    if (is_daemon) {
        synce_trace("Forking into background");
        daemon(0, 0);
    } else {
        synce_trace("Running in foreground");
    }

    if (!vdccm_write_pid_file(pid_file)) {
        return -1;
    }

    vdccm_run_scripts(NULL, (char *) "start");

    while(running) {
        timeout = voc_timer_process_timer(timer);
        select_set_timeval(management.sel, timeout);
        select_select(management.sel);
    }

    vdccm_run_scripts(NULL, (char *) "stop");

    return 0;
}
