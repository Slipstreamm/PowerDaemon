# PowerDaemon

PowerDaemon is a lightweight HTTP server that enables remote execution of power operations—such as sleep, hibernate, shutdown, and restart—on your Windows device. By sending specific HTTP GET requests from an authorized IP address, you can control these operations conveniently.

## Features

- **Remote Control**: Execute power operations on your device via simple HTTP GET requests.
- **IP Whitelisting**: Restrict access to a specific IP address using an `allowed_ip.txt` file.
- **System Tray Integration**: Provides a system tray icon with an exit option for easy termination.

## Requirements

- **Operating System**: Windows
- **Dependencies**:
  - `psshutdown.exe` from Sysinternals (included in the releases)

## Setup Instructions

1. **Download PowerDaemon**: Obtain the latest `powerdaemon.exe` and `psshutdown.exe` from the [Releases](https://github.com/Slipstreamm/PowerDaemon/releases) section.

2. **Configure Allowed IP**:
   - Create a file named `allowed_ip.txt` in the same directory as `powerdaemon.exe`.
   - Add the IP address permitted to send requests. For example:
     ```
     192.168.1.100
     ```
     *Note*: Using `0.0.0.0` to allow all IPs is untested and not recommended.

3. **Run PowerDaemon**:
   - Place `psshutdown.exe` alongside `powerdaemon.exe`.
   - Execute `powerdaemon.exe`. An icon will appear in the system tray.

## Usage

Send HTTP GET requests to the following endpoints to perform power operations:

- `/sleep`: Puts the device to sleep.
- `/hibernate`: Hibernates the device.
- `/shutdown`: Shuts down the device.
- `/restart`: Restarts the device.

**Example**:

To put the device to sleep from an authorized IP (`192.168.1.100`):

```
GET http://192.168.1.100:8000/sleep
```

*Note*: The default port is `8000`. To change this, modify the source code accordingly.

## Compilation

To build PowerDaemon from source:

1. **Download Source Code**: Clone or download the repository.
2. **Open in Visual Studio**: Load the solution file in Visual Studio.
3. **Build the Solution**: Press `F6` to compile.

## Contributing

Contributions are welcome to enhance PowerDaemon's functionality and reliability. Feel free to submit pull requests to improve the codebase.

## Disclaimer

This project was initially created for personal use and may contain unrefined code. Use it at your own discretion.
