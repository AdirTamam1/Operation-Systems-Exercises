// Name: Adir Tamam
// ID: 318936507

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

// Function to get the attack power of an opponent
int get_opponent_attack(int opponent_id) {
    char filename[10];
    sprintf(filename, "G%d.txt", opponent_id);
    
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        perror("Error opening opponent file");
        return 0;
    }
    
    int health, attack;
    if (fscanf(file, "%d, %d,", &health, &attack) != 2) {
        fclose(file);
        return 0;
    }
    
    fclose(file);
    return attack;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <gladiator_file>\n", argv[0]);
        return 1;
    }
    
    // Get the gladiator file name without extension
    char *file_prefix = argv[1];
    
    // Create file names for the gladiator's stats and log
    char stat_filename[20];
    char log_filename[20];
    sprintf(stat_filename, "%s.txt", file_prefix);
    sprintf(log_filename, "%s_log.txt", file_prefix);
    
    // Open the gladiator's stat file
    FILE *stat_file = fopen(stat_filename, "r");
    if (stat_file == NULL) {
        perror(stat_filename);
        return 1;
    }
    
    // Open the log file for writing
    FILE *log_file = fopen(log_filename, "w");
    if (log_file == NULL) {
        perror(log_filename);
        fclose(stat_file);
        return 1;
    }
    
    // Read the gladiator's stats
    int health, attack, opponents[3];
    if (fscanf(stat_file, "%d, %d, %d, %d, %d", 
               &health, &attack, &opponents[0], &opponents[1], &opponents[2]) != 5) {
        fprintf(stderr, "Invalid format in %s\n", stat_filename);
        fclose(stat_file);
        fclose(log_file);
        return 1;
    }
    fclose(stat_file);
    
    // Log the gladiator's starting info
    fprintf(log_file, "Gladiator process started. PID: %d\n", getpid());
    
    // Fight until death
    int opponent_index = 0;
    while (health > 0) {
        // Get the current opponent
        int current_opponent = opponents[opponent_index];
        
        // Get the opponent's attack power
        int opponent_attack = get_opponent_attack(current_opponent);
        
        // Log the fight
        fprintf(log_file, "Facing opponent %d... Taking %d damage\n", 
                current_opponent, opponent_attack);
        
        // Update health
        health -= opponent_attack;
        
        // Log the result
        if (health > 0) {
            fprintf(log_file, "Are you not entertained? Remaining health: %d\n", health);
        } else {
            fprintf(log_file, "The gladiator has fallen... Final health: %d\n", health);
            break;
        }
        
        // Move to the next opponent (loop if needed)
        opponent_index = (opponent_index + 1) % 3;
    }
    
    // Close the log file
    fclose(log_file);
    
    return 0;
}