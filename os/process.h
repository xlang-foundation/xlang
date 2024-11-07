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

#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <memory>
#include <array>
#include <stdexcept>
#include "xpackage.h"
#include "xlang.h"

#include "service.h"

#ifdef __APPLE__
extern char** environ;  // Explicitly declare environ on macOS
#endif

namespace X {

    class Process {
    public:
        BEGIN_PACKAGE(Process)
            APISET().AddFunc<1>("Run", &Process::Run);
            APISET().AddPropWithType<std::string>("ProgName", &Process::progName);
            APISET().AddPropWithType<X::Value>("Parameter", &Process::parameters);
        END_PACKAGE

    public:
        std::string Run(bool waitToFinish) {
            std::string command = progName + " " + ConvertParameters(parameters);

            if (waitToFinish) {
                return ExecuteAndWait(command);
            }
            else {
                Execute(command); // Execute without waiting
                return ""; // No output captured
            }
        }

    private:
        std::string progName;
        X::Value parameters;

        std::string ConvertParameters(X::Value& parameters) {
            if (parameters.IsList()) {
                X::List paramList = parameters;
                std::string paramString;
                int size = (int)paramList.Size();
                for (int i = 0; i < size;i++) {
                    X::Value param = paramList[i];
                    paramString += param.ToString() + " ";
                }
                return paramString;
            }
            else {
                return parameters.ToString();
            }
        }

        std::string Execute(const std::string& command) {
            system(command.c_str()); // Execute the command without waiting
            return ""; // No output captured
        }

        std::string ExecuteAndWait(const std::string& command) {
            std::vector<char> buffer(128);
            std::string result;
#if (WIN32)
            std::unique_ptr<FILE, decltype(&_pclose)> pipe(_popen(command.c_str(), "r"), _pclose);
#else
            std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(command.c_str(), "r"), pclose);
#endif
            if (!pipe) {
                throw std::runtime_error("popen() failed!");
            }
            while (!feof(pipe.get())) {
                size_t bytesRead = fread(buffer.data(), 1, buffer.size(), pipe.get());
                result.append(buffer.data(), bytesRead);
            }
            return result;
        }

    };

    class Environ {
        public:
            BEGIN_PACKAGE(Environ)
                APISET().AddFunc<1>("get", &Environ::Get);         // Retrieve an environment variable
            APISET().AddFunc<2>("set", &Environ::Set);         // Set an environment variable
            APISET().AddFunc<1>("remove", &Environ::Remove);   // Remove an environment variable
            APISET().AddFunc<0>("keys", &Environ::Keys);       // List all environment variable keys
            APISET().AddFunc<0>("values", &Environ::Values);   // List all environment variable values
            APISET().AddFunc<0>("items", &Environ::Items);     // Get all key-value pairs as list of tuples
            END_PACKAGE

        private:
            X::Dict m_environ;  // Store environment variables as X::Dict

        public:
            // Constructor to initialize environ with system environment variables
            Environ() {
#ifdef _WIN32
                // On Windows, use GetEnvironmentStrings to get all environment variables
                LPCH envBlock = GetEnvironmentStrings();
                if (envBlock == nullptr) {
                    throw std::runtime_error("Failed to get environment strings");
                }

                // Iterate through the environment block
                for (LPCH env = envBlock; *env != '\0'; env += strlen(env) + 1) {
                    std::string entry = env;
                    size_t pos = entry.find('=');
                    if (pos != std::string::npos) {
                        std::string key = entry.substr(0, pos);
                        std::string value = entry.substr(pos + 1);
                        m_environ->Set(key, value);  // Store into X::Dict
                    }
                }

                FreeEnvironmentStrings(envBlock);  // Free the environment block
#else
                // Unix-based systems
                for (char** current = environ; *current; ++current) {
                    std::string entry = *current;
                    size_t pos = entry.find('=');
                    if (pos != std::string::npos) {
                        std::string key = entry.substr(0, pos);
                        std::string value = entry.substr(pos + 1);
                        m_environ->Set(key, value);  // Store into X::Dict
                    }
                }
#endif
            }

            // Retrieve an environment variable (like os.environ.get())
            X::Value Get(X::Value key) {
                std::string keyStr = key.ToString();
                X::Value val = m_environ.Query(keyStr);
                return val;
            }

            // Set an environment variable (like os.environ[key] = value)
            void Set(X::Value key, X::Value value) {
                std::string keyStr = key.ToString();
                std::string valueStr = value.ToString();
                m_environ->Set(keyStr, valueStr);
#ifdef _WIN32
                // For Windows, use SetEnvironmentVariableA to set the environment variable
                SetEnvironmentVariableA(keyStr.c_str(), valueStr.c_str());
#else
                // For Unix-based systems, use setenv
                setenv(keyStr.c_str(), valueStr.c_str(), 1);  // The '1' indicates overwrite
#endif
            }

            // Remove an environment variable (like del os.environ[key])
            void Remove(X::Value key) {
                std::string keyStr = key.ToString();
                m_environ["remove"](keyStr);
#ifdef _WIN32
                // For Windows, use SetEnvironmentVariable with NULL
                SetEnvironmentVariable(keyStr.c_str(), NULL);
#else
                // For Unix-based systems, use unsetenv
                unsetenv(keyStr.c_str());
#endif
            }

            // List all keys (like os.environ.keys())
            X::List Keys() {
                X::List keyList;
                m_environ->Enum([&keyList](X::Value& key, X::Value&) {
                    keyList +=key;
                    });
                return keyList;
            }

            // List all values (like os.environ.values())
            X::List Values() {
                X::List valueList;
                m_environ->Enum([&valueList](X::Value&, X::Value& value) {
                    valueList->AddItem(value);
                    });
                return valueList;
            }

            // Get all key-value pairs (like os.environ.items())
            X::List Items() {
                X::List itemList;
                m_environ->Enum([&itemList](X::Value& key, X::Value& value) {
                    X::List tuple;
                    tuple->AddItem(key);
                    tuple->AddItem(value);
                    itemList->AddItem(tuple);
                    });
                return itemList;
            }

        };

    class OSService :
        public Singleton<OSService>
    {
    private:
		X::Value m_environ;
        BEGIN_PACKAGE(OSService)
            APISET().AddClass<0, Process>("Process");
            APISET().AddClass<0, Process>("Service");
            APISET().AddClass<0, Environ>("Environ");

            APISET().AddPropL("environ",
                [](auto* pThis, X::Value v) { },
                [](auto* pThis) { 
                    if (!pThis->m_environ)
                    {
                        X::XPackageValue<X::Environ> env;
                        pThis->m_environ = env;
                    }
                    return pThis->m_environ;
                });
        END_PACKAGE
    };
} // namespace X
