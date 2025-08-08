
#include "main.h"
#include "fatfs.h"
#include "SEGGER_RTT.h"
#include "st7735.h"
#include "fonts.h"
#include <string.h>

extern SD_HandleTypeDef hsd;


/* 4 byte aligned for DMA */
/* alternate*/
uint8_t buf_swap = 0;
/* make exact frame size */
uint8_t buf[2][50000] __attribute__((aligned(4)));

#define NUM_VID_FILES_MAX

char* videos[NUM_VID_FILES_MAX] = { 0 };
char search_query


void app_mount_fs()
{
  if( f_mount(&SDFatFS, (TCHAR const*) SDPath, 0) != FR_OK ) {
    SEGGER_RTT_printf(0, "Could not mount disk\r\n");
    Error_Handler();
  }
  else
  {
    SEGGER_RTT_printf(0, "FS Mount SD!\r\n");
  }
}
void app_index_cycle_files()
{
  // Recursively searches every folder
  // adds filepath to videos list




}

// qmk code will stream over keypresses over serial (only when key pressed)
// TODO: can user_process_record detect keyrpesses on layers, eg CTRL, numbers, symbols, space,
void update_search_query_from_ascii_stream( uint8_t* buf, size_t length )
{
  while(size_t i = 0; i < length; i++)
  {


  }

}

/* TODO: should we accept '/'? would this be used by our search? */
bool is_valid_filename_char( char c )
{
  /* a-z*/
  if( c >= 'a' && c <= 'z' ) return true;

  /* A-Z */
  if( c >= 'A' && c <= 'Z' ) return true;

  /* 0-9 */
  if( c >= '0' && c <= '9' ) return true;

  /* . or - or _ */
  if( c == '.' || c == '-' || c == '_') return true;

  return false;
}

#define SEARCH_QUERY_STR_BUF_SIZE 256
#define ASCII_BACKSPACE 8
char search_query_str[SEARCH_QUERY_STR_BUF_SIZE] = { 0 };
void search_query_put_char( char c )
{
  size_t len = strlen(search_query_str);
  if( is_valid_filename_char(c))
  {
    /* TODO: handle buffer overflow case */
    search_query_str[len ] = c;
    search_query_str[len + 1] = '\0';
  }
  else if( c == ASCII_BACKSPACE )
  {
    if( len > 0 )
    {
      search_query_str[len - 1] = '\0';
    }
  }
  else
  {
    /* invalid ascii code, do nothing */
  }


}

#include "ff.h"     // FatFS header
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define MAX_NUM_VIDS 100

// Static array to store found .raw file paths
//Store path to each .raw video
static char* videos[MAX_NUM_VIDS];
static int video_index = 0; //num videos

// Helper function to check if filename ends with .raw
int has_raw_extension(const char* filename) {
    const char* ext = strrchr(filename, '.');
    return ext && strcasecmp(ext, ".raw") == 0;
}

// Toplevel directory on sd will be reserved for our usage
// Recursive traversal function
FRESULT scan_files(char* path) {
    FRESULT res;
    FILINFO fno;
    DIR dir;

#if _USE_LFN
    char lfn[_MAX_LFN + 1];
    fno.lfname = lfn;
    fno.lfsize = sizeof(lfn);
#endif

    res = f_opendir(&dir, path); // Open the directory
    if (res != FR_OK) return res;

    while (1) {
        res = f_readdir(&dir, &fno); // Read a directory item
        if (res != FR_OK || fno.fname[0] == 0) break; // Break on error or end of dir

        const char* fn = *fno.lfname ? fno.lfname : fno.fname;

        // Skip . and ..
        if (strcmp(fn, ".") == 0 || strcmp(fn, "..") == 0) continue;

        char full_path[256];
        snprintf(full_path, sizeof(full_path), "%s/%s", path, fn);

        if (fno.fattrib & AM_DIR) {
            // Directory, recurse into it
            res = scan_files(full_path);
            if (res != FR_OK) break;
        } else {
            // It's a file, check extension
            if (has_raw_extension(fn)) {
                if (video_index < MAX_NUM_VIDS) {
                    videos[video_index] = malloc(strlen(full_path) + 1);
                    if (videos[video_index]) {
                        strcpy(videos[video_index], full_path);
                        video_index++;
                    }
                }
            }
        }
    }

    f_closedir(&dir);
    return res;
}















