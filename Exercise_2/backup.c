// Name: Adir Tamam
// ID: 318936507

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <errno.h>
#include <libgen.h>

#define MAX_PATH_LENGTH 4096

// Function to create a hard link
void create_hard_link(const char *src, const char *dst) {
    if (link(src, dst) != 0) {
        perror("Failed to create hard link");
        fprintf(stderr, "Source: %s, Destination: %s\n", src, dst);
    }
}

// Function to copy a symbolic link
void copy_symlink(const char *src, const char *dst) {
    char link_target[MAX_PATH_LENGTH];
    ssize_t len = readlink(src, link_target, sizeof(link_target) - 1);
    
    if (len == -1) {
        perror("Failed to read symlink");
        return;
    }
    
    link_target[len] = '\0';
    
    if (symlink(link_target, dst) != 0) {
        perror("Failed to create symlink");
        fprintf(stderr, "Source: %s, Destination: %s, Target: %s\n", src, dst, link_target);
    }
}

// Function to create directory with same permissions
void create_directory(const char *src, const char *dst) {
    struct stat st;
    
    if (stat(src, &st) != 0) {
        perror("Failed to get directory stats");
        return;
    }
    
    if (mkdir(dst, st.st_mode) != 0 && errno != EEXIST) {
        perror("Failed to create directory");
        fprintf(stderr, "Source: %s, Destination: %s\n", src, dst);
    }
}

// Recursive function to copy a directory
void copy_directory_recursive(const char *src_base, const char *dst_base, const char *rel_path) {
    char src_path[MAX_PATH_LENGTH];
    char dst_path[MAX_PATH_LENGTH];
    
    // Construct full paths
    if (rel_path[0] == '\0') {
        // Root directory
        if (strlen(src_base) >= MAX_PATH_LENGTH) {
            fprintf(stderr, "Source path too long: %s\n", src_base);
            return;
        }
        strncpy(src_path, src_base, MAX_PATH_LENGTH - 1);
        src_path[MAX_PATH_LENGTH - 1] = '\0';
        
        if (strlen(dst_base) >= MAX_PATH_LENGTH) {
            fprintf(stderr, "Destination path too long: %s\n", dst_base);
            return;
        }
        strncpy(dst_path, dst_base, MAX_PATH_LENGTH - 1);
        dst_path[MAX_PATH_LENGTH - 1] = '\0';
    } else {
        // Subdirectory
        if (snprintf(src_path, MAX_PATH_LENGTH, "%s/%s", src_base, rel_path) >= MAX_PATH_LENGTH) {
            fprintf(stderr, "Source path too long: %s/%s\n", src_base, rel_path);
            return;
        }
        
        if (snprintf(dst_path, MAX_PATH_LENGTH, "%s/%s", dst_base, rel_path) >= MAX_PATH_LENGTH) {
            fprintf(stderr, "Destination path too long: %s/%s\n", dst_base, rel_path);
            return;
        }
    }
    
    // Create the directory
    create_directory(src_path, dst_path);
    
    // Open the source directory
    DIR *dir = opendir(src_path);
    if (!dir) {
        perror("Failed to open directory");
        fprintf(stderr, "Path: %s\n", src_path);
        return;
    }
    
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        // Skip "." and ".."
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        // Construct new relative path
        char new_rel_path[MAX_PATH_LENGTH];
        if (rel_path[0] == '\0') {
            if (strlen(entry->d_name) >= MAX_PATH_LENGTH) {
                fprintf(stderr, "Entry name too long: %s\n", entry->d_name);
                continue;
            }
            strncpy(new_rel_path, entry->d_name, MAX_PATH_LENGTH - 1);
            new_rel_path[MAX_PATH_LENGTH - 1] = '\0';
        } else {
            if (snprintf(new_rel_path, MAX_PATH_LENGTH, "%s/%s", rel_path, entry->d_name) >= MAX_PATH_LENGTH) {
                fprintf(stderr, "Relative path too long: %s/%s\n", rel_path, entry->d_name);
                continue;
            }
        }
        
        // Construct full paths for the current entry
        char src_entry_path[MAX_PATH_LENGTH];
        char dst_entry_path[MAX_PATH_LENGTH];
        
        if (snprintf(src_entry_path, MAX_PATH_LENGTH, "%s/%s", src_path, entry->d_name) >= MAX_PATH_LENGTH) {
            fprintf(stderr, "Source entry path too long: %s/%s\n", src_path, entry->d_name);
            continue;
        }
        
        if (snprintf(dst_entry_path, MAX_PATH_LENGTH, "%s/%s", dst_path, entry->d_name) >= MAX_PATH_LENGTH) {
            fprintf(stderr, "Destination entry path too long: %s/%s\n", dst_path, entry->d_name);
            continue;
        }
        
        // Get file type
        struct stat st;
        if (lstat(src_entry_path, &st) != 0) {
            perror("Failed to get file stats");
            continue;
        }
        
        // Process based on file type
        if (S_ISDIR(st.st_mode)) {
            // Recursively process subdirectory
            copy_directory_recursive(src_base, dst_base, new_rel_path);
        } else if (S_ISLNK(st.st_mode)) {
            // Copy symbolic link
            copy_symlink(src_entry_path, dst_entry_path);
        } else if (S_ISREG(st.st_mode)) {
            // Create hard link for regular file
            create_hard_link(src_entry_path, dst_entry_path);
        }
        // Other file types are ignored
    }
    
    closedir(dir);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <source_directory> <backup_directory>\n", argv[0]);
        return 1;
    }
    
    // Check if source directory exists
    struct stat st;
    if (stat(argv[1], &st) != 0 || !S_ISDIR(st.st_mode)) {
        perror("src dir");
        return 1;
    }
    
    // Check if backup directory doesn't exist
    if (stat(argv[2], &st) == 0) {
        perror("backup dir");
        return 1;
    }
    
    // Create the backup directory
    if (mkdir(argv[2], 0755) != 0) {
        perror("Failed to create backup directory");
        return 1;
    }
    
    // Start the recursive copy
    copy_directory_recursive(argv[1], argv[2], "");
    
    return 0;
}