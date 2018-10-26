//首先定义一个关于docker 的命名空间
#include<string>
#include<cstring>
#include<iostream>
//系统调用需要的头文件
#include<sys/wait.h>
#include<sys/mount.h>
#include<fcntl.h>
#include<unistd.h>
#include<sched.h>

//网络相关的头文件
#include<net/if.h>  //if_nametoindex
#include<arpa/inet.h> //inet_pton
#include "network.h"

#define STACK_SIZE (512*512)  //子进程栈空间的大小
using namespace std;
namespace docker{
	
	//namesapce 内部定义
	typedef int proc_stat;
	proc_stat proc_erro = -1;
	proc_stat proc_wait = 1;
	proc_stat proc_exit = 0;
	

	//定义容器的参数数据结构
	typedef struct container_config{
		std::string host_name;
		std::string root_dir;
		//网络相关参数
		std::string ip;     
		std::string bridge_name;
		std::string bridge_ip;
	}container_config;


	//定义一个容器类
	class container{
		private:
			//定义子进程栈
			char child_stack[STACK_SIZE];

			//容器参数
			container_config config;

			//进程ID
			typedef int pid;

			//保存网络设备，用于删除
			char* veth1;
			char* veth2;

			//设置容器名
			void set_hostname(){
				sethostname(this->config.host_name.c_str(),this->config.host_name.length());  //使用系统调用sethostname

			}

			//设置根目录
			void set_rootdir(){
				chdir(this->config.root_dir.c_str());   //更改当前目录
				chroot(".");    //将当前目录设置为根目录

			}
			//设置子进程独立的进程空间
			void set_procsys(){
				//挂载文件系统
				mount("none","proc","proc",0,nullptr);
				mount("none","sys","sysfs",0,nullptr);

			}

			//配置容器内部的网络
			void set_network(){
				int ifindex = if_nametoindex("eth0");
				//in_addr 表示的是32位的ipv4地址
				struct in_addr ipv4;
				struct in_addr bcast;
				struct in_addr gateway;

				//ip地址转换函数，将ip地址转为 二进制表示
				inet_pton(AF_INET,this->config.ip.c_str(),&ipv4);
				inet_pton(AF_INET,"255.255.255.0",&bcast);
				inet_pton(AF_INET,this->config.bridge_ip.c_str(),&gateway);

				//配置eth的IP地址
				lxc_ipv4_addr_add(ifindex,&ipv4,&bcast,16);

				//激活lo
				lxc_netdev_up("lo");

				//激活eth0
				lxc_netdev_up("eth0");

				//设置网关
				lxc_ipv4_gateway_add(ifindex,&gateway);

				//设置eth0的MAC地址
				char mac[18];
				new_hwaddr(mac);
				setup_hw_addr(mac,"eth0");
				
			}


		public:
			container(container_config &config1){
				config = config1;
			}
			//启动bash
			void start_bash(){
				std::string bash = "/bin/bash";
				int len =bash.length()+1;
				char* c_bash = new char [len];
				strcpy(c_bash,bash.c_str());   //安全地将string转换为c类型字符串
				char * const args[] = {c_bash,NULL};
				execv(args[0],args);   //子进程中执行bash

				delete [] c_bash;

			}
		        static int setup(void* args){
				container* this_ = reinterpret_cast<container*>(args);
				this_ -> set_hostname();
				this_ -> set_network();
				this_ -> start_bash();

				return proc_wait;
			}
			//启动容器
			void  start(){
				char veth1buf[IFNAMSIZ] = "wucanhao0x";
				char veth2buf[IFNAMSIZ] = "wucanhao0x";

				veth1 = lxc_mkifname(veth1buf);
				veth2 = lxc_mkifname(veth2buf);

				//使用create函数创建虚拟的网咯设备，一个加载到宿主机上的网桥之上，一个加载到容器之内
				lxc_veth_create(veth1,veth2);

				//设置veth1的MAC地址
				setup_private_host_hw_addr(veth1);

				//将veth1添加到网桥上
				lxc_bridge_attach(config.bridge_name.c_str(),veth1);

				//激活veth1
				lxc_netdev_up(veth1);

				//创建子进程，并返回子进程id
				pid child_pid = clone(setup,child_stack+STACK_SIZE,  //移动到栈底
						SIGCHLD|
						CLONE_NEWUTS|   //UTS namespace
						CLONE_NEWNS|    //Mount namespace
						CLONE_NEWPID|    // PID namespace
						CLONE_NEWNET,   //net namespace
						this);        //创建子进程
				std::cout << child_pid <<std::endl;
				//将veth2转移到容器内部，名称为eth0
				lxc_netdev_move_by_name(veth2,child_pid,"eth0");   //ioctl failure 19
				//waitpid(child_pid,nullptr,0); //等待子进程的退出
			}
			~container(){
				//退出时，删除与容器相关的网络设备
				lxc_netdev_delete_by_name(veth1);
				lxc_netdev_delete_by_name(veth2);
				
			}
			

	};


}
