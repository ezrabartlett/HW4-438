#include <iostream>
//#include <memory>
//#include <thread>
//#include <vector>
#include <string>
#include <unistd.h>
#include <grpc++/grpc++.h>
#include "client.h"
#include "tinysns.grpc.pb.h"
//#include "google/protobuf/empty.proto"

using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientReader;
using grpc::ClientReaderWriter;
using grpc::ClientWriter;
using grpc::Status;
using tinysns::User;
using tinysns::ReplyStatus;
using tinysns::Posting;
using tinysns::NewPosting;
using tinysns::ServerInfo;
using tinysns::TinySNS;


class Client : public IClient
{
    public:
        Client(const std::string& hname,
               const std::string& uname,
               const std::string& p)
            :hostname(hname), username(uname), port(p)
            {}
    protected:
        virtual int connectTo();
        virtual IReply processCommand(std::string& input);
        virtual void processTimeline();
    private:
        std::string hostname;
        std::string username;
        std::string port;
        
        // You can have an instance of the client stub
        // as a member variable.
    
        std::unique_ptr<TinySNS::Stub> stub_;
};

int main(int argc, char** argv) {

    std::string hostname = "localhost";
    std::string username = "default";
    std::string port = "3010";
    int opt = 0;
    while ((opt = getopt(argc, argv, "h:u:p:")) != -1){
        switch(opt) {
            case 'h':
                hostname = optarg;break;
            case 'u':
                username = optarg;break;
            case 'p':
                port = optarg;break;
            default:
                std::cerr << "Invalid Command Line Argument\n";
        }
    }

    Client myc(hostname, username, port);
    // You MUST invoke "run_client" function to start business logic
    myc.run_client();

    return 0;
}

int Client::connectTo()
{
	// ------------------------------------------------------------
    // In this function, you are supposed to create a stub so that
    // you call service methods in the processCommand/porcessTimeline
    // functions. That is, the stub should be accessible when you want
    // to call any service methods in those functions.
    // I recommend you to have the stub as
    // a member variable in your own Client class.
    // Please refer to gRpc tutorial how to create a stub.
	// ------------------------------------------------------------

    
    // Router stub
    //***** Uncomment *****
    //std::unique_ptr<TinySNS::Stub> tempStub_;
    //tempStub_ = TinySNS::NewStub(grpc::CreateChannel(hostname + ":" + port, grpc::InsecureChannelCredentials()));
    
    // ***** Delete *****
    stub_ = TinySNS::NewStub(grpc::CreateChannel(hostname + ":" + port, grpc::InsecureChannelCredentials()));

    ClientContext client_context;

    User current_user;
    
    // get master info
    ServerInfo masterServer;
    
    //****** Uncomment ******
    //Status getMasterStatus = tempStub_->getMaster(&client_context, current_user, &masterServer);
    
    //Use new masterInfo to connect to a new stub
    //****** Uncomment ******
    //stub_ = TinySNS::NewStub(grpc::CreateChannel(masterServer.ip() + ":" + masterServer.port(), grpc::InsecureChannelCredentials()));
    
    current_user.set_username(username);
    
    ReplyStatus login_status;
    Status status = stub_->Login(&client_context, current_user, &login_status);
    
    if(login_status.status()=="1"){
        std::cout << username <<": successfully logged in" << std::endl;
        return 1;
    } else if(login_status.status()=="0"){
        std::cout << "new user created" << std::endl;
        return 1;
    }else {
        std::cout << "could not establish a connection to host" << std::endl;
        return -1;
    }
}

IReply Client::processCommand(std::string& input)
{
	// ------------------------------------------------------------
	// GUIDE 1:
	// In this function, you are supposed to parse the given input
    // command and create your own message so that you call an 
    // appropriate service method. The input command will be one
    // of the followings:
	//
	// FOLLOW <username>
	// UNFOLLOW <username>
	// LIST
    // TIMELINE
	//
	// - JOIN/LEAVE and "<username>" are separated by one space.
	// ------------------------------------------------------------
	
    const char* input_copy = input.c_str();
    //std::string string_input = input.copy();
    
    IReply command_reply;
    ReplyStatus status;
    ClientContext command_context;
    User current_user;
    current_user.set_username(username);
    
    if(strncmp(input_copy, "FOLLOW", 6)==0){
        const char* target_name = input.substr(7).c_str();
        tinysns::FollowOp to_follow;
        
        to_follow.set_username(username);
        to_follow.set_follow(target_name);
        
        std::cout << "follow command" << input_copy;
        
        command_reply.grpc_status = stub_->Follow(&command_context, to_follow, &status);
    } else if(strncmp(input_copy, "UNFOLLOW", 8)==0){
        const char* target_name = input.substr(9).c_str();
        
        tinysns::FollowOp to_unfollow;
             
        to_unfollow.set_username(username);
        to_unfollow.set_follow(target_name);
        
        std::cout << (char*)"unfollow command";
        
        if (command_reply.grpc_status = stub_->Unfollow(&command_context, to_unfollow, &status) == NULL){
            std::cout << "Null dfasdf";
        }
    } else if(strncmp(input_copy, "LIST", 4)==0){}
    
    //std::cout << command_reply.grpc_status << "_Status";
    
    // Convert to comm iStatus message before returning
    if(status.status() == "0")
        command_reply.comm_status = SUCCESS;
    else if(status.status() == "1")
        command_reply.comm_status = FAILURE_ALREADY_EXISTS;
    else if(status.status() == "2")
        command_reply.comm_status = FAILURE_NOT_EXISTS;
    else if(status.status() == "3")
        command_reply.comm_status = FAILURE_INVALID_USERNAME;
    else if(status.status() == "4")
        command_reply.comm_status = FAILURE_INVALID;
    else if(status.status() == "5")
        command_reply.comm_status = FAILURE_UNKNOWN;
    else
        command_reply.comm_status = SUCCESS;
    
    
    std::cout << status.status() << "commStatus";
    
    // ------------------------------------------------------------
	// GUIDE 2:
	// Then, you should create a variable of IReply structure
	// provided by the client.h and initialize it according to
	// the result. Finally you can finish this function by returning
    // the IReply.
	// ------------------------------------------------------------
    
	// ------------------------------------------------------------
    // HINT: How to set the IReply?
    // Suppose you have "Join" service method for JOIN command,
    // IReply can be set as follow:
    // 
    //     // some codes for creating/initializing parameters for
    //     // service method
    //     IReply ire;
    //     grpc::Status status = stub_->Join(&context, /* some parameters */);
    //     ire.grpc_status = status;
    //     if (status.ok()) {
    //         ire.comm_status = SUCCESS;
    //     } else {
    //         ire.comm_status = FAILURE_NOT_EXISTS;
    //     }
    //      
    //      return ire;
    // 
    // IMPORTANT: 
    // For the command "LIST", you should set both "all_users" and 
    // "following_users" member variable of IReply.
	// ------------------------------------------------------------
    
    return command_reply;
}

bool checkForInput() {
    struct timeval timeout;
    timeout.tv_usec = 300000;

    int ready;
    fd_set rfd;
    
    int fds[1];
    
    fds[0] = STDIN_FILENO;
    int maxFd = 1 + STDIN_FILENO;
    
    FD_SET(fds[0], &rfd);

    ready = select(maxFd, &rfd, NULL, NULL, &timeout);
    
    if(ready == 0) {
        return 0;
    }
    return 1;
}

void Client::processTimeline()
{
	// ------------------------------------------------------------
    // In this function, you are supposed to get into timeline mode.
    // You may need to call a service method to communicate with
    // the server. Use getPostMessage/displayPostMessage functions
    // for both getting and displaying messages in timeline mode.
    // You should use them as you did in hw1.
	// ------------------------------------------------------------

    // ------------------------------------------------------------
    // IMPORTANT NOTICE:
    //
    // Once a user enter to timeline mode , there is no way
    // to command mode. You don't have to worry about this situation,
    // and you can terminate the client program by pressing
    // CTRL-C (SIGINT)
	// ------------------------------------------------------------
    
    while(true) {
        ClientContext client_context;

        User current_user;

        current_user.set_username(username);

        if(checkForInput()) {
           ReplyStatus input_status;
           
           NewPosting timeline_post;
           timeline_post.set_posting(getPostMessage());
           timeline_post.set_username(username);
           //timeline_post.set_posting_time()
           
           Status postStatus = stub_->PostTimeline(&client_context, timeline_post, &input_status);
           
           if(input_status.status() != "0")
               std::cout << "FAILED TO POST";
       } else {
           User current_user;
           current_user.set_username(username);
           tinysns::Posting post;
           IStatus status;
           
           std::vector<Posting> timeline_posts;
           std::unique_ptr<ClientReader<Posting>> reader(stub_->GetTimeline(&client_context, current_user));
           
           while(reader->Read(&post)) {
                timeline_posts.insert(timeline_posts.begin(), post);
           }
           for(int c = 0; c < timeline_posts.size(); c++) {
               time_t tempTime = timeline_posts.at(c).posting_time();
               displayPostMessage(timeline_posts.at(c).username(), timeline_posts.at(c).posting(), tempTime);
           }
       }
    }
}
