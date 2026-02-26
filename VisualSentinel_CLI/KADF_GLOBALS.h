#ifndef KAFD_GLOBALS_H
#define KAFD_GLOBALS_H

#define ADD_NETWORK_SUCCESS 0
#define ADD_NETWORK_CANCEL 1
#define ADD_NETWORK_ERROR  2

#define MAX_IP 16
#define MAX_CMD 256
#define MAX_TS 64
#define MAX_NETWORK_NAME 50
#define MAX_PATH 512
#define MAX_NETWORKS 100


extern char g_username[64];
extern char g_ssh_key_path[256];
extern int  g_verbose;

#endif
