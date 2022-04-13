#include <stdio.h>

#include "edge-impulse-sdk/classifier/ei_run_classifier.h"

#include<iostream>
#include<chrono>
#include<string>
#include<stdlib.h>
#include"SerialPort.h"

using std::chrono::duration_cast;
using std::chrono::milliseconds;
using std::chrono::seconds;
using std::chrono::system_clock;

char output[MAX_DATA_LENGTH];
char input[MAX_DATA_LENGTH];
char port[10];
float data[1000];
int highest_label = 0;

// Callback function declaration
static int get_signal_data(size_t offset, size_t length, float* out_ptr);

int main(int argc, char** argv) {

    int s = 0;
    int data_read = 0;
    long size_updated = 0;
    int size_updated_float=0;
    char recv_data[100];
    float temp = 0;
    signal_t signal;            // Wrapper for raw input buffer
    ei_impulse_result_t result; // Used to store inference output
    EI_IMPULSE_ERROR res;       // Return code from inference

    // std::cout << "please enter COM port: ";
    // std::cin >> s;
    if(argc == 2)
    {
        s = atoi(argv[1]);
    }
    else
    {
        std::cout << "please add port number" << argc;
        //exit(0);
    }
    sprintf(port,"\\\\.\\COM%d",s);

    SerialPort esp(port);
        if (esp.isConnected()) {
        std::cout << "connection is established";
        std::flush(std::cout);
    }
    else
    {
        std::cout << "\nerror\n";
        std::flush(std::cout);
        while(true){}
        //exit(0);
    }
    auto millisec_start = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();

    // Assign callback function to fill buffer used for preprocessing/inference
    signal.total_length = EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE;

    while (true)
    {
        while (esp.isConnected())
        {
            auto millisec_now = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
            data_read = esp.readSerialPort(input, MAX_DATA_LENGTH);
            if(data_read > 0)
            {  
                for (int i = 0; i < data_read; i++)
                {
                    if (input[i] == '\n')
                    {
                        input[i] = ',';
                    }
                    if (input[i] != ',')
                    {
                        recv_data[size_updated] = input[i];
                        size_updated++;
                    }
                    else
                    {
                        size_updated = 0;
                        data[size_updated_float] = strtof((char *)recv_data, NULL);
                        size_updated_float++;
                        // std::cout << size_updated_float << "\n";
                        if (size_updated_float == 1000)
                        {
                            std::cout << "float size exceeded";
                            size_updated_float = 0;
                        }
                        memset((char *)recv_data, 0, sizeof(recv_data));
                    }
                }
}

if(millisec_now - millisec_start > 1000)
{
    millisec_start = millisec_now;
    signal.get_data = &get_signal_data;
    size_updated_float = 0;
    // Perform DSP pre-processing and inference
    res = run_classifier(&signal, &result, false);
    memset(data, 0, sizeof(data));
        // Print return code and how long it took to perform inference
    // printf("run_classifier returned: %d\r\n", res);
    // printf("Timing: DSP %d ms, inference %d ms, anomaly %d ms\r\n",
    // result.timing.dsp,
    // result.timing.classification,
    // result.timing.anomaly);
    // printf("Predictions:\r\n");
    temp = result.classification[0].value;
    for (uint16_t i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
        // std::cout << ei_classifier_inferencing_categories[i] << ":";
        // std::cout << result.classification[i].value << "\r\n";
        if(temp <=  result.classification[i].value)
        {
            temp = result.classification[i].value;
            highest_label = i;
        }
    }
    if(result.classification[highest_label].value > 0.9f)
    {
        printf("%d\n",highest_label+1);
    }
    std::flush(std::cout);
}
        }
    }

    return 0;
}

// Callback: fill a section of the out_ptr buffer when requested
static int get_signal_data(size_t offset, size_t length, float* out_ptr) {
    for (size_t i = 0; i < length; i++) {
        out_ptr[i] = (data + offset)[i];
    }

    return EIDSP_OK;
}