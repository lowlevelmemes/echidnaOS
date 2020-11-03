#include <stdint.h>
#include <kernel.h>
#include <cio.h>
#include <stivale.h>

extern int ts_enable;

void kernel_init(struct stivale_struct *stivale_struct) {
    #ifdef _SERIAL_KERNEL_OUTPUT_
      debug_kernel_console_init();
    #endif

    init_realmode();

    // setup the PIC's mask
    set_PIC0_mask(0b11111111); // disable all IRQs
    set_PIC1_mask(0b11111111);

    // setup PIC
    map_PIC(0x20, 0x28);

    // enable desc tables
    load_idt();
    load_gdt();

    #ifndef _BIG_FONTS_
      vga_80_x_50();
    #endif

    // disable VGA cursor
    port_out_b(0x3d4, 0x0a);
    port_out_b(0x3d5, 0x20);

    init_tty();
    switch_tty(0);

    // detect memory
    init_pmm((void*)stivale_struct->memory_map_addr,
                    stivale_struct->memory_map_entries);

    // increase speed of the PIT
    set_pit_freq(KRNL_PIT_FREQ);

    task_init();

    // print intro to tty0
    kputs("Welcome to echidnaOS!\n");

    kputs("\nInitialising drivers...");
    // ******* DRIVER INITIALISATION CALLS GO HERE *******
    init_e9();
    init_streams();
    init_tty_drv();
    init_bios_harddisks();
    init_com();
    init_stty();
    init_pcspk();
    keyboard_init();


    // ******* END OF DRIVER INITIALISATION CALLS *******

    kputs("\nInitialising file systems...");
    // ******* FILE SYSTEM INSTALLATION CALLS *******
    install_devfs();
    install_echfs();


    // ******* END OF FILE SYSTEM INSTALLATION CALLS *******


    // END OF EARLY BOOTSTRAP

    // setup the PIC's mask
    ts_enable = 0;
    set_PIC0_mask(0b11111100); // disable all IRQs but timer and keyboard
    set_PIC1_mask(0b11111111);
    ENABLE_INTERRUPTS;

    char shell_path[] = "/sys/init";
    char tty_path[256];
    char root_path[] = "/";
    char shell_name[] = "shell";
    char shell_ser_name[] = "";

    task_info_t shell_exec = {
        shell_path,
        tty_path,
        tty_path,
        tty_path,
        root_path,
        shell_name,
        shell_ser_name,
        0,
        0
    };

    if (vfs_mount("/", ":://bd0p0", "echfs") == -2)
        for(;;);
    vfs_mount("/dev", "devfs", "devfs");

    // launch the shell
    kputs("\nKERNEL INIT DONE!\n");

    //switch_tty(1);

    kstrcpy(tty_path, "/dev/tty0");
    general_execute(&shell_exec);
    kstrcpy(tty_path, "/dev/tty2");
    general_execute(&shell_exec);
    kstrcpy(tty_path, "/dev/tty3");
    general_execute(&shell_exec);

    // wait for task scheduler
    ts_enable = 1;
    ENTER_IDLE;

}
