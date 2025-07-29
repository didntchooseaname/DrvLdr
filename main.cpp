#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define MAX_SERVICE_NAME_LEN 10

// Random service name
void GenerateRandomServiceName(char* buffer, size_t len) {
    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    srand((unsigned int)time(NULL));

    for (size_t i = 0; i < len - 1; i++) {
        buffer[i] = charset[rand() % (sizeof(charset) - 1)];
    }
    buffer[len - 1] = '\0';
}

// Loads driver with a random service name
BOOL LoadDriver(const char* driverPath, char* outServiceName) {
    char serviceName[MAX_SERVICE_NAME_LEN];
    GenerateRandomServiceName(serviceName, MAX_SERVICE_NAME_LEN);

    strcpy_s(outServiceName, MAX_SERVICE_NAME_LEN, serviceName);

    SC_HANDLE hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
    if (!hSCManager) {
        printf("[-] OpenSCManager failed: %lu\n", GetLastError());
        return FALSE;
    }

    SC_HANDLE hService = CreateServiceA(
        hSCManager,
        serviceName,
        serviceName,
        SERVICE_ALL_ACCESS,
        SERVICE_KERNEL_DRIVER,
        SERVICE_DEMAND_START,
        SERVICE_ERROR_IGNORE,
        driverPath,
        NULL, NULL, NULL, NULL, NULL
    );

    if (!hService) {
        printf("[-] CreateService failed: %lu\n", GetLastError());
        CloseServiceHandle(hSCManager);
        return FALSE;
    }

    if (!StartServiceA(hService, 0, NULL)) {
        printf("[-] StartService failed: %lu\n", GetLastError());
        DeleteService(hService);
        CloseServiceHandle(hService);
        CloseServiceHandle(hSCManager);
        return FALSE;
    }

    printf("[+] Driver loaded with service name: %s\n", serviceName);
    CloseServiceHandle(hService);
    CloseServiceHandle(hSCManager);
    return TRUE;
}

// Stop and delete service
BOOL RemoveDriver(const char* serviceName) {
    SC_HANDLE hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (!hSCManager) {
        printf("[-] OpenSCManager failed: %lu\n", GetLastError());
        return FALSE;
    }

    SC_HANDLE hService = OpenServiceA(hSCManager, serviceName, SERVICE_STOP | DELETE);
    if (!hService) {
        printf("[-] OpenService failed: %lu\n", GetLastError());
        CloseServiceHandle(hSCManager);
        return FALSE;
    }

    SERVICE_STATUS status;
    ControlService(hService, SERVICE_CONTROL_STOP, &status);

    if (!DeleteService(hService)) {
        printf("[-] DeleteService failed: %lu\n", GetLastError());
    }
    else {
        printf("[+] Service %s stopped and deleted.\n", serviceName);
    }

    CloseServiceHandle(hService);
    CloseServiceHandle(hSCManager);
    return TRUE;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage:\n");
        printf("  %s load <driver_path>\n", argv[0]);
        printf("  %s remove <service_name>\n", argv[0]);
        return 1;
    }

    const char* action = argv[1];

    if (strcmp(action, "load") == 0) {
        if (argc != 3) {
            printf("Usage: %s load <driver_path>\n", argv[0]);
            return 1;
        }
        char serviceName[MAX_SERVICE_NAME_LEN];
        if (LoadDriver(argv[2], serviceName)) {
            printf("[*] Service name to use for removal: %s\n", serviceName);
        }
    }
    else if (strcmp(action, "remove") == 0) {
        if (argc != 3) {
            printf("Usage: %s remove <service_name>\n", argv[0]);
            return 1;
        }
        RemoveDriver(argv[2]);
    }
    else {
        printf("[-] Unknown action: %s\n", action);
        return 1;
    }

    return 0;
}