// Adir Tamam 318936507
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>

// Global variables to track which signals were received
volatile sig_atomic_t email_received = 0;
volatile sig_atomic_t delivery_received = 0;
volatile sig_atomic_t doorbell_received = 0;

// Signal choices - using SIGUSR1, SIGUSR2, and SIGTERM
#define EMAIL_SIGNAL SIGUSR1
#define DELIVERY_SIGNAL SIGUSR2
#define DOORBELL_SIGNAL SIGTERM

// Signal handler function
void distraction_handler(int signum) {
    switch(signum) {
        case EMAIL_SIGNAL:
            email_received = 1;
            break;
        case DELIVERY_SIGNAL:
            delivery_received = 1;
            break;
        case DOORBELL_SIGNAL:
            doorbell_received = 1;
            break;
    }
}

// Function to set up signal handlers using sigaction
void setup_signal_handlers_Focus() {
    struct sigaction sa;

    // Initialize the sigaction structure
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = distraction_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    // Register handlers for all three signals using sigaction
    sigaction(EMAIL_SIGNAL, &sa, NULL);
    sigaction(DELIVERY_SIGNAL, &sa, NULL);
    sigaction(DOORBELL_SIGNAL, &sa, NULL);
}

// Function to block all distraction signals using sigprocmask
void block_distractions(sigset_t *oldset) {
    sigset_t newset;

    sigemptyset(&newset);
    sigaddset(&newset, EMAIL_SIGNAL);
    sigaddset(&newset, DELIVERY_SIGNAL);
    sigaddset(&newset, DOORBELL_SIGNAL);

    sigprocmask(SIG_BLOCK, &newset, oldset);
}

// Function to unblock all distraction signals using sigprocmask
void unblock_distractions(sigset_t *oldset) {
    sigprocmask(SIG_SETMASK, oldset, NULL);
}

// Function to check and handle pending signals using sigpending and sigismember
void check_pending_distractions() {
    sigset_t pending;

    printf("──────────────────────────────────────────────\n");
    printf("        Checking pending distractions...      \n");
    printf("──────────────────────────────────────────────\n");

    // Get pending signals using sigpending
    sigpending(&pending);

    int any_distractions = 0;

    // Check for each signal in order (1, 2, 3) using sigismember
    if (sigismember(&pending, EMAIL_SIGNAL)) {
        any_distractions = 1;
        printf(" - Email notification is waiting.\n");
        printf("[Outcome:] The TA announced: Everyone get 100 on the exercise!\n");
    }

    if (sigismember(&pending, DELIVERY_SIGNAL)) {
        any_distractions = 1;
        printf(" - You have a reminder to pick up your delivery.\n");
        printf("[Outcome:] You picked it up just in time.\n");
    }

    if (sigismember(&pending, DOORBELL_SIGNAL)) {
        any_distractions = 1;
        printf(" - The doorbell is ringing.\n");
        printf("[Outcome:] Food delivery is here.\n");
    }

    if (!any_distractions) {
        printf("No distractions reached you this round.\n");
    }

    printf("──────────────────────────────────────────────\n");
    printf("             Back to Focus Mode.              \n");
    printf("══════════════════════════════════════════════\n");

    // Temporarily unblock signals to clear pending signals
    sigset_t temp_unblock;
    sigemptyset(&temp_unblock);
    sigaddset(&temp_unblock, EMAIL_SIGNAL);
    sigaddset(&temp_unblock, DELIVERY_SIGNAL);
    sigaddset(&temp_unblock, DOORBELL_SIGNAL);
    sigprocmask(SIG_UNBLOCK, &temp_unblock, NULL);

    // Give a moment for signals to be delivered
    usleep(1000);

    // Block them again for the next round
    sigprocmask(SIG_BLOCK, &temp_unblock, NULL);
}

// Function to simulate a single focus round
void run_focus_round(int round_num, int duration) {
    printf("══════════════════════════════════════════════\n");
    printf("                Focus Round %d                \n", round_num);
    printf("──────────────────────────────────────────────\n");

    for (int i = 0; i < duration; i++) {
        printf("\nSimulate a distraction:\n");
        printf("  1 = Email notification\n");
        printf("  2 = Reminder to pick up delivery\n");
        printf("  3 = Doorbell Ringing\n");
        printf("  q = Quit\n");
        printf(">> ");
        fflush(stdout);

        // Use read() system call for more reliable input in automated testing
        char input_char;
        ssize_t bytes_read = read(STDIN_FILENO, &input_char, 1);

        if (bytes_read <= 0) {
            break;
        }

        // Consume any remaining newline character
        if (input_char != '\n') {
            char temp_char;
            while (read(STDIN_FILENO, &temp_char, 1) > 0 && temp_char != '\n');
        }

        if (input_char == 'q') {
            break;  // Exit the round early
        }
        else if (input_char == '1') {
            // Send email signal to self
            kill(getpid(), EMAIL_SIGNAL);
        }
        else if (input_char == '2') {
            // Send delivery signal to self
            kill(getpid(), DELIVERY_SIGNAL);
        }
        else if (input_char == '3') {
            // Send doorbell signal to self
            kill(getpid(), DOORBELL_SIGNAL);
        }
    }
}

// Main function to run Focus Mode
void runFocusMode(int numOfRounds, int roundDuration) {
    sigset_t oldset;

    // Set up signal handlers using sigaction
    setup_signal_handlers_Focus();

    printf("Entering Focus Mode. All distractions are blocked.\n");

    // Block all distraction signals using sigprocmask
    block_distractions(&oldset);

    // Run all rounds
    for (int round = 1; round <= numOfRounds; round++) {
        run_focus_round(round, roundDuration);

        // Check pending distractions using sigpending and sigismember
        check_pending_distractions();
    }

    // Unblock all signals at the end using sigprocmask
    unblock_distractions(&oldset);

    printf("\nFocus Mode complete. All distractions are now unblocked.\n");
}