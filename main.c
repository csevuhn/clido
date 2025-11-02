#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <pwd.h>
#include <ncurses.h>

#define MAX_TASKS 1024
#define MAX_LENGTH 256
#define SAVE_FILE "~/.clido"

typedef struct {
    char text[MAX_LENGTH];
    bool done;
} Task;

Task *tasks = NULL;
int taskCount = 0;
int capacity = 0;
int selected = 0;
int scrollOffset = 0;
WINDOW *mainWin = NULL;
int winHeight = 0;
int winWidth = 0;
int screenHeight = 0;
int screenWidth = 0;

void ensureCapacity(int required) {
    if (required > capacity) {
        int newCapacity = capacity ? capacity * 2 : 64;
        while (newCapacity < required) newCapacity *= 2;
        tasks = realloc(tasks, newCapacity * sizeof(Task));
        if (!tasks) {
            endwin();
            fprintf(stderr, "[err] realloc failed\n");
            exit(EXIT_FAILURE);
        }
        capacity = newCapacity;
    }
}

void addTask(const char *text) {
    ensureCapacity(taskCount + 1);
    strncpy(tasks[taskCount].text, text, MAX_LENGTH - 1);
    tasks[taskCount].text[MAX_LENGTH - 1] = '\0';
    tasks[taskCount].done = false;
    taskCount++;
}

void deleteTask(int idx) {
    if (idx < 0 || idx >= taskCount) return;
    memmove(&tasks[idx], &tasks[idx + 1], (taskCount - idx - 1) * sizeof(Task));
    taskCount--;
    if (selected >= taskCount) selected = taskCount - 1;
    if (selected < 0) selected = 0;
}

void toggleTask(int idx) {
    if (idx >= 0 && idx < taskCount) tasks[idx].done = !tasks[idx].done;
}

char *expandPath(const char *path) {
    if (path[0] != '~') return strdup(path);
    const char *home = getenv("HOME");
    if (!home) {
        struct passwd *pw = getpwuid(getuid());
        if (pw) home = pw->pw_dir;
    }
    if (!home) home = ".";
    size_t homeLen = strlen(home);
    size_t tailLen = strlen(path + 1);
    char *expanded = malloc(homeLen + tailLen + 1);
    if (!expanded) return NULL;
    strcpy(expanded, home);
    strcat(expanded, path + 1);
    return expanded;
}

void loadTasks(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) return;
    char line[MAX_LENGTH + 10];
    while (fgets(line, sizeof(line), f)) {
        line[strcspn(line, "\n")] = '\0';
        if (strlen(line) == 0) continue;
        bool done = (line[0] == 'x' && line[1] == ' ');
        const char *txt = done ? line + 2 : line;
        addTask(txt);
        if (done) tasks[taskCount - 1].done = true;
    }
    fclose(f);
}

void saveTasks(const char *path) {
    FILE *f = fopen(path, "w");
    if (!f) {
        perror("saveTasks");
        return;
    }
    for (int i = 0; i < taskCount; ++i) {
        fprintf(f, "%s %s\n", tasks[i].done ? "x" : " ", tasks[i].text);
    }
    fclose(f);
}

void refreshWindow(void) {
    if (mainWin) delwin(mainWin);
    getmaxyx(stdscr, screenHeight, screenWidth);
    winHeight = screenHeight - 4; if (winHeight < 10) winHeight = 10;
    winWidth = screenWidth - 4; if (winWidth < 40) winWidth = 40;
    if (winHeight > 40) winHeight = 40;
    if (winWidth > 90) winWidth = 90;
    mainWin = newwin(winHeight, winWidth,
                     (screenHeight - winHeight) / 2,
                     (screenWidth - winWidth) / 2);
    keypad(mainWin, TRUE);
    clear();
    refresh();
}

void draw(void) {
    werase(mainWin);
    box(mainWin, 0, 0);
    const char *title = "clido";
    mvwprintw(mainWin, 1, (winWidth - (int)strlen(title)) / 2, "%s", title);
    int visible = winHeight - 4;
    if (visible < 1) visible = 1;
    if (selected < scrollOffset) scrollOffset = selected;
    if (selected >= scrollOffset + visible) scrollOffset = selected - visible + 1;
    for (int i = 0; i < visible && (i + scrollOffset) < taskCount; ++i) {
        int idx = i + scrollOffset;
        const char *mark = tasks[idx].done ? "[x]" : "[ ]";
        if (idx == selected) wattron(mainWin, A_REVERSE);
        mvwprintw(mainWin, i + 3, 2, "%s %s", mark, tasks[idx].text);
        if (idx == selected) wattroff(mainWin, A_REVERSE);
    }
    wrefresh(mainWin);
}

void promptAddTask(void) {
    echo();
    curs_set(1);
    int inputH = 3, inputW = winWidth - 8;
    if (inputW < 30) inputW = 30;
    WINDOW *input = newwin(inputH, inputW,
                           winHeight / 2 - 1,
                           (winWidth - inputW) / 2);
    box(input, 0, 0);
    mvwprintw(input, 1, 1, "new task: ");
    wrefresh(input);
    char buffer[MAX_LENGTH] = {0};
    wgetnstr(input, buffer, MAX_LENGTH - 1);
    if (strlen(buffer) > 0) {
        addTask(buffer);
        selected = taskCount - 1;
    }
    delwin(input);
    noecho();
    curs_set(0);
    touchwin(stdscr);
    refresh();
}

int main(void) {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
    char *savePath = expandPath(SAVE_FILE);
    if (!savePath) {
        endwin();
        fprintf(stderr, "[err] failed to expand path\n");
        return EXIT_FAILURE;
    }
    loadTasks(savePath);
    refreshWindow();
    draw();
    int ch;
    while ((ch = wgetch(mainWin)) != 'q') {
        switch (ch) {
            case KEY_UP:
            case 'k':
                if (selected > 0) selected--;
                break;
            case KEY_DOWN:
            case 'j':
                if (selected < taskCount - 1) selected++;
                break;
            case KEY_ENTER:
            case 10:
            case 13:
                toggleTask(selected);
                break;
            case 'n':
                promptAddTask();
                break;
            case 'd':
                deleteTask(selected);
                break;
            case KEY_RESIZE:
                refreshWindow();
                break;
        }
        draw();
    }
    saveTasks(savePath);
    free(savePath);
    if (tasks) free(tasks);
    if (mainWin) delwin(mainWin);
    endwin();
    return EXIT_SUCCESS;
}
