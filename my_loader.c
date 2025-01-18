#include <stdio.h>
#include <stdlib.h>
#include <elf.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

// Function to iterate over program headers
int foreach_phdr(void *map_start, void (*func)(Elf32_Phdr *, int), int arg) {
    Elf32_Ehdr *ehdr = (Elf32_Ehdr *)map_start;

    // Locate program headers
    Elf32_Phdr *phdr = (Elf32_Phdr *)((char *)map_start + ehdr->e_phoff);

    // Iterate over all program headers
    for (int i = 0; i < ehdr->e_phnum; i++) {
        func(&phdr[i], arg);  // Call the provided callback function
    }

    return 0;
}
void load_phdr(Elf32_Phdr *phdr, int fd) {
    if (phdr->p_type == PT_LOAD) {
        // Align virtual address and file offset
        void *vaddr = (void *)(phdr->p_vaddr & 0xfffff000);
        size_t offset = phdr->p_offset & 0xfffff000;
        size_t padding = phdr->p_vaddr & 0xfff;
        size_t size = phdr->p_memsz + padding;

        // Get protection flags
        int prot = 0;
        if (phdr->p_flags & PF_R) prot |= PROT_READ;
        if (phdr->p_flags & PF_W) prot |= PROT_WRITE;
        if (phdr->p_flags & PF_X) prot |= PROT_EXEC;

        // Map the segment into memory
        void *mapped = mmap(vaddr, size, prot, MAP_PRIVATE | MAP_FIXED, fd, offset);
        if (mapped == MAP_FAILED) {
            perror("Error mapping segment");
            exit(1);
        }

        // Zero out the BSS section if applicable
        if (phdr->p_memsz > phdr->p_filesz) {
            size_t bss_size = phdr->p_memsz - phdr->p_filesz;
            void *bss_start = (char *)mapped + padding + phdr->p_filesz;
            memset(bss_start, 0, bss_size);
        }

        // Print details about the mapped segment
        char flags[5];
        flags[0] = (phdr->p_flags & PF_R) ? 'R' : ' ';
        flags[1] = (phdr->p_flags & PF_W) ? 'W' : ' ';
        flags[2] = (phdr->p_flags & PF_X) ? 'X' : ' ';
        flags[3] = '\0';

        printf(" LOAD  0x%06lx  0x%08lx  0x%08lx  0x%05lx   0x%05lx   %s  %ld\n",
               (unsigned long)phdr->p_offset, (unsigned long)phdr->p_vaddr,
               (unsigned long)phdr->p_paddr, (unsigned long)phdr->p_filesz,
               (unsigned long)phdr->p_memsz, flags, (unsigned long)phdr->p_align);
    }
}

// Function to get mmap protection flags
int get_mmap_prot(uint32_t p_flags) {
    int prot = 0;
    if (p_flags & PF_R) prot |= PROT_READ;
    if (p_flags & PF_W) prot |= PROT_WRITE;
    if (p_flags & PF_X) prot |= PROT_EXEC;
    return prot;
}

// Function to map segments and print details
void map_segment(Elf32_Phdr *phdr, int fd) {
    static int header_printed = 0;

    if (phdr->p_type == PT_LOAD) {
        // Print the header once at the start
        if (!header_printed) {
            printf("Type   Offset   VirtAddr   PhysAddr   FileSiz   MemSiz   Flg   Align\n");
            header_printed = 1;
        }
// Align Addresses:

// Virtual addresses and offsets must be aligned to the page size (4096 bytes).
// This is done using & 0xfffff000.
// Calculate Sizes:

// p_memsz: Size in memory.
// Padding is added to ensure proper alignment.
// Map Using mmap:

// Maps the segment from the file into memory.
// If .bss exists (uninitialized data), it is zeroed using memset.
        // Align virtual address and file offset
        void *vaddr = (void *)(phdr->p_vaddr & 0xfffff000);
        size_t offset = phdr->p_offset & 0xfffff000;
        size_t padding = phdr->p_vaddr & 0xfff;
        size_t size = phdr->p_memsz + padding;

        // Compute memory protection flags
        int prot = get_mmap_prot(phdr->p_flags);

        //memory mapping
        int mapping_flags = MAP_PRIVATE | MAP_FIXED;
        void *mapped = mmap(vaddr, size, prot, mapping_flags, fd, offset);
        if (mapped == MAP_FAILED) {
            perror("Error mapping segment");
            exit(1);
        }

        //Zero BSS (uninitialized) section
        if (phdr->p_memsz > phdr->p_filesz) {
            size_t bss_size = phdr->p_memsz - phdr->p_filesz;
            void *bss_start = (char *)mapped + padding + phdr->p_filesz;
            memset(bss_start, 0, bss_size);
        }

        //Flag column
        char flags[5];
        flags[0] = (phdr->p_flags & PF_R) ? 'R' : ' ';
        flags[1] = (phdr->p_flags & PF_W) ? 'W' : ' ';
        flags[2] = (phdr->p_flags & PF_X) ? 'X' : ' ';
        flags[3] = '\0';

        printf(" LOAD  0x%06lx  0x%08lx  0x%08lx  0x%05lx   0x%05lx   %s  %ld\n",
               (unsigned long)phdr->p_offset, (unsigned long)phdr->p_vaddr,
               (unsigned long)phdr->p_paddr, (unsigned long)phdr->p_filesz,
               (unsigned long)phdr->p_memsz, flags, (unsigned long)phdr->p_align);

        printf("----Protection flags: %d\n", prot);
        printf("----Mapping flags: %d\n\n", MAP_PRIVATE | MAP_FIXED);
    }
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <ELF File> [args...]\n", argv[0]);
        return 1;
    }

    int fd = open(argv[1], O_RDONLY);
    if (fd < 0) {
        perror("Error opening file");
        return 1;
    }

    struct stat st;
    if (fstat(fd, &st) < 0) {
        perror("Error getting file stats");
        close(fd);
        return 1;
    }
//     mmap loads the ELF file into memory.
// map_start contains the address where the file is mapped.
    void *map_start = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (map_start == MAP_FAILED) {
        perror("Error mapping file");
        close(fd);
        return 1;
    }
    //check if file is ELF with special sig in beginninh
    Elf32_Ehdr *elf_head = (Elf32_Ehdr *)map_start;
    if (elf_head->e_ident[EI_MAG0] != ELFMAG0 ||
        elf_head->e_ident[EI_MAG1] != ELFMAG1 ||
        elf_head->e_ident[EI_MAG2] != ELFMAG2 ||
        elf_head->e_ident[EI_MAG3] != ELFMAG3) {
        fprintf(stderr, "Error: Not a valid ELF file.\n");
        munmap(map_start, st.st_size);
        close(fd);
        return 1;
    }

    foreach_phdr(map_start, map_segment, fd);

    extern int startup(int, char **, void (*start)());
    void (*entry_point)() = (void (*)())elf_head->e_entry;
//     Read the Entry Point:
// e_entry contains the starting address of the loaded program.
// Call startup:
// Transfers control to the loaded program, passing arguments (argc, argv).

    printf("\nStarting program at entry point: %p\n", entry_point);

    startup(argc - 1, argv + 1, entry_point);

    munmap(map_start, st.st_size);
    close(fd);

    return 0;
}
