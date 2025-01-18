#include <stdio.h>
#include <string.h>

// אזור .bss: משתנים סטטיים שלא מאותחלים מקבלים 0 בזיכרון
char buffer[1024]; // אזור זה יופיע בזיכרון, אך לא בקובץ

int main(int argc, char *argv[]) {
    printf("Hello from a static ELF program!\n");

    // הדפסה של הארגומנטים שהועברו
    printf("Number of arguments: %d\n", argc);
    for (int i = 0; i < argc; i++) {
        printf("Argument %d: %s\n", i, argv[i]);
    }

    // שימוש באזור .bss (שאמור להיות מאותחל ל-0)
    printf("\nInitial content of buffer[0]: %d (should be 0)\n", buffer[0]);
    strcpy(buffer, "This is a test string in the buffer.");
    printf("Buffer content: %s\n", buffer);

    return 0;
}
