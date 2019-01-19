//
// Created by yuwenyong on 17-9-13.
//

#include "net4cxx/net4cxx.h"

using namespace net4cxx;


class HelloWorldApp: public CommonBootstrapper {
public:
    void onRun() override {
        NET4CXX_LOG_INFO(gAppLog, "Hello world!");
    }
};

int main (int argc, char **argv) {
   HelloWorldApp app;
   app.run(argc, argv);
   return 0;
}

