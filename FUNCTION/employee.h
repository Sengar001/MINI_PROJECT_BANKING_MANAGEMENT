#ifndef EMPLOYEE_FUNCTIONS
#define EMPLOYEE_FUNCTIONS

#include<stdio.h>     
#include<unistd.h>    
#include<string.h>
#include<stdbool.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<stdlib.h>
#include<errno.h>
#include"../EMPLOYEE/employee_struct.h"
#include"../CUSTOMER/customer_struct.h"
#include"../ACCOUNTS/transaction_struct.h"
#define SALT "34"

struct Employee employee;

bool login_employee(int connFD);
int add_customer(int connFD);
bool modify_customer_byemployee(int connFD);
bool get_transaction_details_byemployee(int connFD);
bool change_password_employee(int connFD);


bool employee_operation(int connFD){
    if (login_employee(connFD)){
        int wBytes,rBytes;            
        char rBuffer[1000], wBuffer[1000]; 
        bzero(wBuffer,sizeof(wBuffer));
        strcpy(wBuffer,"Welcome employee!");
        while(1){
            strcat(wBuffer,"\n");
            strcat(wBuffer,"1. add new customer\n2. modify customer details\n3. process loan application\n4. approve/reject loan\n5. view loan application\n6. view customer transaction\n7. change password\n8. logout");
            wBytes=write(connFD,wBuffer,strlen(wBuffer));
            if(wBytes==-1){
                perror("Error while writing EMPLOYEE_MENU to client!");
                return false;
            }
            bzero(wBuffer,sizeof(wBuffer));
            rBytes=read(connFD,rBuffer,sizeof(rBuffer));
            if(rBytes==-1){
                perror("Error while reading client's choice for EMPLOYEE_MENU");
                return false;
            }
            int choice=atoi(rBuffer);
            switch(choice){
            case 1:
                add_customer(connFD);
                break;
            case 2:
                modify_customer_byemployee(connFD);
                break;
            case 3:
                //get_Joint_account_details(connFD, NULL);
                break;
            case 4:
                //add_account(connFD);
                break;
            case 5:
                break;
            case 6:
                get_transaction_details_byemployee(connFD);
                break;
            case 7:
                change_password_employee(connFD);
                break;
            default:
                wBytes=write(connFD,"Logging out",strlen("Logging out"));
                return false;
                break;
            }
        }
    }
    else
    {
        // ADMIN LOGIN FAILED
        return false;
    }
    return true;
}

bool login_employee(int connFD){
    int rBytes,wBytes;            
    char rBuffer[1000], wBuffer[1000]; 
    char tBuffer[1000];
    int ID;
    bzero(rBuffer, sizeof(rBuffer));
    bzero(wBuffer, sizeof(wBuffer));
    
    strcpy(wBuffer,"Welcome\nEnter your credentials\n");
    strcat(wBuffer,"Enter your login ID");

    wBytes = write(connFD,wBuffer,strlen(wBuffer));
    if(wBytes == -1)
    {
        perror("Error in writing message to the client!");
        return false;
    }

    rBytes = read(connFD,rBuffer,sizeof(rBuffer));
    if (rBytes == -1)
    {
        perror("Error reading login ID from client!");
        return false;
    }
    bzero(tBuffer,sizeof(tBuffer));
    strcpy(tBuffer,rBuffer);
    strtok(tBuffer,"-");
    ID=atoi(strtok(NULL,"-"));
    int fileFD=open("./EMPLOYEE/employee.txt",O_RDONLY);
    if(fileFD==-1){
        perror("error in opening in file");
        return false;
    }
    int offset=lseek(fileFD,ID*sizeof(struct Employee),SEEK_SET);

    if(offset==-1){
        wBytes=write(connFD,"wrong username",sizeof("wrong username"));
        return false;
    }

    struct flock lock;
    lock.l_type=F_RDLCK;
    lock.l_whence=SEEK_SET;
    lock.l_start=ID*sizeof(struct Employee);
    lock.l_len=sizeof(struct Employee);
    lock.l_pid=getpid();

    int lock_check=fcntl(fileFD,F_SETLKW,&lock);
    if(lock_check==-1){
        perror("error in locking\n");
        return false;
    }
    int read_bytes=read(fileFD,&employee,sizeof(employee));
    lock.l_type=F_UNLCK;
    fcntl(fileFD,F_SETLK,&lock);

    bool userFound;
    if(strcmp(employee.username,rBuffer)==0)
        userFound=true;
    close(fileFD);
    if(userFound)
    {
        bzero(wBuffer,sizeof(wBuffer));
        wBytes=write(connFD,"Enter your password",strlen("Enter your password"));
        if(wBytes==-1)
        {
            perror("Error writing PASSWORD message to client!");
            return false;
        }

        bzero(rBuffer,sizeof(rBuffer));
        rBytes=read(connFD,rBuffer,sizeof(rBuffer));
        if(rBytes==-1)
        {
            perror("Error reading password from the client!");
            return false;
        }
        char hashing[1000];
        strcpy(hashing,crypt(rBuffer,SALT));
        if (strcmp(hashing,employee.password)==0)
            return true;

        bzero(wBuffer, sizeof(wBuffer));
        wBytes = write(connFD, "The password specified doesn't match!",strlen("The password specified doesn't match!"));
    }
    else
    {
        bzero(wBuffer, sizeof(wBuffer));
        wBytes = write(connFD,"The login ID specified doesn't exist!",strlen("The login ID specified doesn't exist!"));
    }

    return false;
}

int add_customer(int connFD){
    int rBytes,wBytes;
    char rBuffer[1000],wBuffer[1000];
    struct Customer new_customer,prev_customer;

    int customerFD=open("./CUSTOMER/customer.txt",O_RDONLY);
    if(customerFD==-1 && errno==ENOENT){
        new_customer.account=1;
    }else if(customerFD==-1){
        perror("error in opening file\n");
        return false;
    }else{
        int offset=lseek(customerFD,-sizeof(struct Customer),SEEK_END);
        if(offset==-1){
            perror("Error seeking to last customer record!");
            return false;
        }

        struct flock lock;
        lock.l_type=F_RDLCK;
        lock.l_whence=SEEK_SET;
        lock.l_start=offset;
        lock.l_len=sizeof(struct Customer);
        lock.l_pid=getpid();

        int lockingStatus=fcntl(customerFD,F_SETLKW,&lock);
        if(lockingStatus==-1){
            perror("Error obtaining read lock on customer record!");
            return false;
        }

        rBytes=read(customerFD,&prev_customer,sizeof(struct Customer));
        if(rBytes==-1){
            perror("Error while reading customer record from file!");
            return false;
        }

        lock.l_type = F_UNLCK;
        fcntl(customerFD,F_SETLK,&lock);
        close(customerFD);
        new_customer.account=prev_customer.account + 1;
    }
    // name
    bzero(wBuffer,sizeof(wBuffer));
    sprintf(wBuffer,"Enter the details for the customer\nWhat is the customer's name?");
    wBytes=write(connFD,wBuffer,sizeof(wBuffer));
    if(wBytes==-1){
        perror("Error writing add customer details to client!");
        return false;
    }
    bzero(rBuffer,sizeof(rBuffer));
    rBytes=read(connFD,rBuffer,sizeof(rBuffer));
    if(rBytes==-1){
        perror("Error reading customer name response from client!");
        return false;
    }
    strcpy(new_customer.name,rBuffer);

    //username 
    strcpy(new_customer.username,new_customer.name);
    strcat(new_customer.username,"-");
    sprintf(wBuffer,"%d",new_customer.account);
    strcat(new_customer.username,wBuffer);

    // gender
    bzero(wBuffer,sizeof(wBuffer));
    sprintf(wBuffer,"Enter gender\n1. MALE M\n2. FEMALE F\n3. OTHER O");
    wBytes=write(connFD,wBuffer,sizeof(wBuffer));
    if(wBytes==-1){
        perror("Error gender details to client!");
        return false;
    }
    bzero(rBuffer,sizeof(rBuffer));
    rBytes=read(connFD,rBuffer,sizeof(rBuffer));
    if(rBytes==-1){
        perror("Error reading gender from client!");
        return false;
    }
    char gender=rBuffer[0];
    new_customer.gender=gender;

    // age
    bzero(wBuffer,sizeof(wBuffer));
    sprintf(wBuffer,"Enter age of customer");
    wBytes=write(connFD,wBuffer,sizeof(wBuffer));
    if(wBytes==-1){
        perror("Error age details to client!");
        return false;
    }
    bzero(rBuffer,sizeof(rBuffer));
    rBytes=read(connFD,rBuffer,sizeof(rBuffer));
    if(rBytes==-1){
        perror("Error reading age from client!");
        return false;
    }
    int age=atoi(rBuffer);
    new_customer.age=age;
    
    //status
    new_customer.active=true;

    //balance
    wBytes=write(connFD,"please enter balance!",strlen("please enter balance!"));
    if(wBytes==-1) {
        perror("Writing balance to client!");
        return false;
    }

    bzero(rBuffer,sizeof(rBuffer));
    rBytes=read(connFD,rBuffer,sizeof(rBuffer));
    if(rBytes==-1) {
        perror("reading balance!");
        return false;
    }

    float balance=atof(rBuffer);
    new_customer.balance=balance;

    //transaction
    for(int i=0;i<10;i++){
        new_customer.transaction[i]=-1;
    }

    // password
    char hashing[1000];
    strcpy(hashing,crypt(new_customer.name,SALT));
    strcpy(new_customer.password,hashing);

    customerFD=open("./CUSTOMER/customer.txt",O_CREAT|O_APPEND|O_WRONLY,S_IRWXU);
    if(customerFD==-1){
        perror("Error while creating opening employee file!");
        return false;
    }
    wBytes=write(customerFD,&new_customer,sizeof(new_customer));
    if(wBytes==-1){
        perror("Error while writing employee record to file!");
        return false;
    }

    close(customerFD);

    bzero(wBuffer,sizeof(wBuffer));
    sprintf(wBuffer,"The autogenerated username for the customer is : %s account number : %d\nThe autogenerated password for the customer is : %s",new_customer.username,new_customer.account,new_customer.name);
    wBytes=write(connFD,wBuffer,strlen(wBuffer));
    if(wBytes==-1){
        perror("Error sending employee loginID and password to the client!");
        return false;
    }
    read(connFD,rBuffer,sizeof(rBuffer));
    return new_customer.account;
}

bool modify_customer_byemployee(int connFD){
    int rBytes,wBytes;
    char rBuffer[1000],wBuffer[1000];
    struct Customer customer;
    bzero(rBuffer,sizeof(rBuffer));
    bzero(wBuffer,sizeof(wBuffer));
    sprintf(wBuffer,"enter customer account for details modification");
    wBytes=write(connFD,wBuffer,sizeof(wBuffer));
    if(wBytes==-1){
        perror("Error writing message to client!");
        return false;
    }
    rBytes=read(connFD,rBuffer,sizeof(rBuffer));
    if(rBytes==-1){
        perror("Error reading customer ID from client!");
        return false;
    }

    int ID=atoi(rBuffer)-1;
    int fileFD=open("./CUSTOMER/customer.txt",O_RDONLY);
    if(fileFD==-1){
        perror("error in opening in file");
        return false;
    }
    int offset=lseek(fileFD,ID*sizeof(struct Customer),SEEK_SET);

    if(offset==-1){
        wBytes=write(connFD,"wrong ID",sizeof("wrong ID"));
        return false;
    }

    struct flock lock;
    lock.l_type=F_RDLCK;
    lock.l_whence=SEEK_SET;
    lock.l_start=ID*sizeof(struct Customer);
    lock.l_len=sizeof(struct Customer);
    lock.l_pid=getpid(); 

    int lock_check=fcntl(fileFD,F_SETLKW,&lock);
    if(lock_check==-1){
        perror("error in locking\n");
        return false;
    }
    rBytes=read(fileFD,&customer,sizeof(customer)); 
    if(rBytes==-1){
        perror("error in reading from file!");
    }

    lock.l_type=F_UNLCK;
    fcntl(fileFD,F_SETLK,&lock);
    close(fileFD);

    bzero(wBuffer,sizeof(wBuffer));
    sprintf(wBuffer,"current details of customer:\nID : %d\nname : %s\nage : %d\ngender : %c\nusername : %s\npress YES for modification",customer.account,customer.name,customer.age,customer.gender,customer.username);
    write(connFD,wBuffer,sizeof(wBuffer));
    read(connFD,rBuffer,sizeof(rBuffer));
    int count=0;
    while(1){
        bzero(wBuffer,sizeof(wBuffer));
        strcpy(wBuffer,"enter details you want to modify\n1. name\n2. age\n3. gender\n4. no change");
        write(connFD,wBuffer,sizeof(wBuffer));
        bzero(rBuffer,sizeof(rBuffer));
        read(connFD,rBuffer,sizeof(rBuffer));
        int choice=atoi(rBuffer);
        switch(choice){
            case 1:
                write(connFD,"enter new name",sizeof("enter new name"));
                bzero(rBuffer,sizeof(rBuffer));
                read(connFD,rBuffer,sizeof(rBuffer));
                strcpy(customer.name,rBuffer);

                bzero(wBuffer,sizeof(wBuffer));
                strcpy(customer.username,customer.name);
                strcat(customer.username,"-");
                sprintf(wBuffer,"%d",customer.account);
                strcat(customer.username,wBuffer);
                break;
            case 2:
                write(connFD,"enter new age",sizeof("enter new age"));
                bzero(rBuffer,sizeof(rBuffer));
                read(connFD,rBuffer,sizeof(rBuffer));
                int age=atoi(rBuffer);
                customer.age=age;
                break;
            case 3:
                write(connFD,"enter gender\nMALE M\n FEMALE F\nOTHER O",sizeof("enter gender\nMALE M\n FEMALE F\nOTHER O"));
                bzero(rBuffer,sizeof(rBuffer));
                read(connFD,rBuffer,sizeof(rBuffer));
                char gen=rBuffer[0];
                customer.gender=gen;
                break;
            default:
                count++;
                break;
        }
        if(count)
            break;
    }

    fileFD=open("./CUSTOMER/customer.txt",O_WRONLY);
    if(fileFD==-1){
        perror("error in opening in file");
        return false;
    }
    offset=lseek(fileFD,ID*sizeof(struct Customer),SEEK_SET);

    if(offset==-1){
        wBytes=write(connFD,"wrong ID",sizeof("wrong ID"));
        return false;
    }

    lock.l_type=F_WRLCK;
    lock.l_whence=SEEK_SET;
    lock.l_start=ID*sizeof(struct Customer);
    lock.l_len=sizeof(struct Customer);
    lock.l_pid=getpid(); 

    lock_check=fcntl(fileFD,F_SETLKW,&lock);
    if(lock_check==-1){
        perror("error in locking\n");
        return false;
    }
    wBytes=write(fileFD,&customer,sizeof(customer)); 
    if(wBytes==-1){
        perror("error in writing to file!");
    }

    lock.l_type=F_UNLCK;
    fcntl(fileFD,F_SETLK,&lock);
    close(fileFD);

    bzero(wBuffer,sizeof(wBuffer));
    sprintf(wBuffer,"new details of customer:\nID : %d\nname : %s\nage : %d\ngender : %c\nusername : %s",customer.account,customer.name,customer.age,customer.gender,customer.username);
    write(connFD,wBuffer,sizeof(wBuffer));
    read(connFD,rBuffer,sizeof(rBuffer));

    return true;

}

bool get_transaction_details_byemployee(int connFD){
    int rBytes, wBytes;                              
    char rBuffer[1000],wBuffer[10000],tBuffer[1000]; 
    struct Customer customer;
    write(connFD,"enter account number for which want to see transaction details!",sizeof("enter account number for which want to see transaction details!"));
    read(connFD,rBuffer,sizeof(rBuffer));
    int ID=atoi(rBuffer)-1;
    int fileFD=open("./CUSTOMER/customer.txt",O_RDONLY);
    if(fileFD==-1){
        perror("error in opening in file");
        return false;
    }
    int offset=lseek(fileFD,(ID)*sizeof(struct Customer),SEEK_SET);

    if(offset==-1){
        wBytes=write(connFD,"wrong username",sizeof("wrong username"));
        return false;
    }

    struct flock lock;
    lock.l_type=F_RDLCK;
    lock.l_whence=SEEK_SET;
    lock.l_start=(ID)*sizeof(struct Customer);
    lock.l_len=sizeof(struct Customer);
    lock.l_pid=getpid();

    int lock_check=fcntl(fileFD,F_SETLKW,&lock);
    if(lock_check==-1){
        perror("error in locking\n");
        return false;
    }
    int read_bytes=read(fileFD,&customer,sizeof(customer));
    lock.l_type=F_UNLCK;
    fcntl(fileFD,F_SETLK,&lock);
    close(fileFD);
    
    int ptr=0;

    struct Transaction transaction;
    struct tm transactionTime;

    int transactionFD=open("./ACCOUNTS/transaction.txt",O_RDONLY);
    if (transactionFD==-1){
        perror("error in opening file!");
        return false;
    }
    while(ptr<10 && customer.transaction[ptr]!=-1){
        int offset=lseek(transactionFD,customer.transaction[ptr]*sizeof(struct Transaction),SEEK_SET);
        if(offset==-1){
            perror("Error while seeking to required transaction record!");
            return false;
        }
        struct flock lock;
        lock.l_type=F_RDLCK;
        lock.l_whence=SEEK_SET;
        lock.l_start=customer.transaction[ptr]*sizeof(struct Transaction);
        lock.l_len=sizeof(struct Transaction);
        lock.l_pid=getpid();

        int lockingStatus=fcntl(transactionFD,F_SETLKW,&lock);
        if(lockingStatus==-1){
            perror("error on taking read lock");
            return false;
        }

        rBytes=read(transactionFD,&transaction,sizeof(struct Transaction));
        if(rBytes==-1){
            perror("error reading transaction record from file!");
            return false;
        }

        lock.l_type=F_UNLCK;
        fcntl(transactionFD,F_SETLK,&lock);

        // transactionTime=*localtime(&(transaction.transactionTime));


        bzero(tBuffer,sizeof(tBuffer));
        sprintf(tBuffer,"Details of transaction %d - \n\tDate : %s\tOperation : %s \n\tPrevious Balance : %f \n\tNew Balance : %f",(ptr + 1),ctime(&transaction.transactionTime),(transaction.operation?"Deposit":"Withdrawl"),transaction.oldBalance,transaction.newBalance);

        strcat(wBuffer, tBuffer);
        ptr++;
    }

    close(transactionFD);
        
    if(strlen(wBuffer)==0){
        write(connFD,"No transactions were performed on this account by the customer!",strlen("No transactions were performed on this account by the customer!"));
        read(connFD,rBuffer,sizeof(rBuffer)); 
        return false;
    }
    else{
        write(connFD,wBuffer,strlen(wBuffer));
        read(connFD,rBuffer,sizeof(rBuffer)); 
        return true;
    }   
    
}

bool change_password_employee(int connFD){
    int rBytes,wBytes;            
    char rBuffer[1000],wBuffer[1000];
    int ID=employee.id;
    wBytes=write(connFD,"enter your old password!",sizeof("enter your old password!"));
    if(wBytes==-1){
        perror("error in writing to client\n");
        return false;
    }
    rBytes=read(connFD,rBuffer,sizeof(rBuffer));
    if(rBytes==-1){
        perror("error in reading from client\n");
    }

    int fileFD=open("./EMPLOYEE/employee.txt",O_RDWR);
    if(fileFD==-1){
        perror("error in opening in file");
        return false;
    }

    int offset=lseek(fileFD,(ID)*sizeof(struct Employee),SEEK_SET);

    if(offset==-1){
        wBytes=write(connFD,"wrong username",sizeof("wrong username"));
        return false;
    }

    struct flock lock;
    lock.l_type=F_WRLCK;
    lock.l_whence=SEEK_SET;
    lock.l_start=(ID)*sizeof(struct Employee);
    lock.l_len=sizeof(struct Employee);
    lock.l_pid=getpid();

    int lock_check=fcntl(fileFD,F_SETLKW,&lock);
    if(lock_check==-1){
        perror("error in locking\n");
        return false;
    }
    int read_bytes=read(fileFD,&employee,sizeof(employee));

    char hashing[1000];
    strcpy(hashing,crypt(rBuffer,SALT));
    if(strcmp(hashing,employee.password)==0){
        wBytes=write(connFD,"enter new password!",sizeof("enter new password!"));
        if(wBytes==-1){
            perror("error in writing to client\n");
        }
        bzero(rBuffer,sizeof(rBuffer));
        rBytes=read(connFD,rBuffer,sizeof(rBuffer));
        if(rBytes==-1){
            perror("error in reading from client\n");
        }
    }
    else{
        perror("wrong password\n");
        close(fileFD);
        return false;
    }

    bzero(hashing,sizeof(hashing));
    strcpy(hashing,crypt(rBuffer,SALT));
    strcpy(employee.password,hashing);

    offset=lseek(fileFD,(ID)*sizeof(struct Employee),SEEK_SET);

    if(offset==-1){
        wBytes=write(connFD,"wrong username",sizeof("wrong username"));
        return false;
    }
    wBytes=write(fileFD,&employee,sizeof(employee));
    if(wBytes==-1){
        perror("error in writing to file\n");
    }
    lock.l_type=F_UNLCK;
    fcntl(fileFD,F_SETLK,&lock);
    close(fileFD);
    wBytes=write(connFD,"password updated!",sizeof("password updated!"));
    if(wBytes==-1){
        perror("error in writing to client\n");
    }
    read(connFD,rBuffer,sizeof(rBuffer));
    return false;
}

#endif
