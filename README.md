# DiskRead

## What is DiskRead?

DiskRead is a program that allows you to read raw chunks of a disk or a file.

## Usage 

Usage:  
`diskread <drive | file> [read count] [export <file>]`

Examples:  
`diskread \\.\PhysicalDrive0 512 export bootsect.bak`  
Reads first 512 bytes from physical drive 0 and exports it.

`diskread file.txt 40`  
Reads 40 bytes from file.txt and prints it in hexadecimal.

Return code:  
The number of bytes read is returned in case of success, or an error code on failure.

DiskRead can be used for either hexadecimal dumping and a backup tool for your boot sector.

Note: Due to Windows limitations, disk reading is performed in chunks of 512 bytes.
Values will be rounded up to the nearest multiple of 512.

Second note: Reading from a disk/partition requires administrator privileges. You’ll get the error "`Access is denied. (0x5)`" if you don’t have them. 

## Why use DiskRead?

DiskRead is a very light program (only 6 kB), it's really fast, powerful, portable and easy to use.
It's also a good backup tool for your boot sector in case it gets corrupted or you want to restore it.



**Copyright &copy; 2022 anic17 Software**

<img src="https://hits.seeyoufarm.com/api/count/incr/badge.svg?url=https%3A%2F%2Fgithub.com%2Fanic17%2FDiskRead&count_bg=%23FFFFFF&title_bg=%23FFFFFF&icon=&icon_color=%23FFFFFF&title=hits&edge_flat=false" height=0 width=0>
