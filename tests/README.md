## 🧪 Testing

Follow these steps to build and run the test suite:

1. **Add your test files**  
   Place your source files inside the `./unit` and/or `./integration` directory

2. **Build the project**  
   Run the appropriate script for your operating system from the root folder:
   * **Linux/Unix:** `./scripts/build.sh debug tests`
   * **Windows:** `.\scripts\build.cmd debug tests`
   
   > ℹ️ *Note: The `CMakeLists.txt` file automatically loops through all source files in t and ensure your main entry point is added to `src/test.cpp`.he `src/` directory and builds a separate executable for each one.*

3. **Run the tests**  
   Execute the run scripts to test your code:
   * **Linux/Unix:** `./scripts/run.sh` or `./scripts/run_valgrind.sh`
   * **Windows:** `.\scripts\run.cmd`
   
   > 🛡️ *Note: On Linux, `run_valgrind.sh` will automatically run Valgrind memory analysis on every executable found inside the `bin/` folder.*

---
### Steps for Visual Studio
1. Open the root **`udp_server`** folder in Visual Studio.
2. Go to the top menu: **Project > Delete Cache and Reconfigure**.
3. Switch the Solution Explorer to **CMake Targets View**.
4. Select your test executable and press **F5** to start debugging.
---

## 📂 Test Project Structure

```text
📂 udp_server
├── tests/ ................# Testing environment (GoogleTest)
│   ├── integration/ ..........# Source code of the integration tests
│   ├── unit/ .................# Source code of the unit tests
│   ├── CMakeLists.txt ........# CMake configuration for tests
│   ├── CMakeSettings.json.....# Settings for Visual Studio, automatically resolves path via \${projectDir}
│   └── README.md .............# Tests documentation file
```
---

### 🔨 Dynamically Generated Items (After Build/Run)

```text
📂 udp_server
├── bin/ ................ # Created after build; contains the compiled executables
├── build/ .............. # Created after build; contains temporary CMake objects
├── deps/ ............... # Created after build; contains external dependencies
└── log/ ................ # Created after running run_valgrind.sh; contains Valgrind logs
```