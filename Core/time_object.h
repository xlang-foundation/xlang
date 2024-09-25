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

#include <chrono>
#include <ctime>
#include <thread>
#include <iomanip>
#include <sstream>
#include <string>
#include <cstring>
#include "struct.h"

namespace X
{
    class TimeObject {
        BEGIN_PACKAGE(TimeObject)
            APISET().AddFunc<0>("time", &TimeObject::GetTime);
            APISET().AddVarFunc("localtime", &TimeObject::LocalTime); // Using AddVarFunc for default params
            APISET().AddVarFunc("gmtime", &TimeObject::GMTime);       // Using AddVarFunc for default params
            APISET().AddVarFunc("strftime", &TimeObject::StrfTime);
            APISET().AddVarFunc("strptime", &TimeObject::StrpTime);
            APISET().AddVarFunc("timediff", &TimeObject::TimeDiff);
            APISET().AddVarFunc("timeelapsed", &TimeObject::TimeElapsed);
            APISET().AddVarFunc("timeadd", &TimeObject::TimeAdd);
            APISET().AddFunc<1>("sleep", &TimeObject::Sleep);
            APISET().AddFunc<0>("time_ns", &TimeObject::GetTimeNS);
            APISET().AddFunc<0>("perf_counter", &TimeObject::PerfCounter);
            APISET().AddFunc<0>("perf_counter_ns", &TimeObject::PerfCounterNS);
            APISET().AddFunc<0>("process_time", &TimeObject::ProcessTime);
            APISET().AddFunc<0>("process_time_ns", &TimeObject::ProcessTimeNS);
            APISET().AddFunc<0>("monotonic", &TimeObject::Monotonic);
            APISET().AddFunc<0>("monotonic_ns", &TimeObject::MonotonicNS);
            APISET().AddFunc<1>("get_clock_info", &TimeObject::GetClockInfo);
            APISET().AddFunc<0>("tzset", &TimeObject::Tzset);
        END_PACKAGE

    public:
        inline double GetTime()
        {
            auto now = std::chrono::system_clock::now();
            auto duration = now.time_since_epoch();
            double currentTimeInSeconds = std::chrono::duration_cast<std::chrono::microseconds>(duration).count() / 1e6;
            return currentTimeInSeconds;
        }

        inline bool LocalTime(X::XRuntime* rt, X::XObj* pContext, X::ARGS& params, X::KWARGS& kwParams, X::Value& retValue) {
            double secs = (params.size() > 0) ? (double)params[0] : 0;
            std::time_t time = (secs == 0) ? std::time(nullptr) : static_cast<std::time_t>(secs);
            std::tm local_tm;
#ifdef _WIN32
            localtime_s(&local_tm, &time);
#else
            local_tm = *std::localtime(&time);
#endif
            // Handle milliseconds separately
            int milliseconds = static_cast<int>((secs - static_cast<std::time_t>(secs)) * 1000);

            retValue = CreateXlangStructFromTM(local_tm, milliseconds);
            return true;
        }

        inline bool GMTime(X::XRuntime* rt, X::XObj* pContext, X::ARGS& params, X::KWARGS& kwParams, X::Value& retValue) {
            double secs = (params.size() > 0) ? (double)params[0] : 0;
            std::time_t time = (secs == 0) ? std::time(nullptr) : static_cast<std::time_t>(secs);
            std::tm gm_tm;
#ifdef _WIN32
            gmtime_s(&gm_tm, &time);
#else
            gm_tm = *std::gmtime(&time);
#endif
            // Handle milliseconds separately
            int milliseconds = static_cast<int>((secs - static_cast<std::time_t>(secs)) * 1000);

            retValue = CreateXlangStructFromTM(gm_tm, milliseconds);
            return true;
        }

        bool StrfTime(X::XRuntime* rt, X::XObj* pContext, X::ARGS& params, X::KWARGS& kwParams, X::Value& retValue) {
            if (params.size() < 2) {
                retValue = X::Value("");
                return false;
            }

            X::Value structValue = params[0];
            std::string format = params[1].ToString();
            bool includeMilliseconds = (params.size() > 2) ? static_cast<bool>(params[2]) : false;

            X::Data::XlangStruct* xStruct = dynamic_cast<X::Data::XlangStruct*>(structValue.GetObj());
            if (xStruct == nullptr) {
                retValue = X::Value("");
                return false;
            }

            int milliseconds = 0;
            std::tm tm = ConvertXlangStructToTM(xStruct, milliseconds);

            char buffer[128];
            std::strftime(buffer, sizeof(buffer), format.c_str(), &tm);
            std::string formattedTime(buffer);

            if (includeMilliseconds) {
                std::ostringstream oss;
                oss << formattedTime << '.' << std::setfill('0') << std::setw(3) << milliseconds;
                formattedTime = oss.str();
            }

            retValue = X::Value(formattedTime);
            return true;
        }

        bool StrpTime(X::XRuntime* rt, X::XObj* pContext, X::ARGS& params, 
            X::KWARGS& kwParams, X::Value& retValue) {
            std::string timeStr = params[0].ToString();
            std::string format = params[1].ToString();

            std::tm tm = {};
            std::istringstream ss(timeStr);
            ss >> std::get_time(&tm, format.c_str());

            if (ss.fail()) {
                return false;  // Parsing failed
            }

            retValue = CreateXlangStructFromTM(tm);
            return true;
        }
        bool TimeDiff(X::XRuntime* rt, X::XObj* pContext, X::ARGS& params, X::KWARGS& kwParams, X::Value& retValue) {
            if (params.size() < 2) {
                retValue = X::Value();
                return false;
            }

            X::Data::XlangStruct* startStruct = dynamic_cast<X::Data::XlangStruct*>(params[0].GetObj());
            X::Data::XlangStruct* endStruct = dynamic_cast<X::Data::XlangStruct*>(params[1].GetObj());

            int startMilliseconds = 0;
            int endMilliseconds = 0;
            std::tm startTm = ConvertXlangStructToTM(startStruct, startMilliseconds);
            std::tm endTm = ConvertXlangStructToTM(endStruct, endMilliseconds);

            std::time_t startTime = std::mktime(&startTm);
            std::time_t endTime = std::mktime(&endTm);

            // Calculate the difference in seconds
            double diffSeconds = std::difftime(endTime, startTime);

            // Calculate the difference in milliseconds
            int millisecondDifference = endMilliseconds - startMilliseconds;

            // Combine the seconds and milliseconds difference
            double totalDifference = diffSeconds + (millisecondDifference / 1000.0);

            retValue = X::Value(totalDifference);
            return true;
        }
        bool TimeElapsed(X::XRuntime* rt, X::XObj* pContext, 
            X::ARGS& params, X::KWARGS& kwParams, X::Value& retValue) {
            if (params.size() < 1) {
                retValue = X::Value();
                return false;
            }

            X::Data::XlangStruct* startStruct = dynamic_cast<X::Data::XlangStruct*>(params[0].GetObj());

            int startMilliseconds = 0;
            std::tm startTm = ConvertXlangStructToTM(startStruct, startMilliseconds);
            startTm.tm_isdst = -1; // Let the system determine if DST is in effect

            std::time_t startTime = std::mktime(&startTm);
            std::time_t now = std::time(nullptr);

            // Calculate the difference in seconds
            double elapsedSeconds = std::difftime(now, startTime);

            // Get the current milliseconds part
            auto nowDuration = std::chrono::system_clock::now().time_since_epoch();
            int currentMilliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(nowDuration).count() % 1000;

            // Calculate the elapsed milliseconds
            int millisecondDifference = currentMilliseconds - startMilliseconds;

            // Combine the seconds and milliseconds difference
            double totalElapsed = elapsedSeconds + (millisecondDifference / 1000.0);

            retValue = X::Value(totalElapsed);
            return true;
        }

   
        bool TimeAdd(X::XRuntime* rt, X::XObj* pContext, X::ARGS& params, 
            X::KWARGS& kwParams, X::Value& retValue) {
            if (params.size() < 2) {
                retValue = X::Value();
                return false;
            }

            // Get the time structure and seconds to add
            X::Data::XlangStruct* timeStruct = dynamic_cast<X::Data::XlangStruct*>(params[0].GetObj());
            double secondsToAdd = static_cast<double>(params[1]);

            int milliseconds = 0;
            std::tm tm = ConvertXlangStructToTM(timeStruct, milliseconds);  // Convert the XlangStruct to std::tm and extract milliseconds

            // Break down the secondsToAdd into full seconds and additional milliseconds
            int additionalMilliseconds = static_cast<int>((secondsToAdd - static_cast<std::time_t>(secondsToAdd)) * 1000);
            std::time_t fullSecondsToAdd = static_cast<std::time_t>(secondsToAdd);

            // Add the integral seconds to the time
            std::time_t timeValue = std::mktime(&tm);
            timeValue += fullSecondsToAdd;

            // Add the milliseconds
            milliseconds += additionalMilliseconds;

            // Handle overflow of milliseconds into seconds
            if (milliseconds >= 1000) {
                milliseconds -= 1000;
                timeValue += 1;
            }
            else if (milliseconds < 0) {
                milliseconds += 1000;
                timeValue -= 1;
            }

            // Convert the updated time back to std::tm
            std::tm newTm;
#ifdef _WIN32
            localtime_s(&newTm, &timeValue);
#else
            newTm = *std::localtime(&timeValue);
#endif

            // Create the XlangStruct with the new time and milliseconds
            retValue = CreateXlangStructFromTM(newTm, milliseconds);
            return true;
        }
        void Sleep(double seconds) {
            std::this_thread::sleep_for(std::chrono::duration<double>(seconds));
        }

        int64_t GetTimeNS() {
            return std::chrono::system_clock::now().time_since_epoch() / std::chrono::nanoseconds(1);
        }

        double PerfCounter() {
            return std::chrono::steady_clock::now().time_since_epoch() / std::chrono::seconds(1);
        }

        int64_t PerfCounterNS() {
            return std::chrono::steady_clock::now().time_since_epoch() / std::chrono::nanoseconds(1);
        }

        double ProcessTime() {
            return std::clock() / static_cast<double>(CLOCKS_PER_SEC);
        }

        int64_t ProcessTimeNS() {
            return std::clock() * (1'000'000'000 / CLOCKS_PER_SEC);
        }

        double Monotonic() {
            return std::chrono::steady_clock::now().time_since_epoch() / std::chrono::seconds(1);
        }

        int64_t MonotonicNS() {
            return std::chrono::steady_clock::now().time_since_epoch() / std::chrono::nanoseconds(1);
        }

        std::string GetClockInfo(const std::string& name) {
            if (name == "time") {
                return "System clock (wall clock time)";
            }
            else if (name == "monotonic") {
                return "Monotonic clock (cannot go backward)";
            }
            else if (name == "process_time") {
                return "CPU process time";
            }
            else if (name == "perf_counter") {
                return "Performance counter (high-resolution timer)";
            }
            else {
                return "Unknown clock";
            }
        }

        void Tzset() {
#ifdef _WIN32
            _tzset();
#else
            ::tzset();
#endif
        }

    private:
        X::Value CreateXlangStructFromTM(const std::tm& tm, int milliseconds = 0) {
            auto* xStruct = new X::Data::XlangStruct();
            xStruct->addField("tm_sec", X::Data::CType::c_int);
            xStruct->addField("tm_min", X::Data::CType::c_int);
            xStruct->addField("tm_hour", X::Data::CType::c_int);
            xStruct->addField("tm_mday", X::Data::CType::c_int);
            xStruct->addField("tm_mon", X::Data::CType::c_int);
            xStruct->addField("tm_year", X::Data::CType::c_int);
            xStruct->addField("tm_wday", X::Data::CType::c_int);
            xStruct->addField("tm_yday", X::Data::CType::c_int);
            xStruct->addField("tm_isdst", X::Data::CType::c_int);
            xStruct->addField("tm_millisec", X::Data::CType::c_int);  // Add milliseconds field

            xStruct->Build(); // Finalize the structure layout

            char* data = xStruct->Data();
            std::memcpy(data, &tm, sizeof(std::tm)); // Copy the tm structure into the XlangStruct's data
            std::memcpy(data + sizeof(std::tm), &milliseconds, sizeof(int)); // Copy the milliseconds

            return X::Value(xStruct);
        }

        std::tm ConvertXlangStructToTM(X::Data::XlangStruct* xStruct, int& milliseconds) {
            std::tm tm = {};
            char* data = xStruct->Data();
            std::memcpy(&tm, data, sizeof(std::tm)); // Copy the data back into a tm structure

            X::Value millisecondsValue;
            xStruct->GetFieldValue(9, millisecondsValue);
            milliseconds = (int)millisecondsValue;
            return tm;
        }
    };
} // namespace X