#ifndef HEADER_NETWORKING_H
#define HEADER_NETWORKING_H

//.///////////////////////////////////////////////////////////////////////////
//    SYSTEM IMPORTS FOR SOCKETS AND THREADS     /////////////////////////////
#include <pthread.h> // 'PThreads for Win32' lib is used on windows
#include <sys/types.h>

#ifdef _WIN32
#include <WinSock2.h>
#include <ws2tcpip.h>
#elif __APPLE__ || __linux__
#include <sys/socket.h>
#include <netinet/in.h>

#include <unistd.h>
#endif

#if __linux__
#include <arpa/inet.h>
#endif
//    SYSTEM IMPORTS FOR SOCKETS AND THREADS     /////////////////////////////
//.///////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#ifdef _WIN32
#define PYTHON_CLIENT_PORT "28790"
#else
#define PYTHON_CLIENT_PORT 28501
#endif


#define TRUE 1
#define FALSE 0

#define EXIT_ERROR -1;



namespace YoloNerf {
	class NetworkHandler {
#ifdef _WIN32
		WSADATA wsa_data;
#endif

	private:
		GLubyte* pixels_out;
		uint32_t buffer_width, buffer_height;
		int buffer_size; // buffer_width * buffer_height * 3 + (sizeof(uint32_t) * 2)

		int python_socket;
		pthread_t python_socket_thread;

		char client_response[1000];

		void ReceiveClientResponse();

	public:
		NetworkHandler();
		~NetworkHandler();
		void AwaitConnections();

		void ResizePixelBuffer(uint32_t width, uint32_t height);
		void SendFrame();
	};


	static void* StaticAwaitConnections(void* instance) {
		((NetworkHandler*)instance)->AwaitConnections();
		return NULL;
	}
}

#endif
