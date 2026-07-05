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

## 📂 Test Project Structure

```text
├── unit/                 # Source files for test execution (e.g., test.cpp)
├── integration/          # Build and run scripts (execute as ./scripts/build.sh)
├── CMakeLists.txt        # Main CMake configuration file
└── README.md             # This documentation file
```

### 🔨 Dynamically Generated Items (After Build/Run)

```text
├── bin/                  # Created after build; contains the compiled executables
├── build/                # Created after build; contains temporary CMake objects
├── deps/                 # Created after build; contains external dependencies
└── log/                  # Created after running run_valgrind.sh; contains Valgrind logs
```
