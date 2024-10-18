// xor_files.c
#include <stdio.h>          // Для функций ввода-вывода (printf, fprintf, perror, etc.)
#include <stdlib.h>         // Для функций общего назначения (exit, EXIT_FAILURE, etc.)
#include <sys/socket.h>     // Для сокетов (socket, bind, listen, accept)
#include <sys/un.h>         // Для UNIX-доменных сокетов (struct sockaddr_un)
#include <unistd.h>         // Для системных вызовов (close, fork, execl)
#include <sys/wait.h>       // Для ожидания завершения дочерних процессов (waitpid)
#include <string.h>         // Для функций работы со строками (strncpy, memset)

#define BUFFER_SIZE 1024    // Размер буфера для чтения данных

int main(int argc, char *argv[]) {
    int sockfd1, sockfd2;           // Дескрипторы серверных сокетов
    int connfd1, connfd2;           // Дескрипторы соединений с клиентами
    struct sockaddr_un addr1, addr2; // Структуры адресов для двух сокетов
    char buffer1[BUFFER_SIZE], buffer2[BUFFER_SIZE]; // Буферы для хранения полученных данных
    ssize_t bytes_read1, bytes_read2; // Количество прочитанных байт из каждого соединения
    pid_t pid1, pid2;                // Идентификаторы дочерних процессов
    FILE *output_file;               // Указатель на выходной файл

    // Проверка количества аргументов командной строки
    if (argc != 4) {
        fprintf(stderr, "Использование: %s <файл1> <файл2> <выходной_файл>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *input_file1 = argv[1];       // Путь к первому входному файлу
    const char *input_file2 = argv[2];       // Путь ко второму входному файлу
    const char *output_filename = argv[3];   // Путь к выходному файлу

    // Пути к двум UNIX-доменным сокетам
    const char *socket_path1 = "/tmp/socket1";
    const char *socket_path2 = "/tmp/socket2";

    // Удаляем существующие сокеты, если они есть, чтобы избежать ошибок привязки
    unlink(socket_path1);
    unlink(socket_path2);

    // Создание первого серверного сокета
    sockfd1 = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sockfd1 == -1) {
        perror("Ошибка создания сокета 1");
        exit(EXIT_FAILURE);
    }

    // Обнуление структуры адреса для первого сокета
    memset(&addr1, 0, sizeof(struct sockaddr_un));
    addr1.sun_family = AF_UNIX;  // Установка семейства адресов в AF_UNIX
    strncpy(addr1.sun_path, socket_path1, sizeof(addr1.sun_path) - 1); // Копирование пути к сокету

    // Привязка первого сокета к адресу
    if (bind(sockfd1, (struct sockaddr *)&addr1, sizeof(struct sockaddr_un)) == -1) {
        perror("Ошибка привязки сокета 1");
        close(sockfd1);
        exit(EXIT_FAILURE);
    }

    // Начало прослушивания соединений на первом сокете
    if (listen(sockfd1, 1) == -1) {
        perror("Ошибка прослушивания сокета 1");
        close(sockfd1);
        exit(EXIT_FAILURE);
    }

    // Создание второго серверного сокета
    sockfd2 = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sockfd2 == -1) {
        perror("Ошибка создания сокета 2");
        close(sockfd1);
        exit(EXIT_FAILURE);
    }

    // Обнуление структуры адреса для второго сокета
    memset(&addr2, 0, sizeof(struct sockaddr_un));
    addr2.sun_family = AF_UNIX;  // Установка семейства адресов в AF_UNIX
    strncpy(addr2.sun_path, socket_path2, sizeof(addr2.sun_path) - 1); // Копирование пути к сокету

    // Привязка второго сокета к адресу
    if (bind(sockfd2, (struct sockaddr *)&addr2, sizeof(struct sockaddr_un)) == -1) {
        perror("Ошибка привязки сокета 2");
        close(sockfd1);
        close(sockfd2);
        exit(EXIT_FAILURE);
    }

    // Начало прослушивания соединений на втором сокете
    if (listen(sockfd2, 1) == -1) {
        perror("Ошибка прослушивания сокета 2");
        close(sockfd1);
        close(sockfd2);
        exit(EXIT_FAILURE);
    }

    // Запуск первого дочернего процесса для отправки первого файла
    pid1 = fork();
    if (pid1 == -1) {
        perror("Ошибка создания первого дочернего процесса");
        close(sockfd1);
        close(sockfd2);
        exit(EXIT_FAILURE);
    }
    if (pid1 == 0) {
        // Дочерний процесс: выполняет программу file_sender для первого файла
        execl("./file_sender", "./file_sender", input_file1, socket_path1, NULL);
        // Если execl возвращает, значит произошла ошибка
        perror("Ошибка запуска file_sender 1");
        exit(EXIT_FAILURE);
    }

    // Запуск второго дочернего процесса для отправки второго файла
    pid2 = fork();
    if (pid2 == -1) {
        perror("Ошибка создания второго дочернего процесса");
        close(sockfd1);
        close(sockfd2);
        exit(EXIT_FAILURE);
    }
    if (pid2 == 0) {
        // Дочерний процесс: выполняет программу file_sender для второго файла
        execl("./file_sender", "./file_sender", input_file2, socket_path2, NULL);
        // Если execl возвращает, значит произошла ошибка
        perror("Ошибка запуска file_sender 2");
        exit(EXIT_FAILURE);
    }

    // В родительском процессе продолжаем работу

    // Прием соединения от первого клиента (первого file_sender)
    connfd1 = accept(sockfd1, NULL, NULL);
    if (connfd1 == -1) {
        perror("Ошибка принятия соединения 1");
        close(sockfd1);
        close(sockfd2);
        exit(EXIT_FAILURE);
    }

    // Прием соединения от второго клиента (второго file_sender)
    connfd2 = accept(sockfd2, NULL, NULL);
    if (connfd2 == -1) {
        perror("Ошибка принятия соединения 2");
        close(sockfd1);
        close(sockfd2);
        close(connfd1);
        exit(EXIT_FAILURE);
    }

    // Открытие выходного файла для записи в бинарном режиме
    output_file = fopen(output_filename, "wb");
    if (!output_file) {
        perror("Ошибка открытия выходного файла");
        close(sockfd1);
        close(sockfd2);
        close(connfd1);
        close(connfd2);
        exit(EXIT_FAILURE);
    }

    // Цикл для чтения данных из обоих соединений, выполнения XOR и записи результата
    while (1) {
        // Чтение данных из первого соединения
        bytes_read1 = recv(connfd1, buffer1, BUFFER_SIZE, 0);
        if (bytes_read1 == -1) {
            perror("Ошибка чтения из сокета 1");
            break;
        }

        // Чтение данных из второго соединения
        bytes_read2 = recv(connfd2, buffer2, BUFFER_SIZE, 0);
        if (bytes_read2 == -1) {
            perror("Ошибка чтения из сокета 2");
            break;
        }

        // Если достигнут конец файла в любом из соединений, выходим из цикла
        if (bytes_read1 == 0 || bytes_read2 == 0) {
            break;
        }

        // Определяем минимальное количество байт, чтобы избежать выхода за пределы буфера
        ssize_t min_bytes = (bytes_read1 < bytes_read2) ? bytes_read1 : bytes_read2;

        // Выполнение побитовой операции XOR для каждого байта
        for (ssize_t i = 0; i < min_bytes; ++i) {
            buffer1[i] ^= buffer2[i];
        }

        // Запись результата операции XOR в выходной файл
        if (fwrite(buffer1, 1, min_bytes, output_file) != min_bytes) {
            perror("Ошибка записи в выходной файл");
            break;
        }
    }

    // Закрытие всех открытых файлов и сокетов
    fclose(output_file);
    close(connfd1);
    close(connfd2);
    close(sockfd1);
    close(sockfd2);

    // Удаление файлов сокетов из файловой системы
    unlink(socket_path1);
    unlink(socket_path2);

    // Ожидание завершения дочерних процессов
    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);

    return 0;  // Завершение программы успешно
}
