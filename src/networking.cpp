#include <networking.h>

using namespace YoloNerf;

NetworkHandler::NetworkHandler() :
    pixels_out(NULL),
    buffer_width(0),
    buffer_height(0),
    python_socket(-1)
{
#ifdef _WIN32
    // Initialize Winsock
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed: %d\n", iResult);
        return;
    }
#endif

    ResizePixelBuffer(1920, 1080);

    pthread_t t;
    pthread_create(&t, NULL, &StaticAwaitConnections, this);
}

NetworkHandler::~NetworkHandler() {
    if (python_socket != -1) {
        close(python_socket);
    }
}



void NetworkHandler::ResizePixelBuffer(uint32_t width, uint32_t height) {
    if (width == buffer_width && height == buffer_height) {
        return;
    }

    if (pixels_out != NULL) {
        delete pixels_out;
    }

    buffer_width = width;
    buffer_height = height;

    buffer_size = buffer_width * buffer_height * 3 + (sizeof(uint32_t) * 2);
    pixels_out = new GLubyte[buffer_size];
}

void NetworkHandler::AwaitConnections() {
#ifdef __linux__
    // Create the server socket
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);

    // define the server address
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PYTHON_CLIENT_PORT);
    server_address.sin_addr.s_addr = INADDR_ANY;

    // bind the socket to our specified IP and port
    bind(server_socket, (struct sockaddr*) &server_address, sizeof(server_address));

    listen(server_socket, 0);

    python_socket = accept(server_socket, NULL, NULL);
    printf("\n\nClient Connected!\n\n");

#endif
}

void NetworkHandler::SendFrame() {
    // Todo: ping python appilcation with one byte
    // if response is '1' -> ready_to_send_frame = true
    // else -> ready_to_send_frame = false
    if (python_socket < 0 ) {
        return;
    }

    char ping = 0x0;
    send(python_socket, (char*) &ping, sizeof(char), 0);
    recv(python_socket, &ping, sizeof(char), 0); // Blocks thread

    if (ping == 0x0) { // Acts as a lock for this function
        return; // Wait till next frame has rendered to try again
    }

    /// Fetch and package frame buffer
    glReadPixels(
        0,
        0,
        buffer_width,
        buffer_height,
        GL_RGB,
        GL_UNSIGNED_BYTE,
        (GLubyte*)((void*)pixels_out + (sizeof(uint32_t) * 2)));

    // Set package header
    *((uint32_t*)((void*)pixels_out)) = buffer_width;
    *((uint32_t*)(((void*)pixels_out) + sizeof(uint32_t))) = buffer_height;

    // TODO: Delegate code below to separate thread. glReadPixels has to be on main thread

    printf("Sending packet with size %d\n", buffer_size);
    fflush(stdout);

    // Send data over socket
    int bytes_sent = 0;
    while(bytes_sent < buffer_size) {
        bytes_sent += send(python_socket, (void*)pixels_out + bytes_sent, buffer_size - bytes_sent, 0);
        
        printf("%d bytes sent of %d\n", bytes_sent, buffer_size);
        fflush(stdout);
    }

    // Receive client response
    ReceiveClientResponse(); // Blocks thread
    printf("%s\n", client_response);
}

void NetworkHandler::ReceiveClientResponse() {
    recv(python_socket, &client_response, sizeof(uint32_t), 0);
    uint32_t package_size = *((uint32_t*) &client_response);
    recv(python_socket, &client_response, package_size, 0);
    client_response[package_size] = '\0';
}
