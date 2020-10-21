#include <stddef.h>
#include <stdint.h>
#include <kernel.h>

struct guid {
    uint32_t a;
    uint16_t b;
    uint16_t c;
    uint8_t  d[8];
} __attribute__((packed));

#define CACHE_NOTREADY 0
#define CACHE_READY    1
#define CACHE_DIRTY    2

struct part {
    int fd;
    uint64_t first_sect;
    uint64_t sect_count;
    uint64_t in_cache;
    uint8_t cache[512];
    int cache_state;
    struct guid guid;
};

#define NO_PARTITION  (-1)
#define INVALID_TABLE (-2)
#define END_OF_TABLE  (-3)

struct gpt_table_header {
    // the head
    char     signature[8];
    uint32_t revision;
    uint32_t header_size;
    uint32_t crc32;
    uint32_t _reserved0;

    // the partitioning info
    uint64_t my_lba;
    uint64_t alternate_lba;
    uint64_t first_usable_lba;
    uint64_t last_usable_lba;

    // the guid
    struct guid disk_guid;

    // entries related
    uint64_t partition_entry_lba;
    uint32_t number_of_partition_entries;
    uint32_t size_of_partition_entry;
    uint32_t partition_entry_array_crc32;
} __attribute__((packed));

struct gpt_entry {
    struct guid partition_type_guid;

    struct guid unique_partition_guid;

    uint64_t starting_lba;
    uint64_t ending_lba;

    uint64_t attributes;

    uint16_t partition_name[36];
} __attribute__((packed));

static int gpt_get_part(struct part *ret, int drive, int partition) {
    struct gpt_table_header header = {0};

    // read header, located after the first block
    devfs_seek(drive, 512, SEEK_SET);
    devfs_uread(drive, &header, sizeof(struct gpt_table_header));

    // check the header
    // 'EFI PART'
    if (kstrncmp(header.signature, "EFI PART", 8))
        return INVALID_TABLE;
    if (header.revision != 0x00010000)
        return END_OF_TABLE;

    // parse the entries if reached here
    if ((uint32_t)partition >= header.number_of_partition_entries)
        return END_OF_TABLE;

    struct gpt_entry entry = {0};
    devfs_seek(drive, (header.partition_entry_lba * 512) + (partition * sizeof(entry)), SEEK_SET);
    devfs_uread(drive, &entry, sizeof(struct gpt_entry));

    struct guid empty_guid = {0};
    if (!memcmp(&entry.unique_partition_guid, &empty_guid, sizeof(struct guid)))
        return NO_PARTITION;

    ret->first_sect = entry.starting_lba;
    ret->sect_count = (entry.ending_lba - entry.starting_lba) + 1;
    ret->guid       = entry.unique_partition_guid;

    return 0;
}

struct mbr_entry {
	uint8_t status;
	uint8_t chs_first_sect[3];
	uint8_t type;
	uint8_t chs_last_sect[3];
	uint32_t first_sect;
	uint32_t sect_count;
} __attribute__((packed));

static int mbr_get_part(struct part *ret, int drive, int partition) {
    // Check if actually valid mbr
    uint16_t hint;
    devfs_seek(drive, 444, SEEK_SET);
    devfs_uread(drive, &hint, sizeof(uint16_t));
    if (hint && hint != 0x5a5a)
        return INVALID_TABLE;

    if (partition > 3)
        return END_OF_TABLE;

    uint32_t disk_signature;
    devfs_seek(drive, 440, SEEK_SET);
    devfs_uread(drive, &disk_signature, sizeof(uint32_t));

    struct mbr_entry entry;
    size_t entry_offset = 0x1be + sizeof(struct mbr_entry) * partition;

    devfs_seek(drive, entry_offset, SEEK_SET);
    devfs_uread(drive, &entry, sizeof(struct mbr_entry));

    if (entry.type == 0)
        return NO_PARTITION;

    ret->first_sect = entry.first_sect;
    ret->sect_count = entry.sect_count;

    // We use the same method of generating GUIDs for MBR partitions as Linux
    struct guid empty_guid = {0};
    ret->guid   = empty_guid;
    ret->guid.a = disk_signature;
    ret->guid.b = (partition + 1) << 8;

    return 0;
}

static int get_part(struct part *part, int drive, int partition) {
    int ret;

    ret = gpt_get_part(part, drive, partition);
    if (ret != INVALID_TABLE)
        return ret;

    ret = mbr_get_part(part, drive, partition);
    if (ret != INVALID_TABLE)
        return ret;

    return -1;
}

static int partitioned_medium_io_wrapper(
                struct part *part,
                uint64_t loc,
                int type,
                uint8_t payload) {
    uint64_t sect = loc / 512;
    size_t   off  = loc % 512;

    if (part->cache_state == CACHE_NOTREADY || sect != part->in_cache) {
        if (part->cache_state == CACHE_DIRTY) {
            devfs_seek(part->fd, (part->first_sect + sect) * 512, SEEK_SET);
            devfs_uwrite(part->fd, part->cache, 512);
        }
        devfs_seek(part->fd, (part->first_sect + sect) * 512, SEEK_SET);
        devfs_uread(part->fd, part->cache, 512);
        part->in_cache = sect;
        part->cache_state = CACHE_READY;
    }

    switch (type) {
        case DF_READ:
            return part->cache[off];
        case DF_WRITE:
            part->cache[off]  = payload;
            part->cache_state = CACHE_DIRTY;
            return 0;
    }
}

int add_partitioned_medium(const char *medium) {
    for (size_t i = 0; ; i++) {
        int fd = devfs_open(medium, O_RDONLY, 0, NULL);
        if (fd == -1)
            return -1;

        struct part part;

        switch (get_part(&part, fd, i)) {
            case NO_PARTITION:
                devfs_close(fd);
                continue;
            case END_OF_TABLE:
                devfs_close(fd);
                return 0;
            default:
                break;
        }

        struct part *p = kalloc(sizeof(struct part));
        *p = part;

        p->fd = fd;
        p->in_cache = -1;

        int len = kstrlen(medium);
        char pname[len + 3];
        kstrcpy(pname, medium);
        pname[len] = 'p';
        pname[len+1] = i + '0';
        pname[len+2] = 0;

        kernel_add_device(pname, p, p->sect_count * 512, &partitioned_medium_io_wrapper);
    }
}
