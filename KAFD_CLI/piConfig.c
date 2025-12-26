#include "KAFD_COMMON.h"
#include "KAFD_UTILS.h"
#include "KADF_GLOBALS.h"
#include "KAFD_STRUCTS.h"
#include "KAFD_PATHS.h"

bool loadIPsFromFile();
bool pingPis();
int load_ips(struct ISP *isp) {
    FILE *fp = fopen(IP_FILE, "r");
    if (!fp) {
        perror("Could not open IPAdress.txt");
        return 0;
    }

    char line[256];
    int success = 0;
    if (fgets(line, sizeof(line), fp)) {
        // Matches your specific file format [cite: 38, 52, 53]
        if (sscanf(line, "%49[^:]:pi01{%15[^}]}:pi02{%15[^}]}",
                   isp->NetworkName,
                   isp->pi01IPAddress,
                   isp->pi02IPAddress) == 3) {
            success = 1;
        }
    }
    fclose(fp);
    return success;
}

void execute_remote_setup(const char *ip, const char *pi_name) {
    if (!loadIPsFromFile() || !pingPis()) {
        printf("[-] Error: Could not connect to Pis\n");
        return;
    }

    char full_cmd[1024];

    // The bash script converted into a single SSH string
    const char *remote_script =
        "sudo truncate -s 0 /var/log/imu/ypr.jsonl && "
        "sudo truncate -s 0 /var/log/gps/gps.jsonl && "
        "sudo rm -f /root/.minimu9-ahrs-cal && "
        "rm -f ~/.minimu9-ahrs-cal && "
        "minimu9-ahrs-calibrate && "
        "sudo cp ~/.minimu9-ahrs-cal /root/ && "
        "sudo chown root:root /root/.minimu9-ahrs-cal";

    printf("--- Configuring %s at %s ---\n", pi_name, ip);

    // Build the SSH command [cite: 6, 7]
    snprintf(full_cmd, sizeof(full_cmd), "ssh %s@%s \"%s\"", pi_name, ip, remote_script);

    // Execute the command [cite: 23, 32]
    system(full_cmd);

    printf("%s IMU and GPS has been Truncated...\n", pi_name);
    printf("%s is ready.\n\n", pi_name);
}

int piConfig() {
    struct ISP net;

    if (!load_ips(&net)) {
        printf("Error: Failed to load network IPs.\n");
        return 1;
    }

    // Execute for Pi01
    execute_remote_setup(net.pi01IPAddress, "pi01");

    // Execute for Pi02
    execute_remote_setup(net.pi02IPAddress, "pi02");

    return 0;
}
