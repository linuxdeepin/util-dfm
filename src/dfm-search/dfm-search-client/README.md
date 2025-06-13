# File Search Utilities

A comprehensive file search utility for Deepin File Manager that provides high-performance search capabilities using Lucene++ indexing.

## Features

- **Filename search**: Search files by their names with various search methods:
  - Simple search
  - Wildcard search (using * and ?)
  - Boolean search (multiple keywords with AND/OR operators)
  - Pinyin search (for Chinese characters)
  - File type filtering
  - File extension filtering
  - Combined search (keyword + type/extension/pinyin)

- **Content search**: Search within file contents with features like:
  - Full-text search
  - Result highlighting
  - Preview of matching content

## Search Methods

- **Indexed search**: Fast search using pre-built Lucene indexes
- **Realtime search**: On-the-fly search that doesn't require indexing

## Usage

### Command Line Interface

```bash
dfm6-search-client [options] <keyword> <search_path>
```

### Options

- `--type=<filename|content>`: Search type (default: filename)
- `--method=<indexed|realtime>`: Search method (default: indexed)
- `--query=<simple|boolean>`: Query type (default: simple)
- `--case-sensitive`: Enable case sensitivity
- `--include-hidden`: Include hidden files
- `--pinyin`: Enable pinyin search (for filename search)
- `--file-types=<types>`: Filter by file types, comma separated
- `--file-extensions=<exts>`: Filter by file extensions, comma separated
- `--max-results=<number>`: Maximum number of results
- `--max-preview=<length>`: Max content preview length (for content search)

### Examples

#### Basic filename search
```bash
dfm6-search-client "document" /home/user
```

#### Content search
```bash
dfm6-search-client --type=content "hello world" /home/user/Documents
```

#### Realtime search
```bash
dfm6-search-client --method=realtime "report" /home/user
```

#### Case-sensitive search
```bash
dfm6-search-client --case-sensitive "README" /home/user
```

#### File type filtering
```bash
dfm6-search-client --file-types=doc,pic "" /home/user
```

#### File extension filtering
```bash
dfm6-search-client --file-extensions=txt,pdf "" /home/user
```

#### Combining file types and extensions
```bash
dfm6-search-client --file-types=doc --file-extensions=docx,odt "report" /home/user
```

#### Boolean search
```bash
dfm6-search-client --query=boolean "meeting,notes,2023" /home/user/Documents
```

#### Combined search examples
```bash
# Combine keywords with file types
dfm6-search-client --file-types="dir,doc" --query=boolean "dde,file" /

# Combine keywords with pinyin
dfm6-search-client --pinyin --query=boolean "wendang,xinjian" /

# Combine keywords with file types and pinyin
dfm6-search-client --pinyin --file-types="doc,pic" --query=boolean "wen,dang" /

# Combine keywords with file extensions
dfm6-search-client --file-extensions="txt,pdf" --query=boolean "report,data" /
```

## Supported File Types

- `app`: Application files
- `archive`: Archive files
- `audio`: Audio files
- `doc`: Document files
- `pic`: Picture files
- `video`: Video files
- `dir`: Directories/folders
- `other`: Other file types

## File Extension Support

The `file-extensions` option allows filtering by specific file extensions (without the dot):
- Example: `txt`, `pdf`, `docx`, etc.

## Implementation Details

The file search functionality uses Lucene++ for indexing and searching, providing:
- High-performance search capabilities
- Support for complex queries
- Support for Chinese characters through pinyin
- Multiple filter options (file types, extensions)

The implementation follows SOLID principles with a clean separation of concerns:
- Strategy pattern for different search methods
- Dependency injection for flexible configuration
- Single Responsibility Principle with specialized classes
- Open/Closed Principle through extensible interfaces

## Development

To extend or modify this utility:
1. Clone the repository
2. Build using CMake
3. Run tests to ensure functionality
4. Implement new features following the established patterns

## License

GPL-3.0-or-later
