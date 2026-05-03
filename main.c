/*
 * simple file management project
 * CS149 - Project
 * Author: Bryant Ni
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <time.h>

// constants
#define MAX_PATH   512
#define MAX_CMD    256
#define MAX_BUF   4096
#define FS_ROOT   "./fs_root"   //virtual root dir

// Color codes using ANSI
#define RESET   "\033[0m"
#define GREEN   "\033[1;32m"
#define CYAN    "\033[1;36m"
#define YELLOW  "\033[1;33m"
#define RED     "\033[1;31m"
#define BOLD    "\033[1m"

// cross platform for Mkdir
#ifdef _WIN32
  #include <direct.h>
  #define make_dir(path) _mkdir(path)
#else
  #define make_dir(path) mkdir(path, 0755)
#endif

void cmd_create(const char *filename);
void cmd_open  (const char *filename);
void cmd_close (const char *filename);
void cmd_write (const char *filename, const char *content);
void cmd_read  (const char *filename);
void cmd_search(const char *filename);
void cmd_list  (void);
void cmd_delete(const char *filename);
void cmd_info  (const char *filename);
void cmd_help  (void);
void print_banner(void);
void build_path(char *out, const char *filename);

//helper
void build_path(char *out, const char *filename)
{
    snprintf(out, MAX_PATH, "%s/%s", FS_ROOT, filename);
}

void print_banner(void)
{
    printf(CYAN);
    printf("------------------------------------------------\n");
    printf("|      Simple File Management System (FMS)     |\n");
    printf("|           CS149  -  Bryant Ni                |\n");
    printf("------------------------------------------------\n");
    printf(RESET);
}

/* commands */

// create empty file
void cmd_create(const char *filename)
{
    char path[MAX_PATH];
    build_path(path, filename);

    int fd = open(path, O_CREAT | O_EXCL | O_WRONLY, 0644);
    if (fd == -1)
    {
        if (errno == EEXIST)
            printf(YELLOW "  [WARNING] File '%s' already exists.\n" RESET, filename);
        else
            printf(RED "  [ERROR]  create: %s\n" RESET, strerror(errno));
        return;
    }

    close(fd);
    printf(GREEN "  [OK]   File '%s' created.\n" RESET, filename);
}

//open file with verification of file existance and openable
void cmd_open(const char *filename)
{
    char path[MAX_PATH];
    build_path(path, filename);

    int fd = open(path, O_RDONLY);

    if (fd == -1)
    {
        printf(RED "  [ERROR]  open '%s': %s\n" RESET, filename, strerror(errno));
        return;
    }

    printf(GREEN "  [OK]   File '%s' opened (fd=%d). Closing now.\n" RESET, filename, fd);
    close(fd);
}

//close, simulate an explicit close
void cmd_close(const char *filename)
{
    // as demo, file is opened and closed to show lifetime
    char path[MAX_PATH];
    build_path(path, filename);

    int fd = open(path, O_RDONLY);

    if (fd == -1)
    {
        printf(RED "  [ERROR]  close '%s': %s\n" RESET, filename, strerror(errno));
        return;
    }
    if (close(fd) == 0)
        printf(GREEN "  [OK]   File '%s' closed (fd=%d released).\n" RESET, filename, fd);
    else
        printf(RED "  [ERROR]  close fd=%d: %s\n" RESET, fd, strerror(errno));
}

//Write/append content to a file
void cmd_write(const char *filename, const char *content)
{
    char path[MAX_PATH];
    build_path(path, filename);

    int fd = open(path, O_WRONLY | O_APPEND);

    if (fd == -1)
    {
        printf(RED "  [ERROR]  write open '%s': %s\n" RESET, filename, strerror(errno));
        return;
    }

    ssize_t len   = (ssize_t)strlen(content);
    ssize_t wrote = write(fd, content, (size_t)len);
    write(fd, "\n", 1);
    close(fd);

    if (wrote == len)
        printf(GREEN "  [OK]   Wrote %zd bytes to '%s'.\n" RESET, wrote, filename);
    else
        printf(RED "  [ERROR]  Partial write (%zd/%zd bytes).\n" RESET, wrote, len);
}

// read/display file content
void cmd_read(const char *filename)
{
    char path[MAX_PATH];
    build_path(path, filename);

    int fd = open(path, O_RDONLY);

    if (fd == -1)
    {
        printf(RED "  [ERROR]  read open '%s': %s\n" RESET, filename, strerror(errno));
        return;
    }

    char buf[MAX_BUF];
    ssize_t n;
    printf(BOLD "  --- Contents of '%s' -----------------------\n" RESET, filename);

    while ((n = read(fd, buf, sizeof(buf) - 1)) > 0)
    {
        buf[n] = '\0';
        printf("%s", buf);
    }

    printf(BOLD "\n  ------------------------------------------\n" RESET);
    close(fd);
}

// search, scans FS_ROOT for a filename
void cmd_search(const char *filename)
{
    DIR *dir = opendir(FS_ROOT);

    if (!dir)
    {
        printf(RED "  [ERROR]  Cannot open filesystem root: %s\n" RESET, strerror(errno));
        return;
    }

    struct dirent *entry;
    int found = 0;
    printf("  Searching for '%s' in %s ...\n", filename, FS_ROOT);

    while ((entry = readdir(dir)) != NULL)
    {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        if (strcmp(entry->d_name, filename) == 0)
        {
            char path[MAX_PATH];
            build_path(path, entry->d_name);
            printf(GREEN "  [FOUND] %s\n" RESET, path);
            found = 1;
        }
    }

    closedir(dir);

    if (!found)
        printf(YELLOW "  [NOT FOUND] '%s' does not exist in the filesystem.\n" RESET, filename);
}

// list func, show all files in FS_ROOT with size and modification time stamp
void cmd_list(void)
{
    DIR *dir = opendir(FS_ROOT);

    if (!dir)
    {
        printf(RED "  [ERROR]  Cannot open filesystem root: %s\n" RESET, strerror(errno));
        return;
    }

    struct dirent *entry;
    printf(BOLD "  %-30s  %10s  %s\n" RESET, "Filename", "Size(B)", "Last Modified");
    printf("  %s\n", "---------------------------------------------------------");

    int count = 0;
    while ((entry = readdir(dir)) != NULL)
    {
        if (entry->d_name[0] == '.') continue;

        char path[MAX_PATH];
        build_path(path, entry->d_name);

        struct stat st;

        if (stat(path, &st) == 0)
        {
            char timebuf[64];
            struct tm *tm_info = localtime(&st.st_mtime);
            strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S", tm_info);

            printf("  %-30s  %10lld  %s\n",
                   entry->d_name,
                   (long long)st.st_size,
                   timebuf);
            count++;
        }
    }
    closedir(dir);

    if (count == 0)
        printf(YELLOW "  (no files in filesystem)\n" RESET);

    else
        printf("  Total: %d file(s)\n", count);
}

// delete, removes file
void cmd_delete(const char *filename)
{
    char path[MAX_PATH];
    build_path(path, filename);

    if (unlink(path) == 0)
        printf(GREEN "  [OK]   File '%s' deleted.\n" RESET, filename);

    else
        printf(RED "  [ERROR]  delete '%s': %s\n" RESET, filename, strerror(errno));
}

// info, displays file data
void cmd_info(const char *filename)
{
    char path[MAX_PATH];
    build_path(path, filename);

    struct stat st;
    if (stat(path, &st) == -1)
    {
        printf(RED "  [ERROR]  info '%s': %s\n" RESET, filename, strerror(errno));
        return;
    }

    char timebuf[64];
    struct tm *tm_info = localtime(&st.st_mtime);
    strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S", tm_info);

    printf(BOLD "  File Information: %s\n" RESET, filename);
    printf("  %-20s %lld bytes\n",  "Size:",         (long long)st.st_size);
    printf("  %-20s %o\n",          "Permissions:",  st.st_mode & 0777);
    printf("  %-20s %u\n",          "Hard links:",   (unsigned)st.st_nlink);
    printf("  %-20s %u\n",          "UID:",           st.st_uid);
    printf("  %-20s %s\n",          "Last modified:", timebuf);
    printf("  %-20s %lu\n",         "Inode:",         (unsigned long)st.st_ino);
}

//Help command
void cmd_help(void)
{
    printf(BOLD "\n  Available Commands:\n" RESET);
    printf("  %-38s %s\n", "create <filename>",         "Create a new empty file");
    printf("  %-38s %s\n", "open   <filename>",         "Open a file (verify access)");
    printf("  %-38s %s\n", "close  <filename>",         "Open then close a file");
    printf("  %-38s %s\n", "write  <filename> <text>",  "Append text to a file");
    printf("  %-38s %s\n", "read   <filename>",         "Read and display file contents");
    printf("  %-38s %s\n", "search <filename>",         "Search for a file in the filesystem");
    printf("  %-38s %s\n", "list",                      "List all files with metadata");
    printf("  %-38s %s\n", "delete <filename>",         "Delete a file");
    printf("  %-38s %s\n", "info   <filename>",         "Show detailed file metadata");
    printf("  %-38s %s\n", "help",                      "Show this help message");
    printf("  %-38s %s\n", "exit",                      "Exit the program");
    printf("\n");
}

int main(void)
{
    //ensure root exist
    mkdir(FS_ROOT, 0755);

    print_banner();
    printf("  Type 'help' for a list of commands.\n\n");

    char line[MAX_CMD];
    while (1)
    {
        printf(CYAN "fms> " RESET);
        fflush(stdout);

        if (!fgets(line, sizeof(line), stdin))
            break;

        line[strcspn(line, "\n")] = '\0';
        if (strlen(line) == 0) continue;

        //tokens
        char cmd[64]        = {0};
        char arg1[MAX_PATH] = {0};
        char arg2[MAX_BUF]  = {0};

        int parts = sscanf(line, "%63s %511s %4095[^\n]", cmd, arg1, arg2);

        if      (strcmp(cmd, "exit") == 0) { printf("  Bye.\n"); break; }
        else if (strcmp(cmd, "help") == 0)   cmd_help();
        else if (strcmp(cmd, "list") == 0)   cmd_list();
        else if (strcmp(cmd, "create") == 0)
        {
            if (parts < 2) printf(RED "  Usage: create <filename>\n" RESET);
            else cmd_create(arg1);
        }

        else if (strcmp(cmd, "open")   == 0)
        {
            if (parts < 2) printf(RED "  Usage: open <filename>\n" RESET);
            else cmd_open(arg1);
        }
        else if (strcmp(cmd, "close")  == 0)
        {
            if (parts < 2) printf(RED "  Usage: close <filename>\n" RESET);
            else cmd_close(arg1);
        }
        else if (strcmp(cmd, "write")  == 0)
        {
            if (parts < 3) printf(RED "  Usage: write <filename> <text>\n" RESET);
            else cmd_write(arg1, arg2);
        }
        else if (strcmp(cmd, "read")   == 0)
        {
            if (parts < 2) printf(RED "  Usage: read <filename>\n" RESET);
            else cmd_read(arg1);
        }
        else if (strcmp(cmd, "search") == 0)
        {
            if (parts < 2) printf(RED "  Usage: search <filename>\n" RESET);
            else cmd_search(arg1);
        }
        else if (strcmp(cmd, "delete") == 0)
        {
            if (parts < 2) printf(RED "  Usage: delete <filename>\n" RESET);
            else cmd_delete(arg1);
        }
        else if (strcmp(cmd, "info")   == 0)
        {
            if (parts < 2) printf(RED "  Usage: info <filename>\n" RESET);
            else cmd_info(arg1);
        }
        else
        {
            printf(YELLOW "  Unknown command '%s'. Type 'help'.\n" RESET, cmd);
        }
    }
    return 0;
}
