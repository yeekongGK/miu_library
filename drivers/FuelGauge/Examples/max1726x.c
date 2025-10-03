/******************************************************************************
 * File:        max1726x.c
 * Author:      CYK
 * Created:     05-10-2025
 * Last Update: 05-10-2025
 *
 * Description:
 *   This file contains example code demonstrating the usage of the MAX1726x
 *   fuel gauge driver. It initializes the sensor, checks for a Power-On-Reset
 *   (POR) condition, and enters a loop to periodically read and print the
 *   State of Charge (SOC), voltage, and current.
 *
 * Notes:
 *   - This is example code and may not be part of the main application firmware.
 *
 * To Do:
 *   - -
 *
 ******************************************************************************/


int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_I2C1_Init();
  MX_USART1_UART_Init();

  /* USER CODE BEGIN 2 */
  BatteryMonitor_Init();

  for (uint16_t addr = 1; addr < 128; addr++)
  {
	  if (HAL_I2C_IsDeviceReady(&hi2c1, (uint16_t)(addr << 1), 2, 10) == HAL_OK)
	  {
		  UART_Printf("Found device at 0x%02X\r\n", addr);
	  }
  }


  // if (maxim_max1726x_check_por()) {
  //     UART_Printf("POR detected. Initializing MAX1726x with EZ Config...\r\n");
  //     maxim_max1726x_wait_dnr();                // Wait for the chip to be ready
  //     maxim_max1726x_initialize_ez_config();    // Load your battery parameters
  //     maxim_max1726x_clear_por();               // Clear the POR flag
  //     UART_Printf("Initialization complete.\r\n");
  // } else {
  //     UART_Printf("No POR detected. MAX1726x already configured.\r\n");
  // }
  MAX17260_Register_printout();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

	  // uint16_t value = BatteryMonitor_GetQH();
	  // UART_Printf("1test %d\r\n",value);
    // HAL_Delay(1000);

    float soc_percentage;
    float voltage_f;
    float current_f;

    uint16_t vcell_raw;
    int16_t current_raw; // Current can be negative (discharging)

    // Read State of Charge (%) using the library's helper function
    soc_percentage = maxim_max1726x_get_repsoc();

    // Read raw register values for Voltage and Current
    maxim_max1726x_read_reg(MAX1726X_VCELL_REG, &vcell_raw);
    maxim_max1726x_read_reg(MAX1726X_CURRENT_REG, (uint16_t*)&current_raw);

    // Convert raw values to human-readable format
    // Datasheet: VCell LSB = 78.125 uV
    voltage_f = vcell_raw * 78.125f / 1000.0f; // Result in mV

    // Datasheet: Current LSB = 1.5625 uV / Rsense. Assuming Rsense = 10mOhm (0.01 Ohm)
    // LSB = 1.5625uV / 0.01Ohm = 156.25 uA = 0.15625 mA
    current_f = current_raw * 0.15625f; // Result in mA

    // Print the results
    UART_Printf("SOC: %.2f%%, Voltage: %.0f mV, Current: %.2f mA\r\n",
            soc_percentage, voltage_f, current_f);

    // Wait for a couple of seconds before the next reading
    HAL_Delay(2000);

  }
  /* USER CODE END 3 */
}
