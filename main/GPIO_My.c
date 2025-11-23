#include "GPIO_My.h"
#include <math.h>
#include "winusb_new.h"

// 标定相关静态变量
static bool calib_in_progress = false;
static uint32_t calib_start_tick = 0;
static float calib_left_angle = 0.0f;
static float calib_right_angle = 0.0f;
static bool calib_valid = false;
// 标定结果：参考角与偏差范围（相对于 ref, min_delta..max_delta）
static float calib_ref_deg = 0.0f;
static float calib_min_delta = 0.0f;
static float calib_max_delta = 0.0f;

// NVS 存储 key
static const char *NVS_NAMESPACE = "joystick";
static const char *NVS_KEY = "calib_v1";

// 最小有效跨度（度），低于该值视为标定失败
#define CALIB_MIN_SPAN_DEG 8.0f

// 角度工具
static inline float deg2rad(float d) { return d * 0.017453292519943295769236f; }
static inline float rad2deg(float r) { return r * 57.295779513082320876798f; }
static float normalize_deg_180(float a) {
    while (a <= -180.0f) a += 360.0f;
    while (a >  180.0f) a -= 360.0f;
    return a;
}
static float normalize_deg_360(float a) {
    while (a < 0.0f) a += 360.0f;
    while (a >= 360.0f) a -= 360.0f;
    return a;
}

// 将角度映射到 int16 全范围，基于标定 ref/min/max
// 映射到较小范围 -4096..4096
static int16_t map_angle_to_int16_from_calib(float angle_deg)
{
    float d = normalize_deg_180(angle_deg - calib_ref_deg);
    float span = calib_max_delta - calib_min_delta;
    if (span <= 0.0001f) {
        return 0; // fallback
    }
    float frac = (d - calib_min_delta) / span;
    if (frac < 0.0f) frac = 0.0f;
    if (frac > 1.0f) frac = 1.0f;
    // map to -4096..4096
    const int32_t MINV = -4096;
    const int32_t WIDTH = 8192; // inclusive mapping
    float scaled = (float)MINV + frac * (float)WIDTH;
    if (scaled < (float)MINV) scaled = (float)MINV;
    if (scaled > (float)(MINV + WIDTH)) scaled = (float)(MINV + WIDTH);
    return (int16_t) (int32_t) roundf(scaled);
}

// 将标定结果写入 NVS
static void save_calib_to_nvs(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS 分区需要擦除后重试
        ESP_LOGW("GPIO", "nvs_flash_init requires erase, performing nvs_flash_erase and retry");
        nvs_flash_erase();
        err = nvs_flash_init();
    }
    if (err != ESP_OK) {
        ESP_LOGW("GPIO", "nvs_flash_init failed: %s", esp_err_to_name(err));
        // continue and try open anyway
    }

    nvs_handle_t handle;
    err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        ESP_LOGE("GPIO", "nvs_open failed: %s", esp_err_to_name(err));
        return;
    }

    // 存储三项 float
    struct {
        float ref;
        float min_delta;
        float max_delta;
    } blob = { calib_ref_deg, calib_min_delta, calib_max_delta };

    err = nvs_set_blob(handle, NVS_KEY, &blob, sizeof(blob));
    if (err != ESP_OK) {
        ESP_LOGE("GPIO", "nvs_set_blob failed: %s", esp_err_to_name(err));
    } else {
        err = nvs_commit(handle);
        if (err != ESP_OK) {
            ESP_LOGE("GPIO", "nvs_commit failed: %s", esp_err_to_name(err));
        } else {
            ESP_LOGI("GPIO", "Calibration saved to NVS: ref=%.2f min=%.2f max=%.2f",
                     calib_ref_deg, calib_min_delta, calib_max_delta);
        }
    }

    nvs_close(handle);
}

// 从 NVS 读取标定结果（如果存在）
static void load_calib_from_nvs(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // try erase and reinit
        nvs_flash_erase();
        err = nvs_flash_init();
    }
    if (err != ESP_OK) {
        ESP_LOGI("GPIO", "nvs_flash_init failed: %s", esp_err_to_name(err));
        // proceed to try opening (may fail)
    }

    nvs_handle_t handle;
    err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &handle);
    if (err != ESP_OK) {
        ESP_LOGI("GPIO", "No calibration in NVS (nvs_open: %s)", esp_err_to_name(err));
        calib_valid = false;
        return;
    }

    struct {
        float ref;
        float min_delta;
        float max_delta;
    } blob = {0};
    size_t required = sizeof(blob);
    err = nvs_get_blob(handle, NVS_KEY, &blob, &required);
    if (err != ESP_OK || required != sizeof(blob)) {
        ESP_LOGI("GPIO", "No calibration blob in NVS (err=%s)", esp_err_to_name(err));
        calib_valid = false;
    } else {
        calib_ref_deg = blob.ref;
        calib_min_delta = blob.min_delta;
        calib_max_delta = blob.max_delta;
        calib_valid = true;
        ESP_LOGI("GPIO", "Loaded calibration from NVS: ref=%.2f min=%.2f max=%.2f",
                 calib_ref_deg, calib_min_delta, calib_max_delta);
    }

    nvs_close(handle);
}

// 映射模式状态
static bool mapping_mode = false;
static uint32_t mapping_mode_timer = 0;
static bool simulate_test_press = false;

// 注释掉光电传感器滤波参数
// #define OPTOSENSOR_DEBOUNCE_THRESHOLD 3  // 需要连续检测到相同状态的次数
// #define OPTOSENSOR_SAMPLE_INTERVAL 1     // 采样间隔(ms)

// 初始化指定GPIO为上拉输入
void gpio_my_init_input_pullup(gpio_num_t gpio_num)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << gpio_num),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,    // 启用上拉电阻
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);
}

void BTN_init(void)
{
    gpio_my_init_input_pullup(BTN_L1); 
    gpio_my_init_input_pullup(BTN_L2);
    gpio_my_init_input_pullup(BTN_L3);
    gpio_my_init_input_pullup(BTN_LS);
    gpio_my_init_input_pullup(BTN_LM);
    gpio_my_init_input_pullup(BTN_R1);
    gpio_my_init_input_pullup(BTN_R2);
    gpio_my_init_input_pullup(BTN_R3);
    gpio_my_init_input_pullup(BTN_RS);
    gpio_my_init_input_pullup(BTN_RM);
    gpio_my_init_input_pullup(Key_1);
}
void Ang_init(void)
{
    // UART1 配置参数
    const uart_config_t uart1_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };
    uart_param_config(UART_NUM_1, &uart1_config);
    uart_set_pin(UART_NUM_1, UART_TX, UART_RX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(UART_NUM_1, 1024, 1024, 0, NULL, 0);
}

int PinMap[11][2] = {
    {BTN_L1, 0},
    {BTN_L2, 0},
    {BTN_L3, 0},
    {BTN_LS, 0},
    {BTN_LM, 0},
    {BTN_R1, 0},
    {BTN_R2, 0},
    {BTN_R3, 0},
    {BTN_RS, 0},
    {BTN_RM, 0},
    {Key_1, 0},
};

// 读取UART1数据并解析角度
float ReadAngleFromUART1(void)
{
    uint8_t data[32] = {0};
    int len = uart_read_bytes(UART_NUM_1, data, sizeof(data)-1, 100 / portTICK_PERIOD_MS);
    if (len <= 0) return 0.0f;

    data[len] = '\0';

    char *start = strstr((char *)data, "Angle:");
    if (start) {
        start += strlen("Angle:");
        float angle = atof(start);
        return angle;
    }
    return 0.0f;
}

// 简单滑动平均滤波
// float FilterAngle(float new_angle)
// {
//     static float buffer[ANGLE_FILTER_SIZE] = {0};
//     static int idx = 0;
//     static int count = 0;
//     buffer[idx] = new_angle;
//     idx = (idx + 1) % ANGLE_FILTER_SIZE;
//     if (count < ANGLE_FILTER_SIZE) count++;

//     float sum = 0;
//     for (int i = 0; i < count; i++) {
//         sum += buffer[i];
//     }
//     return sum / count;
// }

// // 示例：读取并滤波
// float GetFilteredAngle(void)
// {
//     float raw_angle = ReadAngleFromUART1();
//     return FilterAngle(raw_angle);
// }

// 注释掉光电传感器状态滤波函数
// 光电传感器状态滤波函数
// 使用状态机和计数器实现滤波
// 返回值：滤波后的传感器状态（0=检测到物体，1=未检测到物体）
// int FilterOptoSensorState(gpio_num_t gpio_num, int raw_state)
// {
//     // 为每个光电传感器创建独立的状态变量
//     static int ls_counter = 0;
//     static int ls_state = 1;  // 初始状态设为1（未检测到）
//     static int rs_counter = 0;
//     static int rs_state = 1;  // 初始状态设为1（未检测到）
//     
//     // 获取相应的计数器和状态指针
//     int* counter = (gpio_num == BTN_LS) ? &ls_counter : &rs_counter;
//     int* state = (gpio_num == BTN_LS) ? &ls_state : &rs_state;
//     
//     // 如果当前读取的状态与已确认的状态相同，重置计数器
//     if (raw_state == *state) {
//         *counter = 0;
//     }
//     // 如果不同，增加计数器
//     else {
//         (*counter)++;
//         
//         // 如果连续检测到不同状态达到阈值次数，更新状态
//         if (*counter >= OPTOSENSOR_DEBOUNCE_THRESHOLD) {
//             *state = raw_state;
//             *counter = 0;
//             
//             // 记录状态变化（调试用）
//             ESP_LOGD("GPIO", "Opto sensor %s state changed to %d", 
//                      (gpio_num == BTN_LS) ? "LS" : "RS", *state);
//         }
//     }
//     
//     return *state;
// }

void Update(output_data_t *out_data)
{
    // 安全检查：确保out_data指针有效
    if (out_data == NULL) {
        ESP_LOGE("GPIO", "Update: out_data is NULL");
        return;
    }
    
    // 清零整个结构，确保干净的状态
    memset(out_data, 0, sizeof(output_data_t));
    
    // 设置固定头部 0x44, 0x44, 0x54
    out_data->header[0] = 0x44;
    out_data->header[1] = 0x44;
    out_data->header[2] = 0x54;
    
    // 验证头部设置是否正确
    if (out_data->header[0] != 0x44 || out_data->header[1] != 0x44 || out_data->header[2] != 0x54) {
        ESP_LOGE("GPIO", "Header corruption detected: %02X %02X %02X", 
                 out_data->header[0], out_data->header[1], out_data->header[2]);
        // 重新设置头部
        out_data->header[0] = 0x44;
        out_data->header[1] = 0x44;
        out_data->header[2] = 0x54;
    }
    
    // 静态变量用于状态稳定性检查
    static bool initialized = false;
    static int init_counter = 0;
    
    // 初始化阶段：等待GPIO状态稳定
    if (!initialized) {
        init_counter++;
        if (init_counter < 50) { // 等待50次更新周期让状态稳定
            // 初始化期间发送空状态
            out_data->buttons[0] = 0;
            out_data->buttons[1] = 0;
            out_data->lever[0] = 0;
            out_data->lever[1] = 0;
            return;
        }
        initialized = true;
        ESP_LOGI("GPIO", "按钮状态初始化完成");
        // 尝试从 NVS 加载先前的标定
        load_calib_from_nvs();
    }
    
    // 采集按钮状态并映射到Ontroller格式
    // 修复后的按钮映射（与GAMEHOOK一致）：
    // Left (buffer[3]): A(0x01), B(0x02), C(0x04), Side(0x08), Menu(0x10)
    // Right (buffer[4]): A(0x01), B(0x02), C(0x04), Side(0x08), Menu(0x10)
    // Operation (buffer[4]): Test(0x20), Service(0x40)  // 使用buffer[4]的高位
    
    uint8_t left_buttons = 0;
    uint8_t right_buttons = 0;
    uint8_t operation_buttons = 0;
    
    // 调试：读取所有GPIO状态
    int btn_l1 = gpio_get_level(BTN_L1);
    int btn_l2 = gpio_get_level(BTN_L2);
    int btn_l3 = gpio_get_level(BTN_L3);
    int btn_ls = gpio_get_level(BTN_LS); // 直接使用原始值
    int btn_lm = gpio_get_level(BTN_LM);
    int btn_r1 = gpio_get_level(BTN_R1);
    int btn_r2 = gpio_get_level(BTN_R2);
    int btn_r3 = gpio_get_level(BTN_R3);
    int btn_rs = gpio_get_level(BTN_RS); // 直接使用原始值
    int btn_rm = gpio_get_level(BTN_RM);
    int key_1 = gpio_get_level(Key_1);
    
    // 注释掉滤波应用逻辑
    // 对光电传感器应用滤波算法
    // static uint32_t last_opto_sample_time = 0;
    uint32_t current_time = xTaskGetTickCount();
    // static int filtered_btn_ls = 1; // 初始状态设为1（未检测到）
    // static int filtered_btn_rs = 1; // 初始状态设为1（未检测到）
    
    // 按照设定的采样间隔对光电传感器进行采样和滤波
    // if (current_time - last_opto_sample_time >= pdMS_TO_TICKS(OPTOSENSOR_SAMPLE_INTERVAL)) {
    //     filtered_btn_ls = FilterOptoSensorState(BTN_LS, raw_btn_ls);
    //     filtered_btn_rs = FilterOptoSensorState(BTN_RS, raw_btn_rs);
    //     last_opto_sample_time = current_time;
    // }
    
    // 映射按钮到修复后的Ontroller格式（与mu3io.c的8位格式一致）
    // 修复：上拉电阻配置下，按下为低电平(0)，松开为高电平(1)
    // 使用MU3_IO_GAMEBTN枚举：1=0x01, 2=0x02, 3=0x04, SIDE=0x08, MENU=0x10
    // 左按钮映射 (buffer[3])
    if (!btn_l1) left_buttons |= 0x01;  // 左A: MU3_IO_GAMEBTN_1 (按下时为0)
    if (!btn_l2) left_buttons |= 0x02;  // 左B: MU3_IO_GAMEBTN_2  
    if (!btn_l3) left_buttons |= 0x04;  // 左C: MU3_IO_GAMEBTN_3
    if (!btn_ls) left_buttons |= 0x08;  // 左Side: 直接使用原始值
    if (!btn_lm) left_buttons |= 0x10;  // 左Menu: MU3_IO_GAMEBTN_MENU
    
    // 右按钮映射 (buffer[4])
    if (!btn_r1) right_buttons |= 0x01;  // 右A: MU3_IO_GAMEBTN_1 (按下时为0)
    if (!btn_r2) right_buttons |= 0x02;  // 右B: MU3_IO_GAMEBTN_2
    if (!btn_r3) right_buttons |= 0x04;  // 右C: MU3_IO_GAMEBTN_3
    if (!btn_rs) right_buttons |= 0x08;  // 右Side: 直接使用原始值
    if (!btn_rm) right_buttons |= 0x10;  // 右Menu: MU3_IO_GAMEBTN_MENU
    
    // 操作按钮映射 (buffer[4]高位) - 使用高位避免与右按钮冲突
    // ESP32在高位发送，WinUSB.io负责重新映射为Mu3io期望的值
    if (!key_1) operation_buttons |= 0x20;  // Test: 使用0x20避免与右按钮冲突 (按下时为0)
    
    // 检查是否进入映射模式
    if (!gpio_get_level(BTN_LM) && !gpio_get_level(BTN_RM)) {
        if (mapping_mode_timer == 0) {
            mapping_mode_timer = xTaskGetTickCount();
        } else if (xTaskGetTickCount() - mapping_mode_timer >= pdMS_TO_TICKS(5000)) {
            mapping_mode = !mapping_mode;
            if (mapping_mode) {
                simulate_test_press = true; // 进入映射模式时，模拟按下TEST键
                // 发送一次LED指示：L1和R3白光，其他灯熄灭
                winusb_new_send_mapping_mode_indicator();
            }
            mapping_mode_timer = 0; // 重置计时器
            ESP_LOGI("GPIO", "映射模式已 %s", mapping_mode ? "启用" : "禁用");
        }
    } else {
        mapping_mode_timer = 0;
    }

    // 进入映射模式后，按下 LS + RS 同时触发标定流程（按下表示低电平，即 == 0）
    if (mapping_mode) {
        if (!btn_ls && !btn_rs && !calib_in_progress) {
            // 开始标定：记录左侧角度并点亮侧边LED为蓝色
            calib_in_progress = true;
            calib_start_tick = current_time;
            calib_left_angle = ReadAngleFromUART1();
            ESP_LOGI("GPIO", "Calibration started: left_angle=%.2f", calib_left_angle);
            // 点亮侧边LED为蓝色
            winusb_new_set_side_leds(0, 0, 255);
        }
    }

    // 如果处于映射模式，则重新映射按钮
    if (mapping_mode) {
        operation_buttons = 0; // 清除原始操作按钮
        if (!btn_l1) operation_buttons |= 0x20; // BTN_L1 -> Test
        if (!btn_r3) operation_buttons |= 0x40; // BTN_R3 -> Service
    }

    // 如果需要，模拟按下TEST键
    if (simulate_test_press) {
        operation_buttons |= 0x20;
        simulate_test_press = false;
    }

    // 组合按钮数据到2字节
    out_data->buttons[0] = left_buttons;
    out_data->buttons[1] = right_buttons | operation_buttons;
    
    // 只在按钮状态变化时输出调试信息
    static uint8_t last_left = 0xFF, last_right = 0xFF, last_operation = 0xFF;
    if (left_buttons != last_left || right_buttons != last_right || operation_buttons != last_operation) {
        ESP_LOGI("GPIO", "按钮状态变化: left=0x%02X, right=0x%02X, operation=0x%02X", 
                 left_buttons, right_buttons, operation_buttons);
        last_left = left_buttons;
        last_right = right_buttons;
        last_operation = operation_buttons;
    }

    // 获取摇杆角度并转换为 lever（2字节）
    static uint32_t last_angle_read = 0;
    static int16_t current_lever_value = 0;
    current_time = xTaskGetTickCount();
    
    // 降低角度读取频率，每5ms读取一次（从20ms提高到5ms以保持响应性但减少处理）
    if (current_time - last_angle_read >= pdMS_TO_TICKS(5)) {
        float angle = ReadAngleFromUART1();
        if (angle < 0) angle = 0;
        if (angle > 360) angle = 360;
        // normalize to 0..360
        angle = normalize_deg_360(angle);
        
        
        // 静态变量用于摇杆状态稳定性
        static bool lever_initialized = false;
        static int16_t stable_lever = 0;
        
        // 降低角度精度以减少数据量和CPU负载
        // 将角度映射到更小的范围，例如-4095到4095（12位精度已足够）
        int16_t new_lever_value = 0;
        // 如果已加载/完成标定，使用标定结果把角度映射到 int16 全范围
        if (calib_valid) {
            new_lever_value = map_angle_to_int16_from_calib(angle);
        } else {
            // 退回到原有的较小范围映射，保持向下兼容
            new_lever_value = (int16_t)((angle / 360.0f) * 8192 - 4096);
        }
        
        // 角度变化阈值过滤，只有变化超过一定值才更新
        if (!lever_initialized || abs(new_lever_value - current_lever_value) > 10) {
            current_lever_value = new_lever_value;
            
            // 摇杆状态稳定性检查（仅初始化时或大变化时记录日志）
            if (!lever_initialized) {
                stable_lever = current_lever_value;
                lever_initialized = true;
                ESP_LOGI("GPIO", "摇杆初始位置: %d (角度: %.1f)", current_lever_value, angle);
            } else if (abs(current_lever_value - stable_lever) > 10) {
                ESP_LOGI("GPIO", "摇杆位置变化: %d -> %d (角度: %.1f)", stable_lever, current_lever_value, angle);
                stable_lever = current_lever_value;
            }
        }
        
        last_angle_read = current_time;
    }

    // 标定流程处理：在开始后等待 2 秒钟，然后记录右侧角度，计算并保存标定结果
    if (calib_in_progress) {
        if (current_time - calib_start_tick >= pdMS_TO_TICKS(2000)) {
            calib_right_angle = ReadAngleFromUART1();
            calib_right_angle = normalize_deg_360(calib_right_angle);
            // 计算从 left 到 right 的最短正向差值
            float d = normalize_deg_180(calib_right_angle - calib_left_angle);
            if (d < 0.0f) d += 360.0f; // 正向跨度
            float span = d;
            if (span < CALIB_MIN_SPAN_DEG) {
                ESP_LOGW("GPIO", "Calibration span too small: %.2f deg (fail)", span);
                calib_valid = false;
            } else {
                // 参考角为 left + span/2
                float ref = calib_left_angle + span / 2.0f;
                ref = normalize_deg_360(ref);
                calib_ref_deg = ref;
                calib_min_delta = -span / 2.0f - 8.0f; // 留出8度余量
                calib_max_delta = span / 2.0f + 8.0f;
                calib_valid = true;
                ESP_LOGI("GPIO", "Calibration done: left=%.2f right=%.2f span=%.2f ref=%.2f",
                         calib_left_angle, calib_right_angle, span, calib_ref_deg);
                // 保存到 NVS
                save_calib_to_nvs();
            }
            // 结束标定：熄灭侧边LED
            winusb_new_set_side_leds(0, 0, 0);
            calib_in_progress = false;
        }
    }
    
    // 直接发送当前的摇杆值，小端序(低字节在前)
    out_data->lever[0] = (uint8_t)(current_lever_value & 0xFF);        // 低字节
    out_data->lever[1] = (uint8_t)((current_lever_value >> 8) & 0xFF); // 高字节
}