//
// Created by yuwenyong on 17-9-13.
//

#ifndef NET4CXX_COMMON_DEBUGGING_CRASHREPORT_H
#define NET4CXX_COMMON_DEBUGGING_CRASHREPORT_H

#include "net4cxx/common/common.h"

NS_BEGIN

class NET4CXX_COMMON_API CrashReport {
public:
    static void crashHandler(int signum);

    static void outputCrashReport();

    static void setCrashReportPath(const std::string &crashReportPath) {
        _crashReportPath = crashReportPath;
    }
protected:
    static std::string _crashReportPath;
};

NS_END

#endif //NET4CXX_COMMON_DEBUGGING_CRASHREPORT_H
