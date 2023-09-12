# DiskRead

## What is DiskRead?

DiskRead is a program that allows you to read raw chunks of a disk or a file. It aims to be the replacement of `xxd` or `hexdump` present on Linux, but for Windows. Furthermore, the low level WinAPI functions it uses make DiskRead able to access anything, including boot sectors and disks.

## Usage
 `diskread <drive | file> [-b <bytes per line>] [-e <export file>] [-h] [-o <offset>] [-s <read size>] [-u] [-x] [-y]`

### Switches
 `-b, --bytes <bytecount>`  
 Change the number of bytes per line displayed  
 
 `-e, --export <file>`  
 Export to a file  
 
 `-h, --hideoffset`  
 Hide the offset display  
 
 `-o, --offset <offset>`  
 Set a custom starting offset for the file  
 
 `-s, --size <read size>`  
 Read a specific amount of bytes from the file. 512 bytes are read by default  

 `-u, --uppercase`  
 Display hexadecimal values in uppercase  

 `-x, --hexadecimal`  
 Only display the hexadecimal representation  
 
 `-y, --yes`  
 Do not prompt for confirmation when exporting to a device file

### Examples
 `diskread \\.\PhysicalDrive0 -s 512 -o 0 -e bootsect.bak`  
 Reads the first 512 bytes from physical drive 0 and writes them to `bootsect.bak` (a boot sector backup).
 The boot sector can be restored if needed using `diskread bootsect.bak -s 512 -o 0 -e \\.\PhysicalDrive0`.

 > **Note**
 > Writing to a boot sector or MBR can cause serious data loss and may leave your computer unbootable if done incorrectly.
 > Option `-y` to avoid the warning message and proceed is strongly unrecommended unless in a script. 

 `diskread file.txt -s 40 -o 10 -h`  
 Prints 40 bytes from file.txt, starting to read at the 10th byte without displaying the offset.

`diskread image.png -x -u -b 12`  
Prints only 512 bytes of image.png in uppercase hexadecimal, displaying 12 bytes per line.

### Return code
 On success, the number of bytes read is returned, or a negative error value on failure.

> **Note**  
> Due to Windows limitations, both disk reading and disk offset are performed in chunks of 512 bytes. Values will be rounded up to the nearest multiple of 512.

## Why use DiskRead?

DiskRead is a tiny and fast program, yet powerful, portable, and easy to use. It's the perfect tool for backing up the boot sector, viewing the raw data of files and disks, or using it as a hexadecimal dump tool for Windows, ranging from the old Windows XP to the latest version, Windows 11.

## Contributing

If you found a bug or want to add a new feature, don't hesitate to create a [pull request](https://github.com/anic17/DiskRead/pulls) or an [issue](https://github.com/anic17/DiskRead/issues)! Contributions are greatly appreciated.

## Article

Here's an <a href="https://batch-man.com/diskread-read-raw-chunks-of-a-disk-or-a-file/">article</a> written on <a href="https://batch-man.com">Batch-Man</a> by me explaining more in-depth an older version of DiskRead.

**Copyright &copy; 2023 anic17 Software**

<img src="https://hits.seeyoufarm.com/api/count/incr/badge.svg?url=https%3A%2F%2Fgithub.com%2Fanic17%2FDiskRead&count_bg=%23FFFFFF&title_bg=%23FFFFFF&icon=&icon_color=%23FFFFFF&title=hits&edge_flat=false" height=0 width=0>
