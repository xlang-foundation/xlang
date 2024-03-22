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


    class OSService :
        public Singleton<OSService>
    {
        BEGIN_PACKAGE(OSService)
            APISET().AddClass<0, Process>("Process");
            APISET().AddClass<0, Process>("Service");
        END_PACKAGE
    };
} // namespace X
