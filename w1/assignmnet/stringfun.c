#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#define BUFFER_SZ 50

// prototypes
void usage(char *);
void print_buff(char *, int);
int setup_buff(char *, char *, int);

// prototypes for functions to handle required functionality
int count_words(char *, int, int);
int reverse_string(char *, int, int);
int word_print(char *, int, int);
int search_replace(char *, int, int, char *, char *);

// Function to prepare the buffer with user input, removing extra spaces
int setup_buff(char *buff, char *user_str, int len)
{
    char *src = user_str;      // Pointer to user input
    char *dst = buff;          // Pointer to the destination buffer
    int buffer_index = 0;      // Track buffer index
    bool in_whitespace = true; // Track whitespace state

    // Copy input to buffer, removing extra spaces
    while (*src != '\0' && buffer_index < len)
    {
        if (*src == ' ' || *src == '\t')
        { // Check for whitespace
            if (!in_whitespace && buffer_index > 0)
            {
                *dst = ' '; // Replace multiple spaces with one
                dst++;
                buffer_index++;
                in_whitespace = true;
            }
        }
        else
        { // Handle non-whitespace characters
            *dst = *src;
            dst++;
            buffer_index++;
            in_whitespace = false;
        }
        src++;
    }

    // Remove trailing whitespace, if any
    if (in_whitespace && buffer_index > 0)
    {
        dst--;
        buffer_index--;
    }

    // Check if the input exceeds buffer size
    if (buffer_index > len)
    {
        return -1; // Error: Input too large
    }

    // Fill remaining buffer space with dots
    while (buffer_index < len)
    {
        *dst = '.';
        dst++;
        buffer_index++;
    }

    return buffer_index; // Return the length of the processed buffer
}

// Function to print the buffer contents
void print_buff(char *buff, int len)
{
    printf("Buffer:  ["); // Begin buffer display
    for (int i = 0; i < len; i++)
    {                         // Iterate through buffer
        putchar(*(buff + i)); // Print each character
    }
    printf("]\n"); // End buffer display
}

// Function to display program usage instructions
void usage(char *exename)
{
    printf("usage: %s [-h|c|r|w|x] \"string\" [other args]\n", exename);
}

// Function to count words in the buffer
int count_words(char *buff, int len, int str_len)
{
    if (!buff || len <= 0 || str_len <= 0)
        return -1; // Validate inputs

    char *ptr = buff; // Pointer to traverse the buffer
    int count = 0;    // Word count
    int in_word = 0;  // Word boundary state

    for (int i = 0; i < str_len; i++)
    {
        if (*ptr == ' ')
        {
            in_word = 0; // End of word
        }
        else if (!in_word)
        {
            count++; // Start of a new word
            in_word = 1;
        }
        ptr++;
    }

    return count; // Return word count
}

// Function to reverse the string in the buffer
int reverse_string(char *buff, int len, int str_len)
{
    if (str_len > len)
        return -1; // Error if string exceeds buffer size

    char *start = buff;             // Pointer to start of the buffer
    char *end = buff + str_len - 1; // Pointer to end of the string
    char temp;                      // Temporary variable for swapping

    // Adjust end pointer to skip trailing dots
    while (*end == '.' && end > buff)
        end--;

    // Swap characters to reverse the string
    while (start < end)
    {
        temp = *start;
        *start = *end;
        *end = temp;
        start++;
        end--;
    }

    return 0; // Success
}

// Function to print words and their lengths from the buffer
int word_print(char *buff, int len, int str_len)
{
    if (!buff || len <= 0 || str_len <= 0)
        return -1; // Validate inputs

    char *ptr = buff; // Pointer to traverse the buffer
    int word_num = 1; // Word numbering
    int word_len = 0; // Current word length
    int in_word = 0;  // Word boundary state

    printf("Word Print\n----------\n");
    for (int i = 0; i < str_len; i++)
    {
        if (*ptr == ' ' || *ptr == '.')
        {
            if (in_word)
            { // Print word when boundary is reached
                printf("%d. ", word_num++);
                char *word_ptr = ptr - word_len;
                for (int j = 0; j < word_len; j++)
                    putchar(*word_ptr++);
                printf(" (%d)\n", word_len);
                word_len = 0;
                in_word = 0;
            }
        }
        else
        {
            word_len++;
            in_word = 1;
        }
        ptr++;
    }
    printf("\nNumber of words returned: %d\n", word_num-1);
    // Print the last word, if any
    if (in_word)
    {
        printf("%d. ", word_num++);
        char *word_ptr = ptr - word_len;
        for (int j = 0; j < word_len; j++)
            putchar(*word_ptr++);
        printf(" (%d)\n", word_len);
    }

    return 0; // Success
}

int search_replace(char *buff, int len, int str_len, char *search, char *replace) {
    if (!buff || len <= 0 || str_len <= 0 || !search || !replace)
        return -1; // Validate inputs

    char temp_buffer[BUFFER_SZ]; // Temporary buffer
    char *src = buff;            // Pointer to traverse the original buffer
    char *dst = temp_buffer;     // Pointer to write to the temp buffer
    int temp_len = 0;            // Length of the updated string
    
    // Calculate lengths of search and replace strings
    int search_len = 0, replace_len = 0;
    char *ptr;
    for (ptr = search; *ptr != '\0'; ptr++) search_len++;
    for (ptr = replace; *ptr != '\0'; ptr++) replace_len++;

    // Initialize temp_buffer with dots
    for (int i = 0; i < len; i++) {
        temp_buffer[i] = '.';
    }

    // Traverse the input string
    while (*src != '\0' && *src != '.' && temp_len < len) {
        // Check if current position matches the search string
        char *search_ptr = search;
        char *src_ptr = src;
        int match = 1;

        // Compare current position with search string
        while (*search_ptr != '\0') {
            if (*src_ptr == '\0' || *src_ptr == '.' || *src_ptr != *search_ptr) {
                match = 0;
                break;
            }
            search_ptr++;
            src_ptr++;
        }

        if (match) {
            // Check if replacement would cause buffer overflow
            if (temp_len + replace_len > len) {
                return -1;
            }
            // Copy replacement string
            ptr = replace;
            while (*ptr != '\0' && temp_len < len) {
                *dst = *ptr;
                ptr++;
                dst++;
                temp_len++;
            }
            src += search_len;
        } else {
            // Copy single character
            *dst = *src;
            dst++;
            src++;
            temp_len++;
        }
    }

    // Copy temp buffer back to original buffer
    dst = temp_buffer;
    for (int i = 0; i < len; i++) {
        buff[i] = temp_buffer[i];
    }

    return 0;
}



int main(int argc, char *argv[])
{
    char *buff;         // placehoder for the internal buffer
    char *input_string; // holds the string provided by the user on cmd line
    char opt;           // used to capture user option from cmd line
    int rc;             // used for return codes
    int user_str_len;   // length of user supplied string

    // TODO:  #1. WHY IS THIS SAFE, aka what if arv[1] does not exist?
    //       PLACE A COMMENT BLOCK HERE EXPLAINING
    /* TODO #1 Answer:
     * The if statement checking (argc < 2) first ensures we have at least one argument
     * before trying to dereference argv[1]. This prevents a segmentation fault that
     * would occur if we tried to access argv[1] when it doesn't exist. The second
     * check (*argv[1] != '-') is only evaluated if the first check passes, making
     * this a safe operation.
     */
    if ((argc < 2) || (*argv[1] != '-'))
    {
        usage(argv[0]);
        exit(1);
    }

    opt = (char)*(argv[1] + 1); // get the option flag

    // handle the help flag and then exit normally
    if (opt == 'h')
    {
        usage(argv[0]);
        exit(0);
    }

    // WE NOW WILL HANDLE THE REQUIRED OPERATIONS

    // TODO:  #2 Document the purpose of the if statement below
    //       PLACE A COMMENT BLOCK HERE EXPLAINING
    /* TODO #2 Answer:
     * This if statement ensures we have the minimum required arguments for all operations
     * except help (-h). We need at least the program name (argv[0]), the option flag
     * (argv[1]), and the input string (argv[2]), so argc must be at least 3.
     */
    if (argc < 3)
    {
        usage(argv[0]);
        exit(1);
    }

    input_string = argv[2]; // capture the user input string

    // TODO:  #3 Allocate space for the buffer using malloc and
    //           handle error if malloc fails by exiting with a
    //           return code of 99
    //  CODE GOES HERE FOR #3

    buff = (char *)malloc(BUFFER_SZ);
    if (buff == NULL)
    {
        printf("Error: Failed to allocate buffer\n");
        exit(2);
    }

    user_str_len = setup_buff(buff, input_string, BUFFER_SZ); // see todos
    if (user_str_len < 0)
    {
        printf("Error setting up buffer, error = %d", user_str_len);
        free(buff);
        exit(2);
    }

    switch (opt)
    {
    case 'c':
        rc = count_words(buff, BUFFER_SZ, user_str_len); // you need to implement
        if (rc < 0)
        {
            printf("Error counting words, rc = %d", rc);
            exit(2);
        }
        printf("Word Count: %d\n", rc);
        break;

    case 'r':
        rc = reverse_string(buff, BUFFER_SZ, user_str_len);
        if (rc < 0)
        {
            printf("Error reversing string, rc = %d\n", rc);
            exit(2);
        }
        break;

    case 'w':
        rc = word_print(buff, BUFFER_SZ, user_str_len);
        if (rc < 0)
        {
            printf("Error printing words, rc = %d\n", rc);
            exit(2);
        }
        break;
        case 'x':
            if (argc < 5) {
                usage(argv[0]);
                exit(1);
            }
            rc = search_replace(buff, BUFFER_SZ, user_str_len, argv[3], argv[4]);
            if (rc < 0) {
                printf("Error replacing string, rc = %d\n", rc);
                exit(2);
            }
            break;

    default:
        usage(argv[0]);
        exit(1);
    }

    // TODO:  #6 Dont forget to free your buffer before exiting
    print_buff(buff, BUFFER_SZ);
    free(buff);
    exit(0);
}

// TODO:  #7  Notice all of the helper functions provided in the
//           starter take both the buffer as well as the length.  Why
//           do you think providing both the pointer and the length
//           is a good practice, after all we know from main() that
//           the buff variable will have exactly 50 bytes?
//
//           PLACE YOUR ANSWER HERE
// TODO: #7
// Why is providing both the pointer and the length a good practice?

// Providing both the pointer and the length is a good practice because it ensures that functions have all the necessary information to safely operate on the buffer. Even though we know the buffer size in this specific case, passing the length explicitly helps to:

// Prevent Buffer Overflows: Functions can use the length to avoid accessing memory beyond the allocated buffer, preventing buffer overflows.
// Code Reusability: The functions become more reusable and flexible, as they can work with buffers of different sizes without relying on hardcoded values.
// Clarity: It makes the code clearer and more self-documenting, as the function signatures explicitly state the expected buffer size.