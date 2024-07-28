/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 **/

#include "pico/stdlib.h"
#include <stdio.h>
#include "xlang.h"
#include "xload.h"
#include "utility.h"
#include "port.h"
#include <vector>
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
#include "DeviceLoop.h"
#include "struct.h"

namespace X
{
    void XLangStaticLoad()
    {
        X::CreatXHost();
        X::Builtin::I().RegisterInternals();
        X::BuildOps();
    }
    void XLangStaticUnload()
    {
        Builtin::I().Cleanup();
        Manager::I().Cleanup();
        X::AST::ModuleObject::cleanup();
        X::Data::Str::cleanup();
        X::Data::List::cleanup();
        X::Data::Dict::cleanup();
        X::Data::Future::cleanup();
        X::Data::Function::cleanup();
        X::Data::DeferredObject::cleanup();
        X::Data::TypeObject::cleanup();
        X::AST::MetaScope().I().Cleanup();
        X::Data::XlangStruct::cleanup();
        Hosting::I().Cleanup();
        G::I().Check();
        DestoryXHost();
    }
    static void XLangInternalInit()
    {
        X::Data::Str::Init();
        X::AST::ModuleObject::Init();
        X::Data::List::Init();
        X::Data::Dict::Init();
        X::Data::mSet::Init();
        X::AST::MetaScope().I().Init();
        X::Data::DeferredObject::Init();
        X::Data::TypeObject::Init();
    }

    void XLangRun(X::Config& config)
    {
        XLangInternalInit();
        Builtin::I().RegisterInternals();
        BuildOps();
        /* 
        if (config.dbg)
        {
            LoadDevopsEngine();
        }
        */

        bool HasCode = false;
        std::string code;
        const char* fileName = config.fileName;
        const char* inlineCode = "print('this is pico')";
        if (inlineCode)
        {
            Value retVal;
            HasCode = true;
            code = inlineCode;
            ReplaceAll(code, "\\n", "\n");
            ReplaceAll(code, "\\t", "\t");
            std::vector<X::Value> passInParams;
            Hosting::I().Run("inline_code", inlineCode,
                (int)strlen(inlineCode),
                passInParams,
                retVal);
        }
    }

}

int main() {

    stdio_init_all();
	
    X::Config config;
    X::XLangStaticLoad();
	X::XLangRun(config);
    
    // Initialize UART0 with TX on GPIO 0 and RX on GPIO 1
    uart_inst_t* uart = uart0;
    uint txPin = 0;
    uint rxPin = 1;

    // Create DeviceLoop instance
    DeviceLoop loop(uart, txPin, rxPin);

    //enter loop
    loop.start();

    XLangStaticUnload();
    return 0;
}
