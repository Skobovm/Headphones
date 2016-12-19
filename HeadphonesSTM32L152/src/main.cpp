#include "mbed.h"
#include "radio_config.h"
#include "si446x_cmd.h"

#define U8 uint8_t
#define U16 uint16_t
#define BIT bool
#define FALSE false
#define TRUE true
#define RADIO_CTS_TIMEOUT 100000
#define RADIO_MAX_PACKET_LENGTH 64

SPI device(PB_5, PB_4, PB_3);
DigitalIn gpio0(PB_8);
DigitalIn gpio1(PB_9);
DigitalIn irqn(PA_8);
DigitalOut nsel(PB_10);

DigitalOut myled(LED1);
DigitalIn mybutton(USER_BUTTON);
Serial pc(USBTX, USBRX);
uint8_t ctsWentHigh = 0;

U8 Pro2Cmd[16];
union si446x_cmd_reply_union Si446xCmd;
U8 RadioConfigurationDataArray[] = RADIO_CONFIGURATION_DATA_ARRAY;
U8 customRadioPacket[RADIO_MAX_PACKET_LENGTH];

bool rxRestarted = true;

U8 radio_comm_PollCTS(void);

//
// Radio HAL
//
void radio_hal_AssertShutdown(void)
{

}

void radio_hal_DeassertShutdown(void)
{

}

void radio_hal_ClearNsel(void)
{
    nsel = 0;
}

void radio_hal_SetNsel(void)
{
    nsel = 1;
}

BIT radio_hal_NirqLevel(void)
{
    return (irqn == 1);
}

void radio_hal_SpiWriteByte(U8 byteToWrite)
{
    device.write(byteToWrite);
}

U8 radio_hal_SpiReadByte(void)
{
    U8 retVal = (U8)(device.write(0x00) & 0xFF);
    //pc.printf("Read byte: %x\n", retVal);
    return retVal;
}

void radio_hal_SpiWriteData(U8 byteCount, U8* pData)
{
    while(byteCount > 0)
    {
        device.write(*pData);
        pData++;
        byteCount--;
    }
}

void radio_hal_SpiReadData(U8 byteCount, U8* pData)
{
    while(byteCount > 0)
    {
        *pData = (U8)device.write(0x00);
        pData++;
        byteCount--;
    }
}


BIT radio_hal_Gpio0Level(void)
{
  BIT retVal = (gpio0 == 1);;

  return retVal;
}

BIT radio_hal_Gpio1Level(void)
{
  BIT retVal = (gpio1 == 1);

  return retVal;
}

BIT radio_hal_Gpio2Level(void)
{
  BIT retVal = FALSE;

  return retVal;
}

BIT radio_hal_Gpio3Level(void)
{
  BIT retVal = FALSE;

  return retVal;
}


//
// Radio Comm
//

/*!
 * Gets a command response from the radio chip
 *
 * @param byteCount     Number of bytes to get from the radio chip
 * @param pData         Pointer to where to put the data
 *
 * @return CTS value
 */
U8 radio_comm_GetResp(U8 byteCount, U8* pData)
{
  bool ctsVal = false;
  U16 errCnt = RADIO_CTS_TIMEOUT;

  while (errCnt != 0)      //wait until radio IC is ready with the data
  {
    radio_hal_ClearNsel();
    wait_us(1);
    radio_hal_SpiWriteByte(0x44);    //read CMD buffer
    ctsVal = radio_hal_SpiReadByte() == 0xFF;
    //ctsVal = radio_hal_Gpio1Level();
    if (ctsVal == true)
    {
      if (byteCount)
      {
        radio_hal_SpiReadData(byteCount, pData);
      }
      radio_hal_SetNsel();
      wait_us(1);
      break;
    }
    radio_hal_SetNsel();
    wait_us(1);
    errCnt--;
  }

  if (errCnt == 0)
  {
    while(1)
    {
      /* ERROR!!!!  CTS should never take this long. */
      pc.printf("Error: radio_comm_GetResp - CTS Timeout\n");
    }
  }

  if (ctsVal == true)
  {
    ctsWentHigh = 1;
  }

  return (ctsVal) ? 0xFF : 0;
}

/*!
 * Sends a command to the radio chip
 *
 * @param byteCount     Number of bytes in the command to send to the radio device
 * @param pData         Pointer to the command to send.
 */
void radio_comm_SendCmd(U8 byteCount, U8* pData)
{
    while (!ctsWentHigh)
    {
        //pc.printf("Polling CTS; Not Ready");
        radio_comm_PollCTS();
    }
    radio_hal_ClearNsel();
    wait_us(1);
    radio_hal_SpiWriteData(byteCount, pData);
    radio_hal_SetNsel();
    wait_us(1);
    ctsWentHigh = 0;
}

/*!
 * Gets a command response from the radio chip
 *
 * @param cmd           Command ID
 * @param pollCts       Set to poll CTS
 * @param byteCount     Number of bytes to get from the radio chip.
 * @param pData         Pointer to where to put the data.
 */
void radio_comm_ReadData(U8 cmd, BIT pollCts, U8 byteCount, U8* pData)
{
    if(pollCts)
    {
        while(!ctsWentHigh)
        {
            radio_comm_PollCTS();
        }
    }
    radio_hal_ClearNsel();
    wait_us(1);
    radio_hal_SpiWriteByte(cmd);
    radio_hal_SpiReadData(byteCount, pData);
    radio_hal_SetNsel();
    wait_us(1);
    ctsWentHigh = 0;
}


/*!
 * Gets a command response from the radio chip
 *
 * @param cmd           Command ID
 * @param pollCts       Set to poll CTS
 * @param byteCount     Number of bytes to get from the radio chip
 * @param pData         Pointer to where to put the data
 */
void radio_comm_WriteData(U8 cmd, BIT pollCts, U8 byteCount, U8* pData)
{
    if(pollCts)
    {
        while(!ctsWentHigh)
        {
            radio_comm_PollCTS();
        }
    }
    radio_hal_ClearNsel();
    wait_us(1);
    radio_hal_SpiWriteByte(cmd);
    radio_hal_SpiWriteData(byteCount, pData);
    radio_hal_SetNsel();
    wait_us(1);
    ctsWentHigh = 0;
}

/*!
 * Waits for CTS to be high
 *
 * @return CTS value
 */
U8 radio_comm_PollCTS(void)
{
    while(!radio_hal_Gpio1Level())
    {
        /* Wait...*/
    }
    ctsWentHigh = 1;
    return 0xFF;
}

/*!
 * Sends a command to the radio chip and gets a response
 *
 * @param cmdByteCount  Number of bytes in the command to send to the radio device
 * @param pCmdData      Pointer to the command data
 * @param respByteCount Number of bytes in the response to fetch
 * @param pRespData     Pointer to where to put the response data
 *
 * @return CTS value
 */
U8 radio_comm_SendCmdGetResp(U8 cmdByteCount, U8* pCmdData, U8 respByteCount, U8* pRespData)
{
    radio_comm_SendCmd(cmdByteCount, pCmdData);
    return radio_comm_GetResp(respByteCount, pRespData);
}

/**
 * Clears the CTS state variable.
 */
void radio_comm_ClearCTS()
{
  ctsWentHigh = 0;
}

/*!
 * Get the Interrupt status/pending flags form the radio and clear flags if requested.
 *
 * @param PH_CLR_PEND     Packet Handler pending flags clear.
 * @param MODEM_CLR_PEND  Modem Status pending flags clear.
 * @param CHIP_CLR_PEND   Chip State pending flags clear.
 */
void si446x_get_int_status(U8 PH_CLR_PEND, U8 MODEM_CLR_PEND, U8 CHIP_CLR_PEND)
{
    Pro2Cmd[0] = SI446X_CMD_ID_GET_INT_STATUS;
    Pro2Cmd[1] = PH_CLR_PEND;
    Pro2Cmd[2] = MODEM_CLR_PEND;
    Pro2Cmd[3] = CHIP_CLR_PEND;

    radio_comm_SendCmdGetResp( SI446X_CMD_ARG_COUNT_GET_INT_STATUS,
                              Pro2Cmd,
                              SI446X_CMD_REPLY_COUNT_GET_INT_STATUS,
                              Pro2Cmd );

    Si446xCmd.GET_INT_STATUS.INT_PEND       = Pro2Cmd[0];
    Si446xCmd.GET_INT_STATUS.INT_STATUS     = Pro2Cmd[1];
    Si446xCmd.GET_INT_STATUS.PH_PEND        = Pro2Cmd[2];
    Si446xCmd.GET_INT_STATUS.PH_STATUS      = Pro2Cmd[3];
    Si446xCmd.GET_INT_STATUS.MODEM_PEND     = Pro2Cmd[4];
    Si446xCmd.GET_INT_STATUS.MODEM_STATUS   = Pro2Cmd[5];
    Si446xCmd.GET_INT_STATUS.CHIP_PEND      = Pro2Cmd[6];
    Si446xCmd.GET_INT_STATUS.CHIP_STATUS    = Pro2Cmd[7];
}

/*!
 * This function is used to load all properties and commands with a list of NULL terminated commands.
 * Before this function @si446x_reset should be called.
 */
U8 si446x_configuration_init(const U8* pSetPropCmd)
{
  U8 col;
  U8 numOfBytes;

  /* While cycle as far as the pointer points to a command */
  while (*pSetPropCmd != 0x00)
  {
    /* Commands structure in the array:
     * --------------------------------
     * LEN | <LEN length of data>
     */

    numOfBytes = *pSetPropCmd++;

    if (numOfBytes > 16u)
    {
      /* Number of command bytes exceeds maximal allowable length */
      pc.printf("Error: si446x_configuration_init - Command\n");
      return SI446X_COMMAND_ERROR;
    }

    for (col = 0u; col < numOfBytes; col++)
    {
      Pro2Cmd[col] = *pSetPropCmd;
      pSetPropCmd++;
    }

    if (radio_comm_SendCmdGetResp(numOfBytes, Pro2Cmd, 0, 0) != 0xFF)
    {
      /* Timeout occured */
      pc.printf("Error: si446x_configuration_init - Timeout\n");
      return SI446X_CTS_TIMEOUT;
    }

    if (radio_hal_NirqLevel() == 0)
    {
      /* Get and clear all interrupts.  An error has occured... */
      si446x_get_int_status(0, 0, 0);
      if (Si446xCmd.GET_INT_STATUS.CHIP_PEND & SI446X_CMD_GET_CHIP_STATUS_REP_CHIP_PEND_CMD_ERROR_PEND_MASK)
      {
        pc.printf("Error: si446x_configuration_init - IRQ level\n");
        return SI446X_COMMAND_ERROR;
      }
    }
  }

  return SI446X_SUCCESS;
}

/*! This function sends the PART_INFO command to the radio and receives the answer
 *  into @Si446xCmd union.
 */
void si446x_part_info(void)
{
    Pro2Cmd[0] = SI446X_CMD_ID_PART_INFO;

    radio_comm_SendCmdGetResp( SI446X_CMD_ARG_COUNT_PART_INFO,
                              Pro2Cmd,
                              SI446X_CMD_REPLY_COUNT_PART_INFO,
                              Pro2Cmd );

    Si446xCmd.PART_INFO.CHIPREV         = Pro2Cmd[0];
    Si446xCmd.PART_INFO.PART            = ((U16)Pro2Cmd[1] << 8) & 0xFF00;
    Si446xCmd.PART_INFO.PART           |= (U16)Pro2Cmd[2] & 0x00FF;
    Si446xCmd.PART_INFO.PBUILD          = Pro2Cmd[3];
    Si446xCmd.PART_INFO.ID              = ((U16)Pro2Cmd[4] << 8) & 0xFF00;
    Si446xCmd.PART_INFO.ID             |= (U16)Pro2Cmd[5] & 0x00FF;
    Si446xCmd.PART_INFO.CUSTOMER        = Pro2Cmd[6];
    Si446xCmd.PART_INFO.ROMID           = Pro2Cmd[7];
}

/*! Sends START_TX command to the radio.
 *
 * @param CHANNEL   Channel number.
 * @param CONDITION Start TX condition.
 * @param TX_LEN    Payload length (exclude the PH generated CRC).
 */
void si446x_start_tx(U8 CHANNEL, U8 CONDITION, U16 TX_LEN)
{
    Pro2Cmd[0] = SI446X_CMD_ID_START_TX;
    Pro2Cmd[1] = CHANNEL;
    Pro2Cmd[2] = CONDITION;
    Pro2Cmd[3] = (U8)(TX_LEN >> 8);
    Pro2Cmd[4] = (U8)(TX_LEN);
    Pro2Cmd[5] = 0x00;

    // Don't repeat the packet,
    // ie. transmit the packet only once
    Pro2Cmd[6] = 0x00;

    radio_comm_SendCmd( SI446X_CMD_ARG_COUNT_START_TX, Pro2Cmd );
}

/*!
 * Sends START_RX command to the radio.
 *
 * @param CHANNEL     Channel number.
 * @param CONDITION   Start RX condition.
 * @param RX_LEN      Payload length (exclude the PH generated CRC).
 * @param NEXT_STATE1 Next state when Preamble Timeout occurs.
 * @param NEXT_STATE2 Next state when a valid packet received.
 * @param NEXT_STATE3 Next state when invalid packet received (e.g. CRC error).
 */
void si446x_start_rx(U8 CHANNEL, U8 CONDITION, U16 RX_LEN, U8 NEXT_STATE1, U8 NEXT_STATE2, U8 NEXT_STATE3)
{
    Pro2Cmd[0] = SI446X_CMD_ID_START_RX;
    Pro2Cmd[1] = CHANNEL;
    Pro2Cmd[2] = CONDITION;
    Pro2Cmd[3] = (U8)(RX_LEN >> 8);
    Pro2Cmd[4] = (U8)(RX_LEN);
    Pro2Cmd[5] = NEXT_STATE1;
    Pro2Cmd[6] = NEXT_STATE2;
    Pro2Cmd[7] = NEXT_STATE3;

    radio_comm_SendCmd( SI446X_CMD_ARG_COUNT_START_RX, Pro2Cmd );
}

/*!
 * Send GPIO pin config command to the radio and reads the answer into
 * @Si446xCmd union.
 *
 * @param GPIO0       GPIO0 configuration.
 * @param GPIO1       GPIO1 configuration.
 * @param GPIO2       GPIO2 configuration.
 * @param GPIO3       GPIO3 configuration.
 * @param NIRQ        NIRQ configuration.
 * @param SDO         SDO configuration.
 * @param GEN_CONFIG  General pin configuration.
 */
void si446x_gpio_pin_cfg(U8 GPIO0, U8 GPIO1, U8 GPIO2, U8 GPIO3, U8 NIRQ, U8 SDO, U8 GEN_CONFIG)
{
    Pro2Cmd[0] = SI446X_CMD_ID_GPIO_PIN_CFG;
    Pro2Cmd[1] = GPIO0;
    Pro2Cmd[2] = GPIO1;
    Pro2Cmd[3] = GPIO2;
    Pro2Cmd[4] = GPIO3;
    Pro2Cmd[5] = NIRQ;
    Pro2Cmd[6] = SDO;
    Pro2Cmd[7] = GEN_CONFIG;

    radio_comm_SendCmdGetResp( SI446X_CMD_ARG_COUNT_GPIO_PIN_CFG,
                              Pro2Cmd,
                              SI446X_CMD_REPLY_COUNT_GPIO_PIN_CFG,
                              Pro2Cmd );

    Si446xCmd.GPIO_PIN_CFG.GPIO[0]        = Pro2Cmd[0];
    Si446xCmd.GPIO_PIN_CFG.GPIO[1]        = Pro2Cmd[1];
    Si446xCmd.GPIO_PIN_CFG.GPIO[2]        = Pro2Cmd[2];
    Si446xCmd.GPIO_PIN_CFG.GPIO[3]        = Pro2Cmd[3];
    Si446xCmd.GPIO_PIN_CFG.NIRQ         = Pro2Cmd[4];
    Si446xCmd.GPIO_PIN_CFG.SDO          = Pro2Cmd[5];
    Si446xCmd.GPIO_PIN_CFG.GEN_CONFIG   = Pro2Cmd[6];
}

void si446x_set_property( U8 GROUP, U8 NUM_PROPS, U8 START_PROP, ... )
{
    va_list argList;
    U8 cmdIndex;

    Pro2Cmd[0] = SI446X_CMD_ID_SET_PROPERTY;
    Pro2Cmd[1] = GROUP;
    Pro2Cmd[2] = NUM_PROPS;
    Pro2Cmd[3] = START_PROP;

    va_start (argList, START_PROP);
    cmdIndex = 4;
    while(NUM_PROPS--)
    {
        Pro2Cmd[cmdIndex] = va_arg (argList, U8);
        cmdIndex++;
    }
    va_end(argList);

    radio_comm_SendCmd( cmdIndex, Pro2Cmd );
}

/*!
 * Issue a change state command to the radio.
 *
 * @param NEXT_STATE1 Next state.
 */
void si446x_change_state(U8 NEXT_STATE1)
{
    Pro2Cmd[0] = SI446X_CMD_ID_CHANGE_STATE;
    Pro2Cmd[1] = NEXT_STATE1;

    radio_comm_SendCmd( SI446X_CMD_ARG_COUNT_CHANGE_STATE, Pro2Cmd );
}

/*!
 * Sends NOP command to the radio. Can be used to maintain SPI communication.
 */
void si446x_nop(void)
{
    Pro2Cmd[0] = SI446X_CMD_ID_NOP;

    radio_comm_SendCmd( SI446X_CMD_ARG_COUNT_NOP, Pro2Cmd );
}

/*!
 * Send the FIFO_INFO command to the radio. Optionally resets the TX/RX FIFO. Reads the radio response back
 * into @Si446xCmd.
 *
 * @param FIFO  RX/TX FIFO reset flags.
 */
void si446x_fifo_info(U8 FIFO)
{
    Pro2Cmd[0] = SI446X_CMD_ID_FIFO_INFO;
    Pro2Cmd[1] = FIFO;

    radio_comm_SendCmdGetResp( SI446X_CMD_ARG_COUNT_FIFO_INFO,
                              Pro2Cmd,
                              SI446X_CMD_REPLY_COUNT_FIFO_INFO,
                              Pro2Cmd );

    Si446xCmd.FIFO_INFO.RX_FIFO_COUNT   = Pro2Cmd[0];
    Si446xCmd.FIFO_INFO.TX_FIFO_SPACE   = Pro2Cmd[1];
}

/*!
 * The function can be used to load data into TX FIFO.
 *
 * @param numBytes  Data length to be load.
 * @param pTxData   Pointer to the data (U8*).
 */
void si446x_write_tx_fifo(U8 numBytes, U8* pTxData)
{
  radio_comm_WriteData( SI446X_CMD_ID_WRITE_TX_FIFO, 0, numBytes, pTxData );
}

/*!
 * Reads the RX FIFO content from the radio.
 *
 * @param numBytes  Data length to be read.
 * @param pRxData   Pointer to the buffer location.
 */
void si446x_read_rx_fifo(U8 numBytes, U8* pRxData)
{
  radio_comm_ReadData( SI446X_CMD_ID_READ_RX_FIFO, 0, numBytes, pRxData );
}

/*!
 * Get property values from the radio. Reads them into Si446xCmd union.
 *
 * @param GROUP       Property group number.
 * @param NUM_PROPS   Number of properties to be read.
 * @param START_PROP  Starting sub-property number.
 */
void si446x_get_property(U8 GROUP, U8 NUM_PROPS, U8 START_PROP)
{
    Pro2Cmd[0] = SI446X_CMD_ID_GET_PROPERTY;
    Pro2Cmd[1] = GROUP;
    Pro2Cmd[2] = NUM_PROPS;
    Pro2Cmd[3] = START_PROP;

    radio_comm_SendCmdGetResp( SI446X_CMD_ARG_COUNT_GET_PROPERTY,
                              Pro2Cmd,
                              Pro2Cmd[2],
                              Pro2Cmd );

    Si446xCmd.GET_PROPERTY.DATA[0 ]   = Pro2Cmd[0];
    Si446xCmd.GET_PROPERTY.DATA[1 ]   = Pro2Cmd[1];
    Si446xCmd.GET_PROPERTY.DATA[2 ]   = Pro2Cmd[2];
    Si446xCmd.GET_PROPERTY.DATA[3 ]   = Pro2Cmd[3];
    Si446xCmd.GET_PROPERTY.DATA[4 ]   = Pro2Cmd[4];
    Si446xCmd.GET_PROPERTY.DATA[5 ]   = Pro2Cmd[5];
    Si446xCmd.GET_PROPERTY.DATA[6 ]   = Pro2Cmd[6];
    Si446xCmd.GET_PROPERTY.DATA[7 ]   = Pro2Cmd[7];
    Si446xCmd.GET_PROPERTY.DATA[8 ]   = Pro2Cmd[8];
    Si446xCmd.GET_PROPERTY.DATA[9 ]   = Pro2Cmd[9];
    Si446xCmd.GET_PROPERTY.DATA[10]   = Pro2Cmd[10];
    Si446xCmd.GET_PROPERTY.DATA[11]   = Pro2Cmd[11];
    Si446xCmd.GET_PROPERTY.DATA[12]   = Pro2Cmd[12];
    Si446xCmd.GET_PROPERTY.DATA[13]   = Pro2Cmd[13];
    Si446xCmd.GET_PROPERTY.DATA[14]   = Pro2Cmd[14];
    Si446xCmd.GET_PROPERTY.DATA[15]   = Pro2Cmd[15];
}

void vRadio_Init(void)
{
  U16 wDelay;

  /* Load radio configuration */
  while (SI446X_SUCCESS != si446x_configuration_init(RadioConfigurationDataArray))
  {
    pc.printf("Radio Init Fail\n");
    /* Error hook */
    myled = !myled;

    // Let the time expire (we'll be stuck toggling the LED)
    for (wDelay = 0x7FFF; wDelay--; ) ;
    /* Power Up the radio chip */
    //vRadio_PowerUp();
  }

  // Read ITs, clear pending ones
  si446x_get_int_status(0, 0, 0);
  pc.printf("Radio Init End\n");
}

/*!
 *  Check if Packet sent IT flag or Packet Received IT is pending.
 *
 *  @return   SI4455_CMD_GET_INT_STATUS_REP_PACKET_SENT_PEND_BIT / SI4455_CMD_GET_INT_STATUS_REP_PACKET_RX_PEND_BIT
 *
 *  @note
 *
 */
U8 bRadio_Check_Tx_RX(void)
{
  if (radio_hal_NirqLevel() == FALSE)
  {
      /* Read ITs, clear pending ones */
      si446x_get_int_status(0u, 0u, 0u);

      if (Si446xCmd.GET_INT_STATUS.CHIP_PEND & SI446X_CMD_GET_CHIP_STATUS_REP_CHIP_PEND_CMD_ERROR_PEND_BIT)
      {
        /* State change to */
        si446x_change_state(SI446X_CMD_CHANGE_STATE_ARG_NEXT_STATE1_NEW_STATE_ENUM_SLEEP);

        /* Reset FIFO */
        si446x_fifo_info(SI446X_CMD_FIFO_INFO_ARG_FIFO_RX_BIT);

        /* State change to */
        si446x_change_state(SI446X_CMD_CHANGE_STATE_ARG_NEXT_STATE1_NEW_STATE_ENUM_RX);
      }

      if(Si446xCmd.GET_INT_STATUS.PH_PEND & SI446X_CMD_GET_INT_STATUS_REP_PH_PEND_PACKET_SENT_PEND_BIT)
      {
        return SI446X_CMD_GET_INT_STATUS_REP_PH_PEND_PACKET_SENT_PEND_BIT;
      }

      if(Si446xCmd.GET_INT_STATUS.PH_PEND & SI446X_CMD_GET_INT_STATUS_REP_PH_PEND_PACKET_RX_PEND_BIT)
      {
        /* Packet RX */
        pc.printf("Received Packet!!!\n");

        /* Get payload length */
        si446x_fifo_info(0x00);

        si446x_read_rx_fifo(Si446xCmd.FIFO_INFO.RX_FIFO_COUNT, &customRadioPacket[0]);

        return SI446X_CMD_GET_INT_STATUS_REP_PH_PEND_PACKET_RX_PEND_BIT;
      }

      if (Si446xCmd.GET_INT_STATUS.PH_PEND & SI446X_CMD_GET_INT_STATUS_REP_PH_STATUS_CRC_ERROR_BIT)
      {
        /* Reset FIFO */
        si446x_fifo_info(SI446X_CMD_FIFO_INFO_ARG_FIFO_RX_BIT);
      }


  }

  return 0;
}

/*!
 *  Set Radio to RX mode. .
 *
 *  @param channel Freq. Channel,  packetLength : 0 Packet handler fields are used , nonzero: only Field1 is used
 *
 *  @note
 *
 */
void vRadio_StartRX(U8 channel, U8 packetLenght )
{
  // Read ITs, clear pending ones
  si446x_get_int_status(0u, 0u, 0u);

   // Reset the Rx Fifo
   si446x_fifo_info(SI446X_CMD_FIFO_INFO_ARG_FIFO_RX_BIT);

  /* Start Receiving packet, channel 0, START immediately, Packet length used or not according to packetLength */
  si446x_start_rx(channel, 0u, packetLenght,
                  SI446X_CMD_START_RX_ARG_NEXT_STATE1_RXTIMEOUT_STATE_ENUM_NOCHANGE,
                  SI446X_CMD_START_RX_ARG_NEXT_STATE2_RXVALID_STATE_ENUM_READY,
                  SI446X_CMD_START_RX_ARG_NEXT_STATE3_RXINVALID_STATE_ENUM_RX );
}

/*!
 *  Set Radio to TX mode, variable packet length.
 *
 *  @param channel Freq. Channel, Packet to be sent length of of the packet sent to TXFIFO
 *
 *  @note
 *
 */
void vRadio_StartTx_Variable_Packet(U8 channel, U8 *pioRadioPacket, U8 length)
{
  /* Leave RX state */
  si446x_change_state(SI446X_CMD_CHANGE_STATE_ARG_NEXT_STATE1_NEW_STATE_ENUM_READY);

  /* Read ITs, clear pending ones */
  si446x_get_int_status(0u, 0u, 0u);

  /* Reset the Tx Fifo */
  si446x_fifo_info(SI446X_CMD_FIFO_INFO_ARG_FIFO_TX_BIT);

  /* Fill the TX fifo with datas */
  si446x_write_tx_fifo(length, pioRadioPacket);

  /* Start sending packet, channel 0, START immediately */
   si446x_start_tx(channel, 0x80, length);

}

int main() {
    pc.printf("Starting up...\n");
    myled = 1;

    wait_ms(1000);

    myled = 0;

    nsel = 0;
    device.write(0x44);
    uint8_t res = (uint8_t)device.write(0x00);
    nsel = 1;
    pc.printf("Result: %d\n", res);
    if(res == 0xFF)
    {
        myled = 1;
        wait_ms(1000);
    }
    myled = 0;

    pc.printf("Begin radio init...\n");
    vRadio_Init();
    pc.printf("Completed radio init...\n");
    //myled = 1;

    //myled = 0;
    pc.printf("Starting RX...\n");
    vRadio_StartRX(0, 7);
    pc.printf("RX Started...\n");
    myled = 1;

    while(1) {
      if(bRadio_Check_Tx_RX() == SI446X_CMD_GET_INT_STATUS_REP_PH_PEND_PACKET_RX_PEND_BIT)
      {
          pc.printf("RX or TX complete\n");
          pc.printf("Data: %x %x %x %x %x %x %x\n", customRadioPacket[0], customRadioPacket[1], customRadioPacket[2], customRadioPacket[3], customRadioPacket[4], customRadioPacket[5], customRadioPacket[6]);
          vRadio_StartRX(0, 7);
      }
      if(mybutton == 0)
      {
        rxRestarted = false;
        myled = 0;
        pc.printf("Sending data...\n");
        //wait_ms(300);
        for(int pos = 0u; pos < 7; pos++)
        {
            customRadioPacket[pos] = 0xFF - pos;
        }

        // Send data
        vRadio_StartTx_Variable_Packet(0 /*Channel number*/, &customRadioPacket[0], 7/*Packet length*/);
        vRadio_StartRX(0, 7);
        myled = 1;
       }
       else
       {
            if(!rxRestarted)
            {
                rxRestarted = true;
                vRadio_StartRX(0, 7);
            }

            if(gpio0 == 1)
            {
                //pc.printf("RX BUFFER FULL\n");
            }
       }
    }
}
