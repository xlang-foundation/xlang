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

#include "singleton.h"
#include <map>
#include <mutex>
#include <condition_variable>
#include <string>
#include <memory>
#include "port.h"

namespace X {
    namespace AST {
        typedef void (*LOAD)(void* pHost, X::Value module);
        class ExtensionLoader : public Singleton<ExtensionLoader> {
        public:
            bool Load(XlangRuntime* rt,std::string& loadDllName) {
                std::unique_lock<std::mutex> lock(m_mutex);

                // If the module is already being loaded, wait for it to finish
                auto& entry = m_loadingModules[loadDllName];
                if (!entry) {
                    // Initialize the entry if it doesn't exist
                    entry = std::make_shared<ModuleLoadStatus>();
                }
                auto status = entry;

                if (status->isLoading) {
                    // Wait until the loading is complete
                    status->condition.wait(lock, [&]() { return !status->isLoading; });
                    return status->isSuccess; // Return the result of the previous load
                }

                // Mark the module as being loaded
                status->isLoading = true;

                lock.unlock(); // Unlock the mutex during the actual loading process

                bool success = true;

                try {
                    // Load the library
                    void* libHandle = LOADLIB(loadDllName.c_str());
                    if (!libHandle) {
                        success = false;
                    }
                    else {
                        // Get the 'Load' function
                        LOAD load = (LOAD)GetProc(libHandle, "Load");
                        if (!load) {
                            success = false;
                        }
                        else {
                            // Create and load the module
                            ModuleObject* pModuleObj = new ModuleObject(rt->M());
                            X::Value curModule = X::Value(pModuleObj);
                            load((void*)g_pXHost, curModule);
                        }
                    }
                }
                catch (...) {
                    success = false;
                }

                // Notify waiting threads and update the status
                lock.lock();
                status->isLoading = false;
                status->isSuccess = success;
                status->condition.notify_all();

                // Clean up if loading failed
                if (!success) {
                    m_loadingModules.erase(loadDllName);
                }

                return success;
            }

        private:
            struct ModuleLoadStatus {
                bool isLoading = false;
                bool isSuccess = false;
                std::condition_variable condition;
            };

            std::mutex m_mutex; // Mutex to ensure thread safety
            std::map<std::string, std::shared_ptr<ModuleLoadStatus>> m_loadingModules; // Tracks module load status
        };
    }
}