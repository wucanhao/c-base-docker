#include "docker.hpp"
#include <iostream>
#include<string>
#include<unistd.h>
int main(int argc,char** argv){
	std::cout << "satrting the container ..." << std::endl;
	std::cout <<"this is the main pid "<< getpid() << std::endl;
	docker::container_config config;
	config.host_name = "wch";
	config.root_dir = "./image";
	config.ip = "192.168.0.100";     //容器IP地址
	config.bridge_name = "docker0";
	config.bridge_ip = "192.168.0.1";   //网桥IP地址
	docker::container container(config);
	container.start();
	std::cout <<"test" << std::endl;
	int num = 0;
	while(1){
		num++;

	}
	std::cout << "stop the container..." << std::endl;
	return 0;

}
