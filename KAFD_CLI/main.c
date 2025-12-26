#include "../../KAFD/include/KAFD_COMMON.h"
#include "../../KAFD/include/KADF_GLOBALS.h"
#include "../../KAFD/include/KAFD_STRUCTS.h"
#include "../../KAFD/include/KAFD_PATHS.h"

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
                else if (result == ADD_NETWORK_ERROR) printf("Error Adding network!\n");
                printf("\nPress Enter to return to menu...");
                getchar();
                break;

            case 3: // Load Network
                printf("\n[+] Running Load Networks...\n");
                int count; struct ISP *networks = loadNetworks(&count);
                if (networks) {
                    printAllNetworks(networks, count); // pass pointer and count
                    free(networks); // free after use
                } else {
                    printf("No networks found.\n");
                }
                printf("\nPress Enter to return to menu...");
                getchar();
                break;

            case 4: // Update Network
                printf("\n[+] Running Update Network...\n");
                if(updateNetwork() != 0) printf("Error Updating Network!\n");
                printf("\nPress Enter to return to menu...");
                getchar();
                break;

            case 5: // Delete Network
                printf("\n[+] Running Delete Network...\n");
                if(deleteNetwork() != 0) printf("Error Delete Network!\n");
                printf("\nPress Enter to return to menu...");
                getchar();
                break;

            case 6: // PIs Config
                printf("\n[+] Running PIs Config...\n");
                if(piConfig() != 0) printf("Error Configing PIs!\n");
                printf("\nPress Enter to return to menu...");
                getchar();
                break;

            case 7: // Record
                printf("\nEnter record filename: ");
                fgets(argument, sizeof(argument), stdin);
                argument[strcspn(argument, "\n")] = 0;
                if (strlen(argument) > 0)
                {
                    snprintf(command, sizeof(command), "~/Desktop/KAFD/Codes/record.o \"%s\"", argument);
                    printf("\n[+] Executing Record and Data Transfer...\n");
                    system(command);
                } else {
                    printf("Error: Argument required.\n");
                }
                printf("\nPress Enter to return to menu...");
                getchar();
                break;

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
