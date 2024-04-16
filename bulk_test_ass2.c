// COMP1511 Assignment 2 Bulk Tester
// Do whatever you want with this

/* INSTRUCTIONS:
    - Navigate to the same directory as crepe_stand.c
    - Back up your crepe_stand.c (just in case)

    - Set the below default values (if you want)
    - Compile this program with `gcc -O2 bulk_test_ass2.c -o bulk_test_ass2`

    - Compile your crepe_stand using one of the following commands:
        - `gcc -fsanitize=address crepe_stand.c main.c -o crepe_stand` to check for a
           range of errors including memory leaks, use after free etc.
           This is quite slow for tests with small inputs (but still way faster than dcc).

        - Or, use `gcc -fsanitize=leak crepe_stand.c main.c -o crepe_stand` to check only for memory leaks.
          Slightly faster than the above.

        - Or, if you don't want leak checking, use `gcc crepe_stand.c main.c -o crepe_stand`.
          Significantly faster than the above.

    - Run the program using `./bulk_test_ass2`
        - It's best to start out with a small input size so that you can easily identify the source of errors.

    - Run `chmod 755 compare.sh`
    - Copy the path of a dump file (e.g. dumps/dump_365263232.txt)
    - Run `./compare.sh <filename>`
        - If the diff is too long for your terminal, run `./compare.sh <filename> --silent`
          and open ./testing/comparison.txt for the diff
            - Use `diff ./testing/ours.txt ./testing/reference.txt` to get the
              line numbers of the errors

    - Remember to occasionally clear out ./dumps
*/

//------------> USER PARAMS <------------//
#define OUR_FILE "./crepe_stand"
#define REFERENCE_FILE "./crepe_stand_reference"

// number of bytes of input to provide
// don't go above ~60000 or it'll deadlock
// smaller values are better for debugging
#define DEFAULT_COMMAND_BUFFER_SIZE 500

// start with something small in case you get a lot of failures
#define DEFAULT_NUM_TESTS 100

// causes lots of false positives, you'll probably want to set this to 0
#define DEFAULT_TEST_MAX_PROFIT_PERIOD 1

// how often to print updated information about passed tests
#define FEEDBACK_EVERY 1000 // tests
//---------------------------------------//

#include <stdio.h>
#include <stdlib.h>
#include <spawn.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <string.h>
#include <signal.h>
#include <sys/random.h>

#pragma GCC diagnostic ignored "-Wformat-overflow"

#define DATE_LENGTH 10
#define SETUP_BUFFER_SIZE 11
#define SINGLE_COMMAND_MAX_LENGTH 60
#define NUM_RECENT_DATES 1000
#define NUM_RECENT_NAMES 1000


char recent_dates[NUM_RECENT_DATES][DATE_LENGTH + 1] = {0};
int recent_dates_idx = 0;

char recent_names[NUM_RECENT_NAMES][32] = {0};
int recent_names_idx = 0;

char trivial_commands[] = {'p', 'p', 'p', 'c', 's', 't', 'd', 'd', 'd', '>', '<', 'w'};

int test_max_profit_period = DEFAULT_TEST_MAX_PROFIT_PERIOD;

// the first "custom" generates a guaranteed valid input
char *crepe_types[] = {"custom", "custom", "matcha", "strawberry", "chocolate"};

int random_date_in_year(char *str, int year) {
    // generate a valid date in the format YYYY-MM-DD

    // pick a month between 1 and 12
    int month = rand() % 12 + 1;

    // "borrowed" from main.c
    int is_leap_year = year % 4 != 0 ? 0 : year % 100 != 0 ? 1 :
                       year % 400 == 0;
    int monthMaxDays[] = { 31, 28 + is_leap_year, 31, 30, 31,
                           30, 31, 31, 30, 31, 30, 31 };
    int maxDays = monthMaxDays[month - 1];

    // pick a day between 1 and maxDays
    int day = rand() % maxDays + 1;

    sprintf(str, "%d-%02d-%02d", year, month, day);

    strcpy(recent_dates[recent_dates_idx % NUM_RECENT_DATES], str);
    recent_dates_idx++;

    return DATE_LENGTH;
}

int random_date(char *str) {
    // pick a year between 1853 and 9999
    int year = rand() % 8147 + 1853;

    return random_date_in_year(str, year);
}

void fill_crepe_data(char *str) {
    // choose crepe type
    int crepe_type_idx = rand() % 5;

    sprintf(str + strlen(str), "%s ", crepe_types[crepe_type_idx]);

    // generate customer name
    int offset = strlen(str);
    int name_len = rand() % 31 + 1;
    for (int i = 0; i < name_len; i++) {
        str[offset + i] = 'A' + rand() % ('Z' - 'A' + 1) + (rand() & 1) * ('a' - 'A');
    }
    str[offset + name_len] = '\0';
    strcpy(recent_names[recent_names_idx % NUM_RECENT_NAMES], str + offset);
    recent_names_idx++;

    if (crepe_type_idx == 0) {
        // guaranteed valid
        // choose batter (0 to 2)
        int batter = rand() % 3;

        // choose topping (0 to 3)
        int topping = rand() % 4;

        // choose gfo (0 to 1)
        int gfo = rand() % 2;

        // choose size (10 to 39)
        int size = rand() % 30 + 10;

        sprintf(str + strlen(str), " %d %d %d %d", batter, topping, gfo, size);
    } else if (crepe_type_idx == 1) {
        // ~90% chance of being invalid
        // choose batter (-1 to 3)
        int batter = rand() % 5 - 1;

        // choose topping (-1 to 4)
        int topping = rand() % 6 - 1;

        // choose gfo (-1 to 2)
        int gfo = rand() % 4 - 1;

        // choose size (-1 to 50)
        int size = rand() % 52 - 1;

        sprintf(str + strlen(str), " %d %d %d %d", batter, topping, gfo, size);
    }
}

int append_crepe(char *str) {
    sprintf(str, "a ");
    fill_crepe_data(str);

    return strlen(str);
}

int insert_crepe(char *str) {
    // generate position (-1 to 50)
    int pos = rand() % 52 - 1;
    sprintf(str, "i %d ", pos);
    
    fill_crepe_data(str);

    return strlen(str);
}

int calculate_price(char *str) {
    // generate position (-1 to 50)
    int pos = rand() % 52 - 1;
    return sprintf(str, "C %d", pos);
}

int new_day(char *str) {
    int num = rand() % 3;
    if (num == 0) {
        // exactly use a recent date
        int idx = (rand() % recent_dates_idx) % NUM_RECENT_DATES;
        return sprintf(str, "n %s", recent_dates[idx]);
    }

    sprintf(str, "n ");

    if (num == 1) {
        // use a recent year
        int idx = (rand() % recent_dates_idx) % NUM_RECENT_DATES;
        char year_s[5];
        strncpy(year_s, recent_dates[idx], 5);
        int year = atoi(year_s);
        return (2 + random_date_in_year(str + 2, year));
    }
    
    // generate date
    return (2 + random_date(str + 2));
}

int remove_crepe(char *str) {
    // generate position (-1 to 50)
    int pos = rand() % 52 - 1;
    return sprintf(str, "r %d", pos);
}

int customer_bill(char *str) {
    // ~75% chance of valid customer name
    if (rand() % 4 == 0) {
        // generate customer name
        sprintf(str, "b ");
        int offset = strlen(str);
        int name_len = rand() % 31 + 1;
        for (int i = 0; i < name_len; i++) {
            str[offset + i] = 'A' + rand() % ('Z' - 'A' + 1) + (rand() & 1) * ('a' - 'A');
        }
        str[offset + name_len] = '\0';
    } else {
        // use recent customer name
        int idx = (rand() % recent_names_idx) % NUM_RECENT_NAMES;
        sprintf(str, "b %s", recent_names[idx]);
    }

    return strlen(str);
}

int remove_day(char *str) {
    // ~75% chance of existing date
    if (rand() % 4 == 0) {
        sprintf(str, "R ");
        // generate date
        return (2 + random_date(str + strlen(str)));
    } else {
        // use recent date
        int idx = (rand() % recent_dates_idx) % NUM_RECENT_DATES;
        return sprintf(str, "R %s", recent_dates[idx]);
    }
}

int trivial_command(char *str) {
    int idx = rand() % 12;
    sprintf(str, "%c", trivial_commands[idx]);

    return 1;
}

int max_profit_period(char *str) {
    if (!test_max_profit_period) {
        // do something else
        return trivial_command(str);
    }

    // ~75% chance of existing year
    if (rand() % 4 == 0) {
        // generate year
        int year = rand() % 8147 + 1853;
        return sprintf(str, "m %d", year);
    } else {
        // use recent date
        int idx = (rand() % recent_dates_idx) % NUM_RECENT_DATES;
        return sprintf(str, "m %.4s", recent_dates[idx]);
    }
}

void generate_setup(char str[SETUP_BUFFER_SIZE]) {
    random_date(str);
}


void generate_commands(char *str, int buffer_size) {
    char *end = str + buffer_size - 1;

    // reset the recent dates and names
    recent_dates_idx = recent_names_idx = 1;
    // make sure there's something in the arrays
    strcpy(recent_dates[0], "2024-01-01");
    strcpy(recent_names[0], "dave");

    while (str + SINGLE_COMMAND_MAX_LENGTH < end) {
        str += sprintf(str, "\n");

        int command_no = rand() % 100;
        if (command_no < 20) {
            str += append_crepe(str);
        } else if (command_no < 28) {
            str += insert_crepe(str);
        } else if (command_no < 35) {
            str += new_day(str);
        } else if (command_no < 39) {
            str += calculate_price(str);
        } else if (command_no < 43) {
            str += customer_bill(str);
        } else if (command_no < 47) {
            str += remove_crepe(str);
        } else if (command_no < 51) {
            str += remove_day(str);
        } else if (command_no < 55) {
            str += max_profit_period(str);
        } else {
            str += trivial_command(str);
        }
    }

    *str = '\0';
}

pid_t run_program(char *path, char **argv, FILE **stdin_fp, FILE **stdout_fp) {
    pid_t child_pid;
    int stdin_pipe[2];
    int stdout_pipe[2];
    posix_spawn_file_actions_t action;

    if(pipe(stdout_pipe) || pipe(stdin_pipe)) {
        printf("pipe returned an error.\n");
    }

    posix_spawn_file_actions_init(&action);
    posix_spawn_file_actions_addclose(&action, stdout_pipe[0]);
    posix_spawn_file_actions_adddup2(&action, stdout_pipe[1], 1);

    posix_spawn_file_actions_addclose(&action, stdout_pipe[1]);

    posix_spawn_file_actions_addclose(&action, stdin_pipe[1]);
    posix_spawn_file_actions_adddup2(&action, stdin_pipe[0], 0);

    posix_spawn_file_actions_addclose(&action, stdin_pipe[0]);

    *stdin_fp = fdopen(stdin_pipe[1], "w");
    *stdout_fp = fdopen(stdout_pipe[0], "r");

    int ret = posix_spawn(&child_pid, path, &action, NULL, argv, NULL);
    if (ret != 0) {
        printf("posix_spawn failed with code %d\n", ret);
    }

    // close the ends we won't be using
    close(stdin_pipe[0]);
    close(stdout_pipe[1]);

    // free the file actions object
    posix_spawn_file_actions_destroy(&action);

    return child_pid;
}

void dump_results(char setup[SETUP_BUFFER_SIZE], char *commands, int exit_code) {
    char filename[50];
    sprintf(filename, "./dumps/dump_%d_exit_%d.txt", rand(), exit_code);
    FILE *fp = fopen(filename, "w");
    if (fp != NULL) {
        fputs(setup, fp);
        fputs(commands, fp);
        fclose(fp);
    } else {
        printf("Couldn't open the file %s! Does the directory ./dumps exist?\n", filename);
    }
}

bool compare(char **argv, char *setup_buffer, char *command_buffer, int command_buffer_size) {
    pid_t child_pid_1, child_pid_2;
    FILE *stdin_fp_1, *stdout_fp_1, *stdin_fp_2, *stdout_fp_2;
    int status_1, status_2;

    child_pid_1 = run_program(OUR_FILE, argv, &stdin_fp_1, &stdout_fp_1);
    child_pid_2 = run_program(REFERENCE_FILE, argv, &stdin_fp_2, &stdout_fp_2);

    // generate setup string
    setup_buffer[0] = '\0';
    generate_setup(setup_buffer);

    // generate commands
    command_buffer[0] = '\0';
    generate_commands(command_buffer, command_buffer_size);

    fputs(setup_buffer, stdin_fp_1);
    fputs(command_buffer, stdin_fp_1);
    fclose(stdin_fp_1);

    fputs(setup_buffer, stdin_fp_2);
    fputs(command_buffer, stdin_fp_2);
    fclose(stdin_fp_2);

    bool equal = true;

    char recv_char_1 = getc(stdout_fp_1);
    char recv_char_2 = getc(stdout_fp_2);
    while(recv_char_1 != EOF || recv_char_2 != EOF) {
        // if 1 file ends earlier than the other, then EOF != whatever char
        if (recv_char_1 != recv_char_2) {
            if (recv_char_1 == EOF) {
                printf("Program 1 output ended before program 2 output\n");
            } else if (recv_char_2 == EOF) {
                printf("Program 2 output ended before program 1 output\n");
            } else {
                printf("Program 1 outputted %c, program 2 outputted %c\n", recv_char_1, recv_char_2);
            }

            equal = false;

            break;
        }

        recv_char_1 = getc(stdout_fp_1);
        recv_char_2 = getc(stdout_fp_2);
    }

    fclose(stdout_fp_1);
    fclose(stdout_fp_2);

    waitpid(child_pid_1, &status_1, 0);
    waitpid(child_pid_2, &status_2, 0);
    if (status_1 != 0) {
        equal = false;
        printf("process 1 exited with non-zero exit code: %d\n", status_1);
    }
    if (status_2 != 0) {
        printf("process 2 exited with non-zero exit code: %d\n", status_2);
    }

    if (equal == false) {
        dump_results(setup_buffer, command_buffer, status_1);
    }

    return equal;
}

int main(int argc, char *argv[], char *env[]) {
    // seed the PRNG
    unsigned int seed;
    if (getrandom(&seed, sizeof(seed), 0) == -1) {
        printf("Failed to initialise RNG.");
        exit(1);
    }
    srand(seed);

    // ignore SIGPIPE errors
    signal(SIGPIPE, SIG_IGN);


    // make the dumps and dumps/old directory if it doesn't exist
    system("mkdir -p ./dumps/old");

    // move old dumps to ./dumps/old
    system("bash -c \"mv ./dumps/* ./dumps/old\" 2>/dev/null");


    // start!
    printf("Remember to recompile your program if you've changed it!\n");
    printf("(by running `gcc -fsanitize=address crepe_stand.c main.c -o crepe_stand`)\n");

    // get the command buffer size
    int command_buffer_size = DEFAULT_COMMAND_BUFFER_SIZE;
    printf("How many bytes of input do you want per test? (default is %d): ", DEFAULT_COMMAND_BUFFER_SIZE);
    char command_buffer_size_s[11];
    fgets(command_buffer_size_s, 11, stdin);
    if (command_buffer_size_s[0] != '\n') {
        command_buffer_size = atoi(command_buffer_size_s);
    }

    if (command_buffer_size < 70) {
        printf("The input size is set very low! This might cause problems.\n");
    } else if (command_buffer_size > 60000) {
        printf("The input size is set very high! This might cause problems.\n");
    }

    // make the command buffer and setup buffer
    char *command_buffer = malloc(sizeof(char) * command_buffer_size);
    char setup_buffer[SETUP_BUFFER_SIZE];

    // get the number of tests
    int num_tests = DEFAULT_NUM_TESTS;
    printf("How many tests do you want to run? (default is %d): ", DEFAULT_NUM_TESTS);
    char num_tests_s[11];
    fgets(num_tests_s, 11, stdin);
    if (num_tests_s[0] != '\n') {
        num_tests = atoi(num_tests_s);
    }

    // get whether or not maximum_profit_period should be tested
    printf("Do you want to test maximum_profit_period? (1 or 0, default is %d): ", DEFAULT_TEST_MAX_PROFIT_PERIOD);
    char test_max_profit_period_s[11];
    fgets(test_max_profit_period_s, 11, stdin);
    if (test_max_profit_period_s[0] != '\n') {
        test_max_profit_period = atoi(test_max_profit_period_s);
    }

    printf("Started testing! Will do %d tests with %d bytes input each.\n", num_tests, command_buffer_size);

    int num_passed = 0;
    for (int i = 0; i < num_tests; i++) {
        if (compare(argv, setup_buffer, command_buffer, command_buffer_size)) {
            num_passed++;
        }
        if ((i + 1) % FEEDBACK_EVERY == 0) {
            printf("Testing... %d/%d tests passed.\n", num_passed, i + 1);
        }
    }

    printf("Testing done. %d/%d tests passed.\n", num_passed, num_tests);

    return 0;
}
