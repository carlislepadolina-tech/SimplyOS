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
void read_disk_sector(int lba, char* buffer);
void write_disk_sector(int lba, const char* buffer);
void matrix_effect(void);

// --- GLOBAL STATE ---
volatile char* const text_vram = (volatile char*)0xB8000;
int vram_offset = 0;
unsigned char current_text_color = 0x0F; // Default White
int shift_pressed = 0;                   // Global Shift tracking state

// --- KERNEL ENTRY POINT ---
void kernel_main(void) {
    clear_screen();
    current_text_color = 0x0F;

    // 1. Shell Boot Guard: Check if the PS/2 Keyboard Controller is responding
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

    // 2. Evaluate if the initialization succeeded
    if (controller_failed) {
        print_string("An error uccured when trying to load the shell. (KEYBOARD_CONTROLLER_TIMEOUT)\n", 0x0C); 
        while(1) { __asm__ volatile("hlt"); } 
    }

    // 3. Initialize working local buffers safely
    char input_buffer[64];
    for (int i = 0; i < 64; i++) input_buffer[i] = 0;
    int buf_idx = 0;
    
    // 4. Render the UI Shell Environment securely
    print_string("SimplyOS CLI [SimplyKrnl Activated]\n", 0x0A);
    print_string("Type 'help' to see all commands.\n\n> ", current_text_color);

    // --- STANDARD CLI RECEIVE LOOP ---
    while (1) {
        if (inb(0x64) & 0x01) {
            unsigned char scancode = inb(0x60);
            
            // Track Shift Key Press
            if (scancode == 0x2A || scancode == 0x36) {
                shift_pressed = 1;
            }
            // Track Shift Key Release (Break Codes)
            else if (scancode == 0xAA || scancode == 0xB6) {
                shift_pressed = 0;
            }
            // Handle regular key presses
            else if (!(scancode & 0x80)) { 
                char c = 0;
                
                if (scancode == 0x1C) { // Enter Key
                    print_string("\n", current_text_color);
                    input_buffer[buf_idx] = '\0';
                    
                    if (buf_idx > 0) {
                        handle_cli_command(input_buffer);
                    }
                    
                    print_string("> ", current_text_color);
                    buf_idx = 0;
                } 
                else if (scancode == 0x0E) { // Backspace
                    if (buf_idx > 0) {
                        buf_idx--;
                        vram_offset -= 2;
                        text_vram[vram_offset] = ' ';
                        text_vram[vram_offset + 1] = 0x07;
                    }
                }
                // Case-Aware Text Translation Map
                else if (scancode == 0x10) c = shift_pressed ? 'Q' : 'q';
                else if (scancode == 0x11) c = shift_pressed ? 'W' : 'w';
                else if (scancode == 0x12) c = shift_pressed ? 'E' : 'e';
                else if (scancode == 0x13) c = shift_pressed ? 'R' : 'r';
                else if (scancode == 0x14) c = shift_pressed ? 'T' : 't';
                else if (scancode == 0x15) c = shift_pressed ? 'Y' : 'y';
                else if (scancode == 0x16) c = shift_pressed ? 'U' : 'u';
                else if (scancode == 0x17) c = shift_pressed ? 'I' : 'i';
                else if (scancode == 0x18) c = shift_pressed ? 'O' : 'o';
                else if (scancode == 0x19) c = shift_pressed ? 'P' : 'p';
                else if (scancode == 0x1E) c = shift_pressed ? 'A' : 'a';
                else if (scancode == 0x1F) c = shift_pressed ? 'S' : 's';
                else if (scancode == 0x20) c = shift_pressed ? 'D' : 'd';
                else if (scancode == 0x21) c = shift_pressed ? 'F' : 'f';
                else if (scancode == 0x22) c = shift_pressed ? 'G' : 'g';
                else if (scancode == 0x23) c = shift_pressed ? 'H' : 'h';
                else if (scancode == 0x24) c = shift_pressed ? 'J' : 'j';
                else if (scancode == 0x25) c = shift_pressed ? 'K' : 'k';
                else if (scancode == 0x26) c = shift_pressed ? 'L' : 'l';
                else if (scancode == 0x2C) c = shift_pressed ? 'Z' : 'z';
                else if (scancode == 0x2D) c = shift_pressed ? 'X' : 'x';
                else if (scancode == 0x2E) c = shift_pressed ? 'C' : 'c';
                else if (scancode == 0x2F) c = shift_pressed ? 'V' : 'v';
                else if (scancode == 0x30) c = shift_pressed ? 'B' : 'b';
                else if (scancode == 0x31) c = shift_pressed ? 'N' : 'n';
                else if (scancode == 0x32) c = shift_pressed ? 'M' : 'm';
                else if (scancode == 0x39) c = ' '; // Spacebar

                if (c != 0 && buf_idx < 63) {
                    input_buffer[buf_idx++] = c;
                    text_vram[vram_offset] = c;
                    text_vram[vram_offset + 1] = current_text_color;
                    vram_offset += 2;
                }
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

// --- HARDWARE DISK READING (ATA PIO) ---
void read_disk_sector(int lba, char* buffer) {
    outb(0x1F6, 0xE0 | ((lba >> 24) & 0x0F));
    outb(0x1F2, 1);
    outb(0x1F3, (unsigned char)lba);
    outb(0x1F4, (unsigned char)(lba >> 8));
    outb(0x1F5, (unsigned char)(lba >> 16));
    outb(0x1F7, 0x20);

    while (!(inb(0x1F7) & 0x08));

    unsigned short* ptr = (unsigned short*)buffer;
    for (int i = 0; i < 256; i++) {
        ptr[i] = inw(0x1F0);
    }
}

// --- HARDWARE DISK WRITING (ATA PIO) ---
void write_disk_sector(int lba, const char* buffer) {
    outb(0x1F6, 0xE0 | ((lba >> 24) & 0x0F));
    outb(0x1F2, 1);
    outb(0x1F3, (unsigned char)lba);
    outb(0x1F4, (unsigned char)(lba >> 8));
    outb(0x1F5, (unsigned char)(lba >> 16));
    outb(0x1F7, 0x30); 

    while (!(inb(0x1F7) & 0x08));

    unsigned short* ptr = (unsigned short*)buffer;
    for (int i = 0; i < 256; i++) {
        outw(0x1F0, ptr[i]);
    }
}

// --- MATRIX EFFECT ---
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

// --- CLI COMMAND EVALUATOR ---
void handle_cli_command(const char* cmd) {
    int is_help       = (cmd[0] == 'h' && cmd[1] == 'e' && cmd[2] == 'l' && cmd[3] == 'p' && cmd[4] == '\0');
    int is_clear      = (cmd[0] == 'c' && cmd[1] == 'l' && cmd[2] == 'e' && cmd[3] == 'a' && cmd[4] == 'r' && cmd[5] == '\0');
    int is_reboot     = (cmd[0] == 'r' && cmd[1] == 'e' && cmd[2] == 'b' && cmd[3] == 'o' && cmd[4] == 'o' && cmd[5] == 't' && cmd[6] == '\0');
    int is_shutdown   = (cmd[0] == 's' && cmd[1] == 'h' && cmd[2] == 'u' && cmd[3] == 't' && cmd[4] == 'd' && cmd[5] == 'o' && cmd[6] == 'w' && cmd[7] == 'n' && cmd[8] == '\0');
    int is_colorgreen = (cmd[0] == 'c' && cmd[1] == 'o' && cmd[2] == 'l' && cmd[3] == 'o' && cmd[4] == 'r' && cmd[5] == 'g' && cmd[6] == 'r' && cmd[7] == 'e' && cmd[8] == 'e' && cmd[9] == 'n' && cmd[10] == '\0');
    int is_colorwhite = (cmd[0] == 'c' && cmd[1] == 'o' && cmd[2] == 'l' && cmd[3] == 'o' && cmd[4] == 'r' && cmd[5] == 'w' && cmd[6] == 'h' && cmd[7] == 'i' && cmd[8] == 't' && cmd[9] == 'e' && cmd[10] == '\0');
    int is_matrix     = (cmd[0] == 'm' && cmd[1] == 'a' && cmd[2] == 't' && cmd[3] == 'r' && cmd[4] == 'i' && cmd[5] == 'x' && cmd[6] == '\0');
    int is_dir        = (cmd[0] == 'd' && cmd[1] == 'i' && cmd[2] == 'r' && cmd[3] == '\0');
    int is_cat        = (cmd[0] == 'c' && cmd[1] == 'a' && cmd[2] == 't' && cmd[3] == ' ' && cmd[4] != '\0');
    int is_echo       = (cmd[0] == 'e' && cmd[1] == 'c' && cmd[2] == 'h' && cmd[3] == 'o' && cmd[4] == ' ' && cmd[5] != '\0');
    int is_edit       = (cmd[0] == 'e' && cmd[1] == 'd' && cmd[2] == 'i' && cmd[3] == 't' && cmd[4] == ' ' && cmd[5] != '\0');

    if (is_help) {
        print_string("SimplyOS v1.0 (SimplyKrnl) Commands:\n", current_text_color);
        print_string("  help       - Show this list\n", current_text_color);
        print_string("  clear      - Wipe the terminal\n", current_text_color);
        print_string("  dir        - List files in root directory\n", current_text_color);
        print_string("  cat <file> - Use 'cat s' for secret, 'cat m' for motd\n", current_text_color);
        print_string("  echo <msg> - Print custom string parameters out\n", current_text_color);
        print_string("  edit <sec> - Write custom string directly into a disk sector\n", current_text_color);
        print_string("  colorgreen - Change terminal text to green\n", current_text_color);
        print_string("  colorwhite - Change terminal text to white\n", current_text_color);
        print_string("  matrix     - Run visual matrix effect\n", current_text_color);
        print_string("  reboot     - Restart the machine\n", current_text_color);
        print_string("  shutdown   - Power off the system\n", current_text_color);
    } 
    else if (is_clear) {
        clear_screen();
    }
    else if (is_dir) {
        char buffer[512];
        for(int i=0; i<512; i++) buffer[i] = 0;
        
        read_disk_sector(100, buffer);
        print_string("Root Directory:\n", 0x0E);
        
        if (buffer[0] != 0) {
            print_string(&buffer[0], current_text_color);
            print_string(" (Sector 101)\n", 0x07);
        }
        if (buffer[40] != 0) {
            print_string(&buffer[40], current_text_color);
            print_string(" (Sector 102)\n", 0x07);
        }
    }
    else if (is_cat) {
        char buffer[512];
        for(int i = 0; i < 512; i++) buffer[i] = 0; 
        
        if (cmd[4] == 's') { 
            read_disk_sector(101, buffer);
            print_string(buffer, current_text_color);
        } else if (cmd[4] == 'm') { 
            read_disk_sector(102, buffer);
            print_string(buffer, current_text_color);
        } else {
            print_string("Error: File not found.\n", 0x0C);
        }
        print_string("\n", current_text_color);
    }
    else if (is_echo) {
        print_string(&cmd[5], current_text_color);
        print_string("\n", current_text_color);
    }
    // --- TEXT EDITOR INTERACTION NODE ---
    else if (is_edit) {
        int target_sector = 0;
        for (int i = 5; cmd[i] != '\0'; i++) {
            if (cmd[i] >= '0' && cmd[i] <= '9') {
                target_sector = target_sector * 10 + (cmd[i] - '0');
            }
        }

        if (target_sector == 0) {
            print_string("Error: Invalid sector target.\n", 0x0C);
            return;
        }

        clear_screen();
        print_string("--- SimplyOS Inline Buffer Editor ---\n", 0x0E);
        print_string("Type your text and press ENTER to write to sector ", 0x07);
        
        char sec_str[4];
        sec_str[0] = (target_sector / 100) + '0';
        sec_str[1] = ((target_sector % 100) / 10) + '0';
        sec_str[2] = (target_sector % 10) + '0';
        sec_str[3] = '\0';
        print_string(sec_str, 0x0A);
        print_string("\n\n> ", 0x07);

        char edit_buffer[512];
        for (int i = 0; i < 512; i++) edit_buffer[i] = 0;
        int edit_idx = 0;
        int editing = 1;

        while (editing) {
            if (inb(0x64) & 0x01) {
                unsigned char scancode = inb(0x60);
                
                if (scancode == 0x2A || scancode == 0x36) {
                    shift_pressed = 1;
                }
                else if (scancode == 0x2A || scancode == 0xB6) {
                    shift_pressed = 0;
                }
                else if (!(scancode & 0x80)) {
                    char c = 0;
                    
                    if (scancode == 0x1C) { 
                        edit_buffer[edit_idx] = '\0';
                        editing = 0;
                    }
                    else if (scancode == 0x0E) { 
                        if (edit_idx > 0) {
                            edit_idx--;
                            vram_offset -= 2;
                            text_vram[vram_offset] = ' ';
                            text_vram[vram_offset + 1] = 0x07;
                        }
                    }
                    else if (scancode == 0x10) c = shift_pressed ? 'Q' : 'q';
                    else if (scancode == 0x11) c = shift_pressed ? 'W' : 'w';
                    else if (scancode == 0x12) c = shift_pressed ? 'E' : 'e';
                    else if (scancode == 0x13) c = shift_pressed ? 'R' : 'r';
                    else if (scancode == 0x14) c = shift_pressed ? 'T' : 't';
                    else if (scancode == 0x15) c = shift_pressed ? 'Y' : 'y';
                    else if (scancode == 0x16) c = shift_pressed ? 'U' : 'u';
                    else if (scancode == 0x17) c = shift_pressed ? 'I' : 'i';
                    else if (scancode == 0x18) c = shift_pressed ? 'O' : 'o';
                    else if (scancode == 0x19) c = shift_pressed ? 'P' : 'p';
                    else if (scancode == 0x1E) c = shift_pressed ? 'A' : 'a';
                    else if (scancode == 0x1F) c = shift_pressed ? 'S' : 's';
                    else if (scancode == 0x20) c = shift_pressed ? 'D' : 'd';
                    else if (scancode == 0x21) c = shift_pressed ? 'F' : 'f';
                    else if (scancode == 0x22) c = shift_pressed ? 'G' : 'g';
                    else if (scancode == 0x23) c = shift_pressed ? 'H' : 'h';
                    else if (scancode == 0x24) c = shift_pressed ? 'J' : 'j';
                    else if (scancode == 0x25) c = shift_pressed ? 'K' : 'k';
                    else if (scancode == 0x26) c = shift_pressed ? 'L' : 'l';
                    else if (scancode == 0x2C) c = shift_pressed ? 'Z' : 'z';
                    else if (scancode == 0x2D) c = shift_pressed ? 'X' : 'x';
                    else if (scancode == 0x2E) c = shift_pressed ? 'C' : 'c';
                    else if (scancode == 0x2F) c = shift_pressed ? 'V' : 'v';
                    else if (scancode == 0x30) c = shift_pressed ? 'B' : 'b';
                    else if (scancode == 0x31) c = shift_pressed ? 'N' : 'n';
                    else if (scancode == 0x32) c = shift_pressed ? 'M' : 'm';
                    else if (scancode == 0x39) c = ' ';

                    if (c != 0 && edit_idx < 511) {
                        edit_buffer[edit_idx++] = c;
                        text_vram[vram_offset] = c;
                        text_vram[vram_offset + 1] = current_text_color;
                        vram_offset += 2;
                    }
                }
                for (volatile int d = 0; d < 20000; d++);
            }
        }

        write_disk_sector(target_sector, edit_buffer);
        clear_screen();
        print_string("Sector write complete! Returning to system console.\n\n", 0x0A);
    }
    else if (is_colorgreen) {
        current_text_color = 0x0A;
        print_string("Color updated to green.\n", current_text_color);
    }
    else if (is_colorwhite) {
        current_text_color = 0x0F;
        print_string("Color updated to white.\n", current_text_color);
    }
    else if (is_matrix) {
        matrix_effect();
    }
    else if (is_reboot) {
        print_string("Rebooting...\n", 0x0C);
        hardware_reboot();
    }
    else if (is_shutdown) {
        print_string("Sending ACPI Shutdown signal...\n", 0x0C);
        hardware_shutdown();
    }
    else {
        print_string("Error: Unknown command node.\n", 0x0C);
    }
}
