/*
 * FastCGI Process Manager
 * Copyright (C) 2015 sw
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <winsock.h>
#include <windows.h>
#include <wininet.h>
#include "win32/getopt.h"

#define MAX_PROCESSES	1024
static char* version = "Version: 0.1";
static char* prog_name;

static void show_help()
{
    fprintf(stderr, ""
        "Usage: %s [options] -f fcgiapp\n"
        "Manage FastCGI processes.\n"
        "\n"
        " -f  filename of the fcgi processor\n"
        " -n  number of processes to keep, default is 1\n"
        " -i  ip address to bind, default is 127.0.0.1\n"
        " -p  port to bind, default is 9000\n"
        " -h  output usage information and exit\n"
        " -v  output version information and exit\n"
        "", prog_name);
}

static void show_version()
{
    fprintf(stdout, "%s %s\n\
FastCGI Process Manager\n\
Copyright(C) 2015 sw\n\
Compiled on %s\n\
", prog_name, version, __DATE__);
}

static int bind_socket(char* host, int port)
{
    int fcgi_fd = -1;
    struct sockaddr_in listen_addr;
	listen_addr.sin_family = PF_INET;
    listen_addr.sin_addr.s_addr = inet_addr( host );
	listen_addr.sin_port = htons( port );

    if (-1 == (fcgi_fd = socket(AF_INET, SOCK_STREAM, 0))) {
        fprintf(stderr, "couldn't create socket\n");
        return -1;
    }

    if (-1 == bind(fcgi_fd, (struct sockaddr*)&listen_addr, sizeof(struct sockaddr_in)) )
    {
        fprintf(stderr, "failed to bind %s:%d\n", host, port );
        closesocket(fcgi_fd);
        return -1;
    }

    if (-1 == listen(fcgi_fd, MAX_PROCESSES)) {
        fprintf(stderr, "listen failed\n");
        closesocket(fcgi_fd);
        return -1;
    }
    return fcgi_fd;
}

static int spawn_process(char* app_path, int fcgi_fd, void* fcgi_job_object)
{
    int ret;
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si,sizeof(STARTUPINFO));
    si.cb = sizeof(STARTUPINFO);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput  = (HANDLE)fcgi_fd;
    si.hStdOutput = INVALID_HANDLE_VALUE;
    si.hStdError  = INVALID_HANDLE_VALUE;
    ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));

    ret = CreateProcess(NULL, app_path,
                            NULL,NULL,
                            TRUE, CREATE_NO_WINDOW | CREATE_SUSPENDED | CREATE_BREAKAWAY_FROM_JOB,
                            NULL,NULL,
                            &si,&pi);
    if(0 == ret)
    {
        fprintf(stderr, "failed to create process %s, ret=%d\n", app_path, ret);
        return 0;
    }

    if(!AssignProcessToJobObject((HANDLE)fcgi_job_object, pi.hProcess))
    {
        TerminateProcess(pi.hProcess, 1);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        fprintf(stderr, "failed to assign process to job object\n");
        return 0;
    }

    if(!ResumeThread(pi.hThread)){
        TerminateProcess(pi.hProcess, 1);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        fprintf(stderr, "failed to resume process thread\n");
        return 0;
    }
//    WaitForSingleObject(pi.hProcess, INFINITE);
//    CloseHandle(pi.hThread);
    return 1;
}

static int init_socket()
{
    static WSADATA wsa_data;
    if(WSAStartup((WORD)(1<<8|1), &wsa_data) != 0)
    {
        fprintf(stderr, "failed to initialize socket\n");
        return 0;
    }
    return 1;
}

static void* init_job_object()
{
    void* fcgi_job_object = NULL;
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION limit;

    fcgi_job_object = CreateJobObject(NULL, NULL);
    if(fcgi_job_object == NULL)
    {
        fprintf(stderr, "failed to create job object\n");
        return NULL;
    }

    if (!QueryInformationJobObject((HANDLE)fcgi_job_object, JobObjectExtendedLimitInformation, &limit, sizeof(limit), NULL))
    {
        CloseHandle((HANDLE)fcgi_job_object);
        fprintf(stderr, "failed to query job object information: JobObjectExtendedLimitInformation\n");
        return NULL;
    }

    limit.BasicLimitInformation.LimitFlags |= JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
    if (!SetInformationJobObject(fcgi_job_object, JobObjectExtendedLimitInformation, &limit, sizeof(limit)))
    {
        CloseHandle((HANDLE)fcgi_job_object);
        fprintf(stderr, "failed to set job object information: JobObjectExtendedLimitInformation\n");
        return NULL;
    }

    return fcgi_job_object;
}

int main(int argc, char **argv)
{
    int fcgi_fd = -1;
    void* fcgi_job_object = NULL;


    int     fcgi_count = 1;
    char   *fcgi_host   = "127.0.0.1";
    int     fcgi_port   = 9000;
    char   *fcgi_path   = NULL;

    int i = 0;
    int o = 0;

    prog_name = strrchr(argv[0], '/');
    if(prog_name == NULL)
        prog_name = strrchr(argv[0], '\\');
    if(prog_name == NULL)
        prog_name = argv[0];
    else
        prog_name++;

    while (-1 != (o = getopt(argc, argv, "hvf:n:i:p:"))) {
        switch(o){
        case 'h':
            show_help();
            return 0;
        case 'v':
            show_version();
            return 0;
        case 'f':
            fcgi_path = optarg;
            break;
        case 'n':
            fcgi_count = strtol(optarg, NULL, 10);
            if(fcgi_count > MAX_PROCESSES){
                fprintf(stderr, "exceeds MAX_PROCESSES!\n");
                fcgi_count = MAX_PROCESSES;
            }
            if(fcgi_count < 1){
                fprintf(stderr, "can not less than 1\n");
                fcgi_count = 1;
            }
            break;
        case 'i':
            fcgi_host = optarg;
            break;
        case 'p':
            fcgi_port = strtol(optarg, NULL, 10);
            break;
        default:
            show_help();
            return 0;
        }
    }

    if(!fcgi_path) {
        show_help();
        return -1;
    }

    if(!init_socket()) {
        WSACleanup();
        fprintf(stderr, "failed to bind socket\n");
        return -1;
    }

    if(-1 == (fcgi_fd = bind_socket(fcgi_host, fcgi_port))){
        WSACleanup();
        fprintf(stderr, "failed to bind socket\n");
        return -1;
    }

    if(!(fcgi_job_object = init_job_object())){
        closesocket(fcgi_fd);
        WSACleanup();
        fprintf(stderr, "failed to initialize job object\n");
        return -1;
    }

    for (i = 0; i < fcgi_count; ++i){
        if(!spawn_process(fcgi_path, fcgi_fd, fcgi_job_object)){
            fprintf(stderr, "failed to spawn process %d\n", i);
        }
    }

    while(1){
        JOBOBJECT_BASIC_ACCOUNTING_INFORMATION job_accounting;
        if (!QueryInformationJobObject(fcgi_job_object, JobObjectBasicAccountingInformation, &job_accounting, sizeof(job_accounting), NULL)){
            fprintf(stderr, "failed to query job object information: JobObjectBasicAccountingInformation\n");
        }

        if(job_accounting.ActiveProcesses < (unsigned int)fcgi_count){
            spawn_process(fcgi_path, fcgi_fd, fcgi_job_object);
        }

        Sleep(1000);
    }

    CloseHandle(fcgi_job_object);
    closesocket(fcgi_fd);
    WSACleanup();
	return 0;
}

