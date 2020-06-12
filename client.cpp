#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <clocale>
#include <ctime>

using namespace std;

void *server_receive(void *data) //клиент отправляет
{
    int fingers[2];
    int scores[2];
    int sock = *((int *)data);
    char message[70];
    char status;
    int id;
    memset(message, 0, sizeof(message));

    recv(sock, &id, sizeof(id), 0); //определяем какой игрок
    if (id == 0)
    {
        cout << "Вы первый игрок!" << endl;
    }
    else
    {
        cout << "Вы второй игрок!" << endl;
    }
    while (true)
    {
        recv(sock, &status, sizeof(status), 0);
        if (status == 0)
        {
            recv(sock, scores, sizeof(scores), 0);
            recv(sock, fingers, sizeof(fingers), 0);
            if (fingers[0] != 0 && fingers[1] != 0)
            {
                cout << "Пальцы первого игрока: " << fingers[0] << endl;
                cout << "Пальцы второго игрока: " << fingers[1] << endl;
                cout << "Сумма первого игрока: " << scores[0] << endl;
                cout << "Сумма второго игрока: " << scores[1] << endl;
                memset(fingers, 0, sizeof(fingers));
            }
        }
        else
        {
            recv(sock, message, sizeof(message), 0);
            cout << message << endl;
            if (status == 2)
            {
                close(sock);
                exit(0);
            }
        }
    }
}

void client(char *ip_port, int isBot)
{
    char *ip = strchr(ip_port, ':');
    int port = atoi(ip + 1);
    ip[0] = 0;
    struct sockaddr_in addr;
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        perror("Ошибка сокета");
        return;
    }
    addr.sin_family = AF_INET;   //Internet-домен
    addr.sin_port = htons(port); // Порт
    addr.sin_addr.s_addr = inet_addr(ip_port);                                       //ip
    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) //	Установить соединение
    {
        perror("Ошибка подключения");
        return;
    }
    else if (isBot)
    {
        pthread_t thread;                                     //идентификатор потока
        pthread_create(&thread, NULL, server_receive, &sock); //создаем поток
        sleep(1);                                             //задержка перед отправкой сообщения
        while (true)
        {
            int buffer = rand() % 3 + 1;
            send(sock, &buffer, sizeof(buffer), 0);
            cout << buffer << endl;
            sleep(1);
        }
    }
    else
    {
        pthread_t thread;
        pthread_create(&thread, NULL, server_receive, &sock);
        while (true)
        {
            int buffer;
            cin >> buffer;
            if (buffer < 1 || buffer > 3)
            {
                cout << "Вы не можете ввести значение меньше 1 и больше 3, пожалуйста введит снова! " << endl;
            }
            else
            {
                send(sock, &buffer, sizeof(buffer), 0); //отправляем сообщение на сервер с нашим значением
            }
        }
    }
}

int main(int argc, char *argv[])
{
    setlocale(LC_ALL, "Russian");
    srand(time(NULL));
    if (argc < 3)
    {
        cout << "Не указан аргумент" << endl;
        return 1;
    }
    char *port = argv[1];
    int isBot = atoi(argv[2]);
    if (isBot < 0 || isBot > 1)
    {
        cout << "Неправильный аргумент, для игры с человеком введите 0, для игры с ботом 1" << endl;
        return 1;
    }
    cout << "Клиент запущен" << endl;
    client(port, isBot);
}