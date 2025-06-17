// NAME: Adir Tamam
// ID: 318936507 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>

#define MAX_PATH_LEN 1024
#define MAX_FILENAME_LEN 256
#define MAX_FILES 100

// Function to check if a path is a directory
int is_directory(const char *path) {
    struct stat st;
    if (stat(path, &st) != 0) {
        return 0;
    }
    return S_ISDIR(st.st_mode);
}

// Function to create a directory
void create_directory(const char *path) {
    pid_t pid = fork();
    
    if (pid < 0) {
        perror("fork failed");
        exit(1);
    } else if (pid == 0) {
        // Child process
        execl("/bin/mkdir", "mkdir", "-p", path, NULL);
        // If execl returns, it means there was an error
        perror("execl mkdir failed");
        exit(1);
    } else {
        // Parent process
        int status;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
            fprintf(stderr, "Error creating directory '%s'\n", path);
            exit(1);
        }
    }
}

// Function to copy a file from source to destination
void copy_file(const char *source_path, const char *dest_path) {
    pid_t pid = fork();
    
    if (pid < 0) {
        perror("fork failed");
        exit(1);
    } else if (pid == 0) {
        // Child process
        execl("/bin/cp", "cp", source_path, dest_path, NULL);
        // If execl returns, it means there was an error
        perror("execl cp failed");
        exit(1);
    } else {
        // Parent process
        int status;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
            fprintf(stderr, "Error copying file from '%s' to '%s'\n", source_path, dest_path);
            exit(1);
        }
    }
}

// Function to compare two files using diff
int compare_files(const char *file1, const char *file2) {
    pid_t pid = fork();
    
    if (pid < 0) {
        perror("fork failed");
        exit(1);
    } else if (pid == 0) {
        // Child process
        // Suppress diff output
        int null_fd = open("/dev/null", O_WRONLY);
        dup2(null_fd, STDOUT_FILENO);
        close(null_fd);
        
        execl("/usr/bin/diff", "diff", "-q", file1, file2, NULL);
        // If execl returns, it means there was an error
        perror("execl diff failed");
        exit(1);
    } else {
        // Parent process
        int status;
        waitpid(pid, &status, 0);
        // If diff returns 0, files are identical
        // If diff returns 1, files are different
        // If diff returns > 1, there was an error
        return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
    }
    
    return -1; // Should not reach here
}

// Function to check if file1 is newer than file2
int is_newer(const char *file1, const char *file2) {
    struct stat st1, st2;
    
    if (stat(file1, &st1) != 0) {
        perror("stat failed");
        exit(1);
    }
    
    if (stat(file2, &st2) != 0) {
        perror("stat failed");
        exit(1);
    }
    
    return st1.st_mtime > st2.st_mtime;
}

// Function to check if a file exists
int file_exists(const char *path) {
    return access(path, F_OK) == 0;
}

// Helper function to sort filenames alphabetically
int compare_strings(const void *a, const void *b) {
    return strcmp(*(const char **)a, *(const char **)b);
}

int main(int argc, char *argv[]) {
    char current_dir[MAX_PATH_LEN];
    char source_path[MAX_PATH_LEN];
    char dest_path[MAX_PATH_LEN];
    char source_file_path[MAX_PATH_LEN];
    char dest_file_path[MAX_PATH_LEN];
    DIR *dir;
    struct dirent *entry;
    char *filenames[MAX_FILES];
    int file_count = 0;
    
    // Get current working directory
    if (getcwd(current_dir, sizeof(current_dir)) == NULL) {
        perror("getcwd failed");
        exit(1);
    }
    printf("Current working directory: %s\n", current_dir);
    
    // Check command-line arguments
    if (argc != 3) {
        printf("Usage: file_sync <source_directory> <destination_directory>\n");
        exit(1);
    }
    
    // Verify source directory exists
    if (!is_directory(argv[1])) {
        printf("Error: Source directory '%s' does not exist.\n", argv[1]);
        exit(1);
    }
    
    // Store absolute paths
    strncpy(source_path, argv[1], MAX_PATH_LEN - 1);
    source_path[MAX_PATH_LEN - 1] = '\0';  // Ensure null-termination
    
    strncpy(dest_path, argv[2], MAX_PATH_LEN - 1);
    dest_path[MAX_PATH_LEN - 1] = '\0';  // Ensure null-termination
    
    // If destination doesn't exist, create it
    if (!is_directory(dest_path)) {
        create_directory(dest_path);
        printf("Created destination directory '%s'.\n", dest_path);
    }
    
    printf("Synchronizing from %s/%s to %s/%s\n", current_dir, source_path, current_dir, dest_path);
    
    // Open source directory
    dir = opendir(source_path);
    if (dir == NULL) {
        perror("opendir failed");
        exit(1);
    }
    
    // Read all filenames and store them for alphabetical processing
    while ((entry = readdir(dir)) != NULL && file_count < MAX_FILES) {
        // Skip directories and special entries
        if (entry->d_type == DT_REG) {  // Regular file
            filenames[file_count] = strdup(entry->d_name);
            if (filenames[file_count] == NULL) {
                perror("strdup failed");
                exit(1);
            }
            file_count++;
        }
    }
    closedir(dir);
    
    // Sort filenames alphabetically
    qsort(filenames, file_count, sizeof(char *), compare_strings);
    
    // Process each file
    for (int i = 0; i < file_count; i++) {
        // Check if combined path would be too long
        if (strlen(source_path) + strlen(filenames[i]) + 2 > MAX_PATH_LEN) {
            fprintf(stderr, "Path too long: %s/%s\n", source_path, filenames[i]);
            continue;
        }
        
        if (strlen(dest_path) + strlen(filenames[i]) + 2 > MAX_PATH_LEN) {
            fprintf(stderr, "Path too long: %s/%s\n", dest_path, filenames[i]);
            continue;
        }
        
        snprintf(source_file_path, MAX_PATH_LEN, "%s/%s", source_path, filenames[i]);
        snprintf(dest_file_path, MAX_PATH_LEN, "%s/%s", dest_path, filenames[i]);
        
        if (!file_exists(dest_file_path)) {
            // File doesn't exist in destination
            printf("New file found: %s\n", filenames[i]);
            copy_file(source_file_path, dest_file_path);
            printf("Copied: %s/%s -> %s/%s\n", current_dir, source_file_path, current_dir, dest_file_path);
        } else {
            // File exists in both directories, compare them
            int diff_result = compare_files(source_file_path, dest_file_path);
            
            if (diff_result == 0) {
                // Files are identical
                printf("File %s is identical. Skipping...\n", filenames[i]);
            } else if (diff_result == 1) {
                // Files are different, check which is newer
                if (is_newer(source_file_path, dest_file_path)) {
                    printf("File %s is newer in source. Updating...\n", filenames[i]);
                    copy_file(source_file_path, dest_file_path);
                    printf("Copied: %s/%s -> %s/%s\n", current_dir, source_file_path, current_dir, dest_file_path);
                } else {
                    printf("File %s is newer in destination. Skipping...\n", filenames[i]);
                }
            } else {
                // Error in diff
                fprintf(stderr, "Error comparing files '%s' and '%s'\n", source_file_path, dest_file_path);
                exit(1);
            }
        }
        
        free(filenames[i]);
    }
    
    printf("Synchronization complete.\n");
    return 0;
}