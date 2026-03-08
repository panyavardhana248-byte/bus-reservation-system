#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_BUSES 5
#define MAX_SEATS 40
#define USER_FILE "users.dat"
#define BOOKING_FILE "bookings.dat"

// Structures
typedef struct {
    char busNum[10];
    char source[30];
    char destination[30];
    int availableSeats[MAX_SEATS]; // 0: Empty, 1: Taken
    float price;
} Bus;

typedef struct {
    char userId[20];
    char password[20];
    char fullName[50];
    char phone[15];
    int isAdmin; 
} User;

typedef struct {
    char userId[20];
    char busNum[10];
    char destination[30];
    int seatNumber;
    float amount;
    int isBooked; 
} Booking;

// Global Data for 5 Buses
Bus busList[MAX_BUSES] = {
    {"B101", "Mumbai", "Pune", {0}, 450.0},
    {"B102", "Delhi", "Agra", {0}, 600.0},
    {"B103", "Bangalore", "Hyderabad", {0}, 1200.0},
    {"B104", "Chennai", "Kochi", {0}, 850.0},
    {"B105", "Pune", "Goa", {0}, 950.0}
};

// Function Prototypes
void signUp();
int login(User *loggedUser);
void bookTicket(User u);
void cancelTicket(User u);
void adminViewAll();
void searchBus();
void sendNotification(char *phone, char *action, char *bus, int seat);

int main() {
    int choice;
    User loggedUser;
    int isLoggedIn = 0;

    while (1) {
        if (!isLoggedIn) {
            printf("\n--- REDBUS PRO: MULTI-CITY SYSTEM ---\n");
            printf("1. Sign Up\n2. Login\n3. Exit\nChoose: ");
            scanf("%d", &choice);
            if (choice == 1) signUp();
            else if (choice == 2) isLoggedIn = login(&loggedUser);
            else exit(0);
        } else {
            printf("\n--- Welcome, %s ---\n", loggedUser.fullName);
            if (loggedUser.isAdmin) {
                printf("1. Check All Bookings\n2. Search Bus Details\n3. Logout\n");
            } else {
                printf("1. Search/View Buses\n2. Book a Ticket\n3. Cancel a Ticket\n4. Logout\n");
            }
            printf("Choose: ");
            scanf("%d", &choice);

            if (loggedUser.isAdmin) {
                if (choice == 1) adminViewAll();
                else if (choice == 2) searchBus();
                else isLoggedIn = 0;
            } else {
                if (choice == 1) searchBus();
                else if (choice == 2) bookTicket(loggedUser);
                else if (choice == 3) cancelTicket(loggedUser);
                else isLoggedIn = 0;
            }
        }
    }
    return 0;
}

// --- Implementation ---

void signUp() {
    FILE *fp = fopen(USER_FILE, "ab");
    User newUser;
    printf("\n--- NEW ACCOUNT ---\nFull Name: ");
    scanf(" %[^\n]s", newUser.fullName);
    printf("Phone: "); scanf("%s", newUser.phone);
    printf("User ID: "); scanf("%s", newUser.userId);
    printf("Password: "); scanf("%s", newUser.password);
    printf("Admin Status (1-Yes / 0-No): "); scanf("%d", &newUser.isAdmin);
    fwrite(&newUser, sizeof(User), 1, fp);
    fclose(fp);
    printf("Account Created Successfully!\n");
}

int login(User *loggedUser) {
    FILE *fp = fopen(USER_FILE, "rb");
    char id[20], pass[20];
    User temp;
    if (fp == NULL) return 0;
    printf("\n--- LOGIN ---\nUser ID: "); scanf("%s", id);
    printf("Password: "); scanf("%s", pass);
    while (fread(&temp, sizeof(User), 1, fp)) {
        if (strcmp(temp.userId, id) == 0 && strcmp(temp.password, pass) == 0) {
            *loggedUser = temp;
            fclose(fp);
            return 1;
        }
    }
    fclose(fp);
    printf("Login Failed.\n");
    return 0;
}

void searchBus() {
    char query[30];
    printf("\nEnter Bus Number or Destination to search: ");
    scanf("%s", query);
    printf("\n%-10s | %-15s | %-15s | %-10s\n", "Bus No", "Source", "Destination", "Price");
    printf("------------------------------------------------------------\n");
    for (int i = 0; i < MAX_BUSES; i++) {
        if (strcmp(busList[i].busNum, query) == 0 || strcmp(busList[i].destination, query) == 0) {
            printf("%-10s | %-15s | %-15s | %-10.2f\n", 
                   busList[i].busNum, busList[i].source, busList[i].destination, busList[i].price);
        }
    }
}

void bookTicket(User u) {
    char bNum[10];
    int found = -1, sNum;
    printf("\nEnter Bus Number to book: ");
    scanf("%s", bNum);

    for (int i = 0; i < MAX_BUSES; i++) {
        if (strcmp(busList[i].busNum, bNum) == 0) { found = i; break; }
    }

    if (found == -1) { printf("Bus not found!\n"); return; }

    printf("Available Seats in %s: ", bNum);
    for(int j=0; j<MAX_SEATS; j++) if(busList[found].availableSeats[j] == 0) printf("%d ", j+1);
    
    printf("\nChoose Seat: ");
    scanf("%d", &sNum);

    if (sNum < 1 || sNum > 40 || busList[found].availableSeats[sNum-1] == 1) {
        printf("Invalid Seat Choice.\n"); return;
    }

    Booking b;
    strcpy(b.userId, u.userId);
    strcpy(b.busNum, bNum);
    strcpy(b.destination, busList[found].destination);
    b.seatNumber = sNum;
    b.amount = busList[found].price;
    b.isBooked = 1;

    FILE *fp = fopen(BOOKING_FILE, "ab");
    fwrite(&b, sizeof(Booking), 1, fp);
    fclose(fp);

    busList[found].availableSeats[sNum-1] = 1;
    sendNotification(u.phone, "CONFIRMED", bNum, sNum);
}

void cancelTicket(User u) {
    FILE *fp = fopen(BOOKING_FILE, "rb+");
    char bNum[10];
    Booking b;
    int found = 0;

    printf("Enter Bus Number for cancellation: ");
    scanf("%s", bNum);

    while (fread(&b, sizeof(Booking), 1, fp)) {
        if (strcmp(b.userId, u.userId) == 0 && strcmp(b.busNum, bNum) == 0 && b.isBooked == 1) {
            b.isBooked = 0;
            fseek(fp, -sizeof(Booking), SEEK_CUR);
            fwrite(&b, sizeof(Booking), 1, fp);
            
            // Logic to free the seat in global array
            for(int i=0; i<MAX_BUSES; i++) {
                if(strcmp(busList[i].busNum, bNum) == 0) busList[i].availableSeats[b.seatNumber-1] = 0;
            }

            sendNotification(u.phone, "CANCELLED", bNum, b.seatNumber);
            found = 1; break;
        }
    }
    if (!found) printf("No active booking found for this bus/user.\n");
    fclose(fp);
}

void adminViewAll() {
    FILE *fp = fopen(BOOKING_FILE, "rb");
    Booking b;
    if (fp == NULL) return;
    printf("\n--- TOTAL BOOKINGS ---\n%-10s | %-10s | %-15s | %-5s | %-8s\n", "User ID", "Bus No", "To", "Seat", "Status");
    while (fread(&b, sizeof(Booking), 1, fp)) {
        printf("%-10s | %-10s | %-15s | %-5d | %-8s\n", 
               b.userId, b.busNum, b.destination, b.seatNumber, b.isBooked ? "ACTIVE" : "CANCEL");
    }
    fclose(fp);
}

void sendNotification(char *phone, char *action, char *bus, int seat) {
    printf("\n[SMS TO %s]: Ticket for %s (Seat %d) is %s.\n", phone, bus, seat, action);
}
