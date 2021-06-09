#include "p2p.h"
#include <pthread.h>

static int status_machine = STATUS_INIT;    //状态机
static int client_selfid = 0x0;                //默认本端的id，需要在main方法中输入

struct sockaddr_in server_addr;                //服务端的信息

client_table p2p_clients[CLIENT_MAX];        //可以连接的P2P对端最大数量
static int p2p_count = 0;

static int buffer_parser(int sockfd,U8 *buffer,int length,struct sockaddr_in *addr){
    U8 status = buffer[PROTO_BUFFER_STATUS_IDX];    //获取状态
    U8 *msg;
    struct sockaddr_in p_addr;        //获取对端的地址信息
    //printf("buffer_parser...%d\n",status);
    switch(status){
        case PROTO_LOGIN_ACK:                //处理登录确认
            printf(" Connect Server Success\n");
            status_machine = STATUS_CONNECT;        //状态转移
            break;
        case PROTO_HEARTBEAT_ACK:
            //printf("recv heartbeat ack!\n");
            break;
        case PROTO_NOTIFY_REQ:                //处理服务端发送的NOTIFY请求
            //printf("recv notify req!\n");
            //获取对端的数据信息
            p_addr.sin_family = AF_INET;
            array_to_addr(buffer+PROTO_NOTIFY_MESSAGE_ADDR_IDX,&p_addr);
            //回复确认消息给服务器
            buffer[PROTO_BUFFER_STATUS_IDX] += 0x80;
            sendto(sockfd,buffer,PROTO_NOTIFY_MESSAGE_ADDR_IDX,0,(struct sockaddr*)&server_addr,sizeof(struct sockaddr_in));    

            status_machine = STATUS_NOTIFY;
            //开始打洞
            send_p2pconnect(sockfd,client_selfid,&p_addr);    //开始打洞！！！
            if(status_machine != STATUS_MESSAGE){			  //注意：需要进行判断，因为是异步操作，所以本机接到NOTIFY请求的时候，可能已经接到对端的P2P连接请求，状态已经变为STATUS_MESSAGE，那么我们不能再变为未就绪状态
	            status_machine = STATUS_P2P_CONNECT;
            }
            break;
        case PROTO_CONNECT_ACK:                //处理CONNECT 确认
            //printf("recv connect ack!\n");
            //获取对端的数据信息
            p_addr.sin_family = AF_INET;
            array_to_addr(buffer+PROTO_CONNECT_MESSAGE_ADDR_IDX,&p_addr);

            send_p2pconnect(sockfd,client_selfid,&p_addr);    //开始打洞！！！
            if(status_machine != STATUS_MESSAGE){			  //注意：需要进行判断，因为是异步操作，所以本机接到NOTIFY请求的时候，可能已经接到对端的P2P连接请求，状态已经变为STATUS_MESSAGE，那么我们不能再变为未就绪状态
            	status_machine = STATUS_P2P_CONNECT;
            }
            break;
        case PROTO_P2P_CONNECT_REQ:            //处理p2p连接请求---表示打洞成功，添加即可
            if(status_machine != STATUS_MESSAGE){
            	//printf("recv p2p connect req!\n");
                int now_count = p2p_count++;

                p2p_clients[now_count].stamp = time_generator();
                p2p_clients[now_count].client_id = *(int*)(buffer+PROTO_P2P_CONNECT_SELFID_IDX);
                addr_to_array(p2p_clients[now_count].addr,addr);

                send_p2pconnect_ack(sockfd,client_selfid,addr);
                status_machine = STATUS_MESSAGE;
                printf("Enter P2P Model!\n");
            }
            break;
        case PROTO_P2P_CONNECT_ACK:            //处理p2p连接确认---表示打洞成功，添加即可
            if(status_machine != STATUS_MESSAGE){
            	//printf("recv p2p connect ack!\n");
                int now_count = p2p_count++;

                p2p_clients[now_count].stamp = time_generator();
                p2p_clients[now_count].client_id = *(int*)(buffer+PROTO_P2P_CONNECT_SELFID_IDX);
                addr_to_array(p2p_clients[now_count].addr,addr);

                send_p2pconnect_ack(sockfd,client_selfid,addr);
                status_machine = STATUS_MESSAGE;
                printf("Enter P2P Model!\n");
            }
            break;
        case PROTO_MESSAGE_REQ:                //p2p数据到达
            //printf("recv p2p data....\n");

            msg = buffer + PROTO_MESSAGE_CONTENT_IDX;
            U32 other_id = *(U32*)(buffer+PROTO_MESSAGE_SELFID_IDX);
            printf("recv p2p data:%s from:%d\n",msg,other_id);

            send_message_ack(sockfd,client_selfid,other_id,addr);
            break;
        case PROTO_MESSAGE_ACK:
            //printf("peer recv message, and send ack to me!\n");
            break;
    }
}

void *recv_callback(void *arg){
    int sockfd = *(int*)arg;                //获取sockfd

    struct sockaddr_in addr;
    int length = sizeof(struct sockaddr_in);
    U8 buffer[BUFFER_LENGTH] = {0};

    while(1){
        int n = recvfrom(sockfd,buffer,BUFFER_LENGTH,0,(struct sockaddr*)&addr,&length);
        printf("recvfrom data...\n");
        if(n > 0){
            buffer[n] = 0;
            buffer_parser(sockfd,buffer,n,&addr);    //解析数据
        }else if(n == 0){
            printf("server closed\n");
            close(sockfd);
            break;
        }else{
            printf("Failed to call recvfrom\n");
            close(sockfd);
            break;
        }
    }
}

void *send_callback(void *arg){                //线程处理发送消息
    int sockfd = *(int*)arg;                //获取sockfd

    char buffer[BUFFER_LENGTH] = {0};

    while(1){
        bzero(buffer,BUFFER_LENGTH);        //置为0

        //printf("===client status====%d===\n",status_machine);
        if(status_machine == STATUS_CONNECT){
            printf("-----> please enter message(eg. C/S otherID: ...):\n");
            gets(buffer);                //获取要输入的数据
            //如果是登录状态，可以进行p2p连接或者服务器转发
            int other_id,idx;
            idx = get_other_id(buffer,&other_id);
            //printf("%d--->%d\n",client_selfid,other_id);

            if(buffer[0] == 'C'){            //开始进行P2P连接
                send_connect_req(sockfd,client_selfid,other_id,&server_addr);
            }else{
                int length = strlen(buffer);

                send_message(sockfd,client_selfid,other_id,&server_addr,buffer+idx+1,length-idx-1);    //发送给服务器进行转发
            }
	        sleep(1);	//等待建立p2p连接
        }else if(status_machine == STATUS_MESSAGE){    //可以进行P2P通信
            printf("-----> please enter p2p message:\n");
            gets(buffer);						  //获取要输入的数据
            //与最新加入的进行p2p通信
            int now_count = p2p_count;            //这个是最新的序号
            struct sockaddr_in c_addr;            //对端的地址信息

            c_addr.sin_family = AF_INET;
            array_to_addr(p2p_clients[now_count-1].addr,&c_addr);

            int length = strlen(buffer);

            send_message(sockfd,client_selfid,0,&c_addr,buffer,length);    //直接发送给对端，P2P通信
        }else if(status_machine == STATUS_NOTIFY || status_machine == STATUS_P2P_CONNECT ){
            printf("-----> please enter message(S otherID:...):\n");
            printf("status:%d\n",status_machine);
            //scanf("%s",buffer);                    //获取要输入的数据
            gets(buffer);						  //获取要输入的数据

            int length = strlen(buffer);
            
            int other_id,idx;
            idx = get_other_id(buffer,&other_id);

            send_message(sockfd,client_selfid,other_id,&server_addr,buffer+idx+1,length-idx-1);    //发送给服务器进行转发
        }
    }

}

int main(int argc,char *argv[]){
    printf("UDP Client......\n");

    if(argc != 4){
        printf("Usage: %s serverIp serverPort clientID\n",argv[0]);
        exit(0);
    }

    int sockfd = socket(AF_INET,SOCK_DGRAM,0);
    if(sockfd < 0){
        printf("Failed to create socket!\n");
        exit(0);
    }

    //创建两个线程，分别处理接收和发送信息
    pthread_t thread_id[2] = {0};
    CALLBACK cb[2] = {send_callback,recv_callback};
    
    int i;
    for(i=0;i<2;i++){
        int ret = pthread_create(&thread_id[i],NULL,cb[i],&sockfd);    //创建线程，获取线程号，传入回调方法和参数
        if(ret){
            printf("Failed to create thread!\n");
            exit(1);
        }
    }

    //主线程进行登录操作
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(argv[1]);
    server_addr.sin_port = htons(atoi(argv[2]));

    client_selfid = atoi(argv[3]);

    status_machine = STATUS_LOGIN;                                    //修改客户端当前状态
    send_login_req(sockfd,client_selfid,&server_addr);                //发送登录请求

    for(i = 0;i<2;i++){
        pthread_join(thread_id[i],NULL);                            //join子线程
    }

    return 0;
}