#include "rpcServer.h"
rpcServer::rpcServer(rpcHandler *handler,int maxThreads)
{
    this->handler=handler;
    this->pool=new threadPool(maxThreads);
}
void rpcServer::free(int sockfd)
{
    if (uncompleted.find(sockfd)!=uncompleted.end())
        uncompleted.erase(sockfd);
}
void rpcServer::doRpc(int* sockfd,std::string httpRequest,std::function<void(int*)> handleClose)
{
    while (handling[*sockfd]);
    handling[*sockfd]=true;
    pool->addThread([=](void *args){     
        signal(SIGPIPE , SIG_IGN);
        rpcMessage request(0,"");
        if (uncompleted.find(*sockfd)!=uncompleted.end())
        {
        if (auto &now=uncompleted[*sockfd];now.second!=0)
        {
               now.second-=httpRequest.length();
               if (now.second>0)
               {
                   now.first+=httpRequest;
                   handling[*sockfd]=false;
                   return;
               }
                else
                {
                    now.second=0;
                    request=rpcParser::parse(now.first+httpRequest);
                    uncompleted.erase(*sockfd);
                }
        }
    }
        else 
        {
            try
            { 
                request=rpcParser::parse(httpRequest);   
                if (request.length>request.message.length())
                {
                    uncompleted[*sockfd]=std::make_pair(httpRequest,request.length-request.message.length());
                    handling[*sockfd]=false;
                    return;
                }
            }
            catch(const std::exception& e)
            {        
                JsonParser errorMessage;
                errorMessage["status"]="failed";
                rpcMessage resp(errorMessage);
                std::string badRequest=resp.toString();
                write(*sockfd,badRequest.c_str(),badRequest.length());
                handling[*sockfd]=false;
                return;
            }
        }
        JsonParser rpc(&request.message);
        auto result=handler->handleRPC(rpc);
        auto responseText=rpcMessage(result).toString();
        int wrote=write(*sockfd,responseText.c_str(),responseText.length());
        handling[*sockfd]=false;
    },&sockfd);
}
rpcServer::~rpcServer()
{
    delete pool;
}