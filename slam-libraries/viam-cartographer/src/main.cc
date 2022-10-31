// This is an Experimental variation of cartographer. It has not yet been
// integrated into RDK.
#include <grpcpp/security/server_credentials.h>
#include <grpcpp/server_builder.h>
#include <signal.h>

#include <iostream>
#include <thread>

#include "glog/logging.h"
#include "slam_service/config.h"
#include "slam_service/slam_service.h"

void exit_loop_handler(int s) {
    LOG(INFO) << "Finishing session.\n";
    viam::b_continue_session = false;
}

int main(int argc, char** argv) {
    // glog only supports logging to files and stderr, not stdout.
    FLAGS_alsologtostderr = 1;
    google::InitGoogleLogging(argv[0]);
    struct sigaction sigIntHandler;

    sigIntHandler.sa_handler = exit_loop_handler;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;

    sigaction(SIGINT, &sigIntHandler, NULL);

    viam::SLAMServiceImpl slamService;
    viam::config::ParseAndValidateConfigParams(argc, argv, slamService);

    // Setup the SLAM gRPC server
    grpc::ServerBuilder builder;

    std::unique_ptr<int> selected_port = std::make_unique<int>(0);
    builder.AddListeningPort(slamService.port,
                             grpc::InsecureServerCredentials(),
                             selected_port.get());
    builder.RegisterService(&slamService);

    // Start the SLAM gRPC server
    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
    LOG(INFO) << "Server listening on " << *selected_port << "\n";

    LOG(INFO) << "Start mapping";
    slamService.ProcessData();
    LOG(INFO) << "Done mapping";

    while (viam::b_continue_session) {
        LOG(INFO) << "Cartographer is running\n";
        std::this_thread::sleep_for(std::chrono::microseconds(
            viam::checkForShutdownIntervalMicroseconds));
    }
}
