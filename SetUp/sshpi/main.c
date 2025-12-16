#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>
#include <string.h>

int getWaitTime();
void countDown();
void test(char* remoteArg, char* pi01_buffer, char* pi02_buffer);
bool pingPis();
void getVideos(char *remoteArg) ;

char *pi01_ip = "192.168.1.122";
char *pi02_ip = "192.168.1.151";

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s \"Argument to pass\"\n", argv[0]);
        return 1;
    }

    if(pingPis() == 0) return 0;

    // ---- Build dynamic command with user's argument ----
    char remoteArg[256];
    snprintf(remoteArg, sizeof(remoteArg), "'%s'", argv[1]); // preserves spaces

    char pi01_buffer[512];
    snprintf(pi01_buffer, sizeof(pi01_buffer),
        "ssh pi01@%s \"sleep $((60 - $(date +%%S))) && DISPLAY=:0 python3 /home/pi01/Desktop/Script/v2.py %s\"",
        pi01_ip,
        remoteArg
    );

    char pi02_buffer[512];
    snprintf(pi02_buffer, sizeof(pi02_buffer),
        "ssh pi02@%s \"sleep $((60 - $(date +%%S))) && DISPLAY=:0 python3 /home/pi02/Desktop/Script/v2.py %s\"",
        pi02_ip,
        remoteArg
    );

    char *pi01cmd[] = {"sh", "-c", pi01_buffer, NULL};
    char *pi02cmd[] = {"sh", "-c", pi02_buffer, NULL};

    test(remoteArg, pi01_buffer, pi02_buffer);

    // ---- FORK 1 ----
    pid_t pi01 = fork();

    if (pi01 == 0) {
        execvp("sh", pi01cmd);
        perror("pi01: execvp failed");
        exit(1);
    }

    // ---- FORK 2 ----
    pid_t pi02 = fork();
    if (pi02 == 0) {
        execvp("sh", pi02cmd);
        perror("pi02: execvp failed");
        exit(1);
    }

    countDown();

    // ---- Parent waits ----
    waitpid(pi01, NULL, 0);
    waitpid(pi02, NULL, 0);

    getVideos(remoteArg);
    return 0;
}

int getWaitTime() {
    time_t now = time(NULL);
    struct tm t = *localtime(&now);
    int sec = t.tm_sec;
    int wait = 60 - sec;
    if (wait == 60) wait = 0;
    return wait;
}

void countDown() {
    // ---- Countdown BEFORE starting fork ----
    int wait = getWaitTime();
    while (wait > 0) {
        printf("\rStarting in: %2d seconds", wait);
        fflush(stdout);
        sleep(1);
        wait = getWaitTime();
    }
    printf("\rStarting in:  0 seconds\n");
}

void test(char* remoteArg, char* pi01_buffer, char* pi02_buffer ) {
    // ---- printing the args and buffer for testing ----
    printf("Remote arg : %s\n", remoteArg);
    printf("Full pi01 cmd : %s\n", pi01_buffer);
    printf("Full pi02 cmd : %s\n", pi02_buffer);
    printf("Connecting to pi01 and pi02…\n");
}

bool pingPis() {

    char pi01_cmd[100];
    char pi02_cmd[100];

    snprintf(pi01_cmd, sizeof(pi01_cmd), "ping -c 1 -W 1 %s > /dev/null 2>&1", pi01_ip);
    snprintf(pi02_cmd, sizeof(pi02_cmd), "ping -c 1 -W 1 %s > /dev/null 2>&1", pi02_ip);

    int pi01_ret = system(pi01_cmd);
    if (pi01_ret == 0) {
        printf("Ping to %s succeeded!\n", pi01_ip);
    } else {
        printf("Ping to %s failed!\n", pi01_ip);
    }

    int pi02_ret = system(pi02_cmd);
    if (pi02_ret == 0) {
        printf("Ping to %s succeeded!\n", pi02_ip);
    } else {
        printf("Ping to %s failed!\n", pi02_ip);
    }

    return (pi01_ret == 0 && pi02_ret == 0);
}

void getVideos(char *remoteArg) {
    printf("Both pi finished.\n");

    char pi01_buffer[512];
    snprintf(pi01_buffer, sizeof(pi01_buffer),
        "scp pi01@%s:/home/pi01/%s.mp4 ~/Desktop/KAFD/Videos/pi01_%s.mp4",
        pi01_ip,
        remoteArg,
        remoteArg
    );

    char pi02_buffer[512];
    snprintf(pi02_buffer, sizeof(pi02_buffer),
        "scp pi02@%s:/home/pi02/%s.mp4 ~/Desktop/KAFD/Videos/pi02_%s.mp4",
        pi02_ip,
        remoteArg,
        remoteArg
    );

    printf("pi01_buffer : %s",pi01_buffer);
    system(pi01_buffer);
    system(pi02_buffer);
}


