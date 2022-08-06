#include <cstdio>
#include <ctime>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <hidapi.h>
#include <numeric> 
#include <iostream>      

const unsigned short VENDOR_ID = 0x16c0;
const unsigned short PRODUCT_ID = 0x0480;
const int MAX_RESULT_CHECK = 5;

int check_temp(hid_device *handle, int devices) {
    int device_result[devices]; 
    int device_result_sum=0;
    int loops = 0;
    for(int i=0; i<devices; i++) {
        device_result[i]=0;
    }
    device_result_sum = std::accumulate(device_result, device_result+devices, 0);
    while ( loops++ < MAX_RESULT_CHECK && device_result_sum < devices ) {
        unsigned char buf[65];
        int num = hid_read(handle, buf, 64);
        char sign[2] = "+"; 
        if (num < 0) {
            fprintf(stderr, "Could not read from device!\n");
            return 5;
        }
        if (num == 64) {
            unsigned char devices = buf[0];
            unsigned char device_id = buf[1];
            time_t now;
            time(&now);
            tm loctm;
            localtime_r(&now, &loctm);
            short temp = *(short *)&buf[4]; //this is fine. just enjoy your tea
            float ftemp = (float)temp/10.0f;
            if (ftemp>= 0) {
                sign[0] ='+';
            } else {
                sign[0] ='-';
            }
            
            printf("%04d-%02d-%02dT%02d:%02d:%02d;%d;%d;%+.1f;%s;%.1f;°C;%d\n", loctm.tm_year+1900, loctm.tm_mon+1, loctm.tm_mday, loctm.tm_hour, loctm.tm_min, loctm.tm_sec, devices, device_id, ftemp, sign, ftemp, device_result_sum);
            device_result[device_id-1]=1;
        }
        device_result_sum = std::accumulate(device_result, device_result+devices, 0);
    }
    return 0;
}

int get_num_devices(hid_device *handle) {
    unsigned char buf[65];
    int num = hid_read(handle, buf, 64);
    if (num < 0) {
        fprintf(stderr, "Could not read from device!\n");
        return -1;
    }
    if (num == 64) {
        return buf[0];
    } else {
        return -1;
    }
}

int print_adapter_info(hid_device *handle, int devices) {
    const int MAX_STR = 255;
    wchar_t wstr[MAX_STR];
    printf("# Device Info:\n");
    int res = hid_get_manufacturer_string(handle, wstr, MAX_STR);
    if (res == -1) {
        fprintf(stderr, "Could not get manufacturer string!\n");
        return 2;
    }
    printf("#   %-25s: %ls\n","Manufacturer String", wstr);
    
    res = hid_get_product_string(handle, wstr, MAX_STR);
    if (res == -1) {
        fprintf(stderr, "Could not get product string!\n");
        return 3;
    }
    printf("#   %-25s: %ls\n", "Product String", wstr);

    res = hid_get_serial_number_string(handle, wstr, MAX_STR);
    if (res == -1) {
        fprintf(stderr, "Could not get serial number string!\n");
        return 4;
    }
    char prefix[32];
    sprintf(prefix, "#   %-25s: ", "Serial Number");
    std::cout << prefix << wstr << "\n";
    printf("#   %-25s: %d\n", "Number of sensors found", devices);
    return 0;

}

int main(int argc, char **argv) {
    bool continuousMode = false;
    int c;
    while ((c = getopt (argc, argv, "c")) != -1)
    switch (c)
      {
      case 'c':
        continuousMode = true;
        break;
      case '?':
        if (isprint (optopt))
          fprintf (stderr, "Unknown option `-%c'.\n", optopt);
        else
          fprintf (stderr,
                   "Unknown option character `\\x%x'.\n",
                   optopt);
        return 1;
      default:
        abort ();
      }
    
    hid_device *handle = hid_open(VENDOR_ID, PRODUCT_ID, nullptr);
    if (!handle) {
        fprintf(stderr, "No temperature sensor found!\n");
        return 1;
    }
    int devices = get_num_devices(handle);    
    if ( devices >0 ) {
        int result = print_adapter_info(handle,devices); 
        if (result != 0 ) {
            return result;
        }
        if (continuousMode) {    
            for (;;) {
                check_temp(handle,devices);
            }
        } else {
            check_temp(handle,devices);
        }
    } else {
        fprintf(stderr, "No devices found");
        return 1;
    }
    hid_close(handle);
    return 0;
}

