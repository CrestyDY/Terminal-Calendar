# Task Manager

![Terminal Calendar](./etc/Terminal_Calendar.png)

A terminal-based task management application with calendar visualization and CalDAV compatibility.

## Installation

### Prerequisites
- C++ compiler with C++17 support
- CMake (version 3.10 or higher)
- nlohmann/json library

### Building from Source

1. Clone the repository:
```bash
git clone [repository-url]
cd taskManager
```

2. Build using CMake:
```bash
cmake .
make
```

3. Run the application:
```bash
./src/taskmanager
```

Alternative build method using Makefile:
```bash
cd src
make
./taskmanager
```

## Features

### Task Management
- Add, edit, and delete tasks with deadlines
- Mark tasks as complete/incomplete
- View all tasks or filter by completion status
- Tasks persist between sessions in `tasks.dat`

### Calendar Integration
- Visual calendar display with task indicators
- Navigate between months to view scheduled tasks
- Customizable calendar appearance (colors, cell dimensions, formatting)
- Export tasks to ICS format for calendar applications

### CalDAV Compatibility
- Generate ICS files that can be imported into CalDAV-compatible applications
- Automatic ICS file generation on task updates
- Integration with standard calendar applications

### Configuration
- Customizable settings stored in `config.json`
- Adjust calendar colors, cell dimensions, and display preferences
- Configure task display modes (summary or full descriptions)
- Settings persist between sessions

## Usage

Launch the application and use the following commands:

### General Commands
- `nt <description> [deadline]` - Add a new task with optional deadline (YYYY-MM-DD [HH:MM])
- `ls` - List all pending tasks
- `lsa` - List all tasks including completed ones
- `ft <id>` - Mark a task as completed
- `dt <id>` - Delete a task
- `ct` - Clear all tasks
- `h` - Show help message
- `exit` - Exit the program

### Calendar Commands
- `c` - Display calendar for current month
- `n` - Display calendar for next month
- `p` - Display calendar for previous month
- `dc <Month name or number>` - Display calendar for specified month (1-12 or month name)

### Configuration Commands
- `fetch` - Get your current configurations
- `sh <height>` - Set a new height for calendar cells (5-10)
- `sw <width>` - Set a new width for calendar cells (12-40)
- `t` - Toggle whether your calendar app is opened upon adding a new task
- `stc` - Change the text color
- `scc` - Change the calendar border color
- `sec` - Change the color of events on the calendar
- `stb` - Change whether the text appears bold
- `scb` - Change whether the calendar borders appear bold
- `sort` - Configure how the events are sorted when listed

### Getting Help
Use the `h` command within the application to display all available commands and their functions.

## Data Storage

- **Tasks**: Stored in binary format in `tasks.dat`
- **Configuration**: JSON format in `config.json`
- **Calendar Export**: ICS files for calendar integration

## Platform Support

The application is cross-platform and supports:
- Linux
- macOS
- Windows

Terminal color support and ANSI escape sequences are used for enhanced visual display.

