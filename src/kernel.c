// =============================================================================
// SimplyOS Core Kernel — "SimplyKrnl"
// =============================================================================

// --- FORWARD DECLARATIONS ---
static inline unsigned char inb(unsigned short port);
static inline unsigned short inw(unsigned short port);
static inline void outb(unsigned short port, unsigned char data);
static inline void outw(unsigned short port, unsigned short data);
void handle_cli_command(const char* cmd);
void print_string(const char* str, unsigned char color);
void clear_screen(void);
void hardware_reboot(void);
void hardware_shutdown(void);
void wait_disk_ready(void);
void read_disk_sector(int lba, char* buffer);
void write_disk_sector(int lba, const char* buffer);
void matrix_effect(void);

// --- HELPER STRINGS ---
int str_length(const char* str) {
    int len = 0;
    while (str[len] != '\0') len++;
    return len;
}

int str_compare_n(const char* s1, const char* s2, int n) {
    for (int i = 0; i < n; i++) {
        if (s1[i] != s2[i]) return 0;
        if (s1[i] == '\0') return 1;
    }
    return 1;
}

// --- GLOBAL STATE ---
volatile char* const text_vram = (volatile char*)0xB8000;
int vram_offset = 0;
unsigned char current_text_color = 0x0F; 

// Keyboard Modifier & Prefix Tracking State
int shift_pressed = 0;                   
volatile int ctrl_pressed = 0;
volatile int alt_pressed = 0;
int extended_key = 0; 

// --- TRANSLATION HELPER ---
char translate_scancode(unsigned char scancode, int shift) {
    if (scancode == 0x02) return shift ? '!' : '1';
    if (scancode == 0x03) return shift ? '@' : '2';
    if (scancode == 0x04) return shift ? '#' : '3';
    if (scancode == 0x05) return shift ? '$' : '4';
    if (scancode == 0x06) return shift ? '%' : '5';
    if (scancode == 0x07) return shift ? '^' : '6';
    if (scancode == 0x08) return shift ? '&' : '7';
    if (scancode == 0x09) return shift ? '*' : '8';
    if (scancode == 0x0A) return shift ? '(' : '9';
    if (scancode == 0x0B) return shift ? ')' : '0';
    
    if (scancode == 0x10) return shift ? 'Q' : 'q';
    if (scancode == 0x11) return shift ? 'W' : 'w';
    if (scancode == 0x12) return shift ? 'E' : 'e';
    if (scancode == 0x13) return shift ? 'R' : 'r';
    if (scancode == 0x14) return shift ? 'T' : 't';
    if (scancode == 0x15) return shift ? 'Y' : 'y';
    if (scancode == 0x16) return shift ? 'U' : 'u';
    if (scancode == 0x17) return shift ? 'I' : 'i';
    if (scancode == 0x18) return shift ? 'O' : 'o';
    if (scancode == 0x19) return shift ? 'P' : 'p';
    if (scancode == 0x1E) return shift ? 'A' : 'a';
    if (scancode == 0x1F) return shift ? 'S' : 's';
    if (scancode == 0x20) return shift ? 'D' : 'd';
    if (scancode == 0x21) return shift ? 'F' : 'f';
    if (scancode == 0x22) return shift ? 'G' : 'g';
    if (scancode == 0x23) return shift ? 'H' : 'h';
    if (scancode == 0x24) return shift ? 'J' : 'j';
    if (scancode == 0x25) return shift ? 'K' : 'k';
    if (scancode == 0x26) return shift ? 'L' : 'l';
    if (scancode == 0x2C) return shift ? 'Z' : 'z';
    if (scancode == 0x2D) return shift ? 'X' : 'x';
    if (scancode == 0x2E) return shift ? 'C' : 'c';
    if (scancode == 0x2F) return shift ? 'V' : 'v';
    if (scancode == 0x30) return shift ? 'B' : 'b';
    if (scancode == 0x31) return shift ? 'N' : 'n';
    if (scancode == 0x32) return shift ? 'M' : 'm';
    
    if (scancode == 0x33) return shift ? '<' : ',';
    if (scancode == 0x34) return shift ? '>' : '.';
    
    if (scancode == 0x39) return ' ';
    return 0; 
}

// --- KERNEL ENTRY POINT ---
void kernel_main(void) {
    clear_screen();
    current_text_color = 0x0F; 

    int timeout = 0;
    int controller_failed = 0;
    while (inb(0x64) & 0x01) { 
        inb(0x60); 
        timeout++;
        if (timeout > 50000) { 
            controller_failed = 1;
            break;
        }
    }

    if (controller_failed) {
        print_string("An error occurred when trying to load the shell. (KEYBOARD_CONTROLLER_TIMEOUT)\n", 0x0C); 
        while(1) { __asm__ volatile("hlt"); } 
    }

    char input_buffer[64];
    for (int i = 0; i < 64; i++) input_buffer[i] = 0;
    int buf_idx = 0;
    
    print_string("SimplyOS CLI [SimplyKrnl Activated]\n", 0x0A);
    print_string("Type 'help' to see all commands.\n\n> ", current_text_color);

    while (1) {
        if (inb(0x64) & 0x01) {
            unsigned char scancode = inb(0x60);

            // Catch Extended Byte Prefixes (e.g., Arrow Keys, Win Key)
            if (scancode == 0xE0) {
                extended_key = 1;
                continue;
            }

            // --- TRACK RELEASES (Break Codes) ---
            if (scancode & 0x80) {
                unsigned char released = scancode & 0x7F;
                if (released == 0x2A || released == 0x36) shift_pressed = 0;
                else if (released == 0x1D) ctrl_pressed = 0;
                else if (released == 0x38) alt_pressed = 0;
                
                extended_key = 0;
            }
            // --- TRACK PRESSES (Make Codes) ---
            else {
                if (scancode == 0x2A || scancode == 0x36) {
                    shift_pressed = 1;
                }
                else if (scancode == 0x1D) {
                    ctrl_pressed = 1;
                }
                else if (scancode == 0x38) {
                    alt_pressed = 1;
                }
                // Intercepting an Extended Key (Example: Windows GUI Trigger Key)
                else if (extended_key && scancode == 0x5B) {
                    print_string("[Win Key Detected]\n", 0x0E);
                }
                // Standard Processing Nodes
                else {
                    if (scancode == 0x1C) { // Enter Key Handshake
                        print_string("\n", current_text_color);
                        input_buffer[buf_idx] = '\0';
                        if (buf_idx > 0) {
                            handle_cli_command(input_buffer);
                        }
                        print_string("> ", current_text_color);
                        buf_idx = 0;
                    } 
                    else if (scancode == 0x0E) { // Backspace Destructor
                        if (buf_idx > 0) {
                            buf_idx--;
                            vram_offset -= 2;
                            text_vram[vram_offset] = ' ';
                            text_vram[vram_offset + 1] = 0x07;
                        }
                    }
                    else {
                        char c = translate_scancode(scancode, shift_pressed);
                        if (c != 0 && buf_idx < 63) {
                            input_buffer[buf_idx++] = c;
                            text_vram[vram_offset] = c;
                            text_vram[vram_offset + 1] = current_text_color;
                            vram_offset += 2;
                        }
                    }
                }
                extended_key = 0; 
            }
            for (volatile int d = 0; d < 20000; d++);
        }
    }
}

// --- TEXT PRINTING UTILITIES ---
void print_string(const char* str, unsigned char color) {
    for (int i = 0; str[i] != '\0'; i++) {
        if (str[i] == '\n') {
            vram_offset = ((vram_offset / 160) + 1) * 160;
        } else {
            text_vram[vram_offset] = str[i];
            text_vram[vram_offset + 1] = color;
            vram_offset += 2;
        }
    }
    if (vram_offset >= 80 * 25 * 2) {
        clear_screen();
    }
}

void clear_screen(void) {
    for (int i = 0; i < 80 * 25 * 2; i += 2) {
        text_vram[i] = ' ';
        text_vram[i+1] = 0x07;
    }
    vram_offset = 0;
}

// --- PORT I/O IMPLEMENTATIONS ---
static inline unsigned char inb(unsigned short port) {
    unsigned char result;
    __asm__ volatile("inb %1, %0" : "=a"(result) : "Nd"(port));
    return result;
}
static inline unsigned short inw(unsigned short port) {
    unsigned short result;
    __asm__ volatile("inw %1, %0" : "=a"(result) : "Nd"(port));
    return result;
}
static inline void outb(unsigned short port, unsigned char data) {
    __asm__ volatile("outb %0, %1" : : "a"(data), "Nd"(port));
}
static inline void outw(unsigned short port, unsigned short data) {
    __asm__ volatile("outw %0, %1" : : "a"(data), "Nd"(port));
}

void hardware_reboot(void) {
    unsigned char good = 0x02;
    while (good & 0x02) good = inb(0x64);
    outb(0x64, 0xFE); 
    while(1);
}

void hardware_shutdown(void) {
    outw(0x604, 0x2000);
    outw(0xB004, 0x2000);
    while(1) { __asm__ volatile("hlt"); }
}

// --- IDE CONTROLLER STATUS HELPER ---
void wait_disk_ready(void) {
    while ((inb(0x1F7) & 0x80) || !(inb(0x1F7) & 0x40));
}

void read_disk_sector(int lba, char* buffer) {
    wait_disk_ready();
    outb(0x1F6, 0xE0 | ((lba >> 24) & 0x0F));
    outb(0x1F2, 1);
    outb(0x1F3, (unsigned char)lba);
    outb(0x1F4, (unsigned char)(lba >> 8));
    outb(0x1F5, (unsigned char)(lba >> 16));
    outb(0x1F7, 0x20);
    while (!(inb(0x1F7) & 0x08));
    unsigned short* ptr = (unsigned short*)buffer;
    for (int i = 0; i < 256; i++) ptr[i] = inw(0x1F0);
}

void write_disk_sector(int lba, const char* buffer) {
    wait_disk_ready();
    outb(0x1F6, 0xE0 | ((lba >> 24) & 0x0F));
    outb(0x1F2, 1);
    outb(0x1F3, (unsigned char)lba);
    outb(0x1F4, (unsigned char)(lba >> 8));
    outb(0x1F5, (unsigned char)(lba >> 16));
    outb(0x1F7, 0x30); 
    while (!(inb(0x1F7) & 0x08));
    unsigned short* ptr = (unsigned short*)buffer;
    for (int i = 0; i < 256; i++) outw(0x1F0, ptr[i]);
}

void matrix_effect(void) {
    clear_screen();
    for (int loops = 0; loops < 60; loops++) {
        for (int i = 0; i < 80 * 25 * 2; i += 2) {
            unsigned char rand_char = (inb(0x40) % 94) + 33; 
            if (inb(0x40) % 5 == 0) {
                text_vram[i] = rand_char;
                text_vram[i+1] = 0x0A; 
            }
        }
        for (volatile int d = 0; d < 300000; d++); 
    }
    clear_screen();
}

// --- CLI COMMAND EVALUATOR WITH REDIRECTION ENGINE ---
void handle_cli_command(const char* cmd) {
    char clean_cmd[64];
    char file_target[32];
    int redirect_mode = 0; // 0 = None, 1 = Overwrite (>), 2 = Append (>>)
    int op_idx = -1;

    for (int i = 0; i < 64; i++) {
        clean_cmd[i] = 0;
        if (i < 32) file_target[i] = 0;
    }

    // Step 1: Detect Redirection Symbols
    for (int i = 0; cmd[i] != '\0'; i++) {
        if (cmd[i] == '>') {
            if (cmd[i+1] == '>') {
                redirect_mode = 2; 
                op_idx = i;
                break;
            } else {
                redirect_mode = 1; 
                op_idx = i;
                break;
            }
        }
    }

    // Step 2: Split Command and Target File
    if (redirect_mode > 0) {
        for (int i = 0; i < op_idx; i++) {
            clean_cmd[i] = cmd[i];
        }
        int c_len = op_idx;
        while (c_len > 0 && clean_cmd[c_len - 1] == ' ') {
            clean_cmd[c_len - 1] = '\0';
            c_len--;
        }

        int f_start = (redirect_mode == 2) ? op_idx + 2 : op_idx + 1;
        while (cmd[f_start] == ' ') f_start++; 
        
        int f_idx = 0;
        while (cmd[f_start] != '\0' && cmd[f_start] != ' ' && f_idx < 31) {
            file_target[f_idx++] = cmd[f_start++];
        }
        file_target[f_idx] = '\0';

        int t_len = str_length(file_target);
        int valid_ext = 0;
        if (t_len > 4) {
            if (str_compare_n(&file_target[t_len - 4], ".txt", 4) || 
                str_compare_n(&file_target[t_len - 4], ".sys", 4)) {
                valid_ext = 1;
            }
        }

        if (!valid_ext) {
            print_string("Error: System only permits writing to .txt and .sys files.\n", 0x0C);
            return;
        }
    } else {
        for (int i = 0; cmd[i] != '\0'; i++) clean_cmd[i] = cmd[i];
    }

    // Step 3: Command Routing
    int is_help       = str_compare_n(clean_cmd, "help", 4) && clean_cmd[4] == '\0';
    int is_clear      = str_compare_n(clean_cmd, "clear", 5) && clean_cmd[5] == '\0';
    int is_reboot     = str_compare_n(clean_cmd, "reboot", 6) && clean_cmd[6] == '\0';
    int is_shutdown   = str_compare_n(clean_cmd, "shutdown", 8) && clean_cmd[8] == '\0';
    int is_colorgreen = str_compare_n(clean_cmd, "colorgreen", 10) && clean_cmd[10] == '\0';
    int is_colorwhite = str_compare_n(clean_cmd, "colorwhite", 10) && clean_cmd[10] == '\0';
    int is_matrix     = str_compare_n(clean_cmd, "matrix", 6) && clean_cmd[6] == '\0';
    int is_dir        = str_compare_n(clean_cmd, "dir", 3) && clean_cmd[3] == '\0';
    int is_cat        = str_compare_n(clean_cmd, "cat ", 4);
    int is_echo       = str_compare_n(clean_cmd, "echo ", 5);
    int is_sysfetch   = str_compare_n(clean_cmd, "sysfetch", 8) && clean_cmd[8] == '\0';
    int is_init       = str_compare_n(clean_cmd, "init", 4) && clean_cmd[4] == '\0';
    int is_rm         = str_compare_n(clean_cmd, "rm ", 3);

    if (is_help) {
        print_string("SimplyOS v1.0 (SimplyKrnl) Commands:\n", current_text_color);
        print_string("  echo <msg> [> filename]  - Print message or save to file (.txt/.sys)\n", current_text_color);
        print_string("  cat <file>               - View contents of file\n", current_text_color);
        print_string("  dir                      - List files\n", current_text_color);
        print_string("  rm <file>                - Delete a file\n", current_text_color);
        print_string("  sysfetch                 - Display hardware system performance specs\n", current_text_color);
        print_string("  init                     - Format/Initialize the directory sector\n", current_text_color);
        print_string("  clear / matrix / reboot / shutdown\n", current_text_color);
    } 
    else if (is_clear) {
        clear_screen();
    }
    else if (is_dir) {
        char buffer[512];
        read_disk_sector(100, buffer);
        print_string("Root Directory:\n", 0x0E);
        
        if (buffer[0] >= 32 && buffer[0] <= 126) { 
            print_string(&buffer[0], current_text_color); print_string(" (Sector 101)\n", 0x07); 
        } else { print_string("[Empty Slot A]\n", 0x08); }
        
        if (buffer[40] >= 32 && buffer[40] <= 126) { 
            print_string(&buffer[40], current_text_color); print_string(" (Sector 102)\n", 0x07); 
        } else { print_string("[Empty Slot B]\n", 0x08); }
    }
    else if (is_cat) {
        char buffer[512];
        char dir_buf[512];
        read_disk_sector(100, dir_buf);
        const char* target = &clean_cmd[4];

        int target_sector = 0;
        if (str_compare_n(dir_buf, target, str_length(target))) target_sector = 101;
        else if (str_compare_n(&dir_buf[40], target, str_length(target))) target_sector = 102;

        if (target_sector > 0) {
            read_disk_sector(target_sector, buffer);
            print_string(buffer, current_text_color);
            print_string("\n", current_text_color);
        } else {
            print_string("Error: File not found.\n", 0x0C);
        }
    }
    else if (is_echo) {
        const char* msg = &clean_cmd[5];

        if (redirect_mode > 0) {
            char dir_buf[512];
            char file_buf[512];
            read_disk_sector(100, dir_buf);

            int target_sector = 0;
            int dir_slot_offset = 0;

            int slotA_valid = (dir_buf[0] >= 32 && dir_buf[0] <= 126);
            int slotB_valid = (dir_buf[40] >= 32 && dir_buf[40] <= 126);

            if (slotA_valid && str_compare_n(dir_buf, file_target, str_length(file_target))) {
                target_sector = 101;
                dir_slot_offset = 0;
            } else if (slotB_valid && str_compare_n(&dir_buf[40], file_target, str_length(file_target))) {
                target_sector = 102;
                dir_slot_offset = 40;
            } else {
                if (!slotA_valid) { target_sector = 101; dir_slot_offset = 0; }
                else if (!slotB_valid) { target_sector = 102; dir_slot_offset = 40; }
            }

            if (target_sector == 0) {
                print_string("Error: Directory allocation full.\n", 0x0C);
                return;
            }

            for(int i=0; i<40; i++) dir_buf[dir_slot_offset + i] = 0;
            for (int i = 0; file_target[i] != '\0' && i < 39; i++) {
                dir_buf[dir_slot_offset + i] = file_target[i];
            }
            write_disk_sector(100, dir_buf);

            for (int i = 0; i < 512; i++) file_buf[i] = 0;
            if (redirect_mode == 2) { 
                read_disk_sector(target_sector, file_buf);
                int existing_len = str_length(file_buf);
                for (int i = 0; msg[i] != '\0' && (existing_len + i) < 511; i++) {
                    file_buf[existing_len + i] = msg[i];
                }
            } else { 
                for (int i = 0; msg[i] != '\0' && i < 511; i++) file_buf[i] = msg[i];
            }

            write_disk_sector(target_sector, file_buf);
            print_string("File updated successfully.\n", 0x0A);
        } else {
            print_string(msg, current_text_color);
            print_string("\n", current_text_color);
        }
    }
    // --- SYSFETCH COMMAND ---
    else if (is_sysfetch) {
        char dir_buf[512];
        read_disk_sector(100, dir_buf);
        
        int used_sectors = 1; 
        if (dir_buf[0] >= 32 && dir_buf[0] <= 126) used_sectors++;
        if (dir_buf[40] >= 32 && dir_buf[40] <= 126) used_sectors++;

        print_string("  #####    SimplyOS v1.0 (SimplyKrnl)\n", 0x0A);
        print_string(" #######   --------------------------\n", 0x0A);
        print_string(" ##   ##   OS Type: Bare-Metal x86 Kernel\n", current_text_color);
        print_string(" #######   RAM Usage: 12 KB / 131072 KB (QEMU Default Footprint)\n", current_text_color);
        
        print_string("  #####    Disk Space: ", current_text_color);
        char disk_out[2];
        disk_out[0] = used_sectors + '0';
        disk_out[1] = '\0';
        print_string(disk_out, 0x0E);
        print_string(" / 2880 Sectors (Floppy Map Mode)\n", current_text_color);
        print_string("           Shell Mode: CLI Native (Redirection Enabled)\n\n", current_text_color);
    }
    // --- INIT COMMAND ---
    else if (is_init) {
        char clear_buf[512];
        for (int i = 0; i < 512; i++) {
            clear_buf[i] = 0;
        }
        print_string("Initializing file system structure on Sector 100...\n", 0x0E);
        write_disk_sector(100, clear_buf);
        print_string("Disk directory initialized successfully. 2 slots ready.\n", 0x0A);
    }
    // --- RM (REMOVE/DELETE) COMMAND ---
    else if (is_rm) {
        char dir_buf[512];
        read_disk_sector(100, dir_buf);
        const char* target = &clean_cmd[3];
        int slot_offset = -1;

        if (dir_buf[0] >= 32 && str_compare_n(dir_buf, target, str_length(target))) {
            slot_offset = 0;
        } else if (dir_buf[40] >= 32 && str_compare_n(&dir_buf[40], target, str_length(target))) {
            slot_offset = 40;
        }

        if (slot_offset >= 0) {
            for (int i = 0; i < 40; i++) {
                dir_buf[slot_offset + i] = 0;
            }
            write_disk_sector(100, dir_buf);
            print_string("File deleted successfully.\n", 0x0A);
        } else {
            print_string("Error: File not found.\n", 0x0C);
        }
    }
    else if (is_colorgreen) { current_text_color = 0x0A; print_string("Color updated to green.\n", current_text_color); }
    else if (is_colorwhite) { current_text_color = 0x0F; print_string("Color updated to white.\n", current_text_color); }
    else if (is_matrix) matrix_effect();
    else if (is_reboot) hardware_reboot();
    else if (is_shutdown) hardware_shutdown();
    else print_string("Error: Unknown command node.\n", 0x0C);
}
