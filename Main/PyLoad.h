/*
Copyright (C) 2024 The XLang Foundation
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#pragma once
#include <string>
#include "PythonRuntime.h"

std::string resolve_pyeng_path(const std::string& folder,
	const std::string& pythonExecutable,
	PythonRuntimeInfo* runtimeInfo = nullptr);

// Preserves the existing system-Python behavior for callers that do not
// provide a host-selected Python executable.
inline std::string resolve_pyeng_path(const std::string& folder)
{
	return resolve_pyeng_path(folder, std::string(), nullptr);
}
