# UDP Server (Header-Only C++)

`⚙️ Build Debug: passing`<br>
`🚀 Build Release: passing`<br>
`📄 License: GPLv3`<br>
`📝 Language: C++`<br>
`💻 OS: Windows | Linux`

---
A multi-platform (Linux/Unix, Windows) **header-only C++ module** developed with the Standard Template Library (STL). Encapsulated in the `ns_usp_server` namespace, it features a **UDP server class with an internal, automated ring-buffer packet manager and custom exception handling**, offering an efficient, zero-configuration solution for real-time systems.
---

### 🚀 Key Features

* **Header-Only Solution**: Just include `udp_server.hpp` in your project. No external compilation, linking, or complex build systems required.
* **Cross-Platform**: Full native support for both Windows (Winsock) and Linux/Unix sockets under a unified API.
* **Automated Packet Management**: Uses an internal, thread-safe ring buffer (`ring_buffer`) to queue and synchronize incoming data automatically under the hood.
* **Robust Exception Handling**: Includes a specialized `udp_server_error` class to catch and manage socket or synchronization errors predictably.
* **Clean Architecture**: Everything is safely isolated inside the `ns_udp_server` namespace to avoid naming conflicts.


## 🚀 How to use

Follow these steps to integrate and manage in your application:

1. **Include the Header File**
   Start by including the main header file:
   ```cpp
   #include "udp_server.hpp"
   ```

2. **Create object udp server**
   Instance of object with the local port argument of the costructor
   ```cpp
   ns_udp_server::udp_server my_udp_server(1581)

3. **Start the udp server**
   Call the `start()` function to start the udp server
   > ⚠️ **Note:** This function must be called first and is **BLOCKING**.

5. **Get Data**
   Call the `get_last_packet()` function to obtain the message from udp server.
   > ⚡ **Note:** This function is **BLOCKING** only the first packet

6. **Stop the udp server**
   After calling `stop()` the usp server shall be stopped.
   * This function is **BLOCKING**.

---

### 💡 Examples
For a practical implementation, please check the `ns_base_udp_server` namespace inside the **`example/base_udp_server.cpp`** file.

---

## 📂 Project Structure

```text
├── udp_server/ ................. # Source code of the library
├── examples/ ................... # Source Code of the examples
├── tests/ ...................... # Unit tests and debugging tools
│   ├── integration ................ # Source code of the integration tests
│   ├── unit ....................... # Source code of the unit tests
│   ├── CMakeLists.txt ............. # Main CMake configuration file
│   └── README.md .................. # Project documentation file
├── scripts/ ................... # Cross-platform automation and build tools (see details below)
└── README.md .................. # Project documentation file
```

---

## 🛠️ Automation Scripts

The project provides automation tools inside the `scripts/` directory to standardize your workflow.

| Feature | Windows (`.cmd`) | Linux/macOS (`.sh`) | Description |
| :--- | :--- | :--- | :--- |
| **Orchestration** | `build.cmd` | `build.sh` | Main script to compile GoogleTest, build project tests, or generate Doxygen documentation. |
| **Execution** | `run.cmd` | `run.sh` | Runs the compiled test binaries locally. |
| **Cleanup** | `clean.cmd` | `clean.sh` | Deep cleans the workspace, wiping build caches and generated documentation. |
| **Environment** | `env.cmd` | `env.sh` | Centralizes variables, compiler flags, and local paths shared across scripts. |
| **Profiling** | *N/A* | `run_valgrind.sh` | Launches the test suite through **Valgrind** to track down memory leaks. |

---

## 🧪 Testing

Follow these steps to build and run the test suite:

1. **Add your test files**  
   Place your source files inside the `src/` directory and ensure your main entry point is added to `src/test.cpp`.

2. **Build the project**  
   Run the appropriate script for your operating system from the root folder:
   * **Linux/Unix:** `./scripts/build.sh debug`
   * **Windows:** `.\scripts\build.cmd release`
   
   > ℹ️ *Note: The `CMakeLists.txt` file automatically loops through all source files in the `test/unit`,`test/integration` or `example/` directory and builds a separate executable for each one.*

3. **Run the tests**  
   Execute the run scripts to test your code:
   * **Linux/Unix:** `./scripts/run.sh` or `./scripts/run_valgrind.sh`
   * **Windows:** `.\scripts\run.cmd`
   
   > 🛡️ *Note: On Linux, `run_valgrind.sh` will automatically run Valgrind memory analysis on every executable found inside the `bin/` folder.*

---

### 🔨 Dynamically Generated Items (After Build/Run)

```text
├── bin/ ....................... # Created after build; contains the compiled executables
├── build/ ..................... # Created after build; contains temporary CMake objects
├── deps/ ...................... # Created after build; contains external dependencies
└── log/ ....................... # Created after running run_valgrind.sh; contains Valgrind logs
```

---

## 📄 License

This project is licensed under the **GNU General Public License v3.0** - see the [LICENSE](LICENSE) file for details.
