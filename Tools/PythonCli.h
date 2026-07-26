#pragma once

namespace X::Tools
{
	// Returns true when argv names a Python project command. In that case,
	// exitCode contains the complete command result.
	bool TryRunPythonCli(int argc, char* argv[], int& exitCode);
}
