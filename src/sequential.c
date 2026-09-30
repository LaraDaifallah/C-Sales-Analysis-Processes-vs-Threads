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

#include <time.h>
/* Sequential baseline: one process reads all twenty files. */
int main(void) {
    char choice, value[MAXINPUT];
    if (!get_filter(&choice, value)) return EXIT_FAILURE;
    clock_t start = clock();
    Totals totals = process_files(0, NUMFILES, choice, value);
    double elapsed = (double)(clock()-start)/CLOCKS_PER_SEC;
    print_results(value, totals, elapsed);
    return EXIT_SUCCESS;
}
