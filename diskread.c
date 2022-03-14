#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <string.h>
#include <ctype.h>

LPSTR ErrorMessage(DWORD error)
{
    LPSTR lpMsgBuf;
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0, NULL);
    lpMsgBuf[strcspn(lpMsgBuf, "\r\n")] = 0; // Trim newline
    return (LPSTR)lpMsgBuf;
}

char *strncpy_n(char *dest, const char *src, size_t count)
{
    char *ret = dest;
    while (count--)
    {
        if (!(*dest++ = *src++))
        {
            return ret;
        }
    }
    *dest = '\0';
    return ret;
}

char *strlower_n(char *s)
{
    char *p = (char *)calloc(sizeof(char), strlen(s) + 1);
    strncpy_n(p, s, strlen(s));
    for (size_t i = 0; i < strlen(p); i++)
    {
        p[i] = tolower(p[i]);
    }
    return p;
}

void Help()
{
    printf("DiskRead v1.0 - Read a disk or a file in raw mode.\n\nUsage:\ndiskread <drive | file> [read count] [export <file>]\n\nExamples:\ndiskread \\\\.\\PhysicalDrive0 512 export bootsect.bak\nReads first 512 bytes from physical drive 0 and exports it.\n\ndiskread file.txt 40\nReads 40 bytes from file.txt and prints it in hexadecimal.\n\nReturn code:\nThe number of bytes read is returned in case of success, or an error code on failure.\n\nDiskRead can be used for either hexadecimal dumping and a backup tool for your boot sector.\n\nNote: Due to Windows limitations, disk reading is performed in chunks of 512 bytes.\nValues will be rounded up to the nearest multiple of 512.\n\nCopyright (c) 2022 anic17 Software\n");
}

int main(int argc, char *argv[])
{
    if (argc < 2 || !strcmp(argv[1], "--help"))
    {
        Help();
        return 0;
    }

    // Variable declarations and memory allocation
    FILE *export;
    /*
        There's a reason I'm using regular FILE structure with fopen() and fwrite() instead of
        HANDLE with CreateFile() and WriteFile().
        As WinAPI functions are more powerful, I wouldn't want anyone to accidentally export the
        file into the MBR or the boot sector by confusing the arguments.

        So it's a security reason to use FILE structure instead of HANDLE.
    */
    BOOL export_mode = 0;

    DWORD bytes_read = 0, bufsize = 512, last_err = 0;
    size_t bytes_per_line = 16, split_count = bytes_per_line;
    PUCHAR buf; // Just declare them, allocation will be done when we know the size of the buffer
    LPSTR device;
    LONG strtol_ret = 0;
    // Copy arguments to variables

    if (argc >= 3)
    {
        strtol_ret = strtol(argv[2], NULL, 10);

        if (strtol_ret <= 0)
        {
            printf("Invalid read count. Using default value (%d).\n", bufsize);
        }
        else
        {
            bufsize = strtol_ret;
        }
    }

    buf = (PUCHAR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(PUCHAR) * bufsize);     // Allocate memory for the buffer
    device = (PUCHAR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(PUCHAR) * MAX_PATH); // Allocate memory for the device name
    strncpy_n(device, argv[1], MAX_PATH);
    if (GetFileAttributes(device) & FILE_ATTRIBUTE_DEVICE && (bufsize % 512) != 0)
    {

        printf("Warning: Disk must be read in chunks of 512 bytes. Adding %d bytes for padding.\n", 512 - (bufsize % 512));
        bufsize += (512 - (bufsize % 512)); // Add bytes for padding
    }
    if (argc >= 4) // Check for export
    {
        if (!strcmp(strlower_n(argv[3]), "export") && argv[4] != NULL && argc < 6)
        {

            export = fopen(argv[4], "wb");
            if (!export)
            {
                last_err = GetLastError();
                printf("%s (0x%x)\n", ErrorMessage(last_err), last_err);
                return -last_err;
            }
            export_mode = 1;
        }
        else
        {
            printf("Error: Invalid or missing argument.\n");
            return -last_err;
        }
    }

    HANDLE diskread = CreateFile(device, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL); // Open file/disk for reading
    if (diskread == INVALID_HANDLE_VALUE)
    {
        last_err = GetLastError();
        fprintf(stderr, "Error: %s (0x%x)\n", ErrorMessage(last_err), last_err);
        return -last_err;
    }

    printf("Trying to read %d bytes from '%s'...", bufsize, device);
    if (ReadFile(diskread, buf, bufsize, &bytes_read, NULL))
    {
        printf("\n%d bytes read successfully.\n", bytes_read);
        for (size_t i = 0; i < bytes_read; i++)
        {
            if ((split_count) >= bytes_per_line)
            {
                printf("\n[0x%08x] ", i);

                split_count = 0;
            }

            printf("%02x ", buf[i]);
            split_count++;
        }
        if (export_mode)
        {
            fwrite(buf, sizeof(char), bytes_read, export);
        }
    }
    else
    {
        last_err = GetLastError();
        fprintf(stderr, "\nFailed to read %d bytes: %s (0x%x)\n", bufsize, ErrorMessage(last_err), last_err);
        return last_err;
    }
    if (export_mode)
    {
        if (fclose(export) != EOF)
        {
            printf("\n%d bytes of '%s' exported successfully into '%s'.\n", bytes_read, device, argv[4]);
        }
    }
    return bytes_read;
}