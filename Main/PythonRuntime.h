/*
Copyright (C) 2024 The XLang Foundation
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0
*/

#pragma once

#include <string>

struct PythonRuntimeInfo
{
	std::string executable;
	std::string home;
	std::string abiTag;
	std::string runtimeLibrary;
};

bool query_python_runtime(const std::string& pythonExecutable,
	PythonRuntimeInfo& runtime,
	std::string& error);
