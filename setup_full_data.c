#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>
#include <crypt.h>
#include <time.h>

// Ensure these match your actual filenames
#include "./EMPLOYEE/employee_struct.h"
#include "./CUSTOMER/customer_struct.h"
#include "./ACCOUNTS/loan_struct.h"
#include "./ACCOUNTS/feedback_struct.h"
#include "./ACCOUNTS/transaction_struct.h"

#define SALT "34"

int main()
{
    // 1. Setup Employee File (Manager & Staff)
    int fd = open("./EMPLOYEE/employee.txt", O_RDWR | O_CREAT | O_TRUNC, 0666);
    if (fd == -1)
    {
        perror("Error opening employee file");
        exit(1);
    }

    // Dummy ID 0
    struct Employee dummy = {0};
    write(fd, &dummy, sizeof(dummy));

    // ID 1: Manager (admin-1 / password)
    struct Employee mgr = {0};
    mgr.id = 1;
    strcpy(mgr.name, "Manager");
    mgr.gender = 'M';
    mgr.age = 35;
    strcpy(mgr.username, "admin-1");
    strcpy(mgr.password, crypt("password", SALT));
    mgr.ismanager = true;
    for (int i = 0; i < 10; i++)
        mgr.loan[i] = -1; // Set all loan slots to -1 (Empty)
    write(fd, &mgr, sizeof(mgr));

    // ID 2: Employee (emp-2 / password)
    struct Employee emp = {0};
    emp.id = 2;
    strcpy(emp.name, "Staff");
    emp.gender = 'F';
    emp.age = 28;
    strcpy(emp.username, "emp-2");
    strcpy(emp.password, crypt("password", SALT));
    emp.ismanager = false;
    for (int i = 0; i < 10; i++)
        emp.loan[i] = -1; // Set all loan slots to -1 (Empty)
    write(fd, &emp, sizeof(emp));

    close(fd);

    // 2. Setup Customer File
    int cfd = open("./CUSTOMER/customer.txt", O_RDWR | O_CREAT | O_TRUNC, 0666);
    if (cfd == -1)
    {
        perror("Error opening customer file");
        exit(1);
    }

    // Dummy for 0 index
    struct Customer dummy_cust = {0};
    write(cfd, &dummy_cust, sizeof(dummy_cust));

    // ID 1: Customer (John-1 / password)
    struct Customer cust = {0};
    cust.account = 1;
    strcpy(cust.name, "John");
    cust.gender = 'M';
    cust.age = 30;
    strcpy(cust.username, "John-1");
    strcpy(cust.password, crypt("password", SALT));
    cust.active = true;
    cust.balance = 5000.0;
    cust.loanID = -1; // No active loan initially

    // Initialize transactions to -1
    for (int i = 0; i < 10; i++)
        cust.transaction[i] = -1;

    // Link to one transaction for testing
    cust.transaction[0] = 1;

    write(cfd, &cust, sizeof(cust));
    close(cfd);

    // 3. Setup Loan File
    int lfd = open("./ACCOUNTS/loan.txt", O_RDWR | O_CREAT | O_TRUNC, 0666);
    if (lfd == -1)
    {
        perror("Error opening loan file");
        exit(1);
    }

    struct Loan loan = {0};
    loan.ID = 1;
    loan.account = 1;
    loan.ammount = 10000.0; // Matches 'float ammount' in struct
    loan.isapprove = false;
    loan.isassign = false;
    write(lfd, &loan, sizeof(loan));
    close(lfd);

    // 4. Setup Feedback File
    int ffd = open("./ACCOUNTS/feedback.txt", O_RDWR | O_CREAT | O_TRUNC, 0666);
    if (ffd == -1)
    {
        perror("Error opening feedback file");
        exit(1);
    }

    struct Feedback fb = {0};
    fb.ID = 1;
    strcpy(fb.buffer, "Great service");
    fb.isreview = false;
    write(ffd, &fb, sizeof(fb));
    close(ffd);

    // 5. Setup Transaction File
    int tfd = open("./ACCOUNTS/transaction.txt", O_RDWR | O_CREAT | O_TRUNC, 0666);
    if (tfd == -1)
    {
        perror("Error opening transaction file");
        exit(1);
    }

    // Dummy transaction 0
    struct Transaction dummy_trans = {0};
    write(tfd, &dummy_trans, sizeof(dummy_trans));

    // Transaction 1 linked to Customer 1
    struct Transaction trans = {0};
    trans.transactionID = 1;
    trans.accountNumber = 1;
    trans.operation = 1; // 1: Credit
    trans.oldBalance = 4000.0;
    trans.newBalance = 5000.0;
    trans.transactionTime = time(NULL); // Set current time
    write(tfd, &trans, sizeof(trans));
    close(tfd);

    printf("SUCCESS: Full test data generated compatible with your header files.\n");
    return 0;
}