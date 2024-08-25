#include <chrono>
#include <ctime>
#include <thread>
#include <iomanip>
#include <sstream>
#include <string>
#include "struct.h"

namespace X
{
    class TimeObject {
        BEGIN_PACKAGE(TimeObject)
            APISET().AddFunc<0>("time", &TimeObject::GetTime);
            APISET().AddVarFunc("localtime", &TimeObject::LocalTime); // Using AddVarFunc for default params
            APISET().AddVarFunc("gmtime", &TimeObject::GMTime);       // Using AddVarFunc for default params
            APISET().AddFunc<2>("strftime", &TimeObject::StrfTime);
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
        inline float GetTime()
        {
            auto ll = getCurMicroTimeStamp();
            return (float)ll / 1000000.0f;
        }
        inline bool LocalTime(X::XRuntime* rt, X::XObj* pContext, X::ARGS& params, X::KWARGS& kwParams, X::Value& retValue) {
            double secs = (params.size() > 0) ? (double)params[0]:0;
            std::time_t time = (secs == 0) ? std::time(nullptr) : static_cast<std::time_t>(secs);
            std::tm local_tm;
#ifdef _WIN32
            localtime_s(&local_tm, &time);
#else
            local_tm = *std::localtime(&time);
#endif
            retValue = CreateXlangStructFromTM(local_tm);
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
            retValue = CreateXlangStructFromTM(gm_tm);
            return true;
        }

        std::string StrfTime(const X::Value& structValue, const std::string& format) {
            X::Data::XlangStruct* xStruct = dynamic_cast<X::Data::XlangStruct*>(structValue.GetObj());
            if(xStruct == nullptr) {
				return "";
			}
            std::tm tm = ConvertXlangStructToTM(xStruct);
            char buffer[128];
            std::strftime(buffer, sizeof(buffer), format.c_str(), &tm);
            return std::string(buffer);
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
            if(params.size() < 2) {
                retValue = X::Value();
				return false;
			}
            X::Data::XlangStruct* startStruct = dynamic_cast<X::Data::XlangStruct*>(params[0].GetObj());
            X::Data::XlangStruct* endStruct = dynamic_cast<X::Data::XlangStruct*>(params[1].GetObj());

            std::tm startTm = ConvertXlangStructToTM(startStruct);
            std::tm endTm = ConvertXlangStructToTM(endStruct);

            std::time_t startTime = std::mktime(&startTm);
            std::time_t endTime = std::mktime(&endTm);

            double diff = std::difftime(endTime, startTime);
            retValue = X::Value(diff);
            return true;
        }

        bool TimeElapsed(X::XRuntime* rt, X::XObj* pContext, X::ARGS& params, X::KWARGS& kwParams, X::Value& retValue) {
            if (params.size() < 1) {
                retValue = X::Value();
                return false;
            }
            X::Data::XlangStruct* startStruct = dynamic_cast<X::Data::XlangStruct*>(params[0].GetObj());

            std::tm startTm = ConvertXlangStructToTM(startStruct);
            startTm.tm_isdst = -1; // Let the system determine if DST is in effect
            std::time_t startTime = std::mktime(&startTm);
            std::time_t now = std::time(nullptr);

            double elapsed = std::difftime(now, startTime);
            retValue = X::Value(elapsed);
            return true;
        }

        bool TimeAdd(X::XRuntime* rt, X::XObj* pContext, X::ARGS& params, X::KWARGS& kwParams, X::Value& retValue) {
            if (params.size() < 2) {
				retValue = X::Value();
				return false;
			}
            X::Data::XlangStruct* timeStruct = dynamic_cast<X::Data::XlangStruct*>(params[0].GetObj());
            double secondsToAdd = (double)params[1];

            std::tm tm = ConvertXlangStructToTM(timeStruct);
            std::time_t timeValue = std::mktime(&tm);

            timeValue += static_cast<std::time_t>(secondsToAdd);
            std::tm newTm;
#ifdef _WIN32
            localtime_s(&newTm, &timeValue);
#else
            newTm = *std::localtime(&timeValue);
#endif
            retValue = CreateXlangStructFromTM(newTm);
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
        X::Value CreateXlangStructFromTM(const std::tm& tm) {
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

            xStruct->Build(); // Finalize the structure layout

            char* data = xStruct->Data();
            std::memcpy(data, &tm, sizeof(std::tm)); // Copy the tm structure into the XlangStruct's data

            return X::Value(xStruct);
        }

        std::tm ConvertXlangStructToTM(X::Data::XlangStruct* xStruct) {
            std::tm tm = {};
            char* data = xStruct->Data();
            std::memcpy(&tm, data, sizeof(std::tm)); // Copy the data back into a tm structure
            return tm;
        }
    };
} // namespace X