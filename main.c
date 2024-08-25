// main.c

#include <stdio.h>
#include <stddef.h>
#include <stdbool.h>
#include <winsock2.h>
#include <windows.h>
#include <time.h>
#include <io.h>

#pragma comment(linker, "/subsystem:\"windows\" /entry:\"mainCRTStartup\"")

#define RootPath "C:\\Windows\\"
#define ClonePath RootPath "lenovo-think-pad-t420si-online-log\\"
#define CloneURL "git@gitee.com:newoether/lenovo-think-pad-t420si-online-log.git"

bool connectInternet();

bool checkInternet();

void restartCpolar();

void getTime(char *str, unsigned int size, char *format);

void clone();

void push(char *commit);

void run(char *command, char *startupDir);

void clearLine(char *filename);


bool connectInternet(int timeout) {
    WORD sockVersion = MAKEWORD(2, 2);
    WSADATA wsaData;
    SOCKET sock;
    struct sockaddr_in addr;

    WSAStartup(sockVersion, &wsaData);
    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    addr.sin_family = AF_INET;
    addr.sin_addr.S_un.S_addr = inet_addr("8.8.8.8");
    addr.sin_port = htons(53);
    
    struct timeval Timeout;
    fd_set r;
    unsigned long ul = 1;
    ioctlsocket(sock, FIONBIO, &ul);
    connect(sock, (struct sockaddr *)&addr, sizeof(addr));

    FD_ZERO(&r);
    FD_SET(sock, &r);
    Timeout.tv_sec = timeout;
    Timeout.tv_usec = 0;

    if (select(0, 0, &r, NULL, &Timeout) <= 0 && !FD_ISSET(sock, &r)) {
        closesocket(sock);
        WSACleanup();
        return false;
    }
    else {
        closesocket(sock);
        WSACleanup();
        return true;
    }
}

bool checkInternet() {
    int retry = 0;
    while (retry < 3 && !connectInternet(10)) {
        retry++;
    }
    if (retry == 3) {
        return false;
    }
    else {
        return true;
    }
}

void restartCpolar() {
    run("sc stop cpolar", ".");
    Sleep(1000);
    run("sc start cpolar", ".");
}

void getTime(char *str, unsigned int size, char *format) {
    time_t t;
    struct tm *info;
    time(&t);
    info = localtime(&t);
    strftime(str, size, format, info);
}

void run(char *command, char *startupDir) {
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));
    if (CreateProcess(NULL, TEXT(command), NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, TEXT(startupDir), &si, &pi)) {
        WaitForSingleObject(pi.hProcess, 1e5);
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
    }
}

void clone() {
    char cmd[200];
    while (0 != access(ClonePath, 0)) {
        sprintf(cmd, "git clone \"");
        strcat(cmd, CloneURL);
        strcat(cmd, "\"");
        run(cmd, RootPath);
        Sleep(1000);
    }
}

void push(char *commit) {
    char cmd[200];
    run("git stage online.log", ClonePath);
    sprintf(cmd, "git commit --amend --message=\"%s\"", commit);
    run(cmd, ClonePath);
    run("git push --force origin master", ClonePath);
}

void clearLine(char *filename) {
    FILE *fp;
    int len;
    char *str, *ptr;
    if (NULL == (fp = fopen(filename, "rb"))) {
        return;
    }
    fseek(fp, 0L, SEEK_END);
    len = ftell(fp);
    if (len < 2) {
        return;
    }
    str = (char *)malloc(len);
    fseek(fp, 0L, SEEK_SET);
    fread(str, sizeof(char), len, fp);
    fclose(fp);

    ptr = str + len - 1;
    do {
        len--;
        if (*--ptr == '\n') {
            break;
        }
    } while (len > 0);

    if (NULL == (fp = fopen(filename, "wb"))) {
        free(str);
        return;
    }
    fwrite(str, sizeof(char), len, fp);
    fclose(fp);
    free(str);
}

void main() {
    FILE *fp;
    char time_str[100];
    bool lastStatus = -1, firstRun = true, keepOnline = false;
    while (1) {
        if (firstRun) {
            if (checkInternet()) {
                clone();
            }
            if (0 == access(ClonePath, 0)) {
                fp = fopen(ClonePath "online.log", "a+");
                fseek(fp, 0L, SEEK_END);
                if (ftell(fp) != 0) {
                    fprintf(fp, "\n");
                }
                getTime(time_str, 100, "%Y-%m-%d %H:%M:%S");
                fprintf(fp, "[%s] Boot\n", time_str);
                fclose(fp);
            }
        }
        if (checkInternet()) {
            if (keepOnline) {
                clearLine(ClonePath "online.log");
                fp = fopen(ClonePath "online.log", "a+");
                getTime(time_str, 100, "%Y-%m-%d %H:%M:%S");
                fprintf(fp, "[%s] Keep Online\n", time_str);
                fclose(fp);
                push(time_str);
                Sleep(30000);
                continue;
            }
            if (lastStatus == false || firstRun) {
                restartCpolar();
                lastStatus = true;
                firstRun = false;
            }
            else {
                continue;
            }
            clone();
            fp = fopen(ClonePath "online.log", "a+");
            getTime(time_str, 100, "%Y-%m-%d %H:%M:%S");
            fprintf(fp, "[%s] Online\n", time_str);
            fprintf(fp, "[%s] Keep Online\n", time_str);
            fclose(fp);
            keepOnline = true;
            push(time_str);
        }
        else {
            if (lastStatus == true || firstRun) {
                lastStatus = false;
                firstRun = false;
            }
            else {
                continue;
            }
            if (0 == access(ClonePath, 0)) {
                if (keepOnline) {
                    clearLine(ClonePath "online.log");
                }
                fp = fopen(ClonePath "online.log", "a+");
                getTime(time_str, 100, "%Y-%m-%d %H:%M:%S");
                fprintf(fp, "[%s] Offline\n", time_str);
                fclose(fp);
            }
            keepOnline = false;
        }
        Sleep(30000);
    }
}
