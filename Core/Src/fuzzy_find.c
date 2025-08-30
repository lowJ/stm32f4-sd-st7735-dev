#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef struct {
    char* path;
    int score;
    int original_index;
} FuzzyMatch;

// Calculate fuzzy match score for a given path and query
// Higher score = better match
int calculate_score(const char* path, const char* query) {
    if (!path || !query) return 0;
    if (strlen(query) == 0) return 1; // Empty query matches everything
    
    const char* filename = strrchr(path, '/');
    filename = filename ? filename + 1 : path; // Get just the filename part
    
    int score = 0;
    int query_len = strlen(query);
    int path_len = strlen(filename);
    int consecutive_matches = 0;
    
    // Convert to lowercase for case-insensitive matching
    char* lower_filename = (char*)malloc(path_len + 1);
    char* lower_query = (char*)malloc(query_len + 1);
    
    for (int i = 0; i <= path_len; i++) {
        lower_filename[i] = tolower(filename[i]);
    }
    for (int i = 0; i <= query_len; i++) {
        lower_query[i] = tolower(query[i]);
    }
    
    // Check for exact substring match (bonus points)
    if (strstr(lower_filename, lower_query)) {
        score += 50;
        
        // Bonus for match at start of filename
        if (strncmp(lower_filename, lower_query, query_len) == 0) {
            score += 30;
        }
    }
    
    // Character-by-character fuzzy matching
    int query_idx = 0;
    for (int path_idx = 0; path_idx < path_len && query_idx < query_len; path_idx++) {
        if (lower_filename[path_idx] == lower_query[query_idx]) {
            score += 10;
            consecutive_matches++;
            query_idx++;
            
            // Bonus for consecutive character matches
            if (consecutive_matches > 1) {
                score += consecutive_matches * 2;
            }
        } else {
            consecutive_matches = 0;
        }
    }
    
    // Penalty for unmatched query characters
    if (query_idx < query_len) {
        score -= (query_len - query_idx) * 10;
    }
    
    // Bonus for shorter paths (prefer more specific matches)
    score += (100 - path_len) / 10;
    
    free(lower_filename);
    free(lower_query);
    
    return score > 0 ? score : 0;
}

// Comparison function for qsort
int compare_matches(const void* a, const void* b) {
    FuzzyMatch* match_a = (FuzzyMatch*)a;
    FuzzyMatch* match_b = (FuzzyMatch*)b;
    
    // Sort by score (descending), then by original index (ascending) for stability
    if (match_a->score != match_b->score) {
        return match_b->score - match_a->score;
    }
    return match_a->original_index - match_b->original_index;
}

// Main fuzzy finder function
// Returns number of matches found
int fuzzy_find(char** file_paths, int num_paths, const char* query, 
               char*** results, int max_results) {
    if (!file_paths || !query || !results || num_paths <= 0) {
        return 0;
    }
    
    // Create array to hold matches with scores
    FuzzyMatch* matches = (FuzzyMatch*)malloc(num_paths * sizeof(FuzzyMatch));
    int match_count = 0;
    
    // Calculate scores for all paths
    for (int i = 0; i < num_paths; i++) {
        int score = calculate_score(file_paths[i], query);
        if (score > 0) {  // Only include paths with positive scores
            matches[match_count].path = file_paths[i];
            matches[match_count].score = score;
            matches[match_count].original_index = i;
            match_count++;
        }
    }
    
    // Sort matches by score
    qsort(matches, match_count, sizeof(FuzzyMatch), compare_matches);
    
    // Limit results
    int result_count = match_count < max_results ? match_count : max_results;
    
    // Allocate result array
    *results = (char**)malloc(result_count * sizeof(char*));
    
    // Copy top matches to results
    for (int i = 0; i < result_count; i++) {
        (*results)[i] = matches[i].path;
    }
    
    free(matches);
    return result_count;
}

// Example usage
//int main() {
//    // Example file paths
//    char* file_paths[] = {
//        "/home/user/documents/readme.txt",
//        "/home/user/projects/myapp/src/main.c",
//        "/home/user/projects/myapp/include/header.h",
//        "/var/log/system.log",
//        "/usr/bin/make",
//        "/home/user/downloads/image.png",
//        "/home/user/projects/webapp/index.html",
//        "/home/user/documents/meeting_notes.md",
//        "/opt/tools/compiler/gcc",
//        "/home/user/projects/myapp/Makefile"
//    };
//    
//    int num_paths = sizeof(file_paths) / sizeof(file_paths[0]);
//    char** results;
//    
//    printf("=== Fuzzy Finder Demo ===\n\n");
//    
//    // Test different queries
//    const char* test_queries[] = {"main", "make", "app", "h", "doc"};
//    int num_queries = sizeof(test_queries) / sizeof(test_queries[0]);
//    
//    for (int q = 0; q < num_queries; q++) {
//        printf("Query: \"%s\"\n", test_queries[q]);
//        printf("Results:\n");
//        
//        int result_count = fuzzy_find(file_paths, num_paths, test_queries[q], 
//                                     &results, 5);
//        
//        if (result_count == 0) {
//            printf("  No matches found.\n");
//        } else {
//            for (int i = 0; i < result_count; i++) {
//                printf("  %d. %s\n", i + 1, results[i]);
//            }
//        }
//        
//        printf("\n");
//        
//        if (result_count > 0) {
//            free(results);
//        }
//    }
//    
//    return 0;
//}