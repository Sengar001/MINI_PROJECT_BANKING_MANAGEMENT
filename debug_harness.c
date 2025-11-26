#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>

// --- 1. DEBUG HELPERS ---
ssize_t real_read_from_stdin(void *buf, size_t count)
{
    return read(0, buf, count);
}

// Intercept strcmp to see why login fails
int my_strcmp(const char *s1, const char *s2)
{
    // Only print interesting comparisons (usernames/passwords)
    if (s1 && s2 && (strlen(s1) > 0 || strlen(s2) > 0))
    {
        printf("[DEBUG] Comparing File: '%s' vs Input: '%s'\n", s1, s2);
    }
    return strcmp(s1, s2);
}

// --- 2. MOCKING INFRASTRUCTURE ---
char *fuzz_input_ptr;
int fuzz_input_len;

ssize_t my_read(int fd, void *buf, size_t count)
{
    if (fuzz_input_len <= 0)
        return 0;
    char *next_newline = memchr(fuzz_input_ptr, '\n', fuzz_input_len);
    size_t length_to_copy = next_newline ? (next_newline - fuzz_input_ptr) : fuzz_input_len;
    if (length_to_copy > count - 1)
        length_to_copy = count - 1;
    memcpy(buf, fuzz_input_ptr, length_to_copy);
    ((char *)buf)[length_to_copy] = '\0';
    int advance = length_to_copy + (next_newline ? 1 : 0);
    fuzz_input_ptr += advance;
    fuzz_input_len -= advance;
    return length_to_copy;
}

ssize_t my_write(int fd, const void *buf, size_t count) { return count; }
int my_semget(key_t key, int nsems, int semflg) { return 1; }
int my_semctl(int semid, int semnum, int cmd, ...) { return 0; }
int my_semop(int semid, struct sembuf *sops, size_t nsops) { return 0; }

// --- 3. ACTIVATE MOCKS ---
#define read my_read
#define write my_write
#define semget my_semget
#define semctl my_semctl
#define semop my_semop
#define strcmp my_strcmp // <--- The Magic Debugger

// --- 4. INCLUDE HEADERS ---
#include "./FUNCTION/admin.h"
#include "./FUNCTION/employee.h"
#include "./FUNCTION/customer.h"

// --- 5. MAIN ---
int main()
{
    static char big_buffer[1024 * 1024];
    ssize_t total_read = real_read_from_stdin(big_buffer, sizeof(big_buffer) - 1);
    if (total_read <= 0)
        return 0;

    big_buffer[total_read] = '\0';
    fuzz_input_ptr = big_buffer;
    fuzz_input_len = total_read;

    char role_buff[100];
    if (my_read(0, role_buff, sizeof(role_buff)) <= 0)
        return 0;
    int choice = atoi(role_buff);

    switch (choice)
    {
    case 1:
        admin_operation(0);
        break;
    case 2:
        employee_operation(0, 2);
        break;
    case 3:
        employee_operation(0, 3);
        break;
    case 4:
        customer_operation(0);
        break;
    }
    return 0;
}