// Name: Adir Tamam
// ID: 318936507

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#define MAX_BUFFER_SIZE 1024
#define MAX_TEXT_SIZE 256

// Function to process read requests (R command)
void process_read(int data_fd, int results_fd, off_t start_offset, off_t end_offset) {
    char buffer[MAX_BUFFER_SIZE];
    int bytes_to_read = end_offset - start_offset + 1;
    
    // Check if start_offset is within file bounds
    struct stat file_stat;
    fstat(data_fd, &file_stat);
    if (start_offset >= file_stat.st_size) {
        // Start offset is beyond the file size, skip this read
        return;
    }
    
    // Adjust end_offset if it's beyond file size
    if (end_offset >= file_stat.st_size) {
        end_offset = file_stat.st_size - 1;
        bytes_to_read = end_offset - start_offset + 1;
    }
    
    // Seek to the start offset
    if (lseek(data_fd, start_offset, SEEK_SET) == -1) {
        perror("lseek");
        return;
    }
    
    // Read the data
    int bytes_read = read(data_fd, buffer, bytes_to_read);
    if (bytes_read == -1) {
        perror("read");
        return;
    }
    
    // Write the data to results file
    buffer[bytes_read] = '\0';
    write(results_fd, buffer, bytes_read);
    write(results_fd, "\n", 1); // Add a new line after each read
}

// Function to process write requests (W command)
void process_write(int data_fd, off_t offset, const char *text) {
    char buffer[MAX_BUFFER_SIZE];
    int text_len = strlen(text);
    
    // Check if the offset is valid
    struct stat file_stat;
    fstat(data_fd, &file_stat);
    
    if (offset > file_stat.st_size) {
        // If offset is beyond file size, skip this write
        return;
    }
    
    // Read the content after the offset
    off_t original_size = file_stat.st_size;
    if (lseek(data_fd, offset, SEEK_SET) == -1) {
        perror("lseek");
        return;
    }
    
    // Read the rest of the file
    int bytes_read = read(data_fd, buffer, MAX_BUFFER_SIZE);
    if (bytes_read == -1) {
        perror("read");
        return;
    }
    
    // Go back to the offset position
    if (lseek(data_fd, offset, SEEK_SET) == -1) {
        perror("lseek");
        return;
    }
    
    // Write the new text
    if (write(data_fd, text, text_len) == -1) {
        perror("write");
        return;
    }
    
    // Write back the original content
    if (write(data_fd, buffer, bytes_read) == -1) {
        perror("write");
        return;
    }
    
    // Truncate the file to the new size if needed
    if (ftruncate(data_fd, offset + text_len + bytes_read) == -1) {
        perror("ftruncate");
    }
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <data_file> <requests_file>\n", argv[0]);
        return 1;
    }
    
    // Open data file
    int data_fd = open(argv[1], O_RDWR);
    if (data_fd == -1) {
        perror(argv[1]);
        return 1;
    }
    
    // Open requests file
    FILE *requests_fp = fopen(argv[2], "r");
    if (requests_fp == NULL) {
        perror(argv[2]);
        close(data_fd);
        return 1;
    }
    
    // Open/create results file
    int results_fd = open("read_results.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (results_fd == -1) {
        perror("read_results.txt");
        fclose(requests_fp);
        close(data_fd);
        return 1;
    }
    
    char line[MAX_BUFFER_SIZE];
    char command;
    off_t offset, start_offset, end_offset;
    char text[MAX_TEXT_SIZE];
    
    // Process each request
    while (fgets(line, sizeof(line), requests_fp) != NULL) {
        // Parse the command
        if (line[0] == 'R') {
            if (sscanf(line, "R %ld %ld", &start_offset, &end_offset) == 2) {
                process_read(data_fd, results_fd, start_offset, end_offset);
            }
        } else if (line[0] == 'W') {
            // Find the offset and text
            char *offset_str = line + 2; // Skip "W "
            char *text_start = strchr(offset_str, ' ');
            
            if (text_start != NULL) {
                *text_start = '\0'; // Null-terminate the offset string
                text_start++; // Move to the text
                
                offset = atol(offset_str);
                
                // Remove trailing newline if present
                int len = strlen(text_start);
                if (len > 0 && text_start[len-1] == '\n') {
                    text_start[len-1] = '\0';
                }
                
                process_write(data_fd, offset, text_start);
            }
        } else if (line[0] == 'Q') {
            break;
        }
    }
    
    // Close all files
    close(results_fd);
    fclose(requests_fp);
    close(data_fd);
    
    return 0;
}