#include "p2p.h"


int recv_buffer_parser(int sockfd,U8 *buffer,U32 length,struct sockaddr_in *c_addr){    //length是传递过来的数据长度
    U8 status = buffer[PROTO_BUFFER_STATUS_IDX];                  //解析状态
    //printf("recv_buffer_parser --->status: %d\n",status);
    int client_id,other_id,index;
    int old,now;
    U8 *msg;

    switch(status){
		case PROTO_LOGIN_REQ:									  //处理登录请求
        	printf("recv login req!\n");
            old = client_count;
            now = old + 1;

            if(0 == cmpxchg((UATOMIC*)&client_count,old,now)){    //使用原子操作赋值
                printf("client_count --> %d,old:%d,now:%d\n", client_count,old,now);
                return RESULT_FAILED;
            }

            //开始登录新用户的信息
            U8 array[CLIENT_ADDR_LENGTH] = {0};                    //6字节存放地址IP：Port信息
            addr_to_array(array,c_addr);

            client_id = *(U32*)(buffer+PROTO_BUFFER_SELFID_IDX);

            printf("now:%d client:[%d],login ---> %d.%d.%d.%d:%d\n",now,client_id,
                *(unsigned char*)(&c_addr->sin_addr.s_addr), *((unsigned char*)(&c_addr->sin_addr.s_addr)+1),                                                    
                *((unsigned char*)(&c_addr->sin_addr.s_addr)+2), *((unsigned char*)(&c_addr->sin_addr.s_addr)+3),                      
                c_addr->sin_port);

            table[now].client_id = client_id;                    //获取4字节长度的用户id信息
            memcpy(table[now].addr,array,CLIENT_ADDR_LENGTH);    //获取用户的Addr地址信息

            //需要回送确认消息-----------
            deal_ack(sockfd,c_addr,buffer,length);
            break;
		case PROTO_HEARTBEAT_REQ:                                //处理心跳包请求
        	printf("recv heartbeat req!\n");
            client_id = *(unsigned int*)(buffer+PROTO_HEARTBEAT_SELFID_IDX);
            index = get_index_by_clientid(client_id);

            table[index].stamp = time_generator();

            //需要回送确认消息-----------
            deal_ack(sockfd,c_addr,buffer,length);
            break;
        case PROTO_CONNECT_REQ:                                    //处理连接请求
            client_id = *(unsigned int*)(buffer+PROTO_CONNECT_SELFID_IDX);            //获取本机id
            other_id = *(unsigned int*)(buffer+PROTO_CONNECT_OTHERID_IDX);            //获取对端id
        	printf("recv connect req from %d to %d!\n",client_id,other_id);

            deal_connect_req(sockfd,client_id,other_id);        //处理连接请求，1.向对端发送信息
            deal_connect_ack(sockfd,client_id,other_id);        //2.回送确认消息
            break;
        case PROTO_NOTIFY_ACK:                                    //处理对端发送回来的确认消息，无用
            printf("recv other notify ack message\n");
            break;
        case PROTO_MESSAGE_REQ:                              //处理要经过服务器转发的数据和p2p无法建立的时候使用
        	printf("recv message req!\n");
            msg = buffer + PROTO_MESSAGE_CONTENT_IDX;        //获取要发送的数据
            client_id = *(unsigned int*)(buffer+PROTO_MESSAGE_SELFID_IDX);
            other_id = *(unsigned int*)(buffer+PROTO_MESSAGE_OTHERID_IDX);

            printf("Client[%d] send to Other[%d]:%s\n",client_id,other_id,msg);
            deal_message_req(sockfd,other_id,buffer,length);    //进行转发
            break;
        case PROTO_MESSAGE_ACK:                                    //转发确认消息
        	printf("recv message ack!\n");
            client_id = *(unsigned int*)(buffer+PROTO_MESSAGE_SELFID_IDX);
            other_id = *(unsigned int*)(buffer+PROTO_MESSAGE_OTHERID_IDX);
            printf("Client[%d] send ack to Other[%d]\n",client_id,other_id);
            deal_message_req(sockfd,other_id,buffer,length);
            break;
    }

    return RESULT_SUCCESS;
}

int main(int argc,char *argv[]){
    int sockfd;
    int n,length;
    char buffer[BUFFER_LENGTH] = {0};
    struct sockaddr_in addr,c_addr;

    printf("UDP Server......\n");

    if(argc != 2){
        printf("Usage: %s port\n",argv[0]);
        exit(0);
    }
    
    sockfd = socket(AF_INET,SOCK_DGRAM,0);                            //获取通信socket
    if(sockfd < 0){
        printf("Failed to open udp socket!\n");
        exit(0);
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(argv[1]));                            //获取端口信息
    addr.sin_addr.s_addr = htonl(INADDR_ANY);                        //允许接收所有网卡的到达数据

    length = sizeof(addr);

    if(bind(sockfd,(struct sockaddr*)&addr,length) < 0){
        printf("Failed to bind udp socket with ip port");
        exit(0);
    }

    while(1){
        n = recvfrom(sockfd,buffer,BUFFER_LENGTH,0,(struct sockaddr*)&c_addr,&length);
        if(n > 0){
            buffer[n] = 0x0;                                        //设置结束符号
            /*
            printf("%d.%d.%d.%d:%d say:%s\n", *(unsigned char*)(&c_addr.sin_addr.s_addr),*((unsigned char*)(&c_addr.sin_addr.s_addr)+1),
                *((unsigned char*)(&c_addr.sin_addr.s_addr)+2),*((unsigned char*)(&c_addr.sin_addr.s_addr)+3),
                c_addr.sin_port, buffer);                            //打印接收到的数据信息
			*/
            int ret = recv_buffer_parser(sockfd,buffer,n,&c_addr);    //解析接收的数据,存储相关信息
            if(ret == RESULT_FAILED)
                continue;

        }else if(n == 0){
            printf("client closed!\n");
        }else{
            printf("recv error\n");
            break;
        }
    }

    return 0;
}
