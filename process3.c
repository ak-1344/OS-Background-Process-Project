#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#define TRACKING_DURATION 60   // Total duration for monitoring in seconds
#define CHECK_INTERVAL 1       // Interval to check active apps
#define NEGATIVE_TIME_LIMIT 20 // Threshold time for negative apps in seconds

// Define a list of negative applications
const char *negativeApps[] = {"firefox", "youtube", "instagram", "reddit", "tiktok"};
const int numNegativeApps = sizeof(negativeApps) / sizeof(negativeApps[0]);

int firstIteration = 0;

void get_top_application(char *app_name, size_t size) {
    FILE *fp;
    char command[128];
    snprintf(command, sizeof(command), "ps --no-headers -eo comm,pcpu --sort=-pcpu | head -n 1");

    fp = popen(command, "r");
    if (fp == NULL) {
        perror("Failed to get top application");
        strncpy(app_name, "Unknown", size);
        return;
    }

    fgets(app_name, size, fp);
    app_name[strcspn(app_name, "\n")] = 0; // Remove newline character
    pclose(fp);
}

int is_negative_app(const char *appName) {
    for (int i = 0; i < numNegativeApps; i++) {
        if (strcmp(appName, negativeApps[i]) == 0) {
            return 1;
        }
    }
    return 0;
}

void track_application_usage() {
    FILE *logFile;

    if (firstIteration == 0) {
        logFile = fopen("app_usage.log", "w");
        firstIteration++;
    } else {
        logFile = fopen("app_usage.log", "a");
    }

    if (logFile == NULL) {
        perror("Failed to open log file");
        return;
    }

    char lastApp[256] = "";
    int appTime = 0;
    time_t startTime = time(NULL);
    time_t currentTime;

    while (1) {
        currentTime = time(NULL);
        if (difftime(currentTime, startTime) >= TRACKING_DURATION) {
            break;
        }

        char currentApp[256];
        get_top_application(currentApp, sizeof(currentApp));

        if (strcmp(currentApp, lastApp) != 0) {
            if (strlen(lastApp) > 0) {
                fprintf(logFile, "%s: %d seconds\n", lastApp, appTime);
            }
            strcpy(lastApp, currentApp);
            appTime = CHECK_INTERVAL;
        } else {
            appTime += CHECK_INTERVAL;
        }

        sleep(CHECK_INTERVAL);
    }

    if (strlen(lastApp) > 0) {
        fprintf(logFile, "%s: %d seconds\n", lastApp, appTime);
    }

    fclose(logFile);
}

void generate_report() {
    FILE *logFile = fopen("app_usage.log", "r");
    if (logFile == NULL) {
        perror("Failed to open log file");
        return;
    }

    FILE *reportFile = fopen("Usage Report.txt", "w");
    if (reportFile == NULL) {
        perror("Failed to create report file");
        fclose(logFile);
        return;
    }

    time_t rawtime;
    struct tm *timeinfo;
    char timeBuffer[80];

    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d %H:%M:%S", timeinfo);

    fprintf(reportFile, "Report generated on: %s\n", timeBuffer);
    fprintf(reportFile, "Application Name               Time (seconds)\n");
    fprintf(reportFile, "--------------------------------------------\n");

    char line[256];
    char apps[100][256];
    int usageTime[100] = {0};
    int appCount = 0;

    while (fgets(line, sizeof(line), logFile)) {
        line[strcspn(line, "\n")] = 0;
        char appName[256];
        int timeSpent;
        sscanf(line, "%[^:]: %d", appName, &timeSpent);

        if (strlen(appName) > 0) {
            int found = 0;
            for (int i = 0; i < appCount; i++) {
                if (strcmp(apps[i], appName) == 0) {
                    usageTime[i] += timeSpent;
                    found = 1;
                    break;
                }
            }
            if (!found) {
                strcpy(apps[appCount], appName);
                usageTime[appCount] = timeSpent;
                appCount++;
            }
        }
    }

    fclose(logFile);

    // Write the usage data to the report file and check for negative apps
    int tooMuchTimeSpent = 0;
    for (int i = 0; i < appCount; i++) {
        fprintf(reportFile, "%-30s %d\n", apps[i], usageTime[i]);
        if (is_negative_app(apps[i]) && usageTime[i] >= NEGATIVE_TIME_LIMIT) {
            tooMuchTimeSpent = 1;
        }
    }

    fclose(reportFile);

    // Notify if the user spent too much time on negative apps
    if (tooMuchTimeSpent) {
        char command[256];
        snprintf(command, sizeof(command),
                 "notify-send 'Time Alert' 'You are spending too much time on negative apps. Please focus on productive tasks!' &");
        system(command);
    } else {
        // Notify that report is ready
        char command[256];
        snprintf(command, sizeof(command),
                 "notify-send 'Detailed Report' 'The detailed report is ready. Click to open.' "
                 "&& xdg-open 'Usage Report.txt' &");
        system(command);
    }
}

int main() {
    track_application_usage();
    generate_report();
    return 0;
}
