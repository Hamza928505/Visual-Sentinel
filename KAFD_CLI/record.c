#include "KAFD_COMMON.h"
#include "KAFD_UTILS.h"
#include "KADF_GLOBALS.h"
#include "KAFD_STRUCTS.h"
#include "KAFD_PATHS.h"

// Function prototypes
bool checkPiLogsActive(const char *pi_ip, const char *pi_name);
int getLastTimestamp(const char *pi_ip, const char *file, char *ts_out, size_t len);
int getWaitTime();
void countDown();
bool pingPis();
void getVideos(const char *remoteArg);
bool loadIPsFromFile();

// Globals
char pi01_ip[MAX_IP];
char pi02_ip[MAX_IP];

// --------- HELPER FUNCTIONS ---------
bool loadIPsFromFile() {
    FILE *fp = fopen(IP_FILE, "r");
    if (!fp) return false;

    char line[256], network[50];
    bool found = false;

    if (fgets(line, sizeof(line), fp)) {
        if (sscanf(line, "%49[^:]:pi01{%15[^}]}:pi02{%15[^}]}",
                   network, pi01_ip, pi02_ip) == 3) {
            printf("[*] Loaded IPs for Network [%s]: Pi01=%s Pi02=%s\n",
                   network, pi01_ip, pi02_ip);
            found = true;
        }
    }
    fclose(fp);
    return found;
}

int getWaitTime() {
    time_t now = time(NULL);
    struct tm t = *localtime(&now);
    int wait = 60 - t.tm_sec;
    return (wait == 60) ? 0 : wait;
}

void countDown() {
    int wait = getWaitTime();
    while (wait > 0) {
        printf("\rStarting in: %2d seconds", wait);
        fflush(stdout);
        sleep(1);
        wait = getWaitTime();
    }
    printf("\rStarting in:  0 seconds\n");
}

bool pingPis() {
    char cmd[128];
    snprintf(cmd, sizeof(cmd), "ping -c 1 -W 1 %s > /dev/null 2>&1", pi01_ip);
    int ret1 = system(cmd);
    snprintf(cmd, sizeof(cmd), "ping -c 1 -W 1 %s > /dev/null 2>&1", pi02_ip);
    int ret2 = system(cmd);

    printf("Ping PI-01 %s\n", ret1 == 0 ? "succeeded" : "failed");
    printf("Ping PI-02 %s\n", ret2 == 0 ? "succeeded" : "failed");

    return (ret1 == 0 && ret2 == 0);
}

int getLastTimestamp(const char *pi_ip, const char *file, char *ts_out, size_t len) {
    char cmd[MAX_CMD];
    snprintf(cmd, sizeof(cmd),
             "ssh %s 'tail -n 1 %s | awk -F\\\" '{print $4}' | tr -d ,'", pi_ip, file);
    FILE *fp = popen(cmd, "r");
    if (!fp) return 0;

    if (fgets(ts_out, len, fp)) {
        ts_out[strcspn(ts_out, "\n")] = 0;
        pclose(fp);
        return 1;
    }
    pclose(fp);
    return 0;
}

bool checkPiLogsActive(const char *pi_ip, const char *pi_name) {
    char imu1[MAX_TS] = {0}, imu2[MAX_TS] = {0};
    char gps1[MAX_TS] = {0}, gps2[MAX_TS] = {0};

    getLastTimestamp(pi_ip, "/var/log/imu/ypr.jsonl", imu1, sizeof(imu1));
    getLastTimestamp(pi_ip, "/var/log/gps/gps.jsonl", gps1, sizeof(gps1));

    sleep(1);

    getLastTimestamp(pi_ip, "/var/log/imu/ypr.jsonl", imu2, sizeof(imu2));
    getLastTimestamp(pi_ip, "/var/log/gps/gps.jsonl", gps2, sizeof(gps2));

    bool imu_active = strcmp(imu1, imu2) != 0;
    bool gps_active = strcmp(gps1, gps2) != 0;

    printf("%s Logs Status:\n  IMU %s -> %s | %s\n  GPS %s -> %s | %s\n\n",
           pi_name, imu1, imu2, imu_active ? "ACTIVE" : "STALLED",
           gps1, gps2, gps_active ? "ACTIVE" : "STALLED");

    return imu_active && gps_active;
}

void getVideos(const char *remoteArg) {
    char cmd[512];
    char *ips[] = {pi01_ip, pi02_ip};
    char *users[] = {"pi01", "pi02"};

    for (int i = 0; i < 2; i++) {
        snprintf(cmd, sizeof(cmd),
                 "scp %s@%s:/home/%s/%s.mp4 ~/Desktop/KAFD/Videos/%s_%s.mp4",
                 users[i], ips[i], users[i], remoteArg, users[i], remoteArg);
        system(cmd);

        snprintf(cmd, sizeof(cmd),
                 "scp %s@%s:/home/%s/%s_imu.jsonl ~/Desktop/KAFD/Videos/%s_%s_imu.jsonl",
                 users[i], ips[i], users[i], remoteArg, users[i], remoteArg);
        system(cmd);

        snprintf(cmd, sizeof(cmd),
                 "scp %s@%s:/home/%s/%s_gps.jsonl ~/Desktop/KAFD/Videos/%s_%s_gps.jsonl",
                 users[i], ips[i], users[i], remoteArg, users[i], remoteArg);
        system(cmd);
    }
}

// --------- RECORD FUNCTION ---------
int record(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s \"Argument to pass\"\n", argv[0]);
        return 1;
    }

    if (!loadIPsFromFile() || !pingPis()) {
        printf("[-] Error: Could not connect to Pis\n");
        return 1;
    }

    if (!checkPiLogsActive(pi01_ip, "PI-01") || !checkPiLogsActive(pi02_ip, "PI-02")) {
        printf("[-] Error: One or more Pis' logs are not active. Stopping record.\n");
        return 1;
    }

    char remoteArg[256];
    snprintf(remoteArg, sizeof(remoteArg), "'%s'", argv[1]);

    char pi01_cmd[512], pi02_cmd[512];
    snprintf(pi01_cmd, sizeof(pi01_cmd),
             "ssh pi01@%s \"sleep $((60 - $(date +%%S))) && DISPLAY=:0 python3 /home/pi01/Desktop/Script/v3.py %s > /dev/null 2>&1\"",
             pi01_ip, remoteArg);
    snprintf(pi02_cmd, sizeof(pi02_cmd),
             "ssh pi02@%s \"sleep $((60 - $(date +%%S))) && DISPLAY=:0 python3 /home/pi02/Desktop/Script/v3.py %s > /dev/null 2>&1\"",
             pi02_ip, remoteArg);

    pid_t pid1 = fork();
    if (pid1 == 0) { execl("/bin/sh", "sh", "-c", pi01_cmd, NULL); perror("pi01 exec"); exit(1); }

    pid_t pid2 = fork();
    if (pid2 == 0) { execl("/bin/sh", "sh", "-c", pi02_cmd, NULL); perror("pi02 exec"); exit(1); }

    countDown();

    printf("[+] Recording ...\n");
    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);

    printf("[+] Transferring files ...\n");
    getVideos(argv[1]);
    return 0;
}
