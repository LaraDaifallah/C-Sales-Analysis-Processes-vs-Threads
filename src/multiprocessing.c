#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#define NUMFILES 20
#define MAXINPUT 40
#define MAXLINE 200
typedef struct {
    char region[MAXINPUT], country[MAXINPUT], itemType[MAXINPUT], salesChannel[MAXINPUT];
    int unitsSold;
    double unitPrice, unitCost;
} Record;
typedef struct { long long orders; double revenue, profit; } Totals;
static const char *filenames[NUMFILES] = {
    "xaa.csv","xab.csv","xac.csv","xad.csv","xae.csv",
    "xaf.csv","xag.csv","xah.csv","xai.csv","xaj.csv",
    "xak.csv","xal.csv","xam.csv","xan.csv","xao.csv",
    "xap.csv","xaq.csv","xar.csv","xas.csv","xat.csv"
};
/* Each worker parses its own files and returns independent totals.
   Input is the project's simple, unquoted CSV format. */
static Totals process_files(int start, int end, char choice, const char *value) {
    Totals totals = {0};
    for (int f = start; f < end; ++f) {
        FILE *fp = fopen(filenames[f], "r");
        if (!fp) { fprintf(stderr, "Error opening file: %s\n", filenames[f]); continue; }
        char line[MAXLINE];
        if (!fgets(line, sizeof line, fp)) { fclose(fp); continue; }
        while (fgets(line, sizeof line, fp)) {
            Record r = {0};
            char *saveptr = NULL;
            char *token = strtok_r(line, ",", &saveptr);
            int col = 0;
            while (token) {
                while (*token == ' ') ++token;
                size_t len = strlen(token);
                while (len && (token[len-1] == '\n' || token[len-1] == '\r' || token[len-1] == ' '))
                    token[--len] = '\0';
                switch (col) {
                    case 0: strncpy(r.region, token, MAXINPUT-1); break;
                    case 1: strncpy(r.country, token, MAXINPUT-1); break;
                    case 2: strncpy(r.itemType, token, MAXINPUT-1); break;
                    case 3: strncpy(r.salesChannel, token, MAXINPUT-1); break;
                    case 8: r.unitsSold = atoi(token); break;
                    case 9: r.unitPrice = atof(token); break;
                    case 10: r.unitCost = atof(token); break;
                }
                ++col;
                token = strtok_r(NULL, ",", &saveptr);
            }
            if (col < 11) continue;
            const char *field = choice == 'a' ? r.region : choice == 'b' ? r.country :
                                choice == 'c' ? r.itemType : r.salesChannel;
            if (strcmp(field, value) == 0) {
                ++totals.orders;
                totals.revenue += r.unitsSold * r.unitPrice;
                totals.profit += r.unitsSold * (r.unitPrice-r.unitCost);
            }
        }
        fclose(fp);
    }
    return totals;
}
static int get_filter(char *choice, char *value) {
    printf("Choose a filter option:\na. Region\nb. Country\nc. Item Type\nd. Sales Channel\nEnter choice (a/b/c/d): ");
    if (scanf(" %c", choice) != 1 || *choice < 'a' || *choice > 'd') {
        fprintf(stderr, "Invalid filter option.\n"); return 0;
    }
    printf("Enter the value to filter: ");
    return scanf(" %39[^\n]", value) == 1;
}
static void print_results(const char *value, Totals totals, double elapsed) {
    printf("\nResults for \"%s\":\nTotal Orders: %lld\nTotal Revenue: %.2f\nTotal Profit: %.2f\nExecution Time: %.6f seconds\n",
           value, totals.orders, totals.revenue, totals.profit, elapsed);
}

#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
/* Transfer the complete result even if a pipe operation is interrupted. */
static int transfer(int fd, void *buffer, size_t size, int writing) {
    char *cursor = buffer;
    while (size) {
        ssize_t n = writing ? write(fd, cursor, size) : read(fd, cursor, size);
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) return 0;
        cursor += n; size -= (size_t)n;
    }
    return 1;
}
/* Child processes read separate file ranges and send totals through pipes. */
int main(void) {
    char choice, value[MAXINPUT];
    int count;
    if (!get_filter(&choice, value)) return EXIT_FAILURE;
    printf("Enter number of processes: ");
    if (scanf("%d", &count) != 1) return EXIT_FAILURE;
    if (count <= 0) count = 2;
    if (count > NUMFILES) count = NUMFILES;
    int pipes[NUMFILES][2], launched = 0, failed = 0;
    pid_t pids[NUMFILES];
    struct timeval begin, end;
    gettimeofday(&begin, NULL);
    int base = NUMFILES/count, rem = NUMFILES%count;
    fflush(NULL);
    for (int p = 0; p < count; ++p) {
        if (pipe(pipes[p]) == -1) { perror("pipe"); failed = 1; break; }
        pids[p] = fork();
        if (pids[p] == -1) {
            perror("fork"); close(pipes[p][0]); close(pipes[p][1]); failed = 1; break;
        }
        if (pids[p] == 0) {
            close(pipes[p][0]);
            for (int j = 0; j < p; ++j) close(pipes[j][0]);
            int start = p*base+(p < rem ? p : rem);
            Totals local = process_files(start, start+base+(p < rem), choice, value);
            int ok = transfer(pipes[p][1], &local, sizeof local, 1);
            close(pipes[p][1]);
            _exit(ok ? EXIT_SUCCESS : EXIT_FAILURE);
        }
        close(pipes[p][1]); ++launched;
    }
    Totals totals = {0};
    for (int p = 0; p < launched; ++p) {
        Totals local = {0};
        if (!transfer(pipes[p][0], &local, sizeof local, 0)) {
            fprintf(stderr, "Failed to receive child result.\n"); failed = 1;
        } else {
            totals.orders += local.orders;
            totals.revenue += local.revenue;
            totals.profit += local.profit;
        }
        close(pipes[p][0]);
    }
    for (int p = 0; p < launched; ++p) {
        int status = 0;
        pid_t result;
        do { result = waitpid(pids[p], &status, 0); } while (result == -1 && errno == EINTR);
        if (result == -1 || !WIFEXITED(status) || WEXITSTATUS(status) != 0) failed = 1;
    }
    if (failed) return EXIT_FAILURE;
    gettimeofday(&end, NULL);
    print_results(value, totals, (end.tv_sec-begin.tv_sec)+(end.tv_usec-begin.tv_usec)/1e6);
    return EXIT_SUCCESS;
}
