#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <wchar.h>
#include <stddef.h>
#include <windows.h>
#include <math.h>

char diskread_version[] = "2.0";
/*
    Variable declarations.
    These variables are declared as global to be accessible from all functions.
*/
DWORD bytes_read = 0;
FILE *export_;
/*
    There's a reason I'm using regular FILE structure with fopen() and fwrite() instead of
    HANDLE with CreateFile() and WriteFile().
    As WinAPI functions are more powerful, I wouldn't want anyone to accidentally export the
    file into the MBR or the boot sector by confusing the arguments.

    So it's a security reason to use FILE structure instead of HANDLE.
*/
PUCHAR buf; // Buffer used to read the disk

LPSTR ErrorMessage(DWORD error)
{
    LPSTR lpMsgBuf;
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&lpMsgBuf, 0, NULL);
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

int hex_digits(LONG64 a)
{
    long c;

    if (a == 0)
    {
        return 1;
    }
    if (a < 0)
    {
        a = -a;
    }
    c = (int)(log((double)a) / log((double)16)); // base 16 logarithm
    return ++c;
}

void Help()
{
    printf("DiskRead v%s - Read a disk or a file in raw mode.\n\nUsage:\n diskread <drive | file> [read count] [offset] [export <file>]\n\nExamples:\n diskread \\\\.\\PhysicalDrive0 512 0 export bootsect.bak\n Reads the first 512 bytes from physical drive 0 and writes them to 'bootsect.bak'.\n\n diskread file.txt 40 10 \n Prints 40 bytes from file.txt, starting to read at the 10th byte.\n\nReturn code:\n On success, the number of bytes read is returned, or a negative error value on failure.\n\nDiskRead can be used for both hexadecimal dumping and a boot sector backup tool.\n\nNote: Due to Windows limitations, both disk reading and disk offset are performed in chunks of 512 bytes.\n      Values will be rounded up to the nearest multiple of 512.\n\nCopyright (c) 2022 anic17 Software\n", diskread_version);
}

int main(int argc, char *argv[])
{
    if (argc < 2 || !strcmp(argv[1], "--help"))
    {
        Help();
        return 0;
    }

    BOOL export_mode = 0, is_device = 0;
    DWORD bufsize = 512, last_err = 0;
    size_t bytes_per_line = 16, substract_optimization = 0;
    LPSTR device, export_file, outbuffer;
    LONG strtol_ret = 0;
    LONG64 strtoll_ret = 0;
    LARGE_INTEGER offset = {0}, sector_number = {0};
    HANDLE diskread = NULL;
    char print_offset[32] = "\n[0x%08x] "; // Default offset size
    // Copy arguments to variables

    if (argc > 2)
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
    outbuffer = (LPSTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (5 * bytes_per_line + 32) * sizeof(char)); // Allocate space for the output buffer to improve performance
    buf = (PUCHAR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(PUCHAR) * bufsize);                      // Allocate memory for the buffer
    device = (LPSTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(LPSTR) * MAX_PATH);                    // Allocate memory for the device name
    export_file = (LPSTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(LPSTR) * MAX_PATH);               // Allocate memory for the export file name
    strncpy_n(device, argv[1], MAX_PATH);
    is_device = GetFileAttributes(device) & FILE_ATTRIBUTE_DEVICE;
    if (is_device && bufsize % 512)
    {

        fprintf(stderr, "Warning: Disk must be read in chunks of 512 bytes. Adding %d bytes for padding.\n", 512 - (bufsize % 512));
        bufsize += (512 - (bufsize % 512)); // Add bytes for padding
    }
    if (argc > 3) // Check for export
    {
        if ((strtoll_ret = _strtoui64(argv[3], NULL, 10)) >= 0)
        {
            if ((strtoll_ret % 512) && is_device)
            {
                fprintf(stderr, "Warning: Offset must be a multiple of 512 bytes. Adding %d bytes for padding.\n", 512 - (strtoll_ret % 512));

                strtoll_ret += (512 - (strtoll_ret % 512));
            }
            offset.QuadPart = strtoll_ret; // Assuming compiler has 64-bit support
        }
        else
        {
            fprintf(stderr, "Invalid export argument.\n");
            return 1;
        }
        if (argc > 4)
        {
            if (argv[4] != NULL && !strcmp(strlower_n(argv[4]), "export") && argv[5] != NULL && argc < 7)
            {

                export_ = fopen(argv[5], "wb");
                export_file = argv[5];
                if (!export_)
                {
                    last_err = GetLastError();
                    fprintf(stderr, "%s (0x%x)\n", ErrorMessage(last_err), last_err);
                    return -last_err;
                }
                export_mode = 1;
            }
            else
            {
                if(argc == 5)
                {
                    fprintf(stderr, "Error: Missing argument (expected a file).\n");
                } else if(argc >= 7)
                {
                    fprintf(stderr, "Error: Too many arguments.\n");
                }
                return -last_err;
            }
        }
    }
    if (offset.QuadPart + bufsize > 0xffffffff)
    {
        snprintf(print_offset, sizeof(print_offset), "\n[0x%%0%dllx] ", hex_digits(offset.QuadPart + bufsize)); // Increase the offset width
    }
    diskread = CreateFile(device, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL); // Open file/disk for reading
    if (diskread == INVALID_HANDLE_VALUE || SetFilePointerEx(diskread, offset, NULL, FILE_BEGIN) == (signed)INVALID_SET_FILE_POINTER) // Check for errors & set file offset (if specified)
    {
        last_err = GetLastError();
        fprintf(stderr, "Error: %s (0x%x)\n", ErrorMessage(last_err), last_err);
        return -last_err;
    }

    printf("Trying to read %llu bytes from '%s'...", bufsize, device);
    if (is_device)
    {
        sector_number.QuadPart = offset.QuadPart / 512; // Get the sector number
    }

    if (ReadFile(diskread, buf, bufsize, &bytes_read, NULL)) // Read the file
    {

        printf(" %llu bytes read.", bytes_read);
        for (size_t i = 0; i < bytes_read; i)
        {
            if (is_device)
            {
                if (!(i & 0x1ff)) // Fast way of doing i % 512
                {
                    printf("\n[Sector %lld]", sector_number.QuadPart++);
                }
            }
            memset(outbuffer, 0, (5 * bytes_per_line + 20) * sizeof(char));                                   // Clear the output buffer
            snprintf(outbuffer, (5 * bytes_per_line + 20) * sizeof(char), print_offset, i + offset.QuadPart); // Print the offset
            substract_optimization = bytes_read - i;
            for (size_t k = 0; k < (((substract_optimization < bytes_per_line)) ? substract_optimization : bytes_per_line); k++)
            {
                snprintf(outbuffer + strlen(outbuffer), (5 * bytes_per_line + 20) * sizeof(char), "%02x ", buf[i + k]); // Print the bytes
            }
            if (substract_optimization < bytes_per_line) 
            {
                memset(outbuffer + strlen(outbuffer), ' ', (bytes_per_line - substract_optimization) * 3); // Fill the print buffer with spaces in case the line is not completed
            }
            fputs(outbuffer, stdout); // Print the hexadecimal values

            for (size_t k = 0; k < (((substract_optimization < bytes_per_line)) ? substract_optimization : bytes_per_line); k++)
            {
                putchar(buf[i + k] > 0x1f ? buf[i + k] : '.'); // Print the ASCII representation
            }
            i += bytes_per_line;
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
        putchar('\n'); // If the output is written to stdout (con) print a newline to avoid mixing outputs.
        if (fwrite(buf, bufsize, sizeof(char), export_) && fclose(export_) != EOF) // If exporting, write and close the file 
        {
            printf("%d bytes of '%s' exported successfully into '%s'.\n", bytes_read, device, export_file);
        }
        else
        {
            fprintf(stderr, "Failed to write the file %s: %s (0x%x)\n", export_file, ErrorMessage(last_err), last_err);
        }
    }

    CloseHandle(diskread);
    return bytes_read;
}