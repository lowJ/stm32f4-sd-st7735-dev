#include "fatfs.h"
#include <stdlib.h>
#include <stdbool.h>
#include "st7735.h"
#include "SEGGER_RTT.h"
#include <string.h>

#define APP_ROOT "/cycle" /* root folder */

#define VIDEOS_LIST_MAX_SIZE 0x10000 /* TODO: check this limit */
#define VIDEO_LIST_MAX_NUM_ITEMS 0x100

#define SEARCH_QUERY_BUFFER_SIZE 64

#define SCREEN_WIDTH 160
#define CHAR_WIDTH 7
#define NUM_CHAR_FIT_ON_SCREEN_WIDTH SCREEN_WIDTH / CHAR_WIDTH
const size_t string_length_screen_width = SCREEN_WIDTH / CHAR_WIDTH;

  #define CMD_SEARCH_OPEN 0x01
  #define CMD_SEARCH_EXIT 0x02
  #define CMD_SEARCH_UP 0x03
  #define CMD_SEARCH_DOWN 0x04
  #define CMD_SEARCH_SELECT 0x05

/* global values */
char* video_list[VIDEO_LIST_MAX_NUM_ITEMS] = { 0 };
uint16_t video_list_num_items = 0;

static bool search_active = false;

/* +1 for null terminator */
static char search_query[NUM_CHAR_FIT_ON_SCREEN_WIDTH + 1] = { 0 };
static size_t cursor_position = 0; // in the search list 

/* TODO: use correct num pixels */
uint8_t frame_buffer[2][50000] __attribute__((aligned(4)));
uint8_t frame_buffer_swap = 0;


void file_path_string_short( char* short_str, const char* full_str, size_t short_str_length );

extern bool drawing_in_progress;

extern SD_HandleTypeDef hsd;

extern SPI_HandleTypeDef hspi4;
extern DMA_HandleTypeDef hdma_spi4_tx;

extern UART_HandleTypeDef huart2;

/* workspace */
typedef struct
{
    FIL fd;
    char* current_file_path;
} window_context_t;

// helpers for qsort
typedef struct
{
    int8_t score;
    char* s;
} string_score_pair_t;

static window_context_t wc;
int string_score_pair_cmp(const void *a, const void *b)
{
    string_score_pair_t* a_pair = (string_score_pair_t*)a;
    string_score_pair_t* b_pair = (string_score_pair_t*)b;
    if( a_pair->score == b_pair->score )
    {
        return 0;
    }

    if( a_pair->score < b_pair->score )
    {
        return -1;
    }
    else
    {
        return 1;
    }
}

/* 4 byte aligned for DMA */
/* alternate*/
uint8_t buf_swap = 0;
uint8_t buf[2][50000] __attribute__((aligned(4)));

void app(){
    //init 
    /* cprintf starting app... */

    /* Mount file system */
    /* cprintf sdio stats  */
    SEGGER_RTT_printf(0, "SD INFO\r\n");
    SEGGER_RTT_printf(0, "Block size %lu\r\n", hsd.SdCard.BlockSize);
    SEGGER_RTT_printf(0, "Block num %lu\r\n", hsd.SdCard.BlockNbr);
    SEGGER_RTT_printf(0, "Card size %lu GB\r\n", ((uint64_t)hsd.SdCard.BlockSize * (uint64_t)hsd.SdCard.BlockNbr) / 1000000000 );


    ST7735_Init();
    bool buttonPressedEvent = false;
    FRESULT res;

    if( f_mount(&SDFatFS, (TCHAR const*) SDPath, 0) != FR_OK ) {
      SEGGER_RTT_printf(0, "Could not mount disk\r\n");
      Error_Handler();
    }
    else
    {
      SEGGER_RTT_printf(0, "FS Mount SD!\r\n");
    }
    /* Index videos */
    //starting at APP_ROOT folder, recursively iterate over each file.
    //store the full path to each file as an array of strings in video_list.
    //note: the size of the video_list is static, but the strings should be dynamically alocated
    res = scan_files_recursive(APP_ROOT);
    if(res == FR_NOT_ENOUGH_CORE)
    {
        SEGGER_RTT_printf(0, "Not enough heap\r\n" ); 

    }
    SEGGER_RTT_printf(0, "Indexed %d videos:\r\n", video_list_num_items); 
    for(size_t i = 0; i < video_list_num_items; i++)
    {
        HAL_Delay( 10 );
        SEGGER_RTT_printf(0, "%d, %s\r\n", i, video_list[i]);
    }

    //loop
    
    while(1)
    {
        bool query_updated_this_cycle = false;
        char c = 0;
        HAL_UART_Receive(&huart2, &c, 1, 0);
        //process incoming bytes/commands
        if( c == 0)
        {
            /* nothing */

        }
        else if( c == CMD_SEARCH_OPEN || c == CMD_SEARCH_EXIT)
        {
            if( search_active )
            {
                SEGGER_RTT_printf(0, "Exiting Search\r\n");

                //close existing search

                //zero search_query
                //memset()

                search_active = false;

            }
            else
            {
                SEGGER_RTT_printf(0, "Opening Search\r\n");
                // open search up 
                search_active = true;
            }
        }
        else if( c == 'A') //else if( c == CMD_SEARCH_UP)
        {
            //bounds check
            cursor_position--;
            SEGGER_RTT_printf(0, "Search Cursor Up\r\n");

        }
        //else if( c == CMD_SEARCH_DOWN)
        else if( c == 'S')
        {
            //bounds check
            cursor_position++;
            SEGGER_RTT_printf(0, "Search Cursor Down\r\n");
        }
        else if( c == 'Q') //else if( c == CMD_SEARCH_SELECT)
        {
            SEGGER_RTT_printf(0, "Select Item\r\n");

        }
        else
        {
            if( c == 0x08)
            {
                SEGGER_RTT_printf(0," backspace\r\n");

            }
            else
            {
                SEGGER_RTT_printf(0," char %c\r\n", c );

            }
            /* c is valid filename/path char */
            //add char to search_query, if bkspc delete char from search_query

            query_updated_this_cycle = true;
            
            /* TODO: check for valid char */
            size_t len = strlen(search_query);
            if( c == 0x08 )
            {
                if(len > 0)
                {
                    search_query[len - 1] = '\0';
                }

                ST7735_FillScreenFast( 65535);
            }
            else
            {
                if( len < (sizeof(search_query) - 1) )
                {
                    // append char
                    search_query[len] = c;
                }
            }

            ST7735_WriteString(0,0, search_query, Font_7x10, 0, 65535 );
        }




        if( search_active && query_updated_this_cycle )
        {
            //run query against video_list
            // ranks earch entry in video_list
            // quick sort the ranked video_list
            //

            // array to store a string and its score, matching against the search_query

            char** video_list_filtered; //TODO: MAKE SURE TO FREE THIS


            SEGGER_RTT_printf(0, "FuzzyFind\r\n");
            size_t result_count = fuzzy_find( video_list, video_list_num_items, search_query, &video_list_filtered, video_list_num_items );
            SEGGER_RTT_printf(0, "ff %d\r\n", result_count);
            for(int j = 0; j < result_count; j++)
            {
                SEGGER_RTT_printf(0, "%s\r\n", video_list_filtered[j]);
            }
           // // score each string
           // //for( size_t i = 0; i < video_list_num_items; i++)
           // //{
           // //    video_list_scores[i].s = video_list[i];
           // //    video_list_scores[i].score = my_scoring_func(video_list[i], search_query);
           // //}

           // //// sort
           // //qsort( video_list_scores, video_list_num_items, sizeof(string_score_pair_t), string_score_pair_cmp);

           // //draw the search box
           // //use cursor_position to determine which filenames to list

           // // figure out how many rows we can fit on screen for given font size
           // #define SCREEN_HEIGHT 128
           // #define CHAR_HEIGHT 10 /* this is determiend by chosen font */
           // const size_t num_rows_fit_on_screen = SCREEN_HEIGHT / CHAR_HEIGHT; /* char height */

           // #define SCREEN_WIDTH 160
           // #define CHAR_WIDTH 7
           // const size_t string_length_screen_width = SCREEN_WIDTH / CHAR_WIDTH;

           // #define TEXT_COLOR 255
           // #define TEXT_BGCOLOR 0

           // //figure out which strings to show.
           // //prioritize cursor being in center, but
           // 
           // //7 rows fit on screen
           // //3
           // //aaaa
           // //bbbb
           // //cccc
           // //dddd
           // //eeee
           // //ffff
           // //gggg


           // if( cursor_position < (num_rows_fit_on_screen / 2) )
           // {
           //     //curosr near top of list
           //     for( size_t i = 0; i < num_rows_fit_on_screen; i++)
           //     {
           //         char short_path[ SCREEN_WIDTH / CHAR_WIDTH ] ;
           //         file_path_string_short( short_path, video_list_filtered[i], string_length_screen_width);


           //         ST7735_WriteString(0,CHAR_HEIGHT * i, short_path, Font_7x10, TEXT_COLOR, TEXT_BGCOLOR  );


           //     }
           // 

           // }
           // else if( cursor_position > video_list_num_items - (num_rows_fit_on_screen / 2))
           // {
           //     //cursor near bottom of list
           //     for( size_t i = 0; i < num_rows_fit_on_screen; i++)
           //     {
           //         char short_path[ SCREEN_WIDTH / CHAR_WIDTH ] ;
           //         file_path_string_short( short_path, video_list_filtered[cursor_position + i - num_rows_fit_on_screen], string_length_screen_width);


           //         ST7735_WriteString(0,CHAR_HEIGHT * i, short_path, Font_7x10, TEXT_COLOR, TEXT_BGCOLOR  );


           //     }

           // }
           // else
           // {
           //     for( size_t i = 0; i < num_rows_fit_on_screen; i++)
           //     {
           //         char short_path[ SCREEN_WIDTH / CHAR_WIDTH ] ;
           //         file_path_string_short( short_path, video_list_filtered[cursor_position + i - (num_rows_fit_on_screen/2)], string_length_screen_width);


           //         ST7735_WriteString(0,CHAR_HEIGHT * i, short_path, Font_7x10, TEXT_COLOR, TEXT_BGCOLOR  );


           //     }
           //     //curso
           // }


           free(video_list_filtered);

        }
        else
        {
            ////read_frame_from_context
            //int ret = read_frame_from_context( &wc, &frame_buffer[frame_buffer_swap]);
            //if( ret == SUCCESS )
            //{
            ////ST7735_DrawImage()
            //}
            //else if( ret == 1) //1 = Failure due to EOF
            //{
            //    //loop, close fd and re open fd
            //    // better way to do this?
            //}
            //else
            //{
            //    //print error 
            //}


        }
    }
}


void file_path_string_short(char* short_str, const char* full_str, size_t short_str_length) {
    if (short_str == NULL || full_str == NULL || short_str_length == 0) {
        return;
    }
    
    // Ensure null termination
    short_str[0] = '\0';
    
    size_t full_len = strlen(full_str);
    
    // If the full string already fits, just copy it
    if (full_len < short_str_length) {
        strncpy(short_str, full_str, short_str_length - 1);
        short_str[short_str_length - 1] = '\0';
        return;
    }
    
    // Count path components
    int component_count = 0;
    const char* components[256];  // Max 256 path components
    size_t component_lens[256];
    
    // Parse path into components
    const char* start = full_str;
    const char* p = full_str;
    
    while (*p) {
        if (*p == '/' || *p == '\\') {
            if (p > start) {
                components[component_count] = start;
                component_lens[component_count] = p - start;
                component_count++;
            }
            start = p + 1;
        }
        p++;
    }
    
    // Add the last component (usually the filename)
    if (p > start) {
        components[component_count] = start;
        component_lens[component_count] = p - start;
        component_count++;
    }
    
    if (component_count == 0) {
        strncpy(short_str, full_str, short_str_length - 1);
        short_str[short_str_length - 1] = '\0';
        return;
    }
    
    // Strategy: Keep the filename and as many parent directories as possible
    // Abbreviate directories starting from the root
    
    // First, check if we need to abbreviate at all
    // Try to keep at least the filename and one parent
    int components_to_keep_full = 0;
    size_t needed_len = 1;  // For null terminator
    
    // Always try to keep the filename (last component) full
    if (component_count > 0) {
        needed_len += component_lens[component_count - 1] + 1;  // +1 for '/'
        components_to_keep_full = 1;
    }
    
    // See how many components we can keep full from the end
    for (int i = component_count - 2; i >= 0 && components_to_keep_full < component_count; i--) {
        size_t test_len = needed_len + component_lens[i] + 1;  // +1 for '/'
        if (test_len < short_str_length) {
            needed_len = test_len;
            components_to_keep_full++;
        } else {
            break;
        }
    }
    
    // Calculate how many components need abbreviation
    int components_to_abbrev = component_count - components_to_keep_full;
    
    // Build the shortened path
    char* dst = short_str;
    size_t remaining = short_str_length - 1;  // -1 for null terminator
    
    // Handle absolute paths
    if (full_str[0] == '/' || full_str[0] == '\\') {
        if (remaining > 0) {
            *dst++ = full_str[0];
            remaining--;
        }
    }
    
    // Add abbreviated components
    if (components_to_abbrev > 0) {
        // Add "..." for abbreviated parts
        const char* ellipsis = "...";
        size_t ellipsis_len = strlen(ellipsis);
        
        if (remaining >= ellipsis_len) {
            memcpy(dst, ellipsis, ellipsis_len);
            dst += ellipsis_len;
            remaining -= ellipsis_len;
        }
        
        // Add separator after ellipsis if there are more components
        if (components_to_keep_full > 0 && remaining > 0) {
            *dst++ = '/';
            remaining--;
        }
    }
    
    // Add the full components from the end
    for (int i = component_count - components_to_keep_full; i < component_count; i++) {
        size_t comp_len = component_lens[i];
        
        // Add the component
        size_t to_copy = (comp_len < remaining) ? comp_len : remaining;
        if (to_copy > 0) {
            memcpy(dst, components[i], to_copy);
            dst += to_copy;
            remaining -= to_copy;
        }
        
        // Add separator if not the last component
        if (i < component_count - 1 && remaining > 0) {
            *dst++ = '/';
            remaining--;
        }
    }
    
    *dst = '\0';
}