// file_sender.c
#include <stdio.h>          // Для функций ввода-вывода (printf, fprintf, perror, etc.)
#include <stdlib.h>         // Для функций общего назначения (exit, EXIT_FAILURE, etc.)
#include <sys/socket.h>     // Для сокетов (socket, connect, send)
#include <sys/un.h>         // Для UNIX-доменных сокетов (struct sockaddr_un)
#include <unistd.h>         // Для закрытия файловых дескрипторов (close)
#include <string.h>         // Для функций работы со строками (strncpy, memset)

#define BUFFER_SIZE 1024    // Размер буфера для чтения данных из файла

int main(int argc, char *argv[]) {
    int sockfd;                        // Дескриптор сокета
    struct sockaddr_un addr;           // Структура адреса UNIX-доменного сокета
    char buffer[BUFFER_SIZE];          // Буфер для хранения данных из файла
    ssize_t bytes_read;                // Количество прочитанных байт из файла
    FILE *file;                        // Указатель на файл

    // Проверка количества аргументов командной строки
    if (argc != 3) {
        fprintf(stderr, "Использование: %s <входной_файл> <путь_к_сокету>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *input_file = argv[1];    // Путь к входному файлу, который нужно отправить
    const char *socket_path = argv[2];   // Путь к UNIX-сокету для подключения

    // Открытие входного файла для чтения в бинарном режиме
    file = fopen(input_file, "rb");
    if (!file) {
        perror("Ошибка открытия файла");
        exit(EXIT_FAILURE);
    }

    // Создание UNIX-доменного сокета
    sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("Ошибка создания сокета");
        fclose(file);
        exit(EXIT_FAILURE);
    }

    // Обнуление структуры адреса
    memset(&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;  // Установка семейства адресов в AF_UNIX для UNIX-доменного сокета

    // Копирование пути к сокету в структуру адреса
    // Используем strncpy для предотвращения переполнения буфера
    strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);

    // Подключение к серверному сокету по указанному пути
    if (connect(sockfd, (struct sockaddr *)&addr, sizeof(struct sockaddr_un)) == -1) {
        perror("Ошибка подключения к сокету");
        fclose(file);
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Чтение данных из файла и отправка их через сокет
    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, file)) > 0) {
        // Отправка прочитанных данных через сокет
        ssize_t bytes_sent = send(sockfd, buffer, bytes_read, 0);
        if (bytes_sent == -1) {
            perror("Ошибка отправки данных");
            fclose(file);
            close(sockfd);
            exit(EXIT_FAILURE);
        }

        // Проверка, что все байты были отправлены
        if (bytes_sent != bytes_read) {
            fprintf(stderr, "Предупреждение: не все байты были отправлены\n");
        }
    }

    // Проверка, была ли ошибка при чтении файла
    if (ferror(file)) {
        perror("Ошибка чтения файла");
    }

    // Закрытие файла и сокета
    fclose(file);
    close(sockfd);

    return 0;  // Завершение программы успешно
}
