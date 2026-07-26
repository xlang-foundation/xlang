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

#include <cstdint>

// ABI-safe configuration passed from xlang_eng to PyEng before CPython is
// initialized. PyEng must copy any string it needs beyond the LoadEx call.
struct PyEngConfig
{
	std::uint32_t structSize = sizeof(PyEngConfig);
	const char* pythonExecutable = nullptr;
	const char* pythonHome = nullptr;
	bool isolated = false;
};
