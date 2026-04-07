# Getting Started with XLang™

Once you have built the XLang engine from source or downloaded a release, you can start executing XLang code immediately.

## Running XLang

Navigate to the XLang executable folder and run the `xlang` command to see the available options:

```bash
$ ./xlang
xlang [-dbg] [-enable_python|-python]
      [-run_as_backend|-backend] [-event_loop]
      [-c "code,use \n as line separator"]
      [-cli]
      [file parameters]
xlang -help | -? | -h for help
```

### Example Commands

- **Running a Script File:**
  To run an XLang script file in execution mode:
  ```bash
  $ ./xlang your_script.x
  ```

- **Running Inline Code with Event Loop:**
  To evaluate a simple expression or one-liner directly from the shell:
  ```bash
  $ ./xlang -c "print('Hello, XLang!')"
  ```

- **Running in Command-Line Interface (CLI) Mode:**
  To start an interactive REPL interpreter without executing a file:
  ```bash
  $ ./xlang -cli
  ```

## Debugging in VS Code

XLang provides a native debugging experience integrated directly into Visual Studio Code.

- Install the **XLang™ plugin** from the VS Code Marketplace.
- Start your XLang instance with the debugger enabled:
  ```bash
  xlang -event_loop -dbg -enable_python
  ```
- You can either launch a new XLang process or attach to an existing application/process that uses XLang embedding.
- Open or create a `.x` file and begin stepping through your code.

> **Note:** Debugging in **VS Code** has not been exhaustively tested on **macOS**.
