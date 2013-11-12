/*
 * Toxic -- Tox Curses Client
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>

#include "toxic_windows.h"
#include "misc_tools.h"

extern uint8_t pending_grp_requests[MAX_FRIENDS_NUM][TOX_CLIENT_ID_SIZE];

void cmd_chat_help(WINDOW *window, ToxWindow *prompt, Tox *m, int num, int argc, char (*argv)[MAX_STR_SIZE])
{
    wattron(window, COLOR_PAIR(CYAN) | A_BOLD);
    wprintw(window, "Chat commands:\n");
    wattroff(window, A_BOLD);

    wprintw(window, "      /status <type> <message>   : Set your status with optional note\n");
    wprintw(window, "      /note <message>            : Set a personal note\n");
    wprintw(window, "      /nick <nickname>           : Set your nickname\n");
    wprintw(window, "      /invite <n>                : Invite friend to a groupchat\n");
    wprintw(window, "      /me <action>               : Do an action\n");
    wprintw(window, "      /myid                      : Print your ID\n");
    wprintw(window, "      /join <n>                  : Join a group chat\n");
    wprintw(window, "      /clear                     : Clear the screen\n");
    wprintw(window, "      /close                     : Close the current chat window\n");
    wprintw(window, "      /sendfile <filepath>       : Send a file\n");
    wprintw(window, "      /savefile <n>              : Receive a file\n");
    wprintw(window, "      /quit or /exit             : Exit Toxic\n");
    wprintw(window, "      /help                      : Print this message again\n");
    
    wattron(window, A_BOLD);
    wprintw(window, "\n * Argument messages must be enclosed in quotation marks.\n");
    wattroff(window, A_BOLD);
    
    wattroff(window, COLOR_PAIR(CYAN));
}

void cmd_groupinvite(WINDOW *window, ToxWindow *prompt, Tox *m, int num, int argc, char (*argv)[MAX_STR_SIZE])
{
    if (argc < 1) {
      wprintw(window, "Invalid syntax.\n");
      return;
    }

    int groupnum = atoi(argv[1]);

    if (groupnum == 0 && strcmp(argv[1], "0")) {    /* atoi returns 0 value on invalid input */
        wprintw(window, "Invalid syntax.\n");
        return;
    }

    if (tox_invite_friend(m, num, groupnum) == -1) {
        wprintw(window, "Failed to invite friend.\n");
        return;
    }

    wprintw(window, "Invited friend to group chat %d.\n", groupnum);
}

void cmd_join_group(WINDOW *window, ToxWindow *prompt, Tox *m, int num, int argc, char (*argv)[MAX_STR_SIZE])
{
    if (argc != 1) {
      wprintw(window, "Invalid syntax.\n");
      return;
    }

    int g_num = atoi(argv[1]);

    if ((g_num == 0 && strcmp(argv[1], "0")) || g_num >= MAX_FRIENDS_NUM) {
        wprintw(window, "No pending group chat invite with that number.\n");
        return;
    }

    uint8_t *groupkey = pending_grp_requests[g_num];

    if (!strlen(groupkey)) {
        wprintw(window, "No pending group chat invite with that number.\n");
        return;
    }

    int groupnum = tox_join_groupchat(m, g_num, groupkey);

    if (groupnum == -1) {
        wprintw(window, "Group chat instance failed to initialize.\n");
        return;
    }

    if (init_groupchat_win(prompt, m, groupnum) == -1) {
        wprintw(window, "Group chat window failed to initialize.\n");
        tox_del_groupchat(m, groupnum);
        return;
    }
}

void cmd_savefile(WINDOW *window, ToxWindow *prompt, Tox *m, int num, int argc, char (*argv)[MAX_STR_SIZE])
{
    if (argc != 1) {
      wprintw(window, "Invalid syntax.\n");
      return;
    }

    uint8_t filenum = atoi(argv[1]);

    if ((filenum == 0 && strcmp(argv[1], "0")) || filenum >= MAX_FILES) {
        wprintw(window, "No pending file transfers with that number.\n");
        return;
    }

    if (!friends[num].file_receiver.pending[filenum]) {
        wprintw(window, "No pending file transfers with that number.\n");
        return;
    }

    uint8_t *filename = friends[num].file_receiver.filenames[filenum];

    if (tox_file_sendcontrol(m, num, 1, filenum, TOX_FILECONTROL_ACCEPT, 0, 0))
        wprintw(window, "Accepted file transfer %u. Saving file as: '%s'\n", filenum, filename);
    else
        wprintw(window, "File transfer failed.\n");

    friends[num].file_receiver.pending[filenum] = false;
}

void cmd_sendfile(WINDOW *window, ToxWindow *prompt, Tox *m, int num, int argc, char (*argv)[MAX_STR_SIZE])
{
    if (num_file_senders >= MAX_FILES) {
        wprintw(window,"Please wait for some of your outgoing file transfers to complete.\n");
        return;
    }

    if (argc < 1) {
      wprintw(window, "Invalid syntax.\n");
      return;
    }

    uint8_t *path = argv[1];

    if (path[0] != '\"') {
        wprintw(window, "File path must be enclosed in quotes.\n");
        return;
    }

    path[strlen(++path)-1] = L'\0';
    int path_len = strlen(path);

    if (path_len > MAX_STR_SIZE) {
        wprintw(window, "File path exceeds character limit.\n");
        return;
    }

    FILE *file_to_send = fopen(path, "r");

    if (file_to_send == NULL) {
        wprintw(window, "File '%s' not found.\n", path);
        return;
    }

    fseek(file_to_send, 0, SEEK_END);
    uint64_t filesize = ftell(file_to_send);
    fseek(file_to_send, 0, SEEK_SET);

    int filenum = tox_new_filesender(m, num, filesize, path, path_len + 1);

    if (filenum == -1) {
        wprintw(window, "Error sending file.\n");
        return;
    }

    int i;

    for (i = 0; i < MAX_FILES; ++i) {
        if (!file_senders[i].active) {
            memcpy(file_senders[i].pathname, path, path_len + 1);
            file_senders[i].active = true;
            file_senders[i].chatwin = window;
            file_senders[i].file = file_to_send;
            file_senders[i].filenum = (uint8_t) filenum;
            file_senders[i].friendnum = num;
            file_senders[i].piecelen = fread(file_senders[i].nextpiece, 1,
                                             tox_filedata_size(m, num), file_to_send);

            wprintw(window, "Sending file: '%s'\n", path);

            if (i == num_file_senders)
                ++num_file_senders;

            return;
        }
    } 
}
