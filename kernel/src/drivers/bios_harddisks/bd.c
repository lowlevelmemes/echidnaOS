#include <stdint.h>
#include <kernel.h>

struct bios_drive_params {
    uint16_t buf_size;
    uint16_t info_flags;
    uint32_t cyl;
    uint32_t heads;
    uint32_t sects;
    uint64_t lba_count;
    uint16_t bytes_per_sect;
    uint32_t edd;
} __attribute__((packed));

#define BIOS_DRIVES_START 0x80
#define BIOS_DRIVES_MAX 26
#define BIOS_DRIVES_LIMIT 0xfe
#define BYTES_PER_SECT 512
#define CACHE_SECTS 64
#define CACHE_SIZE (BYTES_PER_SECT * CACHE_SECTS)

#define CACHE_NOT_READY 0
#define CACHE_READY 1
#define CACHE_DIRTY 2

static int cache_status = 0;
static uint8_t cached_drive;
static uint64_t cached_block;
static uint8_t disk_cache[CACHE_SIZE];

static char* bios_harddrive_names[] = {
    "bd0", "bd1", "bd2", "bd3",
    "bd4", "bd5", "bd6", "bd7",
    "bd8", "bd9", "bd10", "bd11",
    "bd12", "bd13", "bd14", "bd15",
    "bd16", "bd17", "bd18", "bd19",
    "bd20", "bd21", "bd22", "bd23",
    "bd24", "bd25", "bd26"
};

uint8_t bios_harddisk_read(uint8_t drive, uint64_t loc);
int bios_harddisk_write(uint8_t drive, uint64_t loc, uint8_t payload);
int bios_harddisks_io_wrapper(uint32_t disk, uint64_t loc, int type, uint8_t payload);

struct dap {
    uint16_t size;
    uint16_t count;
    uint16_t offset;
    uint16_t segment;
    uint64_t lba;
};

static void disk_read_sector(uint8_t drive, uint8_t *buf, uint64_t block) {
    uint8_t tmp[BYTES_PER_SECT];

    struct dap dap;
    dap.size    = 16;
    dap.count   = 1;
    dap.segment = rm_seg(tmp);
    dap.offset  = rm_off(tmp);

    for (size_t i = 0; i < CACHE_SECTS; i++) {
        dap.lba = block + i;

        struct rm_regs r = {0};
        r.eax = 0x4200;
        r.edx = drive;
        r.esi = rm_off(&dap);
        r.ds  = rm_seg(&dap);

        rm_int(0x13, &r, &r);

        if (r.eflags & EFLAGS_CF) {
            int ah = (r.eax >> 8) & 0xff;
            panic(NULL, false, "Disk error %x. Drive %x, LBA %x.\n", ah, drive, dap.lba);
        }

        memcpy(buf + BYTES_PER_SECT*i, tmp, BYTES_PER_SECT);
    }
}

static void disk_write_sector(uint8_t drive, uint8_t *buf, uint64_t block) {
    uint8_t tmp[BYTES_PER_SECT];

    struct dap dap;
    dap.size    = 16;
    dap.count   = 1;
    dap.segment = rm_seg(tmp);
    dap.offset  = rm_off(tmp);

    for (size_t i = 0; i < CACHE_SECTS; i++) {
        dap.lba = block + i;

        memcpy(tmp, buf + BYTES_PER_SECT*i, BYTES_PER_SECT);

        struct rm_regs r = {0};
        r.eax = 0x4300;
        r.edx = drive;
        r.esi = rm_off(&dap);
        r.ds  = rm_seg(&dap);

        rm_int(0x13, &r, &r);

        if (r.eflags & EFLAGS_CF) {
            int ah = (r.eax >> 8) & 0xff;
            panic(NULL, false, "Disk error %x. Drive %x, LBA %x.\n", ah, drive, dap.lba);
        }
    }
}

static bool read_drive_parameters(struct bios_drive_params *dp, int drive) {
    struct rm_regs r = {0};
    struct bios_drive_params drive_params;

    r.eax = 0x4800;
    r.edx = drive;
    r.ds  = rm_seg(&drive_params);
    r.esi = rm_off(&drive_params);

    drive_params.buf_size = sizeof(struct bios_drive_params);

    rm_int(0x13, &r, &r);

    if (r.eflags & EFLAGS_CF)
        return false;

    *dp = drive_params;

    return true;
}

// kernel_add_device adds an io wrapper to the entries in the dev list
// arg 0 is a char* pointing to the name of the new device
// arg 1 is the general purpose value for the device (which gets passed to the wrapper when called)
// arg 2 is a pointer to the standard io wrapper function

void init_bios_harddisks(void) {
    kputs("\nInitialising BIOS hard disks...");

    struct bios_drive_params drive_parameters;

    int j = BIOS_DRIVES_START;
    for (int i = 0; i < BIOS_DRIVES_MAX; i++) {
        for ( ; j < BIOS_DRIVES_LIMIT; j++) {
            if (j == 0xe0)      // ignore BIOS CD
                continue;
            if (read_drive_parameters(&drive_parameters, j))
                goto found;
        }
        // limit exceeded, return
        return;
found:
        kputs("\nBIOS drive:         "); kxtoa(j);
        kputs("\nCylinder count:     "); kuitoa(drive_parameters.cyl);
        kputs("\nHead count:         "); kuitoa(drive_parameters.heads);
        kputs("\nSect per track:     "); kuitoa(drive_parameters.sects);
        kputs("\nSector count:       "); kuitoa(drive_parameters.lba_count);
        kernel_add_device(bios_harddrive_names[i], j, drive_parameters.lba_count * BYTES_PER_SECT, &bios_harddisks_io_wrapper);
        kputs("\nLoaded "); kputs(bios_harddrive_names[i]);

        add_partitioned_medium(bios_harddrive_names[i]);

        j++;
    }
}

// standard kernel io wrapper expects
// arg 0 as a uint32_t being a general purpose value
// arg 1 as a uint64_t being the location for the i/o access
// arg 2 as an int, qualifing the type of access, being read (0) or write (1)
// arg 3 as a uint8_t being the byte to be written (only used when writing)
// the return value is a uint8_t, and returns the read byte, when reading
// when writing it should return 0

int bios_harddisks_io_wrapper(uint32_t disk, uint64_t loc, int type, uint8_t payload) {
    if (type == DF_READ) {
        return bios_harddisk_read((uint8_t)disk, loc);
    } else if (type == DF_WRITE) {
        return bios_harddisk_write((uint8_t)disk, loc, payload);
    }
}

uint8_t bios_harddisk_read(uint8_t drive, uint64_t loc) {
    uint64_t block = loc / CACHE_SIZE;
    uint32_t offset = loc % CACHE_SIZE;

    if ((block == cached_block) && (drive == cached_drive) && (cache_status))
        return disk_cache[offset];

    // cache miss
    // flush cache if dirty
    if (cache_status == CACHE_DIRTY) {
        DISABLE_INTERRUPTS;
        disk_write_sector(cached_drive, disk_cache, cached_block * CACHE_SECTS);
        ENABLE_INTERRUPTS;
    }

    DISABLE_INTERRUPTS;
    disk_read_sector(drive, disk_cache, block * CACHE_SECTS);
    ENABLE_INTERRUPTS;
    cached_drive = drive;
    cached_block = block;
    cache_status = 1;

    return disk_cache[offset];
}

int bios_harddisk_write(uint8_t drive, uint64_t loc, uint8_t payload) {
    uint64_t block = loc / CACHE_SIZE;
    uint32_t offset = loc % CACHE_SIZE;

    if ((block == cached_block) && (drive == cached_drive) && (cache_status)) {
        disk_cache[offset] = payload;
        cache_status = CACHE_DIRTY;
        return 0;
    }

    // cache miss
    // flush cache if dirty
    if (cache_status == CACHE_DIRTY) {
        DISABLE_INTERRUPTS;
        disk_write_sector(cached_drive, disk_cache, cached_block * CACHE_SECTS);
        ENABLE_INTERRUPTS;
    }

    DISABLE_INTERRUPTS;
    disk_read_sector(drive, disk_cache, block * CACHE_SECTS);
    ENABLE_INTERRUPTS;
    cached_drive = drive;
    cached_block = block;

    disk_cache[offset] = payload;
    cache_status = CACHE_DIRTY;
    return 0;
}
