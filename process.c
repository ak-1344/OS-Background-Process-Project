#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#define TRACKING_DURATION 60 // Duration in seconds
#define CHECK_INTERVAL 1 // Check every 10 seconds

int firstIteration = 0; // Flag to track if it's the first iteration

void track_application_usage() {
    FILE *logFile;

    // Check if it's the first iteration
    if (firstIteration==0) {
        logFile = fopen("app_usage.log", "w"); // Create new log file
        firstIteration++; // Counts the number of times funtion is executed
    } else {
        logFile = fopen("app_usage.log", "a"); // Append to existing log file
    }

    if (logFile == NULL) {
        perror("Failed to open log file");
        return;
    }
    
    char lastApp[256] = "";
    int appTime = 0; // Time spent on the current app

    time_t startTime = time(NULL);
    time_t currentTime;

    while (1) {
        currentTime = time(NULL);
        
        // Break the loop after the tracking duration
        if (difftime(currentTime, startTime) >= TRACKING_DURATION) {
            break;
        }
        
        // Get the name of the currently active window
        char command[256];
        snprintf(command, sizeof(command), "xdotool getactivewindow getwindowname");
        
        FILE *fp = popen(command, "r");
        if (fp == NULL) {
            perror("Failed to run command");
            break;
        }

        char currentApp[256];
        fgets(currentApp, sizeof(currentApp), fp);
        currentApp[strcspn(currentApp, "\n")] = 0; // Remove newline character
        pclose(fp);

        // If the app has changed, log the time spent on the previous app
        if (strcmp(currentApp, lastApp) != 0) {
            if (strlen(lastApp) > 0) {
                fprintf(logFile, "%s: %d seconds\n", lastApp, appTime);
            }
            strcpy(lastApp, currentApp);
            appTime = CHECK_INTERVAL; // Reset timer for new app
        } else {
            appTime += CHECK_INTERVAL; // Increment time for the same app
        }

        sleep(CHECK_INTERVAL);
    }

    // Log the last application's time if it exists
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

    // Get the current date and time
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
        line[strcspn(line, "\n")] = 0; // Remove newline
        char appName[256];
        int timeSpent;
        sscanf(line, "%[^:]: %d", appName, &timeSpent);

        if (strlen(appName) > 0) {
            int found = 0;
            for (int i = 0; i < appCount; i++) {
                if (strcmp(apps[i], appName) == 0) {
                    usageTime[i] += timeSpent; // Add time spent for existing app
                    found = 1;
                    break;
                }
            }
            if (!found) {
                strcpy(apps[appCount], appName);
                usageTime[appCount] = timeSpent; // Add new app
                appCount++;
            }
        }
    }

    fclose(logFile);

    // Write the usage data to the report file
    for (int i = 0; i < appCount; i++) {
        fprintf(reportFile, "%-30s %d\n", apps[i], usageTime[i]);
    }

    fclose(reportFile);

    // Notify user that the report is ready
    char command[256];
    snprintf(command, sizeof(command), 
             "notify-send 'Detailed Report' 'The detailed report is ready. Click to open.' "
             "&& xdg-open 'Usage Report.txt' &");
    system(command);
}

int main() {
    track_application_usage();
    generate_report();
    return 0;
}
