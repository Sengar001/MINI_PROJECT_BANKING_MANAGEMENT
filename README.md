# Banking Management System

## Project Overview

This project simulates the core functionalities of a Banking Management System, including account management, transactions, loan processing, and administrative tasks. The system features a secure login mechanism and role-based access for customers, employees, managers, and administrators.

## Features

- Customer Operations: View balance, deposit, withdraw, transfer funds, apply for loans.
- Bank Employee Operations: Add/modify customer details, process loans.
- Manager Operations: Assign loan applications, manage customer accounts.
- Administrator Operations: Add employees, manage roles, and handle overall system administration.
  
The project implements file management, locking mechanisms, and process synchronization to handle concurrency and race conditions.

---

## Requirements

- GCC compiler
- Unix-based operating system for system calls and process management
- Basic knowledge of C programming, socket programming, and system calls

---

## How to Set Up and Run

### Step 1: Compile the Server

1. Open a terminal window.
2. Navigate to the project directory.
3. Compile the server code using the following command:
   
```bash
gcc server.c -lcrypt -o server
```

The -lcrypt flag is used to include the cryptographic library for password management.
Once compiled, run the server with the following command:

```bash
./server
```

Step 2: Connect a Client
Open another terminal window.

Navigate to the project directory.

Compile the client code using the following command:

```bash
gcc client.c -o client
```

Connect to the server by running the client:

```bash
./client
```

Step 3: Login as Admin
Upon starting the system, you need to log in with predefined admin credentials before proceeding:

<strong>Admin ID: admin-1</strong><br>
<strong>Password: Abhishek</strong>

Step 4: Interact with the System
Once both the server and client are running, you can interact with the banking system from the client side. Depending on your role (customer, employee, manager, or admin), you will have different options available.

Notes
The server must be running before you connect a client.
Each client connects in its own terminal.
Use system calls wherever possible to handle file operations, locks, and semaphores for synchronization.
Ensure you manage proper user sessions by allowing only one active session per user.
Project Structure:
```
BankingManagementSystem/
├── server.c        # Contains the server-side logic
├── client.c        # Contains the client-side logic
├── ADMIN           # Handles ADMIN operations
├── CUSTOMER        # Handles CUSTOMER operations
├── EMPLOYEE        # Handles EMPLOYEE/MANAGER operations
├── LOAN            # Handles LOAN processing
├── FEEDBACK        # Handles FEEDBACK processing
├── TRANSACTION     # Handles TRANSACTION logging
└── README.md       # This readme file
```
