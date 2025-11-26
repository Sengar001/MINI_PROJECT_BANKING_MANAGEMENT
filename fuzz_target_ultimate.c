#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <fcntl.h>
#include <crypt.h>
#include <errno.h>

// Match headers
#include "./EMPLOYEE/employee_struct.h"
#include "./CUSTOMER/customer_struct.h"
#include "./ACCOUNTS/loan_struct.h"
#include "./ACCOUNTS/feedback_struct.h"
#include "./ACCOUNTS/transaction_struct.h"

#define SALT "34"

// --- MOCKS ---
struct Employee mock_mgr;
struct Employee mock_emp;
struct Customer mock_cust;
struct Loan mock_loan;
struct Feedback mock_fb;

// Triggers
int force_open_fail = 0;
int force_read_fail = 0;
int force_sem_fail = 0; // RE-ADDED

void init_mocks()
{
    // Manager
    mock_mgr.id = 1;
    strcpy(mock_mgr.name, "Manager");
    strcpy(mock_mgr.username, "admin-1");
    strcpy(mock_mgr.password, crypt("password", SALT));
    mock_mgr.ismanager = true;
    for (int i = 0; i < 10; i++)
        mock_mgr.loan[i] = -1;

    // Employee
    mock_emp.id = 2;
    strcpy(mock_emp.name, "Staff");
    strcpy(mock_emp.username, "emp-2");
    strcpy(mock_emp.password, crypt("password", SALT));
    mock_emp.ismanager = false;

    // Setup: One loan exists (for View test), rest empty (for Assign test)
    mock_emp.loan[0] = 5;
    for (int i = 1; i < 10; i++)
        mock_emp.loan[i] = -1;

    // Customer
    mock_cust.account = 1;
    strcpy(mock_cust.name, "John");
    strcpy(mock_cust.username, "John-1");
    strcpy(mock_cust.password, crypt("password", SALT));
    mock_cust.active = true;
    mock_cust.balance = 5000.0;
    mock_cust.age = 30;
    mock_cust.gender = 'M';
    for (int i = 0; i < 10; i++)
        mock_cust.transaction[i] = -1;
    mock_cust.transaction[0] = 1;

    // Loan
    mock_loan.ID = 1;
    mock_loan.account = 1;
    mock_loan.ammount = 10000;
    mock_loan.isapprove = false;
    mock_loan.isassign = false; // Default False

    mock_fb.ID = 1;
    strcpy(mock_fb.buffer, "Test");
    mock_fb.isreview = false;
}

char *fuzz_input_ptr;
int fuzz_input_len;
int current_role_choice = 0;
int file_read_limiter = 0;
int emp_read_count = 0;
int conn_fd_global = -1;

ssize_t real_read(int fd, void *buf, size_t count) { return read(fd, buf, count); }
int real_open(const char *path, int flags) { return open(path, flags); }

// --- CUSTOM READ ---
ssize_t my_read(int fd, void *buf, size_t count)
{
    if (fd < 0)
        return -1;

    // NETWORK INPUT
    if (fd == conn_fd_global)
    {
        if (fuzz_input_len <= 0)
            exit(0);

        char *next_newline = memchr(fuzz_input_ptr, '\n', fuzz_input_len);
        size_t len = next_newline ? (next_newline - fuzz_input_ptr) : fuzz_input_len;
        if (len > count - 1)
            len = count - 1;
        memcpy(buf, fuzz_input_ptr, len);
        ((char *)buf)[len] = '\0';
        int adv = len + (next_newline ? 1 : 0);
        fuzz_input_ptr += adv;
        fuzz_input_len -= adv;
        return len;
    }

    // FILE INPUT
    if (force_read_fail)
    {
        memset(buf, 0, count);
        errno = EIO;
        return -1;
    }
    if (file_read_limiter++ > 60)
        return 0;

    if (count == sizeof(struct Employee))
    {
        emp_read_count++;
        if (current_role_choice == 3)
            memcpy(buf, &mock_emp, sizeof(struct Employee));
        else
        {
            if (emp_read_count == 1)
                memcpy(buf, &mock_mgr, sizeof(struct Employee));
            else
                memcpy(buf, &mock_emp, sizeof(struct Employee));
        }
        return sizeof(struct Employee);
    }
    if (count == sizeof(struct Customer))
    {
        memcpy(buf, &mock_cust, sizeof(struct Customer));
        return sizeof(struct Customer);
    }
    if (count == sizeof(struct Loan))
    {
        memcpy(buf, &mock_loan, sizeof(struct Loan));
        return sizeof(struct Loan);
    }
    if (count == sizeof(struct Feedback))
    {
        memcpy(buf, &mock_fb, sizeof(struct Feedback));
        return sizeof(struct Feedback);
    }

    return real_read(fd, buf, count);
}

// --- CUSTOM OPEN ---
int my_open(const char *path, int flags, ...)
{
    if (force_open_fail)
    {
        errno = ENOENT;
        return -1;
    }
    return real_open("/dev/null", O_RDWR);
}

// --- LOGIC & TRIGGERS ---
int my_strcmp(const char *s1, const char *s2)
{
    if (!s1 || !s2)
        return -1;

    // RESET EVERYTHING ON SUCCESSFUL LOGIN
    if (strcmp(s2, "admin-1") == 0 || strcmp(s2, "emp-2") == 0)
    {
        force_open_fail = 0;
        force_read_fail = 0;
        force_sem_fail = 0;
        init_mocks();
        return 0;
    }

    // --- TRIGGERS ---
    // 1. Loan Already Assigned
    if (strcmp(s2, "admin-loan-assigned") == 0)
    {
        init_mocks();
        mock_loan.isassign = true;
        return 0;
    }
    // 2. Inactive Account
    if (strcmp(s2, "admin-inactive") == 0)
    {
        init_mocks();
        mock_cust.active = false;
        return 0;
    }
    // 3. Semaphore Fail
    if (strcmp(s2, "fail_sem") == 0)
    {
        force_sem_fail = 1;
        return 0;
    }
    // 4. File System Fails
    if (strcmp(s2, "fail_open") == 0)
    {
        force_open_fail = 1;
        return 0;
    }
    if (strcmp(s2, "fail_read") == 0)
    {
        force_read_fail = 1;
        return 0;
    }

    return strcmp(s1, s2);
}

// --- SEMAPHORE MOCKS (Fixed) ---
int my_semget(key_t key, int nsems, int semflg)
{
    if (force_sem_fail)
    {
        errno = ENOSPC;
        return -1;
    }
    return 1;
}
int my_semctl(int semid, int semnum, int cmd, ...)
{
    if (force_sem_fail)
        return -1;
    return 0;
}
int my_semop(int semid, struct sembuf *sops, size_t nsops)
{
    if (force_sem_fail)
        return -1;
    return 0;
}

// Mocks
off_t my_lseek(int fd, off_t offset, int whence) { return offset; }
int my_close(int fd) { return 0; }
int my_fcntl(int fd, int cmd, ...) { return 0; }

#define read my_read
#define open my_open
#define lseek my_lseek
#define close my_close
#define fcntl my_fcntl
#define semget my_semget
#define semctl my_semctl
#define semop my_semop
#define strcmp my_strcmp

#include "./FUNCTION/admin.h"
#include "./FUNCTION/employee.h"
#include "./FUNCTION/customer.h"

int main()
{
    init_mocks();
    conn_fd_global = real_open("/dev/null", O_RDWR);
    static char big_buffer[1024 * 1024];
    ssize_t total = real_read(0, big_buffer, sizeof(big_buffer) - 1);
    if (total <= 0)
        return 0;
    big_buffer[total] = '\0';
    fuzz_input_ptr = big_buffer;
    fuzz_input_len = total;
    char role_buff[100];
    if (my_read(conn_fd_global, role_buff, sizeof(role_buff)) <= 0)
        return 0;
    current_role_choice = atoi(role_buff);
    switch (current_role_choice)
    {
    case 1:
        admin_operation(conn_fd_global);
        break;
    case 2:
        employee_operation(conn_fd_global, 2);
        break;
    case 3:
        employee_operation(conn_fd_global, 3);
        break;
    case 4:
        customer_operation(conn_fd_global);
        break;
    }
    return 0;
}