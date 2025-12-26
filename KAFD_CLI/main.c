#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define ADD_NETWORK_SUCCESS 0
#define ADD_NETWORK_CANCEL 1
#define ADD_NETWORK_ERROR  2

#define MAX_NAME 50
#define MAX_IP   16

struct ISP {
    char NetworkName[MAX_NAME];
    char pi01IPAddress[MAX_IP];
    char pi02IPAddress[MAX_IP];
};

// Function prototypes
void show_menu();
struct ISP* loadNetworks(int *count);
void printAllNetworks(struct ISP *networks, int count);
int updateNetwork();
int deleteNetwork();
int addNetwork();
int modifyNetwork();
int piConfig();
int record(int argc, char *argv[]);

int main(int argc, char *argv[]) {  // <- corrected signature

    // Check if stdout is a terminal
    if (!isatty(fileno(stdout))) {
        // We are not in a terminal → launch xterm and replace this process
        char *xterm_args[] = {
            "xterm",
            "-hold",       // Keep terminal open after program exits
            "-e",          // Execute the program
            argv[0],       // This executable
            NULL
        };
        execvp("xterm", xterm_args); // Replace this process with xterm
        perror("execvp failed");     // Only reached if execvp fails
        return 1;
    }

    // Program continues normally if run in terminal
    printf("Hello! Running in terminal.\n");
    // Your menu logic
    int choice;
    char argument[256];
    char command[512];

    while (1) {
        show_menu();

        if (scanf("%d", &choice) != 1) {
            while (getchar() != '\n'); // clear input buffer
            continue;
        }

        while (getchar() != '\n'); // clear newline

        switch (choice) {
            case 1:
                printf("\n[+] Running Modify Network...\n");
                if(modifyNetwork() != 0) printf("Error Modifying Network!");
                printf("\nPress Enter to return to menu...");
                getchar();
                break;

            case 2:
                printf("\n[+] Running Add Network...\n");
                int result = addNetwork();
                if (result == ADD_NETWORK_CANCEL) printf("Add Network canceled by user.\n");
                else if (result == ADD_NETWORK_ERROR) printf("Error adding network!\n");
                printf("\nPress Enter to return to menu...");
                getchar();
                break;

            // ... keep your other cases unchanged ...

            case 8:
                system("clear");
                printf("Exiting KAFD CLI. Goodbye!\n");
                return 0;

            default:
                printf("\nInvalid option. Press Enter to try again.");
                getchar();
        }
    }

    return 0;
}

// Example show_menu function
void show_menu() {
    system("clear");
    printf("\n========== KAFD CLI ==========\n");
    printf("1) Modify Network (Find active IPs)\n");
    printf("2) Add Network\n");
    printf("3) Load Networks\n");
    printf("4) Update Network\n");
    printf("5) Delete Network\n");
    printf("6) PIs Config (Truncate & Calibrate)\n");
    printf("7) Record (Capture & Get All Data)\n");
    printf("8) Exit\n");
    printf("------------------------------\n");
    printf("Select option: ");
}
