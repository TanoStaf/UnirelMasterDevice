/**
  ******************************************************************************
  * @file    Project/STM8L15x_StdPeriph_Template/main.c
  * @author  MCD Application Team
  * @version V1.5.0
  * @date    13-May-2011
  * @brief   Main program body
  ******************************************************************************
  * @attention
  *
  * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
  * TIME. AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY
  * DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
  * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
  * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  * <h2><center>&copy; COPYRIGHT 2011 STMicroelectronics</center></h2>
  ******************************************************************************
  */  
	
/* Includes ------------------------------------------------------------------*/
#include "stm8l15x_conf.h"
#include "main.h"

/** @addtogroup STM8L15x_StdPeriph_Template
  * @{
  */

/* Exported types -----------------------------------------------------------*/

/** @addtogroup GENERAL_Exported_Types
  * @{
  */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define LED_RGB_REL_GPIO_PORT GPIOE
#define LED_RED_GPIO_PIN GPIO_Pin_0
#define LED_BLUE_GPIO_PIN GPIO_Pin_1
#define LED_GREEN_GPIO_PIN GPIO_Pin_2
#define TRIG_READY_GPIO_PIN GPIO_Pin_4
#define TRIG_SET_GPIO_PIN GPIO_Pin_5

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
RTC_InitTypeDef   RTC_InitStr;
RTC_TimeTypeDef   RTC_TimeStr;
RTC_AlarmTypeDef  RTC_AlarmStr;
RTC_DateTypeDef  RTC_DateStr;

/* Флаг сработавшего таймера */
__IO bool AlarmOccurred = FALSE;
/* Часы, минуты, секунды для отсчета */
__IO unsigned int HHH, MM, SS;
/* Режим - 0 - Timr, 1 - Safe */
__IO bool _mode;
/* Отсчет - 0 выключен, 1 - включен */
__IO bool _timer_set;
__IO bool _alarm_set;
__IO bool _trig_ready;
__IO bool _trig_set;
/* Маска вывода времени */
uint8_t UARTStringCurrTime[9] = "HH:MM:SS"; /* [Current Time] */
uint8_t UARTStringAlarmTime[9] = "HH:MM:SS"; /* [Alarm Time] */
/* Маска вывода даты */
uint8_t UARTStringCurrDate[9] = "mm:dd"; /* [Date] */
/* Пароль */
char *_password;
/* Длина пароля */
uint8_t _passLenght;
/* Строка окончания ввода */
const char *prompt = "\r\n> ";

/* Private function prototypes -----------------------------------------------*/
#define PUTCHAR_PROTOTYPE int putchar (int c)
#define GETCHAR_PROTOTYPE int getchar (void)

void init(void);
void UART_Config(void);
void Timer_Init(void);
void Calendar_Init(void);
void LED_Init(void);
void Trig_Init(void);

void LSE_StabTime(void);
void Delay_Seconds(uint8_t seconds);
void Read_String(unsigned char* outString);
long Read_Int(void);

void ProcessCommand(char *cmd);

void Conf_Reset(void);

void Alarm_Set(uint8_t days, uint8_t hours, uint8_t minutes, uint8_t seconds);
void Alarm_PrepToShow(void);
void Alarm_Show(void);
void Alarm_Start(void);
void Alarm_Stop(void);
void Alarm_Pause(void);
void Alarm_Reset(void);

void Time_Set(uint8_t hours, uint8_t minutes, uint8_t seconds);
void Time_PrepToShow(void);
void Time_Show(void);
void Time_Start(void);
void Time_Stop(void);
void Time_Pause(void);
void Time_Reset(void);

void Trig_Ready(void);
void Trig_Set(void);
void Trig_Reset(void);

void _setPass(char *pass);
bool _validPass(char *password);
void _setConfig(char *mode, char *state, char *time);
void _getConfig(char *answer);
void _getConfig_integr(char *answer);
void _setConfig_integr(char *cmd);
void _setMode(char *mode, char *state);
void _setCurTime(char *time);


/* Private functions ---------------------------------------------------------*/
/**
  * @brief  инициализация перефирии.
  * @param  None
  * @retval None
  */
void init(void) {
  /* Конфигурируем таймер */
  Timer_Init();
  /* Calendar Configuration */
  Calendar_Init();
  /* Конфигуриреув индикацию */
  LED_Init();
  /* Конфигурируем выход триггера */
  Trig_Init();
  /* Конфигурируем UART */
  UART_Config();
  /* Enable general Interrupt*/
  enableInterrupts();
  /* Обнуляем время и будильник */
  Time_Stop();
  Alarm_Stop();
}

/**
  * @config UART for 115200 speed, 8bit, 1stop, NoParity
  * Port C pin 2 - tx, pin 3 - rx
  * @param  None.
  * @retval None.
  */
void UART_Config(void) {
  //Подключаем подтягивающие резисторы
  GPIO_ExternalPullUpConfig(GPIOC, GPIO_Pin_2, ENABLE);
  GPIO_ExternalPullUpConfig(GPIOC, GPIO_Pin_3, ENABLE);
  //Подаем тактирование на USART
  CLK_PeripheralClockConfig(CLK_Peripheral_USART1 ,ENABLE);
  //Инициализируем USART
  USART_Init(USART1, (u32)115200,USART_WordLength_8b,USART_StopBits_1,
             USART_Parity_No, (USART_Mode_TypeDef)(USART_Mode_Rx | USART_Mode_Tx));
  //Включаем прерывание на освобождение буфера передачи
  USART_ITConfig(USART1, USART_IT_TC, ENABLE);
  //Включаем прерывание на заполнение буфера приема
  USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
  //Включаем USART
  USART_Cmd(USART1,ENABLE);
}

/**
  * @brief  Calendar Configuration.
  * @param  None
  * @retval None
  */
void Calendar_Init(void) {
  RTC_AlarmCmd(DISABLE);
  RTC_InitStr.RTC_HourFormat = RTC_HourFormat_24;
  RTC_InitStr.RTC_AsynchPrediv = 0x7F;
  RTC_InitStr.RTC_SynchPrediv = 0x00FF;
  RTC_Init(&RTC_InitStr);

  RTC_DateStructInit(&RTC_DateStr);
  RTC_DateStr.RTC_WeekDay = RTC_Weekday_Sunday;
  RTC_DateStr.RTC_Date = 22;
  RTC_DateStr.RTC_Month = RTC_Month_February;
  RTC_DateStr.RTC_Year = 15;
  RTC_SetDate(RTC_Format_BIN, &RTC_DateStr);

  RTC_TimeStructInit(&RTC_TimeStr);
  RTC_TimeStr.RTC_Hours   = 00;
  RTC_TimeStr.RTC_Minutes = 00;
  RTC_TimeStr.RTC_Seconds = 00;
  RTC_SetTime(RTC_Format_BIN, &RTC_TimeStr);
  
  RTC_AlarmStructInit(&RTC_AlarmStr);
  RTC_AlarmStr.RTC_AlarmTime.RTC_Hours   = 00;
  RTC_AlarmStr.RTC_AlarmTime.RTC_Minutes = 00;
  RTC_AlarmStr.RTC_AlarmTime.RTC_Seconds = 00;
  RTC_AlarmStr.RTC_AlarmMask = RTC_AlarmMask_DateWeekDay;
  RTC_SetAlarm(RTC_Format_BIN, &RTC_AlarmStr);
}

/**
  * @brief  Wait 1 sec for LSE stabilisation .
  * @param  None.
  * @retval None.
  * Note : TIM4 is configured for a system clock = 2MHz
  */
void LSE_StabTime(void) {
  CLK_PeripheralClockConfig(CLK_Peripheral_TIM4, ENABLE);

  /* Configure TIM4 to generate an update event each 1 s */
  TIM4_TimeBaseInit(TIM4_Prescaler_16384, 123);

  /* Enable TIM4 */
  TIM4_Cmd(ENABLE);

  /* Wait 1 sec */
  while ( TIM4_GetFlagStatus(TIM4_FLAG_Update) == RESET );

  TIM4_ClearFlag(TIM4_FLAG_Update);

  /* Disable TIM4 */
  TIM4_Cmd(DISABLE);

  CLK_PeripheralClockConfig(CLK_Peripheral_TIM4, DISABLE);
}

/**
  * @brief  Инициализация часов реального времени.
  * @param  None
  * @retval None
  */
void Timer_Init(void) {
  /* Включаем тактирование на низкой настоте */
  CLK_LSEConfig(CLK_LSE_ON);
  /* Wait for LSE clock to be ready */
  while (CLK_GetFlagStatus(CLK_FLAG_LSERDY) == RESET);
  /* wait for 1 second for the LSE Stabilisation */
  LSE_StabTime();
  /* Select LSE (32.768 KHz) as RTC clock source */
  CLK_RTCClockConfig(CLK_RTCCLKSource_LSE, CLK_RTCCLKDiv_1);
  /* Подаем тактирование */
  CLK_PeripheralClockConfig(CLK_Peripheral_RTC, ENABLE);
}

/**
  * @brief  Инициализация выводов индикации.
  * @param  None
  * @retval None
  */
void LED_Init(void) {
  //Порт P0-P2 - RGB LED, 
  GPIO_Init(LED_RGB_REL_GPIO_PORT, 
            LED_RED_GPIO_PIN | LED_BLUE_GPIO_PIN | LED_GREEN_GPIO_PIN,
            GPIO_Mode_Out_PP_Low_Slow);
  //Порт B0-B7 - сегментный индикатор
  //TODO
}

/**
  * @brief  Инициализация управляемого вывода.
  * @param  None
  * @retval None
  */
void Trig_Init(void) {
  //Порт P4-P5 - Выходы реле - Ready, Trig, 
  GPIO_Init(LED_RGB_REL_GPIO_PORT, 
            TRIG_READY_GPIO_PIN | TRIG_SET_GPIO_PIN,
            GPIO_Mode_Out_PP_Low_Slow);
}

/**
  * @brief  Delay x sec
  * @param  Seconds : number of seconds to delay
  * @retval None.
  * Note : TIM4 is configured for a system clock = 2MHz
  */
void Delay_Seconds(uint8_t seconds) {
  uint8_t i = 0;

  /* Enable TIM4 Clock */
  CLK_PeripheralClockConfig(CLK_Peripheral_TIM4, ENABLE);

  /* Configure TIM4 to generate an update event each 1 s */
  TIM4_TimeBaseInit(TIM4_Prescaler_16384, 123);

  /* Enable TIM4 */
  TIM4_Cmd(ENABLE);

  /* Clear the Flag */
  TIM4_ClearFlag(TIM4_FLAG_Update);

  for (i = 0; i < seconds; i++)
  {
    /* Wait 1 sec */
    while ( TIM4_GetFlagStatus(TIM4_FLAG_Update) == RESET )
    {}

    /* Clear the Flag */
    TIM4_ClearFlag(TIM4_FLAG_Update);
  }

  /* Disable TIM4 */
  TIM4_Cmd(DISABLE);

  /* Disable TIM4 Clock */
  CLK_PeripheralClockConfig(CLK_Peripheral_TIM4, DISABLE);
}

/**
  * @brief  Читает строку из UART до первого символа <enter>.
  * Максимальная длина строки 512Байт
  * @param  outString - указатель на строку ввода
  * @retval None
  */
void Read_String(unsigned char* outString) {
  unsigned char temp;
  unsigned int index=0;
  
  //Пока не заполнится буффер считываем посимвольно данные
  while(index<512) {
    temp=getchar();
    //Добавляем к выходной строке если символ не <enter>
    if(temp!=13) {
      *outString++=temp;
    } else {
      index=512;
    }
    index++;
  }
}

/**
  * @brief  Читаем число (в ASCII) с UART. Максимальной длиной в 8 знаков.
  * Завершение ввода по вводу любого не цифрового символа
  * @param  None
  * @retval None
  */
long Read_Int(void) {
  unsigned char temp=0, index=0, flag=0;
  long value=0;
  temp = getchar();
  if (temp=='-') {
    flag=1;
    temp=getchar();
    index++;
  }
  do {
    index++;
    if((47<temp)&&(temp<58)) {
      value=value*10+temp-48;
    }
    else {
      index=255;
    }
    if(index<7) {
      temp=getchar();
    }
  } while (index<7);
  if (flag==1) {
          value=-value;
  }
  return value;
}

/**
  * @brief  Total Reset.
  * @param  None.
  * @retval None.
  */
void Conf_Reset(void) {
  Time_Stop();
  Alarm_Stop();
  Trig_Reset();
}

/**
  * @brief  Устанавливает значения времени будильника.
  * @param  {int} Days    дни
  * @param  {int} Hours   часы
  * @param  {int} Minutes минуты
  * @param  {int} Seconds секунды
  * @retval None
  */
void Alarm_Set(uint8_t Days, uint8_t Hours,
 uint8_t Minutes, uint8_t Seconds) {
  RTC_AlarmCmd(DISABLE);
  RTC_AlarmStr.RTC_AlarmTime.RTC_Hours   = Hours;
  RTC_AlarmStr.RTC_AlarmTime.RTC_Minutes = Minutes;
  RTC_AlarmStr.RTC_AlarmTime.RTC_Seconds = Seconds;
  RTC_SetAlarm(RTC_Format_BIN, &RTC_AlarmStr);
}

/**
  * @brief  Reset Alarm to zero.
  * @param  None.
  * @retval None.
  */
void Alarm_Reset(void) {
  RTC_AlarmCmd(DISABLE);
  RTC_AlarmStr.RTC_AlarmTime.RTC_Hours   = 0;
  RTC_AlarmStr.RTC_AlarmTime.RTC_Minutes = 0;
  RTC_AlarmStr.RTC_AlarmTime.RTC_Seconds = 0;
  RTC_SetAlarm(RTC_Format_BIN, &RTC_AlarmStr);
}

/**
  * @brief  Форматирует значение установленного будильника
  * @param  None.
  * @retval None.
  */
void Alarm_PrepToShow(void) {
    /* Get the configured Alarm Time*/
    RTC_GetAlarm(RTC_Format_BCD, &RTC_AlarmStr);

    /* Fill the UARTStringTime fields with the Alarm Time*/
    UARTStringAlarmTime[0] = (uint8_t)(((uint8_t)(RTC_AlarmStr.RTC_AlarmTime.RTC_Hours & 0xF0) >> 4) + ASCII_NUM_0);
    UARTStringAlarmTime[1] = (uint8_t)(((uint8_t)(RTC_AlarmStr.RTC_AlarmTime.RTC_Hours & 0x0F)) + ASCII_NUM_0);
                       
    UARTStringAlarmTime[3] = (uint8_t)(((uint8_t)(RTC_AlarmStr.RTC_AlarmTime.RTC_Minutes & 0xF0) >> 4) + ASCII_NUM_0);
    UARTStringAlarmTime[4] = (uint8_t)(((uint8_t)(RTC_AlarmStr.RTC_AlarmTime.RTC_Minutes & 0x0F)) + (uint8_t)ASCII_NUM_0);
                       
    UARTStringAlarmTime[6] = (uint8_t)(((uint8_t)(RTC_AlarmStr.RTC_AlarmTime.RTC_Seconds & 0xF0) >> 4) + ASCII_NUM_0);
    UARTStringAlarmTime[7] = (uint8_t)(((uint8_t)(RTC_AlarmStr.RTC_AlarmTime.RTC_Seconds & 0x0F)) + ASCII_NUM_0);
}

/**
  * @brief  Выводит форматированное значение установленного будильника
  * @param  None.
  * @retval None.
  */
void Alarm_Show(void) {
  Alarm_PrepToShow();
  /* Print the Alarm Time to UART*/
  printf((char*)UARTStringAlarmTime);
}

/**
  * @brief  Enable Alarm IT
  * @param  None.
  * @retval None.
  */
void Alarm_Start(void) {
  RTC_ITConfig(RTC_IT_ALRA, ENABLE);
  RTC_AlarmCmd(ENABLE);
  _alarm_set = TRUE;
}

/**
  * @brief  Disable Alarm IT
  * @param  None.
  * @retval None.
  */
void Alarm_Pause(void) {
  RTC_ITConfig(RTC_IT_ALRA, DISABLE);
  RTC_AlarmCmd(DISABLE);
  _alarm_set = FALSE;
}

/**
  * @brief  Stop and Reset Alarm
  * @param  None.
  * @retval None.
  */
void Alarm_Stop(void) {
  Alarm_Reset();
  Alarm_Pause();
}

/**
  * @brief  Выводит форматированное значение текущего времени
  * @param  None.
  * @retval None.
  */
void Time_Show(void) {
  Time_PrepToShow();
  printf((char*)UARTStringCurrTime);
}

/**
  * @brief  Форматирует значение текущего времени
  * @param  None.
  * @retval None.
  */
void Time_PrepToShow(void) {
  /* Get the current Time*/
  RTC_GetTime(RTC_Format_BCD, &RTC_TimeStr);

  /* Fill the UARTStringTime fields with the current Time*/
  UARTStringCurrTime[0] = (uint8_t)(((uint8_t)(RTC_TimeStr.RTC_Hours & 0xF0) >> 4) + ASCII_NUM_0);
  UARTStringCurrTime[1] = (uint8_t)(((uint8_t)(RTC_TimeStr.RTC_Hours & 0x0F)) + ASCII_NUM_0);
                      
  UARTStringCurrTime[3] = (uint8_t)(((uint8_t)(RTC_TimeStr.RTC_Minutes & 0xF0) >> 4) + ASCII_NUM_0);
  UARTStringCurrTime[4] = (uint8_t)(((uint8_t)(RTC_TimeStr.RTC_Minutes & 0x0F)) + (uint8_t)ASCII_NUM_0);
                      
  UARTStringCurrTime[6] = (uint8_t)(((uint8_t)(RTC_TimeStr.RTC_Seconds & 0xF0) >> 4) + ASCII_NUM_0);
  UARTStringCurrTime[7] = (uint8_t)(((uint8_t)(RTC_TimeStr.RTC_Seconds & 0x0F)) + ASCII_NUM_0);
}

/**
  * @brief  Set Chrono time.
  * @param  {uint8_t} hours часов.
  * @param  {uint8_t} minutes минут.
  * @param  {uint8_t} seconds секунд.
  * @retval None.
  */
void Time_Set(uint8_t hours, uint8_t minutes, uint8_t seconds) {
  RTC_TimeStr.RTC_H12     = RTC_H12_AM;
  RTC_TimeStr.RTC_Hours   = hours;
  RTC_TimeStr.RTC_Minutes = minutes;
  RTC_TimeStr.RTC_Seconds = seconds;
  RTC_SetTime(RTC_Format_BIN, &RTC_TimeStr);
}
  
/**
  * @brief  Reset Chrono to zero.
  * @param  None.
  * @retval None.
  */
void Time_Reset(void) {
  RTC_TimeStr.RTC_H12     = RTC_H12_AM;
  RTC_TimeStr.RTC_Hours   = 00;
  RTC_TimeStr.RTC_Minutes = 00;
  RTC_TimeStr.RTC_Seconds = 00;
  RTC_SetTime(RTC_Format_BIN, &RTC_TimeStr);
}

/**
  * @brief  Pause Chronometer
  * @param  None.
  * @retval None.
  */
void Time_Pause(void) {
  /* Отключаем тактирование RTC */
  CLK_RTCClockConfig(CLK_RTCCLKSource_Off, CLK_RTCCLKDiv_1);
  _timer_set = FALSE;
}

/**
  * @brief  Start Chronometer
  * @param  None.
  * @retval None.
  */
void Time_Start(void) {
  /* Возвращаем тактирование RTC */
  CLK_RTCClockConfig(CLK_RTCCLKSource_LSE, CLK_RTCCLKDiv_1);
  _timer_set = TRUE;
}

/**
  * @brief  Stop Chronometer
  * @param  None.
  * @retval None.
  */
void Time_Stop(void) {
  /* Обнуляем часы и выключаем тактирование */
  Time_Reset();
  Time_Pause();
}

/**
  * @brief  Устанавливает вывод порта готовности в SET
  * @param None
  * @retval None
  */
void Trig_Ready(void) {
  GPIO_SetBits(LED_RGB_REL_GPIO_PORT, TRIG_READY_GPIO_PIN);
  _trig_ready = TRUE;
}

/**
  * @brief  Устанавливает вывод порта сработки в SET в случае если уже установлен READY
  * @param None
  * @retval None
  */
void Trig_Set(void) {
  if (_trig_ready) {
      GPIO_SetBits(LED_RGB_REL_GPIO_PORT, TRIG_SET_GPIO_PIN);
    _trig_set = TRUE;
  }
}

/**
  * @brief  Устанавливает SET b READY в низкий уровень
  * @param None
  * @retval None
  */
void Trig_Reset(void) {
  GPIO_ResetBits(LED_RGB_REL_GPIO_PORT, TRIG_SET_GPIO_PIN);
  GPIO_ResetBits(LED_RGB_REL_GPIO_PORT, TRIG_READY_GPIO_PIN);
  _trig_set = FALSE;
  _trig_ready = FALSE;
}

/**
  * @brief  Обработчик комманд ввода
  * @param {*char} *cmd строка пользовательского ввода
  * @retval None
  */
void ProcessCommand(char *cmd) {
   char answer[50]  = "";
   char time[50] = "";
   char password[50]= "";
   char command[50] = "";
   char mode[50] = "";
   char state[50] = "";
   sscanf(cmd, "%s %s %s %s %s", command, password, mode, state, time);
   
   if(_validPass(password)){
       if (strncmp(command, "getconf", 7) == 0) {
         _getConfig(answer);
       } else if (strncmp(command, "getintconf", 10) == 0) {
         _getConfig_integr(answer);
       } else if (strncmp(command, "setintconf", 10) == 0) {
         _setConfig_integr(mode);
         _getConfig_integr(answer);
       } else if (strncmp(command, "setconf", 7) == 0) {
          _setConfig(mode, state, time);
          _getConfig(answer);
       } else if (strncmp(command, "setmode", 7) == 0) {
          _setMode(mode, state);
           _getConfig(answer);
       } else if (strncmp(command, "reset", 5) == 0) {
          Conf_Reset();
          _getConfig(answer);
       } else if (strncmp(command, "timepause", 5) == 0) {
          Time_Pause();
           _getConfig(answer);
       } else if (strncmp(command, "timestart", 9) == 0) {
          Time_Start();
           _getConfig(answer);
       } else if (strncmp(command, "timestop", 8) == 0) {
          Time_Stop();
           _getConfig(answer);
       } else if (strncmp(command, "trigready", 9) == 0) {
          Trig_Ready();
          _getConfig(answer);
       } else if (strncmp(command, "trigset", 7) == 0) {
          Trig_Set();
          _getConfig(answer);
       } else if (strncmp(command, "trigreset", 9) == 0) {
          Trig_Reset();
          _getConfig(answer);
       } else if (strncmp(command, "trigbroot", 7) == 0) {
          Trig_Ready();
          Trig_Set();
          _getConfig(answer);
       } else {
          sprintf(answer, "unknown command");
       }
   } else {
      sprintf(answer, "invalid password");
   }
   
   printf(answer);
   printf(prompt);
   printf(prompt);
}

/**
  * @brief  Проверяет входящую строку на соответствие сохраненному паролю
  * @param  {*char} password строка пароля
  * @retval None
  */
bool _validPass(char *password) {
  if((strncmp(password, _password, _passLenght) == 0)) {
      return TRUE;
  }
  return FALSE;
}

/**
  * @brief  Устанавливает настройки таймера, режима работы и включения 
  * @param  {*char} mode режим работы - TIMER или SAFETY
  * @param  {*char} state состояние включеня - ON или OFF
  * @param  {*char} time абсолютное время будильника HH:MM:SS
  * @retval None
  */
void _setConfig(char *mode, char *state, char *time) {
  sscanf(time, "%u:%u:%u", &HHH, &MM, &SS );
  uint8_t days = HHH/24;
  Alarm_Set(days, HHH, MM, SS);
  _setMode(mode, state);
}

/**
  * @brief  Устанавливает режим работы
  * @param  {*char} mode режим работы - TIMER или SAFETY
  * @param  {*char} state состояние включеня - ON или OFF
  * @retval None
  */
void _setMode(char *mode, char *state) {
  if (strncmp(mode, "TIMER", 5) == 0) {
    _mode = TRUE;
  } else if (strncmp(mode, "SAFETY", 5) == 0) {
    _mode = FALSE;
  }
  
  if (strncmp(state, "ON", 2) == 0) {
    Time_Start();
    Alarm_Start();
    
  } else if(strncmp(mode, "OFF", 3) == 0) {
    Time_Pause();
    Alarm_Pause();
  }
}

/**
  * @brief  Устанавливает текущее время
  * @param  {*char} time текущее время HH:MM:SS
  * @retval None
  */
void _setCurTime(char *time) {
  sscanf(time, "%u:%u:%u", &HHH, &MM, &SS );
  Time_Start();
  Time_Set(HHH, MM, SS);
  Time_Pause();
}

/**
  * @brief  Формирует отчет по текущему состоянию
  * @param  {*char} ссылка на строку в которую будет сформирован отчет
  * @retval None
  */
void _getConfig(char *answer) {
  //sprintf(answer, "%s/%s", (char*)(_mode ? "TIMER" : "SAFETY"), (char*)(_timer_set ? "ON" : "OFF"));
  printf("%s SET TO: ", (char*)(_mode ? "TIMER " : "SAFETY"));
  Alarm_Show();
  printf(prompt);
  printf("CURRENT TIME : ");
  Time_Show();
  printf(prompt);
  printf("COUNT, ALARM : %s %s", (char*)(_timer_set ? "ON" : "OFF"), (char*)(_alarm_set ? "ON" : "OFF"));
  printf(prompt);
  printf("TRIG RDY,SET : %s %s", (char*)(_trig_ready ? "READY" : "OFF"),
         (char*)(_trig_set ? "SET" : "OFF"));
  printf(prompt);
}

/**
  * @brief  Формирует однострочный отчет по текущему состоянию для интеграций
  * @param  {*char} ссылка на строку в которую будет сформирован отчет
  * @retval None
  */
void _getConfig_integr(char *answer) {
  //mode state trig_ready trig_set alarm_time curr_time 
  Alarm_PrepToShow();
  Time_PrepToShow();
  sprintf(answer, "%c/%c/%c/%c/%s/%s", (_mode ? '1' : '0'), (_timer_set ? '1' : '0'), (_trig_ready ? '1' : '0'),
         (char)(_trig_set? '1' : '0'), UARTStringAlarmTime, UARTStringCurrTime );
}

/**
  * @brief  Устанавливает настройки таймера, режима работы и триггера 
  * @param  {*char} cmd абсолютное время будильника mode/state/trig_ready/trig_set/alarm_time/curr_time 
  * @retval None
  */
void _setConfig_integr(char *cmd) {
   __IO bool mode = (bool)(cmd[0] == '1');
   __IO bool state = (bool)(cmd[2] == '1');
   __IO bool trig_ready = (bool)(cmd[4] == '1');
   __IO bool trig_set = (bool)(cmd[6] == '1');
   char alarm_time[9] = "";
   char curr_time[9] = "";
   
   //Очищаем строку в памяти
   memset(alarm_time, 0, sizeof(alarm_time));
   memset(curr_time, 0, sizeof(curr_time));
   
   //TODO: Парсинг сырой строки по элементам
   strncpy(alarm_time, cmd+8, sizeof(alarm_time) - 1);
   strncpy(curr_time, cmd+17, sizeof(curr_time) -1);

   _setConfig((mode? "TIMER":"SAFETY"), (state? "ON": "OFF"), alarm_time);
   _setCurTime(curr_time);
   
   if (trig_ready) {
     Trig_Ready();
   }
   if (trig_set) {
     Trig_Set();
   }
}

/**
  * @brief  Main program.
  * @param  None
  * @retval None
  */
void main(void) {
  /* Режим по умолчанию - дальнее взведение */
  _mode = FALSE;
  /* Пароль по умолчанию */
  _password = "root";
  /* Длина пароля */
  _passLenght = 4;
  /* Настройка делителя для тактирования = 1 (16 МГц) */
  CLK_SYSCLKDivConfig(CLK_SYSCLKDiv_1);
  /* Инициализация переферии */
  init();
  printf(prompt);
  /* Infinite loop */
  while (1) {
    if (AlarmOccurred != FALSE && _alarm_set && _timer_set) {
      Trig_Ready();
      printf("TRIG READY");
      printf(prompt);
          
      if (_mode) {
        printf("TRIG SET");
        printf(prompt);
        Trig_Set();
      }
      
      Alarm_Stop();
      Time_Stop();
      AlarmOccurred = FALSE;
    }
  }
}

/**
  * @brief Retargets the C library printf function to the USART.
  * @param[in] c Character to send
  * @retval char Character sent
  * @par Required preconditions:
  * - None
  */
PUTCHAR_PROTOTYPE {
  /* Write a character to the USART */
  USART_SendData8(USART1, c);
  /* Loop until the end of transmission */
  while (USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET);
  return (c);
}

/**
  * @brief Retargets the C library scanf function to the USART.
  * @param[in] None
  * @retval char Character to Read
  * @par Required preconditions:
  * - None
  */
GETCHAR_PROTOTYPE {
  int c = 0;
  /* Loop until the Read data register flag is SET */
  while (USART_GetFlagStatus(USART1, USART_FLAG_RXNE) == RESET);
  c = USART_ReceiveData8(USART1);
  return (c);
}
#ifdef  USE_FULL_ASSERT

/**
  * @brief  Reports the name of the source file and the source line number
  *   where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{ 
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {
  }
}
#endif

/**
  * @}
  */

/******************* (C) COPYRIGHT 2011 STMicroelectronics *****END OF FILE****/
