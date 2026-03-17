#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <switch.h>

// Remove non-directory file and create directory
static int mkdir_if_not_exists(const char *path) {
    struct stat st;

    if (stat(path, &st) == 0) {
        if (S_ISDIR(st.st_mode))
            return 0;

        // Exists but not a directory → remove it
        if (unlink(path) != 0) {
            perror("unlink");
            return 1;
        }
        if (mkdir(path, 0755) != 0) {
            perror("mkdir");
            return 1;
        }
        return 0;
    }

    // Does not exist → create it
    if (mkdir(path, 0755) != 0) {
        perror("mkdir");
        return 1;
    }

    return 0;
}

// Ensure parent directory exists
static int ensure_parent_dir(const char *filepath) {
    char tmp[4096];
    char *slash;

    strncpy(tmp, filepath, sizeof(tmp));
    tmp[sizeof(tmp)-1] = '\0';

    slash = strrchr(tmp, '/');
    if (!slash)
        return 0; // no directory component

    *slash = '\0';
    return mkdir_if_not_exists(tmp);
}

int copyfile(const char *src_path, const char *dst_path) {
    if (ensure_parent_dir(dst_path) != 0)
        return 1;

    int src = open(src_path, O_RDONLY);
    if (src < 0) {
        perror("open src");
        return 1;
    }

    int dst = open(dst_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (dst < 0) {
        perror("open dst");
        close(src);
        return 1;
    }

    // Large buffer for high throughput
    char buffer[131072]; // 128 KB
    ssize_t n;

    while ((n = read(src, buffer, sizeof(buffer))) > 0) {
        ssize_t written = 0;
        while (written < n) {
            ssize_t w = write(dst, buffer + written, n - written);
            if (w < 0) {
                perror("write");
                close(src);
                close(dst);
                return 1;
            }
            written += w;
        }
    }

    if (n < 0) {
        perror("read");
        close(src);
        close(dst);
        return 1;
    }

    close(src);
    close(dst);
    return 0;
}

int main(int argc, char **argv)
{
    consoleInit(NULL);

    // Configure our supported input layout: a single player with standard controller styles
    padConfigureInput(1, HidNpadStyleSet_NpadStandard);

    // Initialize the default gamepad (which reads handheld mode inputs as well as the first connected controller)
    PadState pad;
    padInitializeDefault(&pad);

    Result rc = romfsMountFromCurrentProcess("romfsproc");
    if (R_FAILED(rc))
        printf("romfsMountFromCurrentProcess: %08X\n", rc);
    else
    {
        u64 programId = 0;
        Result rc2 = svcGetInfo(&programId, InfoType_ProgramId, CUR_PROCESS_HANDLE, 0);
        if (R_SUCCEEDED(rc2))
        {
            switch (programId)
            {
            case 0x010038E016264000:  //BNLC VOL 1
                printf("Following ROM images were extracted to sdmc:/trill_roms/ :\n\n");
                if (copyfile("romfsproc:/exe1/rom.srl", "sdmc:/trill_roms/exe1.gba") == 0)
                {
                    printf("Battle Network - Rockman EXE (Japanese) -> exe1.gba\n");
                }
                if (copyfile("romfsproc:/exe1/rom_e.srl", "sdmc:/trill_roms/bn1.gba") == 0)
                {
                    printf("Mega Man Battle Network (English US) -> bn1.gba\n");
                }
                if (copyfile("romfsproc:/exe2j/rom.srl", "sdmc:/trill_roms/exe2.gba") == 0)
                {
                    printf("Battle Network - Rockman EXE 2 (Japanese) -> exe2.gba\n");
                }
                if (copyfile("romfsproc:/exe2j/rom_e.srl", "sdmc:/trill_roms/bn2.gba") == 0)
                {
                    printf("Mega Man Battle Network 2 (English US) -> bn2.gba\n");
                }
                if (copyfile("romfsproc:/exe3/rom.srl", "sdmc:/trill_roms/exe3.gba") == 0)
                {
                    printf("Battle Network - Rockman EXE 3 (Japanese) -> exe3.gba\n");
                }
                if (copyfile("romfsproc:/exe3/rom_e.srl", "sdmc:/trill_roms/bn3w.gba") == 0)
                {
                    printf("Mega Man Battle Network 3 - White Version (English US) -> bn3w.gba\n");
                }
                if (copyfile("romfsproc:/exe3/rom_b.srl", "sdmc:/trill_roms/exe3b.gba") == 0)
                {
                    printf("Battle Network - Rockman EXE 3 (Japanese) -> exe3b.gba\n");
                }
                if (copyfile("romfsproc:/exe3/rom_b_e.srl", "sdmc:/trill_roms/bn3b.gba") == 0)
                {
                    printf("Mega Man Battle Network 3 - White Version (English US) -> bn3b.gba\n");
                }
            case 0x0100734016266000: //BNLC VOL 2
                printf("Following ROM images were extracted to sdmc:/trill_roms/ :\n\n");
                if (copyfile("romfsproc:/exe4/rom.srl", "sdmc:/trill_roms/exe4rs.gba") == 0)
                {
                    printf("Rockman EXE 4 - Tournament Red Sun (Japanese) -> exe4rs.gba\n");
                }
                if (copyfile("romfsproc:/exe4/rom_e.srl", "sdmc:/trill_roms/bn4rs.gba") == 0)
                {
                    printf("Mega Man Battle Network 4 - Red Sun (English US) -> bn4rs.gba\n");
                }
                if (copyfile("romfsproc:/exe4/rom_b.srl", "sdmc:/trill_roms/exe4bm.gba") == 0)
                {
                    printf("Rockman EXE 4 - Tournament Blue Moon (Japanese) -> exe4bm.gba\n");
                }
                if (copyfile("romfsproc:/exe4/rom_b_e.srl", "sdmc:/trill_roms/bn4bm.gba") == 0)
                {
                    printf("Mega Man Battle Network 4 - Blue Moon (English US) -> bn4bm.gba\n");
                }
                if (copyfile("romfsproc:/exe5/rom.srl", "sdmc:/trill_roms/exe5tb.gba") == 0)
                {
                    printf("Rockman EXE 5 - Team of Blues (Japanese) -> exe5tb.gba\n");
                }
                if (copyfile("romfsproc:/exe5/rom_e.srl", "sdmc:/trill_roms/bn5tp.gba") == 0)
                {
                    printf("Mega Man Battle Network 5 - Team Protoman (English US) -> bn5tp.gba\n");
                }
                if (copyfile("romfsproc:/exe5/rom_k.srl", "sdmc:/trill_roms/exe5tc.gba") == 0)
                {
                    printf("Rockman EXE 5 - Team of Colonel (Japanese) -> exe5tc.gba\n");
                }
                if (copyfile("romfsproc:/exe5/rom_k_e.srl", "sdmc:/trill_roms/bn5tc.gba") == 0)
                {
                    printf("Mega Man Battle Network 5 - Team Colonel (English US) -> bn5tc.gba\n");
                }
                if (copyfile("romfsproc:/exe6/rom.srl", "sdmc:/trill_roms/exe6g.gba") == 0)
                {
                    printf("Rockman EXE 6 - Dennoujuu Greiga (Japanese) -> exe6g.gba\n");
                }
                if (copyfile("romfsproc:/exe6/rom_e.srl", "sdmc:/trill_roms/bn6g.gba") == 0)
                {
                    printf("Mega Man Battle Network 6 - Cybeast Gregar (English US) -> bn6g.gba\n");
                }
                if (copyfile("romfsproc:/exe6/rom_f.srl", "sdmc:/trill_roms/exe6f.gba") == 0)
                {
                    printf("Rockman EXE 6 - Dennoujuu Faltzer (Japanese) -> exe6f.gba\n");
                }
                if (copyfile("romfsproc:/exe6/rom_f_e.srl", "sdmc:/trill_roms/bn6f.gba") == 0)
                {
                    printf("Mega Man Battle Network 6 - Cybeast Falzar (English US) -> bn6f.gba\n");
                }
                printf("\nTo extract GBA ROM images between EXE1/BN1 - EXE3/BN3, run this homebrew by\ndoing title takeover from Volume 2.\n"); break;
            default:
                printf("Please run this homebrew by doing title takeover from either of these 2 games:\n");
                printf("Mega Man Battle Network Legacy Collection Volume 1\n");
                printf("Mega Man Battle Network Legacy Collection Volume 2\n");
                printf("\nTo perform title takeover, run either games while holding [R] by default,\n");
                printf("then release when you see Homebrew Menu without seeing Applet Mode message.\n");
                //printf("Mega Man Star Force Legacy Collection Volume 2\n");
                break;
            }
        }
        printf("\nPress [+] to exit.\n");
    }

    // Main loop
    while (appletMainLoop())
    {
        // Scan the gamepad. This should be done once for each frame
        padUpdate(&pad);

        // padGetButtonsDown returns the set of buttons that have been newly pressed in this frame compared to the previous one
        u64 kDown = padGetButtonsDown(&pad);

        if (kDown & HidNpadButton_Plus)
            break; // break in order to return to hbmenu

        consoleUpdate(NULL);
    }

    romfsExit();
    consoleExit(NULL);
    return 0;
}
