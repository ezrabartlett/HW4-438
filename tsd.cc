#include <iostream>
//#include <memory>
//#include <thread>
//#include <vector>
#include <string>
#include <unistd.h>
#include <fstream>
#include <grpc++/grpc++.h>
#include <iomanip>
#include "json.hpp"
#include "client.h"
#include "tinysns.grpc.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerReader;
using grpc::ServerReaderWriter;
using grpc::ServerWriter;
using grpc::Status;
using tinysns::User;
using tinysns::FollowOp;
using tinysns::ReplyStatus;
using tinysns::Posting;
using tinysns::NewPosting;
using tinysns::TinySNS;

using json = nlohmann::json;

struct Profile {
    User usr;
    std::string username;
    std::vector<std::string> following;
    std::vector<std::string> followers;
    std::vector<Posting> postings;
};


class TinySNSImpl final : public TinySNS::Service {
private:
    std::vector<Profile> network;
    bool network_loaded = false;

    void LoadNetwork(){
        std::ifstream input("network.json");
        std::string line;
        if(input.eof()){
            input.close();
            return;
        }
        else{
            json profiles = json::parse(input);
            Profile user;
            for(auto& el: profiles["profiles"].items()){
                User usr;
                user.username = el.value()["username"];
                usr.set_username(user.username);
                for(auto& fl : el.value()["following"].items()){
                    user.following.push_back(fl.value());
                }
                for(auto& fo : el.value()["followers"].items()){
                    user.followers.push_back(fo.value());
                }
                for(auto& po : el.value()["postings"].items()){
                    Posting post;
                    post.set_username(po.value()["username"]);
                    post.set_posting_time(po.value()["time"]);
                    post.set_posting(po.value()["posting"]);
                    user.postings.push_back(post);
                }
                network.push_back(user);
            }
        }
    }

    void SaveNetwork(){
        json profiles;
        for(int i = 0; i < network.size(); i++){
            json profile;
            profile["username"] = network[i].username;
            profile["following"] = network[i].following;
            profile["follwers"] = network[i].followers;
            for(int j = 0; j < network[i].postings.size(); j++){
                json post;
                post["username"] = network[i].postings[i].username();
                post["time"] = network[i].postings[i].posting_time();
                post["post"] = network[i].postings[i].posting();
                profile["postings"].push_back(post);
            }
            profiles["profiles"].push_back(profile);
        }
        std::ofstream out("network.json");
        out << std::setw(4) << profiles << std::endl;
    }

    bool userExists(std::string name) {
        for(int i = 0; i < network.size(); i++){
            if(network[i].username == name)
                return true;
        }
        return false;
    }
    
    int userIndex(std::string name) {
        for(int i = 0; i < network.size(); i++){
            if(network[i].username == name){
                return i;
            }
        }
        return -1;
    }

    bool existsInVector(std::string name, std::vector<std::string> vec){
        for(int i = 0; i < vec.size(); i++){
            if(vec[i] == name)
                return true;
        }
        return false;
    }

public:
    
    Status Login(ServerContext* context, const User* user, ReplyStatus* reply)  {
        if(!network_loaded){
            LoadNetwork();
            network_loaded = true;
        }
        Profile profile;
        if(userExists(user->username()))
        {
            reply->set_status("1");
            return Status::OK;
        }
        else{
            User new_user;
            new_user.set_username(user->username());
            profile.usr = new_user;
            profile.username = user->username();
            profile.followers.push_back(user->username());
            network.push_back(profile);
            reply->set_status("0");
            SaveNetwork();
        }
        return Status::OK;

    } 

    Status GetList(ServerContext* context, const User* user, ServerWriter<User>* writer)  {
        Profile profile;
        int index = userIndex(user->username());
        
        //Send all users
        for(int i = 0; i < network.size(); i++){
            User users;
            users.set_username(network[i].username);
            User& user_send = users;
            writer->Write(user_send);
        }

        //send signal 
        User all_user;
        all_user.set_username("NO_MORE_USERS");
        User& end_user = all_user;
        writer->Write(end_user);

        //send all followers
        for(int i = 0; i < network[index].followers.size(); i++){
            User follower;
            follower.set_username(network[index].followers[i]);
            User& follower_send = follower;
            writer->Write(follower_send);
        }
        return Status::OK;
    }

    Status Follow(ServerContext* context, const FollowOp* follow, ReplyStatus* reply)  {
        std::string username = follow->username();
        std::string toFollow = follow->follow();
        //following self
        if(username == toFollow){
            reply->set_status("4");
            return Status::OK;
        }
        int usrIndex = userIndex(username);
        int followIndex = userIndex(toFollow);
        //toFollow doesn't exist
        if(followIndex < 0){
            reply->set_status("3");
            return Status::OK;
        }
        //check not already following
        if(!existsInVector(toFollow, network[usrIndex].following)){
            network[usrIndex].following.push_back(toFollow);
            network[followIndex].followers.push_back(username);
            SaveNetwork();
            reply->set_status("0");
            return Status::OK;
        }
        reply->set_status("1");
        return Status::OK;
    }

    Status Unfollow(ServerContext* context, const FollowOp* unfollow, ReplyStatus* reply)  {
        std::string username = unfollow->username();
        std::string toUnfollow = unfollow->follow();
        //cant unfollow self
        if(username == toUnfollow){
            reply->set_status("3");
            return Status::OK;
        }
        
        int userInd = userIndex(username);
        //check already following
        if(!existsInVector(toUnfollow, network[userInd].following)){
            reply->set_status("4");
            return Status::OK;
        }

        //remove from user
        for(int i = 0; i < network[userInd].following.size(); i++){
            if(network[userInd].following[i] == toUnfollow){
                network[userInd].following.erase(network[userInd].following.begin()+i);
            }
        }
        
        //remove from followers
        int followIndex = userIndex(toUnfollow);
        for(int i = 0; i < network[followIndex].followers.size(); i++){
            if(network[followIndex].followers[i]==username){
                network[followIndex].followers.erase(network[followIndex].followers.begin()+i);
            }
        }
        SaveNetwork();
        reply->set_status("0");
        return Status::OK;
    }

    Status GetTimeline(ServerContext* context, const User* user, ServerWriter<Posting>* writer)  {
        std::string username = user->username();
        int index = userIndex(username);
        for(int i = 0; i < network[index].postings.size(); i++){
            if(i == 20)
                break;
            Posting& post = network[index].postings[i];    
            writer->Write(post);
        }
        return Status::OK;
    }

    Status PostTimeline(ServerContext* context, const NewPosting* posting, ReplyStatus* reply)  {
        std::string username = posting->username();
        int index = userIndex(username);

        //Create the post
        Posting newPost;
        newPost.set_username(username);
        newPost.set_posting_time(std::time(nullptr));
        newPost.set_posting(posting->posting());

        //add to users postings
        network[index].postings.push_back(newPost);

        //add to followers postings
        for(int i = 0; i < network[index].followers.size(); i++){
            int followerIndex = userIndex(network[index].followers[i]);
            network[followerIndex].postings.push_back(newPost);
        }
        reply->set_status("0");
        return Status::OK;
    }
};

int main(int argc, char** argv) {
    std::string port;
    std::string host;
    host = "0.0.0.0:";
    int opt = 0;
    while((opt = getopt(argc, argv, "p:h:")) != -1){
        switch(opt) {    
            case 'p':
                port = optarg;
                break;
            case 'h':
                host = optarg;
                break;
            default:
                std::cerr << "Invalid Command Line Argument\n";
        }
    }

    TinySNSImpl tinySNS;

    ServerBuilder builder;
    builder.AddListeningPort(host + ":" + port, grpc::InsecureServerCredentials());
    builder.RegisterService(&tinySNS);
    std::unique_ptr<Server> server(builder.BuildAndStart());
    std::cout << "Server listening on " << host + ":" + port << std::endl;
    
    server->Wait();
    return 0;
}
