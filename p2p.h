#ifndef __P2P_H__
#define __P2P_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>    //互联网地址族

#include <time.h>

//---------------------------定义数据占用空间大小---------------------------
#define CLIENT_MAX 1024                //定义客户端中与对方连接的数量
#define CLIENT_ADDR_LENGTH 6        //定义空间存放客户端地址信息，IP占4字节，端口占2字节
#define BUFFER_LENGTH 512            //定义发送和接收的缓冲区大小，512字节
#define NUMBER_ID_LENGTH 4            //定义客户端ID的长度，占4字节

//---------------------------定义协议的状态:注意响应比请求大于0x80,方便计算---------------------------
#define PROTO_LOGIN_REQ     0x01    //登录服务器的请求与响应
#define PROTO_LOGIN_ACK     0x81

#define PROTO_HEARTBEAT_REQ 0x02    //心跳包的请求与响应，防止P2P连接被NAT网关关闭
#define PROTO_HEARTBEAT_ACK 0x82
 
#define PROTO_CONNECT_REQ   0x11     //连接请求与响应，向服务端发送P2P连接请求----（服务器与本端）
#define PROTO_CONNECT_ACK   0x91

#define PROTO_NOTIFY_REQ    0x12       //服务端处理PROTO_CONNECT_REQ请求之后，发送PROTO_NOTIFY_REQ请求给对端----（服务器与对端）
#define PROTO_NOTIFY_ACK    0x92

#define PROTO_P2P_CONNECT_REQ 0x13  //对端接收到PROTO_NOTIFY_REQ请求之后，开始与本端建立P2P连接;本端接收到PROTO_P2P_CONNECT_REQ之后，回送PROTO_P2P_CONNECT_ACK给对端，双方状态机变为P2P建立完成，可以进行P2P传输
#define PROTO_P2P_CONNECT_ACK 0x93

#define PROTO_MESSAGE_REQ	  0x21  //原始数据到达（是添加了自定义的首部之后的数据）---包含服务端转发和P2P发送！！！
#define PROTO_MESSAGE_ACK     0xA1

//---------------------------定义协议的索引，和各个协议状态对应的索引位置---------------------------
#define PROTO_BUFFER_VERSION_IDX       0        //版本字段位置索引，索引0，占1个字节
#define PROTO_BUFFER_STATUS_IDX        1        //协议的状态信息，索引1,占1个字节

#define PROTO_BUFFER_LENGTH_IDX        (PROTO_BUFFER_STATUS_IDX+1)    //协议的长度字段，索引2，占2个字节
#define PROTO_BUFFER_SELFID_IDX        (PROTO_BUFFER_LENGTH_IDX+2)    //协议的本端的ID信息字段，索引4，占4个字节

//login
#define PROTO_LOGIN_SELFID_IDX         PROTO_BUFFER_SELFID_IDX        //登录时，需要添加本机的id到协议中去，在self id字段中，索引为4

//login ack
#define PROTO_LOGIN_ACK_SELFID_IDX     PROTO_BUFFER_SELFID_IDX        //回送确认消息，需要添加本端Id信息，放入self id字段，索引为4

//heartbeat
#define PROTO_HEARTBEAT_SELFID_IDX     PROTO_BUFFER_SELFID_IDX        //心跳检测，需要添加本机的id到协议中去，在self id字段中，索引为4

//heartbeat ack
#define PROTO_HEARTBEAT_ACK_SELFID_IDX PROTO_BUFFER_SELFID_IDX        //回送确认消息，需要添加本端Id信息，放入self id字段，索引为4

//connect
#define PROTO_CONNECT_SELFID_IDX       PROTO_BUFFER_SELFID_IDX        //连接相关，需要添加本端和对端的id信息，而本端的id放入self id字段，索引4
#define PROTO_CONNECT_OTHERID_IDX      (PROTO_BUFFER_SELFID_IDX+NUMBER_ID_LENGTH)        //对端的id放入other id字段，索引为8

//connect ack
#define PROTO_CONNECT_ACK_SELFID_IDX   PROTO_BUFFER_SELFID_IDX          //回送确认消息，需要添加本端Id信息，放入self id字段，索引为4
#define PROTO_CONNECT_ACK_OTHERID_IDX  (PROTO_CONNECT_ACK_SELFID_IDX+NUMBER_ID_LENGTH)  //对端的id放入other id字段，索引为8
#define PROTO_CONNECT_MESSAGE_ADDR_IDX (PROTO_CONNECT_ACK_OTHERID_IDX+NUMBER_ID_LENGTH)    //这里开始存放地址数据，索引12。占6个字节,存放地址信息！！！---本机需要获取到的地址信息，才能发送p2p请求，而之前并没有获取过这个数据，所以最好携带过去

//notify
#define PROTO_NOTIFY_SELFID_IDX        PROTO_BUFFER_SELFID_IDX          //通知对端字段，需要添加本端Id信息放入self id字段，索引为4
#define PROTO_NOTIFY_OTHERID_IDX       (PROTO_BUFFER_SELFID_IDX+NUMBER_ID_LENGTH)        //对端的id放入other id字段，索引为8
#define PROTO_NOTIFY_MESSAGE_ADDR_IDX  (PROTO_NOTIFY_OTHERID_IDX+NUMBER_ID_LENGTH)         //这里开始存放地址数据，索引12。占6个字节,存放地址信息！！！---对端需要获取到本机的地址信息，才能发送p2p请求，而之前并没有获取过这个数据，所以最好携带过去

//notify ack
#define PROTO_NOTIFY_ACK_SELFID_IDX       PROTO_BUFFER_SELFID_IDX          //回送确认消息，需要添加本端Id信息，放入self id字段，索引为4

//p2p connect
#define PROTO_P2P_CONNECT_SELFID_IDX    PROTO_BUFFER_SELFID_IDX       //P2P连接请求时，需要加入本端的Id信息放入self id这段，索引为4

//p2p connect ack
#define PROTO_P2P_CONNECT_ACK_SELFID_IDX    PROTO_BUFFER_SELFID_IDX   //P2P连接响应时，需要加入本端的Id信息放入self id这段，索引为4

//message
#define PROTO_MESSAGE_SELFID_IDX        PROTO_BUFFER_SELFID_IDX       //开始发送数据，需要添加本端Id信息，放入self id字段，索引为4
#define PROTO_MESSAGE_OTHERID_IDX       (PROTO_MESSAGE_SELFID_IDX+NUMBER_ID_LENGTH)    //需要加入对端ID信息到other id字段中，索引为8
#define PROTO_MESSAGE_CONTENT_IDX       (PROTO_MESSAGE_OTHERID_IDX+NUMBER_ID_LENGTH)   //从这里开始添加数据，索引为12

//message ack
#define PROTO_MESSAGE_ACK_SELFID_IDX    PROTO_BUFFER_SELFID_IDX       //数据发送结束，需要进行响应，索引为4
#define PROTO_MESSAGE_ACK_OTHERID_IDX   (PROTO_BUFFER_SELFID_IDX+NUMBER_ID_LENGTH)     //数据发送结束，需要进行响应，索引为4


typedef unsigned int U32;
typedef unsigned short U16;
typedef unsigned char U8;

//volatile的学习：https://www.runoob.com/w3cnote/c-volatile-keyword.html
typedef volatile long UATOMIC;    //当要求使用 volatile 声明的变量的值的时候，系统总是重新从它所在的内存读取数据，即使它前面的指令刚刚从该处读取过数据。
//可以用于实现原语操作

//定义回调函数
typedef void* (*CALLBACK)(void* arg);    

//定义返回状态
typedef enum{
    RESULT_FAILED = -1,
    RESULT_SUCCESS = 0
}RESULT;

//---------------------------定义客户端状态---------------------------
typedef enum {
    STATUS_INIT,
    STATUS_LOGIN,
    STATUS_HEARTBEAT,
    STATUS_CONNECT,
    STATUS_NOTIFY,
    STATUS_P2P_CONNECT,
    STATUS_MESSAGE
} STATUS_SET;

//---------------------------定义一个映射结构体，id==>地址和时间戳信息---------------------------
typedef struct __CLIENT_TABLE
{
    U8 addr[CLIENT_ADDR_LENGTH];    //6字节存放地址信息
    U32    client_id;                    //4字节存放客户端id
    long stamp;                        //存放时间戳信息
}client_table;

//---------------------------服务器端数据结构---------------------------
int client_count = 0;
client_table table[CLIENT_MAX] = {0};

//---------------------------客户端端数据结构---------------------------

//---------------------------服务器端函数---------------------------
/*
cmpxchg(void* ptr, int old, int new)
如果ptr和old的值一样，则把new写到ptr内存，
否则写入ptr的值到old中
整个操作是原子的。
res返回值为0（失败）或1（成功）表明cas(对比和替换）操作是否成功.
下面__asm__学习：https://www.jianshu.com/p/fa6d9d9c63b4
-----------`x++`是否是原子的？
不是，是3个指令，`取x，x+1，存入x`。
>在单处理器上，如果执行x++时，禁止多线程调度，就可以实现原子。因为单处理的多线程并发是伪并发。
在多处理器上，需要借助cpu提供的Lock功能。
锁总线。读取内存值，修改，写回内存三步期间禁止别的CPU访问总线。
同时我估计使用Lock指令锁总线的时候，OS也不会把当前线程调度走了。要是调走了，那就麻烦了。
*/
static unsigned long cmpxchg(UATOMIC* addr,unsigned long _old,unsigned long _new){
    U8 res;
    //"__asm__"表示后面的代码为内嵌汇编
    //"__volatile__"表示编译器不要优化代码，后面的指令保留原样，"volatile"是它的别名
    __asm__ volatile (
        "lock; cmpxchg %3, %1;sete %0"            //加锁以及比较和替换原子操作，按后面顺序ret 0 , addr 1 , old 2, new 3
        : "=a" (res)                            //"=a"是说要把__asm__操作结果写到__ret中
        : "m" (*addr), "a" (_old), "r" (_new)    //各个值存放的位置
        : "cc", "memory");

    return res;    //返回结果，0（失败）或1（成功）
}

//返回时间戳信息
static long time_generator(){
    static long lTimeStamp = 0;                    //局部静态变量
    static long timeStampMutex = 0;                //局部静态变量

    if(cmpxchg(&timeStampMutex,0,1)){            //注意：只有TimeStampMutex原子操作成功才行进入下面语句
        lTimeStamp = time(NULL);                //生成时间戳,精度为s
        timeStampMutex = 0;
    }

    return lTimeStamp;                            //返回时间戳信息
}


//将sockaddr地址转为array格式
static void addr_to_array(U8 *array, struct sockaddr_in *p_addr){
    //存放IP和端口，需要6个字节
    int i = 0;
    for(i = 0; i < 4; i++){
        array[i] = *((unsigned char*)(&(p_addr->sin_addr.s_addr))+i);        //获取IP，顺序存储
    }

    for(i = 0; i < 2; i++){
        array[4+i] = *((unsigned char*)(&(p_addr->sin_port))+i);            //获取Port信息
    }
}

//将array数组转为sockaddr地址格式
static void array_to_addr(U8 *array,struct sockaddr_in *p_addr){
    int i=0;
    for(i = 0;i < 4;i++){
        *((unsigned char*)(&p_addr->sin_addr.s_addr)+i) = array[i];            //获取IP，存放到sockaddr_in格式
    }
    for(i = 0;i < 2;i++){
        *((unsigned char*)(&p_addr->sin_port)+i) = array[4+i];                //获取Port，存放到sockaddr_in格式
    }
}


static int get_index_by_clientid(int client_id){
    int i = 0;
    int now_count = client_count;
    for(i = 1;i<=now_count;i++){
        if(table[i].client_id == client_id)
            return i;
    }
    return RESULT_FAILED;
}

static int deal_connect_req(int sockfd,int client_id,int other_id){
    U8 buffer[BUFFER_LENGTH] = {0};
    buffer[PROTO_BUFFER_STATUS_IDX] = PROTO_NOTIFY_REQ;                      //发送PROTO_NOTIFY_REQ请求
    buffer[PROTO_NOTIFY_SELFID_IDX] = client_id;
    buffer[PROTO_NOTIFY_OTHERID_IDX] = other_id;

    int index = get_index_by_clientid(client_id);                            //获取本端信息，一会发送给对端
    //填充数据,6字节的IP和端口信息
    memcpy(buffer+PROTO_NOTIFY_MESSAGE_ADDR_IDX,table[index].addr,CLIENT_ADDR_LENGTH);    

    index = get_index_by_clientid(other_id);                                //获取对端信息，开始发送
    //获取sockaddr信息
    struct sockaddr_in c_addr;
    c_addr.sin_family = AF_INET;
    array_to_addr(table[index].addr,&c_addr);

    int len = PROTO_NOTIFY_MESSAGE_ADDR_IDX + BUFFER_LENGTH;                //18字节，12的头部，6字节的数据
    len = sendto(sockfd,buffer,len,0,(struct sockaddr*)&c_addr,sizeof(c_addr));
    if(len < 0){
        printf("Failed in deal_connect_req, send to other peer:%d\n",other_id);
        return RESULT_FAILED;
    }

    return RESULT_SUCCESS;
}


static int deal_connect_ack(int sockfd,int client_id,int other_id){            //可以和deal_connect_req合并
	//printf("call deal_connect_ack!\n");
    U8 buffer[BUFFER_LENGTH] = {0};
    buffer[PROTO_BUFFER_STATUS_IDX] = PROTO_CONNECT_ACK;                    //回送PROTO_CONNECT_ACK
    buffer[PROTO_NOTIFY_SELFID_IDX] = client_id;
    buffer[PROTO_NOTIFY_OTHERID_IDX] = other_id;

    int index = get_index_by_clientid(other_id);                            //获取本端信息，一会发送给对端
    //填充数据,6字节的IP和端口信息
    memcpy(buffer+PROTO_CONNECT_MESSAGE_ADDR_IDX,table[index].addr,CLIENT_ADDR_LENGTH);    

    index = get_index_by_clientid(client_id);                                //获取对端信息，开始发送
    //获取sockaddr信息
    struct sockaddr_in c_addr;
    c_addr.sin_family = AF_INET;
    array_to_addr(table[index].addr,&c_addr);

    int len = PROTO_NOTIFY_MESSAGE_ADDR_IDX + BUFFER_LENGTH;                //18字节，12的头部，6字节的数据
    len = sendto(sockfd,buffer,len,0,(struct sockaddr*)&c_addr,sizeof(c_addr));
    if(len < 0){
        printf("Failed in deal_connect_ack, send to client peer:%d\n",client_id);
        return RESULT_FAILED;
    }

    return RESULT_SUCCESS;
}

static int deal_message_req(int sockfd,int other_id,U8 *buffer,int length){
    int index = get_index_by_clientid(other_id);                                //获取对端信息，开始发送
    //获取sockaddr信息
    struct sockaddr_in c_addr;
    c_addr.sin_family = AF_INET;
    array_to_addr(table[index].addr,&c_addr);
    //printf("send to peer: %d.%d.%d.%d:%d\n",table[index].addr[0],table[index].addr[1],table[index].addr[2],table[index].addr[3],c_addr.sin_port);
    int n = sendto(sockfd,buffer,length,0,(struct sockaddr*)&c_addr,sizeof(c_addr));
    if(n < 0){
        printf("Failed in deal_message_req!\n");
        return RESULT_FAILED;
    }
    return RESULT_SUCCESS;
}


static int deal_ack(int sockfd,struct sockaddr_in *c_addr,U8 *buffer,int length){        //处理通用ACK消息，原来协议+0x80即可
    buffer[PROTO_BUFFER_STATUS_IDX] += 0x80;
    int n = sendto(sockfd,buffer,length,0,(struct sockaddr*)c_addr,sizeof(*c_addr));
    if(n < 0){
        printf("Failed in deal_ack!\n");
        return RESULT_FAILED;
    }
    return RESULT_SUCCESS;
}


//---------------------------客户端函数---------------------------
static int send_login_req(int sockfd,int client_id,struct sockaddr_in *ser_addr){
    U8 buffer[BUFFER_LENGTH] = {0};            //buffer长度512

    buffer[PROTO_BUFFER_STATUS_IDX] = PROTO_LOGIN_REQ;
    *(int *)(buffer+PROTO_LOGIN_SELFID_IDX) = client_id;

    int n = PROTO_LOGIN_SELFID_IDX + NUMBER_ID_LENGTH;
    n = sendto(sockfd,buffer,n,0,(struct sockaddr*)ser_addr,sizeof(struct sockaddr_in));

    if(n < 0){
        printf("Failed to login server!\n");
        return RESULT_FAILED;
    }
    return RESULT_SUCCESS;
}

static int get_other_id(U8 *buffer,int *other_id){
    int id=0,i;
    for(i=2;buffer[i]!=':'&&buffer[i]!='\0';i++){        //还可以进行其他严格处理    
        id += id*10 + buffer[i]-'0';
    }
    *other_id = id;
    return i;                                //返回索引
}

static int send_connect_req(int sockfd,int client_id,int other_id,struct sockaddr_in *ser_addr){
    U8 buffer[BUFFER_LENGTH] = {0};            //buffer长度512

    buffer[PROTO_BUFFER_STATUS_IDX] = PROTO_CONNECT_REQ;
    *(int *)(buffer+PROTO_CONNECT_SELFID_IDX) = client_id;
    *(int *)(buffer+PROTO_CONNECT_OTHERID_IDX) = other_id;

    int n = PROTO_CONNECT_OTHERID_IDX + NUMBER_ID_LENGTH;
    n = sendto(sockfd,buffer,n,0,(struct sockaddr*)ser_addr,sizeof(struct sockaddr_in));

    if(n < 0){
        printf("Failed to login server!\n");
        return RESULT_FAILED;
    }
    return RESULT_SUCCESS;
}

static int send_message(int sockfd,int client_id,int other_id,struct sockaddr_in *addr,U8 *msg,int length){
    U8 buffer[BUFFER_LENGTH] = {0};

    buffer[PROTO_BUFFER_STATUS_IDX] = PROTO_MESSAGE_REQ;    //处理消息
    *(int*)(buffer+PROTO_MESSAGE_SELFID_IDX) = client_id;
    *(int*)(buffer+PROTO_MESSAGE_OTHERID_IDX) = other_id;

    memcpy(buffer + PROTO_MESSAGE_CONTENT_IDX,msg,length);    //初始化数据部分

    int n = PROTO_MESSAGE_CONTENT_IDX + length;
    *(U16*)(buffer+PROTO_BUFFER_LENGTH_IDX) = (U16)n;        //存放数据长度

    n = sendto(sockfd,buffer,n,0,(struct sockaddr*)addr,sizeof(struct sockaddr_in));
    if(n < 0){
        printf("Failed to send message to peer!\n");
        return RESULT_FAILED;
    } 
    return RESULT_SUCCESS;
}

static int send_p2pconnect(int sockfd,int client_id,struct sockaddr_in *p_addr){
    U8 buffer[BUFFER_LENGTH] = {0};

    buffer[PROTO_BUFFER_STATUS_IDX] = PROTO_P2P_CONNECT_REQ;
    *(int*)(buffer+PROTO_P2P_CONNECT_SELFID_IDX) = client_id;
    
    int n = PROTO_P2P_CONNECT_SELFID_IDX + NUMBER_ID_LENGTH;

    n = sendto(sockfd,buffer,n,0,(struct sockaddr*)p_addr,sizeof(struct sockaddr_in));
    if(n<0){
        printf("Failed to send p2p connect req!\n");
        return RESULT_FAILED;
    }
    return RESULT_SUCCESS;
}

static int send_p2pconnect_ack(int sockfd,int client_id,struct sockaddr_in *p_addr){
    U8 buffer[BUFFER_LENGTH] = {0};

    buffer[PROTO_BUFFER_STATUS_IDX] = PROTO_P2P_CONNECT_ACK;
    *(int*)(buffer+PROTO_P2P_CONNECT_SELFID_IDX) = client_id;

    int n = PROTO_P2P_CONNECT_SELFID_IDX + NUMBER_ID_LENGTH;
    n = sendto(sockfd,buffer,n,0,(struct sockaddr*)p_addr,sizeof(struct sockaddr_in));
    if(n < 0){
        printf("Failed to send p2p connect ack!\n");
        return RESULT_FAILED;
    }
    return RESULT_SUCCESS;
}


static int send_message_ack(int sockfd,int client_id,int other_id,struct sockaddr_in *p_addr){
    U8 buffer[BUFFER_LENGTH] = {0};

    buffer[PROTO_BUFFER_STATUS_IDX] = PROTO_MESSAGE_ACK;
    *(int*)(buffer+PROTO_MESSAGE_ACK_SELFID_IDX) = client_id;
    *(int*)(buffer+PROTO_MESSAGE_ACK_OTHERID_IDX) = other_id;

    int n=PROTO_MESSAGE_ACK_OTHERID_IDX + NUMBER_ID_LENGTH;
    n = sendto(sockfd,buffer,n,0,(struct sockaddr*)p_addr,sizeof(struct sockaddr_in));
    if(n < 0){
        printf("Failed to send message ack");
        return RESULT_FAILED;
    }
    return RESULT_SUCCESS;
}
#endif