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

// Callback function declaration
static int get_signal_data(size_t offset, size_t length, float* out_ptr);

// Raw features copied from test sample (Edge Impulse > Model testing)
static float input_buf[] = {
    /* Paste your raw features here! */
    77482.3438, 5991.8633, 12915.3584, -892.4052, 29.4200, -558.9791, -313.8128, -98.0665, -6305.6763, 76962.5938, 7139.2412, 13288.0107, -1382.7377, -2706.6355, 254.9729, -343.2328, -127.4865, -6364.5161, 77786.3516, 4815.0654, 14317.7090, -2853.7351, -3314.6477, 1627.9039, -431.4926, -235.3596, -6384.1294, 80247.8203, 9277.0908, 7570.7339, -4069.7598, -5383.8511, 1127.7648, -480.5258, -245.1663, -6335.0962, 84307.7734, 1480.8042, 764.9187, -2471.2759, -5795.7305, 5344.6245, -480.5258, -137.2931, -6256.6426, 83042.7109, -8698.4990, -58.8399, -4746.4185, -2177.0764, 3353.8743, -509.9458, -107.8732, -6187.9961, 81512.8750, -6992.1416, -2500.6958, -3648.0740, -2530.1157, 2265.3362, -549.1724, -147.0997, -6207.6094, 77717.7031, -8512.1719, 441.2993, -2530.1157, -1500.4175, 2206.4963, -549.1724, -137.2931, -6227.2227, 76099.6016, -12679.9990, 2333.9827, -892.4052, -2726.2488, 2059.3965, -558.9791, -117.6798, -6227.2227, 73304.7109, -10120.4629, 5158.2979, -382.4594, -3196.9680, 509.9458, -608.0123, -98.0665, -6197.8027, 95134.3125, -11159.9678, -3010.6416, 1196.4114, -10218.5293, -2647.7957, -568.7857, -58.8399, -6109.5430, 81365.7734, 19.6133, -19064.1270, -4677.7720, 8168.9395, -862.9852, -500.1392, -29.4200, -6050.7031, 77090.0781, -5050.4248, 9041.7314, 843.3719, -1961.3301, -2902.7686, -529.5591, -39.2266, -6070.3164, 83454.5938, 2108.4297, -10267.5625, -490.3325, -19.6133, -3677.4939, -509.9458, -39.2266, -6099.7363, 80061.4922, 912.0185, 3226.3879, 245.1663, 294.1995, -568.7857, -470.7192, 0.0000, -6060.5098, 80983.3203, -117.6798, 1176.7980, -254.9729, -274.5862, -608.0123, -451.1059, -19.6133, -6089.9297, 80865.6406, 107.8732, 19.6133, -127.4865, -1216.0247, -343.2328, -500.1392, -39.2266, -6129.1563, 81385.3906, 2991.0283, -1284.6711, -205.9397, -823.7586, 558.9791, -558.9791, -58.8399, -6129.1563, 79934.0078, -4167.8262, -294.1995, 823.7586, -1039.5049, 3059.6748, -519.7524, -29.4200, -6080.1230, 80551.8281, -6776.3950, -6246.8359, 1520.0308, -1412.1576, 1588.6774, -451.1059, 0.0000, -6011.4766
};

int main(int argc, char** argv) {

    int s = 0;
    int data_read = 0;
    long size_updated = 0;
    int size_updated_float=0;
    char recv_data[100];
    signal_t signal;            // Wrapper for raw input buffer
    ei_impulse_result_t result; // Used to store inference output
    EI_IMPULSE_ERROR res;       // Return code from inference

    std::cout << "please enter COM port: ";
    std::cin >> s;
    sprintf(port,"\\\\.\\COM%d",s);

	SerialPort esp(port);
    auto millisec_start = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();

    // Calculate the length of the buffer
    size_t buf_len = sizeof(input_buf) / sizeof(input_buf[0]);

    // Make sure that the length of the buffer matches expected input length
    if (buf_len != EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE) {
        printf("ERROR: The size of the input buffer is not correct.\r\n");
        printf("Expected %d items, but got %d\r\n",
            EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE,
            (int)buf_len);
        return 1;
    }

    // Assign callback function to fill buffer used for preprocessing/inference
    signal.total_length = EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE;

    // Print the prediction results (object detection)
#if EI_CLASSIFIER_OBJECT_DETECTION == 1
    printf("Object detection bounding boxes:\r\n");
    for (uint32_t i = 0; i < EI_CLASSIFIER_OBJECT_DETECTION_COUNT; i++) {
        ei_impulse_result_bounding_box_t bb = result.bounding_boxes[i];
        if (bb.value == 0) {
            continue;
        }
        printf("  %s (%f) [ x: %u, y: %u, width: %u, height: %u ]\r\n",
            bb.label,
            bb.value,
            bb.x,
            bb.y,
            bb.width,
            bb.height);
    }

    // Print the prediction results (classification)
#else
    printf("Predictions:\r\n");
    for (uint16_t i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
        printf("  %s: ", ei_classifier_inferencing_categories[i]);
        printf("%.5f\r\n", result.classification[i].value);
    }
#endif

    // Print anomaly result (if it exists)
#if EI_CLASSIFIER_HAS_ANOMALY == 1
    printf("Anomaly prediction: %.3f\r\n", result.anomaly);
#endif

	if (esp.isConnected()) {
		std::cout << "connection is established";
	}
	else
	{
		std::cout << "error occured!!!!!";
	}
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
                        // std::cout << size_updated_float << '\n';
                        // std::cout << (char *)recv_data << "\n";
                        data[size_updated_float] = strtof((char *)recv_data, NULL);
                        size_updated_float++;
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
                // std::cout << "\n-------------\n";
                // std::cout << "\n" << size_updated_float << "\n"; 
                // for(int j=0; j<size_updated_float;j++)
                // { 
                //     std::cout << data[j];
                // }
                // std::cout << "\n-------------\n";
                signal.get_data = &get_signal_data;

                // Perform DSP pre-processing and inference
                res = run_classifier(&signal, &result, false);
                size_updated_float = 0;
                memset(data, 0, sizeof(data));
                    // Print return code and how long it took to perform inference
                printf("run_classifier returned: %d\r\n", res);
                printf("Timing: DSP %d ms, inference %d ms, anomaly %d ms\r\n",
                result.timing.dsp,
                result.timing.classification,
                result.timing.anomaly);
                    printf("Predictions:\r\n");
    for (uint16_t i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
        printf("  %s: ", ei_classifier_inferencing_categories[i]);
        printf("%.5f\r\n", result.classification[i].value);
    }
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