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

//=============================================================================
// core_all.cpp - Unity build file for Core folder
// All Core .cpp files included in one translation unit for maximum inlining
//=============================================================================

// Global and runtime
#include "glob.cpp"
#include "runtime.cpp"
#include "scope.cpp"
#include "stackframe.cpp"

// Base object
#include "object.cpp"
#include "typeobject.cpp"

// Data structures
#include "list.cpp"
#include "dict.cpp"
#include "set.cpp"
#include "str.cpp"
#include "table.cpp"
#include "struct.cpp"
#include "complex.cpp"

// Function and class objects
#include "function.cpp"
#include "xclass_object.cpp"
#include "prop.cpp"

// Module and package
#include "moduleobject.cpp"
#include "package.cpp"
#include "import.cpp"
#include "namespacevar_object.cpp"

// Built-in and utilities
#include "builtin.cpp"
#include "constexpr.cpp"
#include "bin.cpp"

// Remote and proxy
#include "remote_object.cpp"
#include "pyproxyobject.cpp"
#include "deferred_object.cpp"

// Debug and error
#include "dbg.cpp"
#include "error_obj.cpp"