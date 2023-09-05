#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stddef.h>
#include <windows.h>
#include <math.h> // log()

#ifndef __bool_true_false_are_defined
#define __bool_true_false_are_defined
#define bool int
#define false 0
#define true 1
#endif

char diskread_version[] = "3.0";
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
    printf("DiskRead v%s - Read a disk or a file in raw mode.\n\nUsage:\n diskread <drive | file> [-b <bytes per line>] [-e <export file>] [-h] [-o <offset>] [-s <read size>]\n\nSwitches:\n -b, --bytes <bytecount>  Change the number of bytes per line displayed\n -e, --export <file>      Export to a file\n -h, --hideoffset         Hide the offset display\n -o, --offset <offset>    Set a custom starting offset for the file\n -s, --size <read size>   Read a specific amount of bytes from the file\n\nExamples:\n diskread \\\\.\\PhysicalDrive0 -s 512 -o 0 -e bootsect.bak\n Reads the first 512 bytes from physical drive 0 and writes them to 'bootsect.bak' (a boot sector backup).\n\n diskread file.txt -s 40 -o 10 -h \n Prints 40 bytes from file.txt, starting to read at the 10th byte without displaying the offset.\n\nReturn code:\n On success, the number of bytes read is returned, or a negative error value on failure.\n\nDiskRead can be used for both hexadecimal dumping and a boot sector backup tool.\n\nNote: Due to Windows limitations, both disk reading and disk offset are performed in chunks of 512 bytes.\n      Values will be rounded up to the nearest multiple of 512.\n\nCopyright (c) 2023 anic17 Software\n", diskread_version);
}

void missing_param(char *s)
{
    fprintf(stderr, "Error: Required parameter after '%s'. See 'diskread --help' for more information.\n", s);
    exit(1);
}

int main(int argc, char *argv[])
{
    if (argc < 2 || !strcmp(argv[1], "--help") || !strcmp(argv[1], "/?"))
    {
        Help();
        return 0;
    }

    BOOL is_device = 0, loaded_device = false;
    DWORD bufsize = 512, last_err = 0;
    size_t bytes_per_line = 16, bytes_pl_temp = 0, substract_optimization = 0;
    LPSTR device, export_file, outbuffer;
    LONG strtol_ret = 0;
    LONG64 strtoll_ret = 0;
    LARGE_INTEGER offset = {0}, sector_number = {0};
    HANDLE diskread = NULL;
    char print_offset[32] = "\n[0x%08x] "; // Default offset size
    // Copy arguments to variables

    outbuffer = (LPSTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (5 * bytes_per_line + 32) * sizeof(char)); // Allocate space for the output buffer to improve performance
    buf = (PUCHAR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(PUCHAR) * bufsize);                      // Allocate memory for the buffer
    device = (LPSTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(LPSTR) * MAX_PATH);                    // Allocate memory for the device name
    export_file = (LPSTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(LPSTR) * MAX_PATH);               // Allocate memory for the export file name

    bool show_offset = true;
    bool export_mode = false;

    for (int i = 1; i < argc; i++)
    {
        if (!strcmp(argv[i], "-e") || !strcmp(argv[i], "--export"))
        {
            if (i + 1 < argc)
            {
                if (export_mode)
                {
                    fprintf(stderr, "Error: Already exporting to file '%s'\n", export_file);
                    return 1;
                }
                strncpy_n(export_file, argv[i + 1], MAX_PATH);
                export_ = fopen(export_file, "wb");
                if (!export_)
                {
                    last_err = GetLastError();
                    fprintf(stderr, "Error: %s (0x%lx)\n", ErrorMessage(last_err), last_err);
                    return -last_err;
                }
                export_mode = true;
                i++;
            }
            else
            {
                missing_param(argv[i]);
            }
        }
        else if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--hideoffset"))
        {
            show_offset = false;
        }
        else if (!strcmp(argv[i], "-o") || !strcmp(argv[i], "--offset"))
        {
            if (i + 1 < argc)
            {
                if ((strtoll_ret = _strtoui64(argv[i + 1], NULL, 10)) >= 0)
                {
                    offset.QuadPart = strtoll_ret; // Assuming compiler has 64-bit support
                    i++;
                }
            }
            else
            {
                missing_param(argv[i]);
            }
        }
        else if (!strcmp(argv[i], "-s") || !strcmp(argv[i], "--size"))
        {
            if (i + 1 < argc)
            {
                if ((strtol_ret = strtol(argv[i + 1], NULL, 10)) > 0)
                {
                    bufsize = strtol_ret;
                }
                else
                {
                    fprintf(stderr, "Warning: Invalid read count. Using default value (%lu).\n", bufsize);
                }
                i++;
            }
            else
            {
                missing_param(argv[i]);
            }
        }
        else if (!strcmp(argv[i], "-b") || !strcmp(argv[i], "--bytesline"))
        {
            if (i + 1 < argc)
            {
                if ((bytes_pl_temp = strtol(argv[i + 1], NULL, 10)) > 0)
                {
                    bytes_per_line = bytes_pl_temp;
                }
                else
                {
                    fprintf(stderr, "Warning: Invalid number of bytes per line. Using default value (%u).\n", bytes_per_line);
                }
                i++;
            }
            else
            {
                missing_param(argv[i]);
            }
        }
        else
        {
            if (!loaded_device)
            {
                strncpy_n(device, argv[i], MAX_PATH);
                loaded_device = true;
                is_device = GetFileAttributes(device) & FILE_ATTRIBUTE_DEVICE;
            }
            else
            {
                fprintf(stderr, "Error: Already reading file '%s'\n", device);
                return 1;
            }
        }
    }
    if (!loaded_device)
    {
        fprintf(stderr, "Error: No file specified");
        return 1;
    }
    if (is_device && bufsize % 512)
    {
        fprintf(stderr, "Warning: Disk must be read in chunks of 512 bytes. Adding %lu bytes for padding.\n", 512 - (bufsize % 512));
        bufsize += (512 - (bufsize % 512)); // Add bytes for padding
    }
    if ((offset.QuadPart % 512) && is_device)
    {
        fprintf(stderr, "Warning: Disk offset must be a multiple of 512. Adding %lld bytes for padding.\n", 512 - (offset.QuadPart % 512));
        offset.QuadPart += (512 - (offset.QuadPart % 512));
    }

    if (offset.QuadPart + bufsize > 0xffffffff)
    {
        snprintf(print_offset, sizeof(print_offset), "\n[0x%%0%dllx] ", hex_digits(offset.QuadPart + bufsize)); // Increase the offset width
    }
    diskread = CreateFile(device, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);                    // Open file/disk for reading
    if (diskread == INVALID_HANDLE_VALUE || SetFilePointerEx(diskread, offset, NULL, FILE_BEGIN) == (signed)INVALID_SET_FILE_POINTER) // Check for errors & set file offset (if specified)
    {
        last_err = GetLastError();
        fprintf(stderr, "Error: %s (0x%lx)\n", ErrorMessage(last_err), last_err);
        return -last_err;
    }
    fprintf(stderr, "Trying to read %lu bytes from '%s'...", bufsize, device);
    if (is_device)
    {
        sector_number.QuadPart = offset.QuadPart / 512; // Get the sector number
    }

    if (ReadFile(diskread, buf, bufsize, &bytes_read, NULL)) // Read the file
    {
        DWORD i = 0;
        fprintf(stderr, " %lu bytes read.", bytes_read);
        while (i < bytes_read)
        {
            memset(outbuffer, 0, (5 * bytes_per_line + 20) * sizeof(char)); // Clear the output buffer

            if (show_offset)
            {
                if (is_device)
                {
                    if (!(i & 0x1ff)) // Fast way of doing i % 512
                    {
                        printf("\n[Sector %lld]", sector_number.QuadPart++);
                    }
                }
                snprintf(outbuffer, (5 * bytes_per_line + 20) * sizeof(char), print_offset, i + offset.QuadPart); // Print the offset
            }
            else
            {
                putchar('\n');
            }
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
        fprintf(stderr, "\nFailed to read %lu bytes: %s (0x%lx)\n", bufsize, ErrorMessage(last_err), last_err);
        return last_err;
    }
    if (export_mode)
    {
        putchar('\n');                                                             // If the output is written to stdout (con) print a newline to avoid mixing outputs.
        if (fwrite(buf, bufsize, sizeof(char), export_) && fclose(export_) != EOF) // If exporting, write and close the file
        {
            printf("%lu bytes of '%s' exported successfully into '%s'.\n", bytes_read, device, export_file);
        }
        else
        {
            fprintf(stderr, "Failed to write the file %s: %s (0x%lx)\n", export_file, ErrorMessage(last_err), last_err);
        }
    }

    CloseHandle(diskread);
    return bytes_read;
}