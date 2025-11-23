#include "GPIO_My.h"

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
float FilterAngle(float new_angle)
{
    static float buffer[ANGLE_FILTER_SIZE] = {0};
    static int idx = 0;
    static int count = 0;
    buffer[idx] = new_angle;
    idx = (idx + 1) % ANGLE_FILTER_SIZE;
    if (count < ANGLE_FILTER_SIZE) count++;

    float sum = 0;
    for (int i = 0; i < count; i++) {
        sum += buffer[i];
    }
    return sum / count;
}

// 示例：读取并滤波
float GetFilteredAngle(void)
{
    float raw_angle = ReadAngleFromUART1();
    return FilterAngle(raw_angle);
}

void Update(output_data_t *out_data)
{
    // 设置固定头部 0x44, 0x44, 0x54
    out_data->header[0] = 0x44;
    out_data->header[1] = 0x44;
    out_data->header[2] = 0x54;
    
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
    int btn_ls = gpio_get_level(BTN_LS);
    int btn_lm = gpio_get_level(BTN_LM);
    int btn_r1 = gpio_get_level(BTN_R1);
    int btn_r2 = gpio_get_level(BTN_R2);
    int btn_r3 = gpio_get_level(BTN_R3);
    int btn_rs = gpio_get_level(BTN_RS);
    int btn_rm = gpio_get_level(BTN_RM);
    int key_1 = gpio_get_level(Key_1);
    
    // 映射按钮到修复后的Ontroller格式（与mu3io.c的8位格式一致）
    // 使用MU3_IO_GAMEBTN枚举：1=0x01, 2=0x02, 3=0x04, SIDE=0x08, MENU=0x10
    // 左按钮映射 (buffer[3])
    if (btn_l1) left_buttons |= 0x01;  // 左A: MU3_IO_GAMEBTN_1
    if (btn_l2) left_buttons |= 0x02;  // 左B: MU3_IO_GAMEBTN_2  
    if (btn_l3) left_buttons |= 0x04;  // 左C: MU3_IO_GAMEBTN_3
    if (btn_ls) left_buttons |= 0x08;  // 左Side: MU3_IO_GAMEBTN_SIDE
    if (btn_lm) left_buttons |= 0x10;  // 左Menu: MU3_IO_GAMEBTN_MENU
    
    // 右按钮映射 (buffer[4])
    if (btn_r1) right_buttons |= 0x01;  // 右A: MU3_IO_GAMEBTN_1
    if (btn_r2) right_buttons |= 0x02;  // 右B: MU3_IO_GAMEBTN_2
    if (btn_r3) right_buttons |= 0x04;  // 右C: MU3_IO_GAMEBTN_3
    if (btn_rs) right_buttons |= 0x08;  // 右Side: MU3_IO_GAMEBTN_SIDE
    if (btn_rm) right_buttons |= 0x10;  // 右Menu: MU3_IO_GAMEBTN_MENU
    
    // 操作按钮映射 (buffer[4]高位) - 与PC端映射一致
    if (key_1) operation_buttons |= 0x20;  // Test
    
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
    float angle = GetFilteredAngle();
    if (angle < 0) angle = 0;
    if (angle > 360) angle = 360;
    
    // 修复摇杆映射：与PC端转换公式一致
    // PC端公式：raw * 80 - short.MaxValue
    // 因此ESP32端应该是：(lever_value + 32767) / 80
    // 但这里需要反向计算：从角度直接得到raw值
    
    // 将角度映射到int16_t范围 (-32768 到 32767)
    int16_t lever_value = (int16_t)((angle / 360.0f) * 65536 - 32768);
    
    // 转换为PC端期望的raw格式
    // 反向计算：如果PC端是 raw * 80 - 32767，那么ESP32应该是 (lever_value + 32767) / 80
    uint16_t raw_lever = (uint16_t)((lever_value + 32767) / 80);
    
    out_data->lever[0] = (uint8_t)(raw_lever & 0xFF);        // 低字节
    out_data->lever[1] = (uint8_t)((raw_lever >> 8) & 0xFF); // 高字节
}