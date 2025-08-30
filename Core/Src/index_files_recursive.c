#include "ff.h"
#include <string.h>
#include <stdlib.h>
#include "SEGGER_RTT.h"

// Assuming these are defined elsewhere in your code

#define VIDEO_LIST_MAX_NUM_ITEMS 0x100
extern char* video_list[VIDEO_LIST_MAX_NUM_ITEMS];

extern uint16_t video_list_num_items;

/**
 * Helper function to check if a file has a video extension
 * Modify this based on your needs
 */
//static int is_video_file(const char* filename) {
//    const char* extensions[] = {".mp4", ".avi", ".mkv", ".mov", ".wmv", ".flv", ".webm", NULL};
//    int len = strlen(filename);
//    
//    for (int i = 0; extensions[i] != NULL; i++) {
//        int ext_len = strlen(extensions[i]);
//        if (len >= ext_len) {
//            // Case-insensitive comparison
//            int match = 1;
//            for (int j = 0; j < ext_len; j++) {
//                char c1 = filename[len - ext_len + j];
//                char c2 = extensions[i][j];
//                // Convert to lowercase for comparison
//                if (c1 >= 'A' && c1 <= 'Z') c1 += 32;
//                if (c2 >= 'A' && c2 <= 'Z') c2 += 32;
//                if (c1 != c2) {
//                    match = 0;
//                    break;
//                }
//            }
//            if (match) return 1;
//        }
//    }
//    return 0;
//}

/**
 * Recursive function to scan directories and index files
 * @param path Current directory path to scan
 * @return FR_OK on success, or FatFS error code
 */
FRESULT scan_files_recursive(char* path) {
    FRESULT res;
    DIR dir;
    FILINFO fno;
    char* new_path;
    int path_len;
    
    // Open the directory
    res = f_opendir(&dir, path);
    if (res != FR_OK) {
        return res;
    }
    
    // Read directory items
    while (1) {
        res = f_readdir(&dir, &fno);
        if (res != FR_OK || fno.fname[0] == 0) {
            break;  // Error or end of directory
        }
        
        // Skip hidden files and directories (starting with '.')
        if (fno.fname[0] == '.') {
            continue;
        }
        
        // Calculate the full path length
        path_len = strlen(path) + strlen(fno.fname) + 2; // +2 for '/' and '\0'
        
        // Check if we've reached the maximum number of items
        if (video_list_num_items>= VIDEO_LIST_MAX_NUM_ITEMS) {
            f_closedir(&dir);
            return FR_OK;  // Stop indexing when list is full
        }
        
        if (fno.fattrib & AM_DIR) {
            // It's a directory, recurse into it
            new_path = (char*)malloc(path_len);
            if (new_path == NULL) {
                f_closedir(&dir);
                return FR_NOT_ENOUGH_CORE;
            }
            
            // Build the new path
            sprintf(new_path, "%s/%s", path, fno.fname);
            
            SEGGER_RTT_printf(0, "recurse %s\r\n", new_path);
            // Recurse into the subdirectory
            res = scan_files_recursive(new_path);
            
            // Free the temporary path
            free(new_path);
            
            if (res != FR_OK) {
                f_closedir(&dir);
                return res;
            }
        } else {
            // It's a file, check if it's a video file (optional filter)
            // Remove this check if you want to index all files
            // Allocate memory for the full path
            new_path = (char*)malloc(path_len);
            if (new_path == NULL) {
                f_closedir(&dir);
                return FR_NOT_ENOUGH_CORE;
            }
            
            // Build the full path
            if (strlen(path) == 0 || (strlen(path) == 1 && path[0] == '/')) {
                // Root directory
                sprintf(new_path, "/%s", fno.fname);
            } else {
                sprintf(new_path, "%s/%s", path, fno.fname);
            }
            SEGGER_RTT_printf(0, "Add %s\r\n", new_path);
            
            // Add to the video list
            video_list[video_list_num_items] = new_path;
            video_list_num_items++;
        }
    }
    
    f_closedir(&dir);
    return FR_OK;
}

/**
 * Main function to index all files
 * Call this after f_mount has been successfully called
 * @return FR_OK on success, or FatFS error code
 */
//FRESULT index_all_files(void) {
//    FRESULT res;
//    
//    // Clear the existing list (free allocated memory if needed)
//    for (int i = 0; i < VIDEO_LIST_MAX_NUM_ITEMS; i++) {
//        if (video_list[i] != NULL) {
//            free(video_list[i]);
//            video_list[i] = NULL;
//        }
//    }
//    
//    // Reset the index counter
//    video_list_index = 0;
//    
//    // Start scanning from the root directory
//    // Use "" or "/" depending on your FatFS configuration
//    res = scan_files_recursive("/");
//    
//    if (res == FR_OK) {
//        // Optional: Print results for debugging
//        printf("Indexing complete. Found %d files.\n", video_list_index);
//        
//        // Optional: Print all indexed files
//        /*
//        for (int i = 0; i < video_list_index; i++) {
//            if (video_list[i] != NULL) {
//                printf("[%d] %s\n", i, video_list[i]);
//            }
//        }
//        */
//    } else {
//        printf("Indexing failed with error: %d\n", res);
//    }
//    
//    return res;
//}
//
/**
 * Get the number of indexed files
 * @return Number of files in the video_list
 */
//int get_indexed_file_count(void) {
//    return video_list_index;
//}

/**
 * Clean up allocated memory for all indexed files
 * Call this before unmounting or when done with the list
 */
//void cleanup_file_index(void) {
//    for (int i = 0; i < VIDEO_LIST_MAX_NUM_ITEMS; i++) {
//        if (video_list[i] != NULL) {
//            free(video_list[i]);
//            video_list[i] = NULL;
//        }
//    }
//    video_list_index = 0;
//}
