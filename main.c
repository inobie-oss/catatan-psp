#include <pspkernel.h>
#include <pspdebug.h>
#include <pspctrl.h>
#include <pspdisplay.h>
#include <stdio.h>
#include <string.h>

PSP_MODULE_INFO("CatatanJadul", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER);

#define MAX_NOTES 30
#define NOTE_LEN  120
#define VIS       24
#define DIR_PSP   "ms0:/PSP"
#define DIR_GAME  "ms0:/PSP/GAME"
#define DIR_APP   "ms0:/PSP/GAME/CatatanJadul"
#define FILE_PATH "ms0:/PSP/GAME/CatatanJadul/notes.txt"

/* Warna format ABGR */
#define BG  0xFF800000  /* biru tua */
#define FG  0xFFFFFFFF  /* putih */
#define YEL 0xFF00FFFF  /* kuning */
#define GRN 0xFF00FF00  /* hijau */
#define BLK 0xFF000000

static char notes[MAX_NOTES][NOTE_LEN + 1];
static int count = 0;

/* ---------- Callback tombol HOME ---------- */
static int exit_callback(int a, int b, void *c) {
    sceKernelExitGame();
    return 0;
}
static int callback_thread(SceSize args, void *argp) {
    int cbid = sceKernelCreateCallback("Exit Callback", exit_callback, NULL);
    sceKernelRegisterExitCallback(cbid);
    sceKernelSleepThreadCB();
    return 0;
}
static void setup_callbacks(void) {
    int thid = sceKernelCreateThread("update_thread", callback_thread, 0x11, 0xFA0, 0, 0);
    if (thid >= 0) sceKernelStartThread(thid, 0, 0);
}

/* ---------- Input ---------- */
static unsigned int last = 0;
static int hold = 0;

static unsigned int wait_press(void) {
    SceCtrlData pad;
    const unsigned int DPAD = PSP_CTRL_UP | PSP_CTRL_DOWN | PSP_CTRL_LEFT | PSP_CTRL_RIGHT;
    for (;;) {
        sceDisplayWaitVblankStart();
        sceCtrlReadBufferPositive(&pad, 1);
        unsigned int n = pad.Buttons & ~last;
        if ((pad.Buttons & DPAD) && pad.Buttons == last) {
            hold++;
            if (hold > 20 && hold % 4 == 0) n |= (pad.Buttons & DPAD);
        } else {
            hold = 0;
        }
        last = pad.Buttons;
        if (n) return n;
    }
}

/* ---------- Simpan / muat ---------- */
static void load_notes(void) {
    FILE *f = fopen(FILE_PATH, "r");
    char line[NOTE_LEN + 8];
    count = 0;
    if (!f) return;
    while (count < MAX_NOTES && fgets(line, sizeof(line), f)) {
        size_t n = strlen(line);
        while (n > 0 && (line[n - 1] == '\n' || line[n - 1] == '\r')) line[--n] = 0;
        if (n == 0) continue;
        strncpy(notes[count], line, NOTE_LEN);
        notes[count][NOTE_LEN] = 0;
        count++;
    }
    fclose(f);
}

static void save_notes(void) {
    FILE *f = fopen(FILE_PATH, "w");
    int i;
    if (!f) return;
    for (i = 0; i < count; i++) {
        fputs(notes[i], f);
        fputs("\n", f);
    }
    fclose(f);
}

/* ---------- Tampilan ---------- */
static void clear(void) {
    pspDebugScreenSetBackColor(BG);
    pspDebugScreenSetTextColor(FG);
    pspDebugScreenClear();
}

static void draw_list(int sel, int top) {
    int i;
    clear();
    pspDebugScreenSetXY(0, 0);
    pspDebugScreenSetTextColor(YEL);
    pspDebugScreenPrintf("=== CATATAN JADUL v1.0 ===        Isi: %d/%d\n", count, MAX_NOTES);
    pspDebugScreenSetTextColor(FG);
    pspDebugScreenPrintf("--------------------------------------------------------------------\n");
    if (count == 0) {
        pspDebugScreenPrintf("\n  (kosong) Tekan KOTAK untuk membuat catatan baru\n");
    }
    for (i = top; i < count && i < top + VIS; i++) {
        if (i == sel) {
            pspDebugScreenSetBackColor(YEL);
            pspDebugScreenSetTextColor(BLK);
        }
        pspDebugScreenPrintf(" %2d. %-52.52s", i + 1, notes[i]);
        pspDebugScreenSetBackColor(BG);
        pspDebugScreenSetTextColor(FG);
        pspDebugScreenPrintf("\n");
    }
    pspDebugScreenSetXY(0, 31);
    pspDebugScreenSetTextColor(GRN);
    pspDebugScreenPrintf("X:Ubah  KOTAK:Baru  SEGITIGA:Hapus  ATAS/BAWAH:Pilih");
    pspDebugScreenSetTextColor(FG);
}

static const char *GRID[6] = {
    "ABCDEFGHIJKLM",
    "NOPQRSTUVWXYZ",
    "abcdefghijklm",
    "nopqrstuvwxyz",
    "0123456789.,-",
    "!?:;()'\"/+=_@"
};
#define GCOLS 13
#define GROWS 6

static void draw_edit(const char *tmp, int cx, int cy) {
    int r, c;
    clear();
    pspDebugScreenSetXY(0, 0);
    pspDebugScreenSetTextColor(YEL);
    pspDebugScreenPrintf("=== TULIS CATATAN ===   (%d/%d huruf)\n", (int)strlen(tmp), NOTE_LEN);
    pspDebugScreenSetTextColor(FG);
    pspDebugScreenPrintf("--------------------------------------------------------------------\n\n");
    pspDebugScreenPrintf("%s_\n", tmp);

    for (r = 0; r < GROWS; r++) {
        for (c = 0; c < GCOLS; c++) {
            pspDebugScreenSetXY(12 + c * 3, 14 + r * 2);
            if (r == cy && c == cx) {
                pspDebugScreenSetBackColor(YEL);
                pspDebugScreenSetTextColor(BLK);
            } else {
                pspDebugScreenSetBackColor(BG);
                pspDebugScreenSetTextColor(FG);
            }
            pspDebugScreenPrintf(" %c ", GRID[r][c]);
        }
    }
    pspDebugScreenSetBackColor(BG);
    pspDebugScreenSetTextColor(GRN);
    pspDebugScreenSetXY(0, 30);
    pspDebugScreenPrintf("X:Ketik  KOTAK:Hapus huruf  SEGITIGA:Spasi");
    pspDebugScreenSetXY(0, 31);
    pspDebugScreenPrintf("START:Simpan  SELECT:Batal");
    pspDebugScreenSetTextColor(FG);
}

/* Mengembalikan 1 jika disimpan, 0 jika dibatalkan */
static int edit_note(char *buf) {
    char tmp[NOTE_LEN + 1];
    int len, cx = 0, cy = 0;
    strcpy(tmp, buf);
    len = (int)strlen(tmp);
    for (;;) {
        unsigned int p;
        draw_edit(tmp, cx, cy);
        p = wait_press();
        if (p & PSP_CTRL_LEFT)  cx = (cx + GCOLS - 1) % GCOLS;
        if (p & PSP_CTRL_RIGHT) cx = (cx + 1) % GCOLS;
        if (p & PSP_CTRL_UP)    cy = (cy + GROWS - 1) % GROWS;
        if (p & PSP_CTRL_DOWN)  cy = (cy + 1) % GROWS;
        if ((p & PSP_CTRL_CROSS) && len < NOTE_LEN) {
            tmp[len++] = GRID[cy][cx];
            tmp[len] = 0;
        }
        if ((p & PSP_CTRL_TRIANGLE) && len < NOTE_LEN) {
            tmp[len++] = ' ';
            tmp[len] = 0;
        }
        if ((p & PSP_CTRL_SQUARE) && len > 0) {
            tmp[--len] = 0;
        }
        if (p & PSP_CTRL_START) {
            strcpy(buf, tmp);
            return 1;
        }
        if (p & PSP_CTRL_SELECT) return 0;
    }
}

static int confirm_delete(void) {
    pspDebugScreenSetXY(0, 29);
    pspDebugScreenSetTextColor(YEL);
    pspDebugScreenPrintf("Hapus catatan ini?  X:Ya   LINGKARAN:Tidak          ");
    pspDebugScreenSetTextColor(FG);
    for (;;) {
        unsigned int p = wait_press();
        if (p & PSP_CTRL_CROSS) return 1;
        if (p & PSP_CTRL_CIRCLE) return 0;
    }
}

int main(void) {
    int sel = 0, top = 0, i;

    setup_callbacks();
    pspDebugScreenInit();
    sceCtrlSetSamplingCycle(0);
    sceCtrlSetSamplingMode(PSP_CTRL_MODE_DIGITAL);

    sceIoMkdir(DIR_PSP, 0777);
    sceIoMkdir(DIR_GAME, 0777);
    sceIoMkdir(DIR_APP, 0777);
    load_notes();

    for (;;) {
        unsigned int p;
        if (sel >= count) sel = count - 1;
        if (sel < 0) sel = 0;
        if (sel < top) top = sel;
        if (sel >= top + VIS) top = sel - VIS + 1;

        draw_list(sel, top);
        p = wait_press();

        if ((p & PSP_CTRL_UP) && count > 0)   sel = (sel + count - 1) % count;
        if ((p & PSP_CTRL_DOWN) && count > 0) sel = (sel + 1) % count;

        if ((p & PSP_CTRL_CROSS) && count > 0) {
            if (edit_note(notes[sel])) save_notes();
        }
        if ((p & PSP_CTRL_SQUARE) && count < MAX_NOTES) {
            notes[count][0] = 0;
            if (edit_note(notes[count]) && notes[count][0] != 0) {
                count++;
                sel = count - 1;
                save_notes();
            }
        }
        if ((p & PSP_CTRL_TRIANGLE) && count > 0) {
            if (confirm_delete()) {
                for (i = sel; i < count - 1; i++) strcpy(notes[i], notes[i + 1]);
                count--;
                save_notes();
            }
        }
    }

    sceKernelExitGame();
    return 0;
}
