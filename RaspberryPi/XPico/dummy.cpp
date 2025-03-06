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

#include "xlang.h"
#include "xload.h"
#include "utility.h"
#include "port.h"
#include "PyEngObject.h"
#include "pyproxyobject.h"
#include "cli.h"
#include "Hosting.h"
#include "action.h"
#include "xhost_impl.h"
#include "builtin.h"
#include "AddScripts.h"
#include "Proxy.h"
#include "EventLoopInThread.h"
#include "Proxy.h"
#include "str.h"
#include "list.h"
#include "dict.h"
#include "bin.h"
#include "metascope.h"
#include "pyproxyobject.h"
#include "moduleobject.h"
#include "BlockStream.h"
#include "future.h"
#include "tensor.h"
#include "utility.h"
#include "set.h"
#include "deferred_object.h"
#include "typeobject.h"
#include "tensor.h"
#include "tensor_graph.h"
#include "manager.h"

PyEngHost* g_pPyHost = nullptr;

namespace X
{
	XLoad* g_pXload = nullptr;
	bool LoadPythonEngine()
	{
		return true;
	}
}
