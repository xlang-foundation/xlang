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

namespace X
{
    void XLangStaticLoad()
    {
        X::CreatXHost();
        X::Builtin::I().RegisterInternals();
        X::BuildOps();
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

#ifndef PICO_DEFAULT_LED_PIN
#warning blink example requires a board with a regular LED
#else

    const uint LED_PIN = PICO_DEFAULT_LED_PIN;
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    while (true) {
        gpio_put(LED_PIN, 1);
        sleep_ms(250);
        gpio_put(LED_PIN, 0);
        printf("xxx");
        sleep_ms(250);
    }
#endif
}
