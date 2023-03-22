# DiskRead

## What is DiskRead?

DiskRead is a program that allows you to read raw chunks of a disk or a file.

## Usage 

`diskread <drive | file> [read count] [offset] [export <file>]`

### Examples
 `diskread \\.\PhysicalDrive0 512 0 export bootsect.bak`  
Reads the first 512 bytes from physical drive 0 and writes them to 'bootsect.bak'.

 `diskread file.txt 40 10`  
Prints 40 bytes from file.txt, starting to read at the 10th byte.

### Return code
 On success, the number of bytes read is returned, or a negative error value on failure.  

Note: Due to Windows limitations, both disk reading and disk offset are performed in chunks of 512 bytes.  
      Values will be rounded up to the nearest multiple of 512.  

## Why use DiskRead?

DiskRead is a tiny program (only 9 kB), it's fast, powerful, portable and easy to use. Works on Windows XP and higher.
It's also a good backup tool for your boot sector in case it gets corrupted or you want to restore it.

## Article

Here's an <a href="https://batch-man.com/diskread-read-raw-chunks-of-a-disk-or-a-file/">article</a> written on <a href="https://batch-man.com">Batch-Man</a> by me explaining more in-depth DiskRead.

**Copyright &copy; 2023 anic17 Software**

<img src="https://hits.seeyoufarm.com/api/count/incr/badge.svg?url=https%3A%2F%2Fgithub.com%2Fanic17%2FDiskRead&count_bg=%23FFFFFF&title_bg=%23FFFFFF&icon=&icon_color=%23FFFFFF&title=hits&edge_flat=false" height=0 width=0>
