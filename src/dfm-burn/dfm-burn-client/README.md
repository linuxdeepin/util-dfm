# Optical Disc Burner Utilities

A command-line tool for optical disc operations in Deepin File Manager, powered by libisoburn.

## Features

- **Disc information**: Query device, media type, capacity, write speeds
- **Burn files**: Burn files and directories to disc with various filesystem options
- **ISO operations**: Write ISO images to disc, or dump disc content to ISO
- **Disc erase**: Erase rewritable discs (CD-RW, DVD-RW, DVD+RW, BD-RE)
- **Quality check**: Verify disc media quality with detailed statistics
- **Packet writing**: UDF packet writing for incremental file operations

## Supported Media Types

CD-ROM, CD-R, CD-RW, DVD-ROM, DVD-R, DVD-RW, DVD+R, DVD+R DL, DVD-R DL, DVD-RAM, DVD+RW, BD-ROM, BD-R, BD-RE

## Usage

```
dfm-burner <command> [options] [arguments]
```

### Commands

| Command | Description |
|---------|-------------|
| `info` | Show optical disc information |
| `burn` | Burn files to disc |
| `write-iso` | Write an ISO image to disc |
| `dump-iso` | Dump disc content to an ISO image |
| `erase` | Erase a rewritable disc |
| `check` | Check media quality |
| `pw` | Packet writing (UDF) operations |

### info — Show disc information

```
dfm-burner info [--json] <device>
```

```bash
# Show disc info
dfm-burner info /dev/sr0

# JSON output for scripting
dfm-burner info --json /dev/sr0
```

### burn — Burn files to disc

```
dfm-burner burn [options] <device> <file_or_dir> [<file_or_dir>...]
```

Options:
- `--volume-id=<name>` — Set disc label shown in file manager (default: ISOIMAGE)
- `--speed=<number>` — Write speed in CD/DVD multiplier, 0 = auto (default)

Filesystem (pick one, default is iso9660):
- `--iso9660` — Basic format, works on virtually all systems
- `--joliet` — Long filenames and CJK characters, recommended for Windows
- `--rockridge` — Preserves Linux permissions and symlinks, Linux-only
- `--udf` — Modern format, files > 2GB, Unicode names. Best for mixed-OS

Behavior:
- `--appendable` — Leave disc open to add more data later (multi-session)
- `--verify` — Read back data after burning to detect errors

```bash
# Burn a directory
dfm-burner burn /dev/sr0 /home/user/data

# Burn with custom volume label
dfm-burner burn --volume-id=BACKUP /dev/sr0 /home/user/files

# Burn with UDF and appendable (multi-session)
dfm-burner burn --udf --appendable /dev/sr0 /home/user/archive

# Burn with Joliet + Rock Ridge + verification
dfm-burner burn --joliet --rockridge --verify /dev/sr0 file1.txt file2.txt
```

### write-iso — Write ISO to disc

```
dfm-burner write-iso [--speed=<number>] <device> <iso_path>
```

```bash
dfm-burner write-iso /dev/sr0 /home/user/image.iso
dfm-burner write-iso --speed=8 /dev/sr0 /home/user/image.iso
```

### dump-iso — Dump disc to ISO

```
dfm-burner dump-iso <device> <output_path>
```

```bash
dfm-burner dump-iso /dev/sr0 /home/user/backup.iso
```

### erase — Erase disc

```
dfm-burner erase <device>
```

```bash
dfm-burner erase /dev/sr0
```

### check — Check media quality

```
dfm-burner check [--json] <device>
```

```bash
dfm-burner check /dev/sr0
dfm-burner check --json /dev/sr0
```

### pw — Packet writing (UDF)

```
dfm-burner pw <action> <device> <working_path> [additional args]
```

Actions:
- `open <device> <working_path>` — Open a packet writing session
- `close <device> <working_path>` — Close the session
- `put <device> <working_path> <file>` — Add a file to the disc
- `mv <device> <working_path> <src> <dest>` — Rename a file on the disc
- `rm <device> <working_path> <file>` — Remove a file from the disc

```bash
# Open packet writing session
dfm-burner pw open /dev/sr0 /home/user/pw-work

# Add files incrementally
dfm-burner pw put /dev/sr0 /home/user/pw-work /home/user/data.txt
dfm-burner pw put /dev/sr0 /home/user/pw-work /home/user/photo.jpg

# Rename a file on disc
dfm-burner pw mv /dev/sr0 /home/user/pw-work old_name.txt new_name.txt

# Remove a file from disc
dfm-burner pw rm /dev/sr0 /home/user/pw-work unwanted.txt

# Close session
dfm-burner pw close /dev/sr0 /home/user/pw-work
```

## Implementation

The tool wraps all public APIs of the dfm-burn library:

- `DOpticalDiscInfo` — Disc properties and capabilities
- `DOpticalDiscManager` — Burn, erase, check, and ISO operations
- `DPacketWritingController` — UDF packet writing operations

Async operations (burn, write-iso, dump-iso, erase, check) display real-time progress via Qt's event loop. Sync operations (info, pw-*) return immediately.

## License

GPL-3.0-or-later
