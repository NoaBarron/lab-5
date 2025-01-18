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

    if (ehdr->e_ident[EI_MAG0] != ELFMAG0 || 
        ehdr->e_ident[EI_MAG1] != ELFMAG1 || 
        ehdr->e_ident[EI_MAG2] != ELFMAG2 || 
        ehdr->e_ident[EI_MAG3] != ELFMAG3) {
        fprintf(stderr, "Error: Not a valid ELF file.\n");
        return -1;
    }

    Elf32_Phdr *phdr = (Elf32_Phdr *)((char *)map_start + ehdr->e_phoff);
    for (int i = 0; i < ehdr->e_phnum; i++) {
        func(&phdr[i], arg);
    }

    return 0;
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

        // Align virtual address and file offset
        void *vaddr = (void *)(phdr->p_vaddr & 0xfffff000);
        size_t offset = phdr->p_offset & 0xfffff000;
        size_t padding = phdr->p_vaddr & 0xfff;
        size_t size = phdr->p_memsz + padding;

        // Compute memory protection flags
        int prot = get_mmap_prot(phdr->p_flags);

        // Perform memory mapping
        int mapping_flags = MAP_PRIVATE | MAP_FIXED;
        void *mapped = mmap(vaddr, size, prot, mapping_flags, fd, offset);
        if (mapped == MAP_FAILED) {
            perror("Error mapping segment");
            exit(1);
        }

        // Zero out BSS (uninitialized) section
        if (phdr->p_memsz > phdr->p_filesz) {
            size_t bss_size = phdr->p_memsz - phdr->p_filesz;
            void *bss_start = (char *)mapped + padding + phdr->p_filesz;
            memset(bss_start, 0, bss_size);
        }

        // Compute the Flg column
        char flags[5]; // Space for "R W X\0"
        flags[0] = (phdr->p_flags & PF_R) ? 'R' : ' ';
        flags[1] = (phdr->p_flags & PF_W) ? 'W' : ' ';
        flags[2] = (phdr->p_flags & PF_X) ? 'X' : ' ';
        flags[3] = '\0'; // Null terminator for the string

        // Print segment details in the exact format from your example
        printf(" LOAD  0x%06lx  0x%08lx  0x%08lx  0x%05lx   0x%05lx   %s  %ld\n",
               (unsigned long)phdr->p_offset, (unsigned long)phdr->p_vaddr,
               (unsigned long)phdr->p_paddr, (unsigned long)phdr->p_filesz,
               (unsigned long)phdr->p_memsz, flags, (unsigned long)phdr->p_align);

        // Print protection and mapping flags
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

    void *map_start = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (map_start == MAP_FAILED) {
        perror("Error mapping file");
        close(fd);
        return 1;
    }

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

    printf("\nStarting program at entry point: %p\n", entry_point);

    startup(argc - 1, argv + 1, entry_point);

    munmap(map_start, st.st_size);
    close(fd);

    return 0;
}
