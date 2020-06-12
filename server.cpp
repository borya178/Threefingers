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

using namespace std;

#define MAX_CLIENTS 2

int fingers[2];
int scores[2];
int roundCounter = 0;
bool poor_player[2] = {false, false};

struct Client
{
    int socket;
    int id;
    int round_count;
    pthread_t thread;
};
Client *clients[MAX_CLIENTS] = {NULL};

int findSocket() //поиск свободного сокета
{
    for (size_t i = 0; i < MAX_CLIENTS; ++i)
    {
        if (clients[i] == NULL)
        {
            return i;
        }
    }
    return -1;
}

void *client_receive(void *data) //Сервер отправляет
{
    Client *player = (Client *)data;
    send(clients[player->id]->socket, &player->id, sizeof(player->id), 0); //отправляем номер игрока клиенту
    int summa;
    int round_count = player->round_count;

    while (findSocket() != -1)
    {
        sleep(1);
    }
    

    while (true)
    {
        int buffer;
        char message[70];
        if (roundCounter == round_count) //проверка на окончание игры
        {
            if (scores[0] > scores[1])
            {
                strcpy(message, "Поздравляем с победой первого игрока!");
            }
            else if (scores[1] > scores[0])
            {
                strcpy(message, "Поздравляем с победой второго игрока!");
            }
            else if (scores[0] = scores[1])
            {
                strcpy(message, "Ничья");
            }
            for (int i = 0; i < MAX_CLIENTS; i++)
            {
                char status = 2;
                if (clients[i] != NULL)
                {
                    send(clients[i]->socket, &status, sizeof(status), 0);  //отправляем статус = завершение игры
                    send(clients[i]->socket, message, sizeof(message), 0); //отправляем сообщение с победителем
                }
            }
            close(clients[0]->socket); //закрывает соединение с сокетом
            close(clients[1]->socket); //закрывает соединение с сокетом
            exit(0);                   //выход
        }
        int n;
        do
        {
            n = recv(player->socket, &buffer, sizeof(int), 0);
        } while (poor_player[player->id] && n != 0);
        poor_player[player->id] = true;
        if (n == 0)
        {
            return 0;
        }
        //cout << "Полученно сообщение от клиента № " << player->id << ": " << buffer << endl;
        char status = 1;
        if (player->id == 0)
        {
            fingers[0] = buffer;
            strcpy(message, "Первый игрок сделал ход и ждёт вас!");
            if (!poor_player[1])
            {
                send(clients[1]->socket, &status, sizeof(status), 0);
                send(clients[1]->socket, message, sizeof(message), 0);
            }
        }
        else if (player->id == 1)
        {
            fingers[1] = buffer;
            strcpy(message, "Второй игрок сделал ход и ждёт вас!");
            if (!poor_player[0])
            {
                send(clients[0]->socket, &status, sizeof(status), 0);
                send(clients[0]->socket, message, sizeof(message), 0);
            }
        }
        if (fingers[0] != 0 && fingers[1] != 0)
        {
            poor_player[0] = false;
            poor_player[1] = false;
            summa = fingers[0] + fingers[1];
            if (summa % 2 == 0)
            {
                scores[1] += summa;
                scores[0] -= summa;
                if (scores[0] < 0)
                {
                    scores[0] = 0;
                }
            }
            else
            {
                scores[0] += summa;
                scores[1] -= summa;
                if (scores[1] < 0)
                {
                    scores[1] = 0;
                }
            }
            for (int i = 0; i < MAX_CLIENTS; i++)
            {
                char status = 0;
                if (clients[i] != NULL)
                {
                    send(clients[i]->socket, &status, sizeof(status), 0);
                    send(clients[i]->socket, scores, sizeof(scores), 0);
                    send(clients[i]->socket, fingers, sizeof(fingers), 0);
                }
            }
            memset(fingers, 0, sizeof(fingers));
            roundCounter++;
        }
    }
}

void server(int port, int n)
{
    for (size_t i = 0; i < MAX_CLIENTS; ++i)
    {
        clients[i] = NULL;
    }
    struct sockaddr_in addr;
    int listener = socket(AF_INET, SOCK_STREAM, 0); //В аргументах, первый параметр домен, второй канал связи, третий протокол для канала связи
    if (listener < 0)
    {
        perror("Ошибка сокета");
        return;
    }
    addr.sin_family = AF_INET;                                      //Internet-домен
    addr.sin_port = htons(port);                                    //Порт, Функция htonl() преобразует узловой порядок расположения байтов положительного целого hostlong в сетевой порядок расположения байтов
    addr.sin_addr.s_addr = htonl(INADDR_ANY);                       //INADDR_ANY - это "any address"
    if (bind(listener, (struct sockaddr *)&addr, sizeof(addr)) < 0) //Связать сокет с IP-адресом и портом
    {
        perror("Ошибка бинда");
        return;
    }
    listen(listener, 1); //Объявить о желании принимать соединения. Слушает порт и ждет когда будет установлено соединение

    while (true)
    {
        int sock = accept(listener, NULL, NULL); //Принять запрос на установку соединения
        if (sock < 0)
        {
            perror("Accept error");
        }
        else //создаем нового клиента
        {
            Client *user = new Client;
            user->socket = sock;
            int id = findSocket();
            user->id = id;
            user->round_count = n;
            clients[id] = user;
            pthread_create(&user->thread, NULL, client_receive, user);
        }
    }
}

int main(int argc, char *argv[])
{
    setlocale(LC_ALL, "Russian");
    if (argc < 3)
    {
        cout << "Не указан аргумент" << endl;
        return 1;
    }
    int port = atoi(argv[1]);
    int n = atoi(argv[2]);
    if (n <= 0)
    {
        cout << "Раундов не может быть меньше 1!" << endl;
        return 1;
    }
    cout << "Сервер запущен" << endl;
    server(port, n);
}