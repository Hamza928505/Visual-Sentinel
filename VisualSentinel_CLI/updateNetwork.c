#include "KAFD_COMMON.h"
#include "KAFD_UTILS.h"
#include "KADF_GLOBALS.h"
#include "KAFD_STRUCTS.h"
#include "KAFD_PATHS.h"

/* ----------------- Helpers ----------------- */

/*void ensure_dir(const char *path) {
    struct stat st;
    if (stat(path, &st) == -1) {
        if (mkdir(path, 0755) == -1 && errno != EEXIST) {
            perror("mkdir");
            exit(1);
        }
    }
}

void build_paths() {
    static int initialized = 0;
    if (initialized) return;   // already done

    const char *home = getenv("HOME");
    if (!home) {
        fprintf(stderr, "HOME environment variable not set\n");
        exit(1);
    }

    char desktop[MAX_PATH];
    char kafd[MAX_PATH];

    snprintf(desktop, MAX_PATH, "%s/Desktop", home);
    snprintf(kafd, MAX_PATH, "%s/Desktop/KAFD", home);

    ensure_dir(desktop);
    ensure_dir(kafd);

    snprintf(IP_FILE, MAX_PATH, "%s/IPAdress.txt", kafd);
    snprintf(NETWORKS_FILE, MAX_PATH, "%s/NetworksFile.txt", kafd);

    FILE *fp;
    fp = fopen(IP_FILE, "a");
    if (fp) fclose(fp);

    fp = fopen(NETWORKS_FILE, "a");
    if (fp) fclose(fp);

    initialized = 1;
} */

// Count lines in a file
int count_lines(const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) return -1;
    int lines = 0, c, prev = '\0';
    while ((c = fgetc(fp)) != EOF) {
        if (c == '\n') lines++;
        prev = c;
    }
    if (prev != '\n' && prev != '\0') lines++;
    fclose(fp);
    return lines;
}

/* Parse one line into ISP struct */
int parse_line(const char *line, struct ISP *isp)
{
    return sscanf(
               line,
               "%49[^:]:pi01{%15[^}]}:pi02{%15[^}]}",
               isp->NetworkName,
               isp->pi01IPAddress,
               isp->pi02IPAddress
           ) == 3;
}

// Validate IPv4 address
int isValidIPv4(const char *ip) {
    int a,b,c,d;
    char extra;
    if (sscanf(ip,"%d.%d.%d.%d%c",&a,&b,&c,&d,&extra)!=4) return 0;
    if (a<0||a>255||b<0||b>255||c<0||c>255||d<0||d>255) return 0;
    return 1;
}

// Ping an IP
int ping_ip(const char *ip) {
    char cmd[128];
    snprintf(cmd,sizeof(cmd),"ping -c 1 -W 1 %s > /dev/null 2>&1", ip);
    return system(cmd)==0;
}

// Load networks from file
struct ISP* loadNetworks(int *count) {

    FILE *file = fopen(NETWORKS_FILE,"r");
    if (!file) { *count=0; return NULL; }

    struct ISP *networks = malloc(sizeof(struct ISP)*MAX_NETWORKS);
    if (!networks) { *count=0; fclose(file); return NULL; }

    char line[256];
    int i=0;
    while (fgets(line,sizeof(line),file) && i<MAX_NETWORKS) {
        line[strcspn(line,"\n")]=0;
        char *p = strstr(line, ":pi01{");
        if(!p) continue;
        *p=0;
        strcpy(networks[i].NetworkName, line);

        char *q = p+6;
        char *r = strstr(q,"}:pi02{");
        if(!r) continue;
        *r=0;
        strcpy(networks[i].pi01IPAddress,q);

        q = r+7;
        r = strchr(q,'}');
        if(!r) continue;
        *r=0;
        strcpy(networks[i].pi02IPAddress,q);

        i++;
    }
    fclose(file);
    *count=i;
    return networks;
}

// Print all networks
void printAllNetworks(struct ISP *networks, int count) {
    if(count==0){ printf("No networks found.\n"); return; }

    printf("\n--- Networks (File Format) ---\n");
    for(int i=0;i<count;i++)
        printf("%s:pi01{%s}:pi02{%s}\n",
               networks[i].NetworkName,
               networks[i].pi01IPAddress,
               networks[i].pi02IPAddress);

    printf("\n--- Networks (Human-Readable) ---\n");
    printf("%-5s %-20s %-15s %-15s\n","No.","Network Name","PI-01 IP","PI-02 IP");
    printf("---------------------------------------------------------\n");
    for(int i=0;i<count;i++)
        printf("%-5d %-20s %-15s %-15s\n",
               i+1,
               networks[i].NetworkName,
               networks[i].pi01IPAddress,
               networks[i].pi02IPAddress);
}

/* ----------------- SSH Helpers ----------------- */

// Ensure local RSA key exists
bool ensureSSHKey() {
    char username[64]={0};
    const char *envUser = getenv("USER");
    if(envUser) strncpy(username,envUser,sizeof(username)-1);
    else {
        FILE *fp = popen("whoami","r");
        if(!fp) return false;
        if(!fgets(username,sizeof(username),fp)) { pclose(fp); return false; }
        pclose(fp);
    }
    username[strcspn(username,"\n")]=0;

    char sshDir[256], keyPath[256];
    snprintf(sshDir,sizeof(sshDir),"/home/%s/.ssh",username);
    mkdir(sshDir,0700); // create .ssh if not exists
    snprintf(keyPath,sizeof(keyPath),"%s/id_rsa.pub",sshDir);

    if(access(keyPath,F_OK)!=0) {
        printf("[*] RSA key not found, generating one...\n");
        char cmd[512];
        snprintf(cmd,sizeof(cmd),"ssh-keygen -t rsa -b 2048 -f %s/id_rsa -N \"\"",sshDir);
        if(system(cmd)!=0) { printf("[-] Failed to generate RSA key.\n"); return false; }
        printf("[+] RSA key generated.\n");
    } else printf("[*] RSA key exists.\n");
    return true;
}

// Test SSH access without password
bool testSSHAccess(const char *user, const char *host) {
    char cmd[256];
    snprintf(cmd,sizeof(cmd),
             "ssh -o BatchMode=yes -o ConnectTimeout=5 %s@%s 'echo ok' > /dev/null 2>&1",
             user, host);
    return system(cmd)==0;
}

// Copy local key to remote authorized_keys
bool ensureRemoteAuth(const char *user, const char *host) {
    char cmd[512];
    snprintf(cmd,sizeof(cmd),
             "ssh-copy-id -i ~/.ssh/id_rsa.pub %s@%s > /dev/null 2>&1",
             user, host);
    return system(cmd)==0;
}

/* ----------------- Modify Network ----------------- */

int modifyNetwork() {
//    build_paths();

    int total = count_lines(NETWORKS_FILE);
    if(total <= 0){ printf("No valid data found\n"); return 1; }

    struct ISP *networks = malloc(sizeof(struct ISP)*total);
    if(!networks){ perror("malloc"); return 1; }

    FILE *fp = fopen(NETWORKS_FILE,"r");
    if(!fp){ perror("fopen"); free(networks); return 1; }

    char line[256];
    int index=0;
    while(fgets(line,sizeof(line),fp) && index<total){
        line[strcspn(line,"\n")]=0;
        char *p=strstr(line,":pi01{");
        if(!p) continue;
        *p=0;
        strcpy(networks[index].NetworkName,line);

        char *q=p+6;
        char *r=strstr(q,"}:pi02{");
        if(!r) continue;
        *r=0;
        strcpy(networks[index].pi01IPAddress,q);

        q=r+7;
        r=strchr(q,'}');
        if(!r) continue;
        *r=0;
        strcpy(networks[index].pi02IPAddress,q);

        index++;
    }
    fclose(fp);

    // Iterate networks
    for(int i=0;i<index;i++){
        int pi01_ok=ping_ip(networks[i].pi01IPAddress);
        int pi02_ok=ping_ip(networks[i].pi02IPAddress);
        if(pi01_ok && pi02_ok){
            printf("Network: %s\nPI-01: %s\nPI-02: %s\n",
                   networks[i].NetworkName,
                   networks[i].pi01IPAddress,
                   networks[i].pi02IPAddress);

            // Save to output file
            FILE *out=fopen(IP_FILE,"w");
            if(!out){ perror("fopen"); free(networks); return 1; }
            fprintf(out,"%s:pi01{%s}:pi02{%s}\n",
                    networks[i].NetworkName,
                    networks[i].pi01IPAddress,
                    networks[i].pi02IPAddress);
            fclose(out);

            // Ensure SSH key
            if(!ensureSSHKey()){ printf("[-] Cannot ensure local SSH key.\n"); free(networks); return 1; }

            // PI-01
            if(!testSSHAccess("pi01",networks[i].pi01IPAddress)){
                printf("[*] PI-01 SSH not authorized, copying key...\n");
                if(!ensureRemoteAuth("pi01",networks[i].pi01IPAddress)){
                    printf("[-] Failed to authorize SSH on PI-01.\n");
                    free(networks);
                    return 1;
                }
            }

            // PI-02
            if(!testSSHAccess("pi02",networks[i].pi02IPAddress)){
                printf("[*] PI-02 SSH not authorized, copying key...\n");
                if(!ensureRemoteAuth("pi02",networks[i].pi02IPAddress)){
                    printf("[-] Failed to authorize SSH on PI-02.\n");
                    free(networks);
                    return 1;
                }
            }

            printf("[+] SSH keys are ready and authorized for both Pis.\n");
            free(networks);
            return 0;
        }
    }

    printf("No network found with both IPs reachable.\n");
    free(networks);
    return 1;
}

// ------------------- Add Network -------------------
int addNetwork() {
    struct ISP network;
    char input[MAX_NETWORK_NAME];
    int cancel_option = 0;

    /* -------- Network Name -------- */
    while (1) {
        printf("Enter Network Name (0 to cancel): ");
        fgets(input, MAX_NETWORK_NAME, stdin);
        input[strcspn(input, "\n")] = '\0';

        if (strcmp(input, "0") == 0) {
            return ADD_NETWORK_CANCEL;
        }

        if (strlen(input) == 0) {
            printf("Error: Network name cannot be empty.\n");
            continue;
        }

        strcpy(network.NetworkName, input);
        break;
    }

    /* -------- PI-01 IP -------- */
    while (1) {
        printf("Enter PI-01 IP Address (0 to cancel): ");
        fgets(input, MAX_IP, stdin);
        input[strcspn(input, "\n")] = '\0';

        if (strcmp(input, "0") == 0) {
            return ADD_NETWORK_CANCEL;
        }

        if (!isValidIPv4(input)) {
            printf("Error: Invalid IPv4 address.\n");
            continue;
        }

        strcpy(network.pi01IPAddress, input);
        break;
    }

    /* -------- PI-02 IP -------- */
    while (1) {
        printf("Enter PI-02 IP Address (0 to cancel): ");
        fgets(input, MAX_IP, stdin);
        input[strcspn(input, "\n")] = '\0';

        if (strcmp(input, "0") == 0) {
            return ADD_NETWORK_CANCEL;
        }

        if (!isValidIPv4(input)) {
            printf("Error: Invalid IPv4 address.\n");
            continue;
        }

        strcpy(network.pi02IPAddress, input);
        break;
    }

    /* -------- Save to File -------- */
    FILE *file = fopen(NETWORKS_FILE, "a");
    if (!file) {
        perror("fopen");
        return ADD_NETWORK_ERROR;
    }

    fprintf(file, "%s:pi01{%s}:pi02{%s}\n",
            network.NetworkName,
            network.pi01IPAddress,
            network.pi02IPAddress);

    fclose(file);

    /* -------- Print AFTER Save -------- */
    printf("\n--- Network Info ---\n");
    printf("Network: %s\n", network.NetworkName);
    printf("PI-01:   %s\n", network.pi01IPAddress);
    printf("PI-02:   %s\n", network.pi02IPAddress);
    printf("\nNetwork saved successfully.\n");

    return ADD_NETWORK_SUCCESS;
}

// ------------------- Update Network -------------------
int updateNetwork() {
    int count;
    struct ISP *networks = loadNetworks(&count);
    if (!networks || count == 0) {
        printf("No networks to update.\n");
        free(networks);
        return 1;
    }

    printAllNetworks(networks, count);

    int choice;
    printf("\nEnter the network number to update (0 to cancel): ");
    if (scanf("%d", &choice) != 1 || choice < 0 || choice > count) {
        while (getchar() != '\n');
        printf("Invalid selection.\n");
        free(networks);
        return 1;
    }
    while (getchar() != '\n');

    if (choice == 0) {
        free(networks);
        return 0;
    }

    struct ISP *net = &networks[choice - 1];
    char input[MAX_NETWORK_NAME];

    // Update network name
    printf("Enter new Network Name (Enter to keep '%s'): ", net->NetworkName);
    fgets(input, MAX_NETWORK_NAME, stdin);
    input[strcspn(input, "\n")] = 0;
    if (strlen(input) > 0) strcpy(net->NetworkName, input);

    // Update PI-01 IP
    printf("Enter new PI-01 IP (Enter to keep '%s'): ", net->pi01IPAddress);
    fgets(input, MAX_IP, stdin);
    input[strcspn(input, "\n")] = 0;
    if (strlen(input) > 0 && isValidIPv4(input)) strcpy(net->pi01IPAddress, input);

    // Update PI-02 IP
    printf("Enter new PI-02 IP (Enter to keep '%s'): ", net->pi02IPAddress);
    fgets(input, MAX_IP, stdin);
    input[strcspn(input, "\n")] = 0;
    if (strlen(input) > 0 && isValidIPv4(input)) strcpy(net->pi02IPAddress, input);

    // Save updated networks back to file
    FILE *file = fopen(NETWORKS_FILE, "w");
    if (!file) {
        perror("fopen");
        free(networks);
        return 1;
    }

    for (int i = 0; i < count; i++) {
        fprintf(file, "%s:pi01{%s}:pi02{%s}\n",
                networks[i].NetworkName,
                networks[i].pi01IPAddress,
                networks[i].pi02IPAddress);
    }

    fclose(file);
    printf("Network updated successfully.\n");

    free(networks);
    return 0;
}

// ------------------- Delete Network -------------------
int deleteNetwork() {
    int count;
    struct ISP *networks = loadNetworks(&count);
    if (!networks || count == 0) {
        printf("No networks to delete.\n");
        free(networks);
        return 1;
    }

    printAllNetworks(networks, count);

    int choice;
    printf("\nEnter the network number to delete (0 to cancel): ");
    if (scanf("%d", &choice) != 1 || choice < 0 || choice > count) {
        while (getchar() != '\n');
        printf("Invalid selection.\n");
        free(networks);
        return 1;
    }
    while (getchar() != '\n');

    if (choice == 0) {
        free(networks);
        return 0;
    }

    // Shift remaining networks up
    for (int i = choice - 1; i < count - 1; i++) {
        networks[i] = networks[i + 1];
    }
    count--;

    // Save remaining networks back to file
    FILE *file = fopen(NETWORKS_FILE, "w");
    if (!file) {
        perror("fopen");
        free(networks);
        return 1;
    }

    for (int i = 0; i < count; i++) {
        fprintf(file, "%s:pi01{%s}:pi02{%s}\n",
                networks[i].NetworkName,
                networks[i].pi01IPAddress,
                networks[i].pi02IPAddress);
    }

    fclose(file);
    printf("Network deleted successfully.\n");

    free(networks);
    return 0;
}
