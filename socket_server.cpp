#include <opencv2/opencv.hpp>

//#include <WinSock2.h>
#include <WS2tcpip.h>

#include <iostream>
#include <string>
#include <stdio.h>




#pragma comment(lib,"ws2_32.lib")

std::vector<int> remote;
std::mutex mtx;

// string转wstring
std::wstring StringToWstring(const std::string str) {
	unsigned len = str.size() * 2;// 预留字节数
	setlocale(LC_CTYPE, "");     //必须调用此函数
	wchar_t* p = new wchar_t[len];// 申请一段内存存放转换后的字符串
	mbstowcs(p, str.c_str(), len);// 转换
	std::wstring str1(p);
	delete[] p;// 释放申请的内存
	return str1;
}


void Respond(int sock,std::string rec) {
	std::string message;
	if (rec == "1") {
		message = "1,204.113,-156.626,-823.226,"\
			"-7.720,250.000,206.000,180.000;1,-34.116,-136.625,"\
			"-820.571,-3.973,300.000,255.000,200.000;";
	}
	else if (rec == "2") {
		message = "2,204.113,-156.626,-823.226,"\
			"-7.720,250.000,206.000,180.000;1,-34.116,-136.625,"\
			"-820.571,-3.973,300.000,255.000,200.000;";
	}
	else if (rec == "close_server") {
		message = "closed";
		send(sock, message.c_str(), message.size(), 0);
		std::cout << "send data: " << message << " to " << sock << std::endl;
		exit(0);
	}
	else {
		message = "Unknown Command";
	}
	send(sock, message.c_str(), message.size(), 0);
	std::cout << "send data: " << message << " to " << sock << std::endl;
}




int main()
{
	std::string address = "127.0.0.1";
	int port = 9999;
	cv::FileStorage fs;
	fs.open("netInfo.json", cv::FileStorage::READ);
	if (!fs.isOpened()) {
		std::cout << "Can't open file [netInfo.json]" << std::endl;
	}
	else {
		fs["ADDRESS"] >> address;
		fs["PORT"] >> port;
	}
	fs.release();
  
	std::cout << "IP: " << address << std::endl;
	std::cout << "port: " << port << std::endl;
	std::wstring ws = StringToWstring(address);
	const WCHAR* ip = ws.c_str();

	WSADATA wsa; // 初始化socket资源.
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
		std::cout << "Failed to load Winsock" << std::endl;
		return -1;//fail.
	}
	//创建可识别套接字
	SOCKET sock_srv = socket(AF_INET, SOCK_STREAM, 0);
	// 开启一对多模式
	// 监听设置为非阻塞,recv()和accept()
	unsigned long ulMode = 1;
	ioctlsocket(sock_srv, FIONBIO, &ulMode);





	SOCKADDR_IN addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);//port

	InetPton(addr.sin_family, ip, &addr.sin_addr.S_un.S_addr);

	//bind
	int retVal = bind(sock_srv, (LPSOCKADDR)&addr, sizeof(SOCKADDR_IN));
	if (retVal == SOCKET_ERROR) {
		std::cout << "Failed bind : " << WSAGetLastError() << std::endl;
		return -1;
	}
	//listen start
	retVal = listen(sock_srv, 10); // 最大连接数
	if (retVal == SOCKET_ERROR) {
		std::cout << "Listen failed !" << std::endl;
		return -1;
	}

	//SOCKET sock_conn;
	//SOCKADDR_IN addr_client;
	//int len = sizeof(SOCKADDR);
	////等待接入请求，接受.
	//sock_conn = accept(sock_srv, (SOCKADDR*)&addr_client, &len);
	//if (sock_conn == SOCKET_ERROR) {
	//	std::cout << "Accept failed :" << WSAGetLastError() << std::endl;
	//	exit(0);
	//}

	//char cltip_buf[16];
	//inet_ntop(AF_INET, (void*)&addr_client.sin_addr, cltip_buf, 16);
	//std::cout << "客户端 [" << cltip_buf << "] 已连接!" << std::endl;


	/*thread listenThread(Listen, listenSock);*/
	std::thread listenThread([](int listenSock) {
		sockaddr_in remoteAddr{};
		int size = sizeof(remoteAddr);
		while (true) {
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			int remoteSock = accept(listenSock, (sockaddr*)(&remoteAddr), &size);
			if (remoteSock == -1)
				continue;
			if (remoteSock != INVALID_SOCKET) {
				std::lock_guard<std::mutex> mtx_locker(mtx);
				remote.emplace_back(remoteSock);
				char cltip_buf[16];
				inet_ntop(AF_INET, (void*)&remoteAddr.sin_addr, cltip_buf, 16);

				std::cout << "新客户端:" << remoteSock <<"已接入，IP: "<<cltip_buf
					<< std::endl;
				std::cout << "当前有[" << remote.size() << "]个客户端在线" << std::endl;
				for (int i = 0; i < remote.size(); i++) {
					std::cout << remote[i] << std::endl;
				}
				//Respond(remoteSock);

			}
		}
		}, sock_srv);


	int sdf = 999;
	std::thread recvThread([&]() {
		int error_code;
		constexpr int maxSize = 255;
		char buff[maxSize];
		int recvBytes;
		auto respond_lamda = [&](int sock, std::string rec) {
			std::string message;
			if (rec == "1") {
				message = "1,204.113,-156.626,-823.226,"\
					"-7.720,250.000,206.000,180.000;1,-34.116,-136.625,"\
					"-820.571,-3.973,300.000,255.000,200.000;";
				std::cout << sdf << std::endl;
			}
			else if (rec == "2") {
				message = "2,204.113,-156.626,-823.226,"\
					"-7.720,250.000,206.000,180.000;1,-34.116,-136.625,"\
					"-820.571,-3.973,300.000,255.000,200.000;";
			}
			else if (rec == "close_server") {
				message = "closed";
				send(sock, message.c_str(), message.size(), 0);
				std::cout << "send data: " << message << " to " << sock << std::endl;
				exit(0);
			}
			else {
				message = "Unknown Command";
			}
			send(sock, message.c_str(), message.size(), 0);
			std::cout << "send data: " << message << " to " << sock << std::endl;
		};


		while (true) {
			std::this_thread::sleep_for(std::chrono::milliseconds(300));

			std::lock_guard<std::mutex> mtx_locker(mtx);



			for (auto iter = remote.begin(); iter != remote.end();) {
				recvBytes = recv(*iter, buff, maxSize, 0);
				if (recvBytes == 0) {
					iter = remote.erase(iter);
					//char cltip_buf[16];
					//inet_ntop(AF_INET, *iter.sin_addr, cltip_buf, 16);
					std::cout << "客户端:" << *iter << "已断开" << std::endl;
					std::cout << "当前有[" << remote.size() << "]个客户端在线" << std::endl;
					for (int i = 0; i < remote.size(); i++) {
						std::cout << remote[i] << std::endl;
					}
					continue;
				}
				else if (recvBytes < 0) {
					error_code = WSAGetLastError();
					if (error_code == WSAEWOULDBLOCK) {
						error_code = WSAGetLastError();
						
						++iter;
					}
					else {
						iter = remote.erase(iter);
						//char cltip_buf[16];
						//inet_ntop(AF_INET, *iter.sin_addr, cltip_buf, 16);
						std::cout << "客户端:" << *iter << "已断开" << std::endl;
						std::cout << "当前有[" << remote.size() << "]个客户端在线" << std::endl;
						for (int i = 0; i < remote.size(); i++) {
							std::cout << remote[i] << std::endl;
						}
						continue;
					}
					//iter = remote.erase(iter);
					////char cltip_buf[16];
					////inet_ntop(AF_INET, *iter.sin_addr, cltip_buf, 16);
					//std::cout << "客户端:" << *iter << "已断开" << std::endl;
					//++iter;
				}
				else {
					std::cout << "[receive data]:" << std::string(buff, recvBytes) <<
						" [from sock]: " << *iter << std::endl;
					//Respond(*iter, std::string(buff, recvBytes));
					respond_lamda(*iter, std::string(buff, recvBytes));
					++iter;
				}
			}
		}
		});

	listenThread.join();
	recvThread.join();


	//释放服务器资源
	closesocket(sock_srv);
	//释放资源
	WSACleanup();
	return 0;
}

